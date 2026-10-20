// ds2 - small Switch-only queries kept out of the files that include rex headers (switch.h typedefs clash).
#pragma once
#include <cstdint>

namespace ds2::platform {
// Process memory as Horizon counts it (svcGetInfo UsedMemorySize / TotalMemorySize). false off the Switch.
bool HostMemory(uint64_t* used, uint64_t* total);
// Guest backing committed / mapped by the SDK's Switch guest memory (0 off the Switch).
uint64_t GuestCommitted();
uint64_t GuestMapped();
// SDK bridge that publishes FPS to SaltyNX overlays (sdk/src/ui/switch_saltynx.cpp). No-op off the Switch.
void SetSaltyNxBridge(bool enabled);
}  // namespace ds2::platform
