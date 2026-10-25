#include <rex/graphics/pipeline/shader/spirv_library.h>

#include <bit>
#include <chrono>
#include <cstring>
#include <mutex>
#include <vector>

#include <xxhash.h>

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/logging.h>

REXCVAR_DEFINE_STRING(ds2_shader_library, "", "GPU",
                      "Offline SPIR-V library; empty disables it. Unknown variants use the SDK translator.")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

REXCVAR_DEFINE_BOOL(ds2_shader_library_index, true, "GPU",
                    "Load the compact .idx sidecar instead of scanning every shader record")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

namespace rex::graphics {
namespace {
constexpr std::array<uint32_t, 4> kHeader = {0x53325344, 0x314C5650, 1,
                                           SpirvShaderTranslator::Modification::kVersion};
using Record = std::array<uint64_t, 10>;
using IndexRecord = std::array<uint64_t, 5>;
constexpr uint64_t kIndexMagic = UINT64_C(0x3158444932535344);

bool PayloadSize(const Record& r, uint64_t& size) {
  if (r[3] > 1 || r[4] > 262144 || !r[5] || r[5] > (4 << 20) || (r[5] & 3) ||
      r[6] > 64 || r[7] > 64) return false;
  size = r[4] * 4 + r[5] + (r[6] + r[7]) * 4;
  return true;
}

void AppendWords(std::vector<uint8_t>& bytes, const void* data, size_t size) {
  const auto* p = static_cast<const uint8_t*>(data);
  bytes.insert(bytes.end(), p, p + size);
}
}  // namespace

SpirvShaderLibrary::~SpirvShaderLibrary() {
  if (file_) fclose(file_);
}

bool SpirvShaderLibrary::Open(const std::filesystem::path& path, uint64_t expected_signature) {
  static_assert(std::endian::native == std::endian::little);
  if (file_) fclose(file_);
  file_ = nullptr;
  file_size_ = 0;
  indexed_ = false;
  entries_.clear();
  FILE* file = rex::filesystem::OpenFile(path, "rb");
  if (!file) return false;
  auto fail = [&]() { fclose(file); entries_.clear(); return false; };
  if (fseek(file, 0, SEEK_END)) return fail();
  const long file_size = ftell(file);
  if (file_size < long(sizeof(kHeader)) || fseek(file, 0, SEEK_SET)) return fail();
  std::array<uint32_t, 4> header;
  if (fread(header.data(), 1, sizeof(header), file) != sizeof(header)) return fail();
  if (header != kHeader) {
    REXGPU_WARN("[shader library] incompatible header/ABI {}, expected {}; runtime fallback", header[3], kHeader[3]);
    return fail();
  }
  file_size_ = uint64_t(file_size);
  Record first_record;
  uint64_t first_bytes;
  if (file_size_ < sizeof(header) + sizeof(first_record) ||
      fread(first_record.data(), 1, sizeof(first_record), file) != sizeof(first_record) ||
      !PayloadSize(first_record, first_bytes) || first_bytes > file_size_ - sizeof(header) - sizeof(first_record)) return fail();
  file_signature_ = first_record[2];
  if (expected_signature && file_signature_ != expected_signature) {
    REXGPU_WARN("[shader library] incompatible pack signature {:016X}, expected {:016X}; runtime fallback",
                file_signature_, expected_signature);
    return fail();
  }
  if (fseek(file, long(sizeof(header)), SEEK_SET)) return fail();
  auto index_path = path;
  index_path += ".idx";
  if (REXCVAR_GET(ds2_shader_library_index) && ReadIndex(index_path)) {
    file_ = file;
    indexed_ = true;
    return true;
  }
  uint64_t offset = sizeof(header);
  while (offset < uint64_t(file_size)) {
    Record record;
    if (uint64_t(file_size) - offset < sizeof(record) ||
        fread(record.data(), 1, sizeof(record), file) != sizeof(record)) return fail();
    uint64_t bytes;
    if (!PayloadSize(record, bytes) || bytes > uint64_t(file_size) - offset - sizeof(record) ||
        entries_.size() >= 200000) return fail();
    entries_.emplace(Key{record[0], record[1], record[2], record[3]}, offset);
    if (fseek(file, long(bytes), SEEK_CUR)) return fail();
    offset += sizeof(record) + bytes;
  }
  file_ = file;
  return true;
}

bool SpirvShaderLibrary::ReadIndex(const std::filesystem::path& path) {
  FILE* index = rex::filesystem::OpenFile(path, "rb");
  if (!index) return false;
  if (file_size_ < sizeof(kHeader) + sizeof(Record)) {
    fclose(index);
    return false;
  }
  std::array<uint64_t, 6> header;
  if (fread(header.data(), 1, sizeof(header), index) != sizeof(header) ||
      header[0] != kIndexMagic || header[1] != 1 || header[2] != file_size_ ||
      !header[3] || header[3] > 200000 || header[5] != kHeader[3]) {
    fclose(index);
    return false;
  }
  std::vector<IndexRecord> records(static_cast<size_t>(header[3]));
  const size_t bytes = records.size() * sizeof(IndexRecord);
  const bool valid = fread(records.data(), 1, bytes, index) == bytes && fgetc(index) == EOF &&
                     !ferror(index) && XXH3_64bits(records.data(), bytes) == header[4];
  fclose(index);
  if (!valid) return false;
  Key previous = {};
  bool first = true;
  for (const auto& r : records) {
    const Key key = {r[0], r[1], r[2], r[3]};
    if (r[2] != file_signature_ || r[3] > 1 || r[4] < sizeof(kHeader) || r[4] > file_size_ - sizeof(Record) ||
        (!first && key <= previous)) {
      entries_.clear();
      return false;
    }
    entries_.emplace_hint(entries_.end(), key, r[4]);
    previous = key;
    first = false;
  }
  return true;
}

bool SpirvShaderLibrary::WriteIndex(const std::filesystem::path& path) const {
  if (!file_ || entries_.empty()) return false;
  std::vector<IndexRecord> records;
  records.reserve(entries_.size());
  for (const auto& [key, offset] : entries_) {
    records.push_back({key[0], key[1], key[2], key[3], offset});
  }
  const size_t bytes = records.size() * sizeof(IndexRecord);
  const std::array<uint64_t, 6> header = {kIndexMagic, 1, file_size_, records.size(),
                                        XXH3_64bits(records.data(), bytes), kHeader[3]};
  FILE* index = rex::filesystem::OpenFile(path, "wbx");
  if (!index) return false;
  const bool written = fwrite(header.data(), 1, sizeof(header), index) == sizeof(header) &&
                       fwrite(records.data(), 1, bytes, index) == bytes;
  return fclose(index) == 0 && written;
}

bool SpirvShaderLibrary::Restore(Shader::Translation& translation, uint64_t signature) {
  last_failure_ = "shader/state unavailable";
  auto* shader = dynamic_cast<SpirvShader*>(&translation.shader());
  if (!shader || !file_ || translation.is_translated()) return false;
  last_failure_ = "key not found";
  const auto found = entries_.find(Key{shader->ucode_data_hash(), translation.modification(), signature,
                                      uint64_t(shader->type())});
  if (found == entries_.end() || fseek(file_, long(found->second), SEEK_SET)) return false;
  last_failure_ = "record/signature mismatch";
  Record record;
  if (fread(record.data(), 1, sizeof(record), file_) != sizeof(record)) return false;
  uint64_t bytes;
  if (!PayloadSize(record, bytes) || record[4] != shader->ucode_dword_count() ||
      record[0] != shader->ucode_data_hash() || record[1] != translation.modification() ||
      record[2] != signature || record[3] != uint64_t(shader->type())) return false;
  last_failure_ = "payload/checksum/microcode mismatch";
  std::vector<uint8_t> payload(size_t(bytes), 0);
  if (fread(payload.data(), 1, payload.size(), file_) != payload.size() ||
      XXH3_64bits(payload.data(), payload.size()) != record[9] ||
      std::memcmp(payload.data(), shader->ucode_dwords(), size_t(record[4] * 4))) return false;
  const size_t binary_start = size_t(record[4] * 4);
  uint32_t magic = 0;
  std::memcpy(&magic, payload.data() + binary_start, 4);
  if (magic != 0x07230203) return false;
  size_t cursor = binary_start + size_t(record[5]);
  std::vector<SpirvShader::TextureBinding> textures;
  std::vector<SpirvShader::SamplerBinding> samplers;
  for (size_t i = 0; i < record[6]; ++i) {
    uint32_t word;
    std::memcpy(&word, payload.data() + cursor, 4); cursor += 4;
    SpirvShader::TextureBinding binding = {};
    binding.fetch_constant = word & 31;
    binding.dimension = xenos::FetchOpDimension((word >> 5) & 3);
    binding.is_signed = (word >> 7) & 1;
    textures.push_back(binding);
  }
  for (size_t i = 0; i < record[7]; ++i) {
    uint32_t word;
    std::memcpy(&word, payload.data() + cursor, 4); cursor += 4;
    SpirvShader::SamplerBinding binding = {};
    binding.fetch_constant = word & 31;
    binding.mag_filter = xenos::TextureFilter((word >> 5) & 3);
    binding.min_filter = xenos::TextureFilter((word >> 7) & 3);
    binding.mip_filter = xenos::TextureFilter((word >> 9) & 3);
    binding.aniso_filter = xenos::AnisoFilter((word >> 11) & 7);
    samplers.push_back(binding);
  }
  if (!shader->bindings_setup_entered_.test_and_set(std::memory_order_relaxed)) {
    shader->texture_bindings_ = std::move(textures);
    shader->sampler_bindings_ = std::move(samplers);
    shader->used_texture_mask_ = uint32_t(record[8]);
  }
  translation.translated_binary_.assign(payload.begin() + binary_start,
                                       payload.begin() + binary_start + size_t(record[5]));
  translation.errors_.clear();
  translation.is_translated_ = true;
  translation.is_valid_ = true;
  return true;
}

bool SpirvShaderLibrary::Begin(FILE* file) {
  return fwrite(kHeader.data(), 1, sizeof(kHeader), file) == sizeof(kHeader);
}

bool SpirvShaderLibrary::Append(FILE* file, const Shader::Translation& translation, uint64_t signature) {
  const auto* shader = dynamic_cast<const SpirvShader*>(&translation.shader());
  if (!shader || !translation.is_valid() || !translation.is_translated()) return false;
  Record record = {shader->ucode_data_hash(), translation.modification(), signature,
                   uint64_t(shader->type()), shader->ucode_dword_count(), translation.translated_binary().size(),
                   shader->texture_bindings_.size(), shader->sampler_bindings_.size(), shader->used_texture_mask_, 0};
  uint64_t bytes;
  if (!PayloadSize(record, bytes)) return false;
  std::vector<uint8_t> payload;
  payload.reserve(size_t(bytes));
  AppendWords(payload, shader->ucode_dwords(), shader->ucode_dword_count() * 4);
  AppendWords(payload, translation.translated_binary().data(), translation.translated_binary().size());
  for (const auto& binding : shader->texture_bindings_) {
    const uint32_t word = uint32_t(binding.fetch_constant) | (uint32_t(binding.dimension) << 5) |
                          (uint32_t(binding.is_signed) << 7);
    AppendWords(payload, &word, sizeof(word));
  }
  for (const auto& binding : shader->sampler_bindings_) {
    const uint32_t word = uint32_t(binding.fetch_constant) | (uint32_t(binding.mag_filter) << 5) |
                          (uint32_t(binding.min_filter) << 7) | (uint32_t(binding.mip_filter) << 9) |
                          (uint32_t(binding.aniso_filter) << 11);
    AppendWords(payload, &word, sizeof(word));
  }
  record[9] = XXH3_64bits(payload.data(), payload.size());
  return fwrite(record.data(), 1, sizeof(record), file) == sizeof(record) &&
         fwrite(payload.data(), 1, payload.size(), file) == payload.size();
}

bool RestoreConfiguredSpirvLibrary(Shader::Translation& translation, uint64_t signature) {
  const std::string path = REXCVAR_GET(ds2_shader_library);
  if (path.empty()) return false;
  static std::mutex mutex;
  static SpirvShaderLibrary library;
  static bool initialized = false, opened = false;
  static uint64_t hits = 0, misses = 0;
  std::lock_guard lock(mutex);
  if (!initialized) {
    std::filesystem::path full_path(path);
    if (full_path.is_relative()) full_path = rex::filesystem::GetExecutableFolder() / full_path;
    const auto start = std::chrono::steady_clock::now();
    opened = library.Open(full_path, signature);
    const double open_ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
    initialized = true;
    REXGPU_INFO("[shader library] {}: {} entries, translator signature {:016X}, open {:.2f} ms ({})",
                opened ? "loaded" : "unavailable; runtime fallback", library.size(), signature,
                open_ms, library.indexed() ? "compact index" : "record scan");
  }
  if (!opened) return false;
  const bool restored = library.Restore(translation, signature);
  if (restored) ++hits; else ++misses;
  if (hits + misses <= 8 || (hits + misses) % 64 == 0) {
    REXGPU_INFO("[shader library] {} hits, {} misses; last {:016X}/{:016X} {}",
                hits, misses, translation.shader().ucode_data_hash(), translation.modification(),
                restored ? "offline" : library.last_failure());
  }
  return restored;
}

}  // namespace rex::graphics
