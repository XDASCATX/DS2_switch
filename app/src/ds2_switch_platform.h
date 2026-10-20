// ds2 - small Switch-only queries kept out of the files that include rex headers (switch.h typedefs clash).
#pragma once
#include <cstdint>
#include <string>

namespace ds2::platform {
// Process memory as Horizon counts it (svcGetInfo UsedMemorySize / TotalMemorySize). false off the Switch.
bool HostMemory(uint64_t* used, uint64_t* total);
// Guest backing committed / mapped by the SDK's Switch guest memory (0 off the Switch).
uint64_t GuestCommitted();
uint64_t GuestMapped();
// Kernel-side accounting that can run out before memory does (2001-0103 "resource exhausted"):
//   blocks       memory blocks in this process's address space (svcQueryMemory walk; each separately mapped or
//                protected range is one block, and the kernel's block objects are a limited resource)
//   sysres_*     InfoType_SystemResourceSizeUsed/Total (page tables and memory management of the process)
//   last_gm      last result code of the SDK's guest-memory mapper (RexGmLastResult)
struct KernelStats {
  uint64_t blocks = 0, blocks_inaccessible = 0, sysres_used = 0, sysres_total = 0;
  uint32_t last_gm = 0;
  uint64_t coalesced = 0, coalesce_failures = 0;  // SDK view-run merges (guest_memory_switch.cpp)
  bool valid = false;
};
KernelStats QueryKernelStats();
// The same walk, as text: blocks grouped by (inside/outside the guest window, memory type, permission, attribute),
// largest groups first. For the 10 s snapshots, to see which kind of mapping multiplies.
std::string KernelBlockHistogram(size_t max_groups);

// SDK bridge that publishes FPS to SaltyNX overlays (sdk/src/ui/switch_saltynx.cpp). No-op off the Switch.
void SetSaltyNxBridge(bool enabled);
}  // namespace ds2::platform
