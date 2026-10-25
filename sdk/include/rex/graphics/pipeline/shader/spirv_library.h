#pragma once

#include <array>
#include <cstdio>
#include <filesystem>
#include <map>

#include <rex/graphics/pipeline/shader/spirv.h>

namespace rex::graphics {

class SpirvShaderLibrary {
 public:
  ~SpirvShaderLibrary();
  bool Open(const std::filesystem::path& path, uint64_t expected_signature = 0);
  bool Restore(Shader::Translation& translation, uint64_t signature);
  bool WriteIndex(const std::filesystem::path& path) const;
  bool indexed() const { return indexed_; }
  const char* last_failure() const { return last_failure_; }
  static bool Begin(FILE* file);
  static bool Append(FILE* file, const Shader::Translation& translation, uint64_t signature);
  size_t size() const { return entries_.size(); }

 private:
  using Key = std::array<uint64_t, 4>;
  bool ReadIndex(const std::filesystem::path& path);
  FILE* file_ = nullptr;
  uint64_t file_size_ = 0;
  bool indexed_ = false;
  uint64_t file_signature_ = 0;
  const char* last_failure_ = "key not found";
  std::map<Key, uint64_t> entries_;
};

bool RestoreConfiguredSpirvLibrary(Shader::Translation& translation, uint64_t signature);

}  // namespace rex::graphics
