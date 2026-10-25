#include <rex/graphics/native/upload_content_cache.h>
#include <array>
#include <cassert>
#include <cstring>

int main() {
  rex::graphics::native::UploadContentCache cache;
  std::array<uint8_t, 256> mapping = {};
  const std::array<uint8_t, 4> original = {1, 2, 3, 4};
  std::memcpy(mapping.data() + 64, original.data(), original.size());
  uint64_t offset = 0;
  assert(!cache.Find(123, 1, original.data(), 4, mapping.data(), 128, offset));
  cache.Insert(123, 1, 4, 64);
  assert(cache.Find(123, 1, original.data(), 4, mapping.data(), 128, offset) && offset == 64);
  assert(!cache.Find(123, 2, original.data(), 4, mapping.data(), 128, offset));
  assert(!cache.Find(123, 1, original.data(), 4, mapping.data(), 66, offset));
  auto changed = original;
  changed[2] = 99;
  // Hash collision / same address mutated during the frame cannot return old data.
  assert(!cache.Find(123, 1, changed.data(), 4, mapping.data(), 128, offset));
  cache.Insert(123, 1, 4, 128);
  assert(cache.Find(123, 1, original.data(), 4, mapping.data(), 128, offset) && offset == 64);
  cache.Reset();
  assert(!cache.Find(123, 1, original.data(), 4, mapping.data(), 128, offset));
}
