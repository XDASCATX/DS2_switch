#include "ds2_switch_platform.h"

#if defined(__SWITCH__)
#include <cstddef>
#include <switch.h>
extern "C" {
size_t RexGmCommittedBytes(void);
size_t RexGmMappedBytes(void);
void RexSwitchSaltyNxHabilitar(int habilitado);
uint32_t RexGmLastResult(void);
uint8_t* RexGmBase(void);
size_t RexGmCoalescedRuns(void);
size_t RexGmCoalesceFailures(void);
size_t RexGmSize(void);
}
#include <algorithm>
#include <cstdio>
#include <map>
#include <tuple>
#include <vector>
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
  k.coalesced = RexGmCoalescedRuns();
  k.coalesce_failures = RexGmCoalesceFailures();
  k.valid = true;
#endif
  return k;
}

std::string KernelBlockHistogram(size_t max_groups) {
#if defined(__SWITCH__)
  const u64 gb = reinterpret_cast<u64>(RexGmBase()), ge = gb + RexGmSize();
  std::map<std::tuple<int, u32, u32, u32>, std::pair<u64, u64>> groups;  // -> (blocks, bytes)
  u64 addr = 0;
  for (int guard = 0; guard < 1000000; ++guard) {
    MemoryInfo mi{};
    u32 pi = 0;
    if (R_FAILED(svcQueryMemory(&mi, &pi, addr))) break;
    const int in_window = (gb && mi.addr >= gb && mi.addr < ge) ? 1 : 0;
    auto& g = groups[{in_window, mi.type & 0xFF, mi.perm, mi.attr}];
    g.first += 1;
    g.second += mi.size;
    const u64 next = mi.addr + mi.size;
    if (next <= addr || next == 0) break;
    addr = next;
  }
  std::vector<std::pair<std::tuple<int, u32, u32, u32>, std::pair<u64, u64>>> v(groups.begin(), groups.end());
  std::sort(v.begin(), v.end(), [](const auto& a, const auto& b) { return a.second.first > b.second.first; });
  std::string out;
  char buf[160];
  for (size_t i = 0; i < v.size() && i < max_groups; ++i) {
    const auto& [key, val] = v[i];
    std::snprintf(buf, sizeof(buf), "%s%s type=0x%02X perm=%u attr=0x%X: %llu blocks %llu MB", i ? "; " : "",
                  std::get<0>(key) ? "window" : "other", std::get<1>(key), std::get<2>(key), std::get<3>(key),
                  (unsigned long long)val.first, (unsigned long long)(val.second >> 20));
    out += buf;
  }
  return out;
#else
  (void)max_groups;
  return {};
#endif
}

void SetSaltyNxBridge(bool enabled) {
#if defined(__SWITCH__)
  RexSwitchSaltyNxHabilitar(enabled ? 1 : 0);
#else
  (void)enabled;
#endif
}

}  // namespace ds2::platform
