#include "ds2_switch_platform.h"

#if defined(__SWITCH__)
#include <cstddef>
#include <switch.h>
extern "C" {
size_t RexGmCommittedBytes(void);
size_t RexGmMappedBytes(void);
void RexSwitchSaltyNxHabilitar(int habilitado);
uint32_t RexGmLastResult(void);
}
#endif

namespace ds2::platform {

bool HostMemory(uint64_t* used, uint64_t* total) {
#if defined(__SWITCH__)
  u64 u = 0, t = 0;
  if (R_FAILED(svcGetInfo(&u, InfoType_UsedMemorySize, CUR_PROCESS_HANDLE, 0)) ||
      R_FAILED(svcGetInfo(&t, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0)))
    return false;
  *used = u;
  *total = t;
  return true;
#else
  (void)used;
  (void)total;
  return false;
#endif
}

uint64_t GuestCommitted() {
#if defined(__SWITCH__)
  return RexGmCommittedBytes();
#else
  return 0;
#endif
}

uint64_t GuestMapped() {
#if defined(__SWITCH__)
  return RexGmMappedBytes();
#else
  return 0;
#endif
}

KernelStats QueryKernelStats() {
  KernelStats k;
#if defined(__SWITCH__)
  u64 v = 0;
  if (R_SUCCEEDED(svcGetInfo(&v, InfoType_SystemResourceSizeUsed, CUR_PROCESS_HANDLE, 0))) k.sysres_used = v;
  if (R_SUCCEEDED(svcGetInfo(&v, InfoType_SystemResourceSizeTotal, CUR_PROCESS_HANDLE, 0))) k.sysres_total = v;
  u64 addr = 0;
  for (int guard = 0; guard < 1000000; ++guard) {
    MemoryInfo mi{};
    u32 pi = 0;
    if (R_FAILED(svcQueryMemory(&mi, &pi, addr))) break;
    ++k.blocks;
    if (mi.type != MemType_Unmapped && (mi.perm & Perm_Rw) == 0) ++k.blocks_inaccessible;
    const u64 next = mi.addr + mi.size;
    if (next <= addr || next == 0) break;
    addr = next;
  }
  k.last_gm = RexGmLastResult();
  k.valid = true;
#endif
  return k;
}

void SetSaltyNxBridge(bool enabled) {
#if defined(__SWITCH__)
  RexSwitchSaltyNxHabilitar(enabled ? 1 : 0);
#else
  (void)enabled;
#endif
}

}  // namespace ds2::platform
