// ds2 - the game's Direct3D GPU wait sleeps instead of spinning (v122).
//
// Direct3D's waits for the GPU (the ring read pointer to pass a point, e.g. when the CPU is a full ring ahead) loop
// on one iteration function, sub_829E9E28 (6 callers). r3 = a wait record: [0] the device, [8] the read pointer
// seen last time ([[device+11024]] is the word the GPU writes the read pointer back to), [12] the tick of the last
// change. One iteration: a 4-step delay loop (32 db16cyc on the console, nothing here), then: GPU declared hung
// (bit 0x02 of [device+11069]) -> 0; the read pointer moved -> remember it; no movement for 5 s -> the hang check
// sub_829F13A8; else 1 (keep waiting). On the console the delays make it a gentle wait; recompiled it is a hot spin
// that keeps one of the Switch's three cores busy while the GPU command thread, which the game is waiting for,
// needs that core. ds2scholar-nx measured this wait at 28 % of the main thread's samples when the GPU is the limit
// (its ds2_cpu_gpu_wait_sleep; same SotFS executable, same address, same device offsets).
//
// What this does: before an iteration in which nothing can have changed - the same record as the previous iteration
// on this thread, the read pointer still the one seen then, and no command processor progress since that iteration
// returned (rex::graphics::Ds2RingProgress: bumped when the read pointer is published, at the end of each ring
// segment and after each interrupt) - it sleeps until the next progress, at most ds2_gpu_wait_max_us. Then the
// original iteration runs unchanged. The game reads the same memory and takes the same decisions, at most one
// wake-up later.
//
// The direct calls to sub_829E9E28 in the generated code must call sub_829E9E28 (not __imp__sub_829E9E28) for this
// hook to see them: the address below keeps it out of tools' direct-call rewriting (any 82XXXXXX in app/src counts
// as hooked).

#include <rex/cvar.h>
#include <rex/hook.h>
#include <rex/logging.h>

#include <atomic>
#include <cstdint>

REXCVAR_DEFINE_BOOL(ds2_gpu_wait_sleep, true, "DS2",
                    "v122: the game's Direct3D GPU wait (sub_829E9E28) sleeps until the command processor makes "
                    "progress instead of spinning; false = spins as before");
REXCVAR_DEFINE_INT32(ds2_gpu_wait_max_us, 1000, "DS2",
                     "ds2_gpu_wait_sleep: longest sleep per iteration of the game's GPU wait, in microseconds");

namespace rex::graphics {
uint32_t Ds2RingProgress();
void Ds2WaitRingProgress(uint32_t seen, uint32_t max_us);
}  // namespace rex::graphics

extern "C" void __imp__sub_829E9E28(PPCContext& __restrict ctx, uint8_t* base);

namespace {

constexpr uint32_t kDeviceReadPointerWord = 11024;

inline uint32_t Ld(const uint8_t* base, uint32_t a) {
  if (a < 0x10000u) return 0;
  return __builtin_bswap32(*reinterpret_cast<const uint32_t*>(base + a));
}

struct WaitState {
  uint32_t record = 0;
  uint32_t read_pointer = 0;
  uint32_t progress = 0;
  bool valid = false;
};
thread_local WaitState t_wait;
std::atomic<uint64_t> g_sleeps{0};
std::atomic<bool> g_logged{false};

uint32_t ReadPointer(const uint8_t* base, uint32_t record) {
  const uint32_t device = Ld(base, record);
  const uint32_t word = device ? Ld(base, device + kDeviceReadPointerWord) : 0;
  return word ? Ld(base, word) : 0;
}

}  // namespace

REX_HOOK_RAW(sub_829E9E28) {
  const uint32_t record = ctx.r3.u32;
  static const bool enabled = REXCVAR_GET(ds2_gpu_wait_sleep);
  if (!enabled) {
    __imp__sub_829E9E28(ctx, base);
    return;
  }
  if (!g_logged.exchange(true)) {
    REXLOG_INFO("ds2: GPU wait sleep on (sub_829E9E28 sleeps until the command processor makes progress, at most "
                "{} us)",
                REXCVAR_GET(ds2_gpu_wait_max_us));
  }
  WaitState& w = t_wait;
  if (w.valid && w.record == record && rex::graphics::Ds2RingProgress() == w.progress &&
      ReadPointer(base, record) == w.read_pointer) {
    static const uint32_t max_us = uint32_t(REXCVAR_GET(ds2_gpu_wait_max_us) < 50 ? 50 : REXCVAR_GET(ds2_gpu_wait_max_us));
    rex::graphics::Ds2WaitRingProgress(w.progress, max_us);
    g_sleeps.fetch_add(1, std::memory_order_relaxed);
  }
  __imp__sub_829E9E28(ctx, base);
  // Read after the iteration returned, before its caller re-checks: a change the caller may miss shows up as
  // progress, and the next iteration does not sleep.
  w.progress = rex::graphics::Ds2RingProgress();
  w.read_pointer = ReadPointer(base, record);
  w.record = record;
  w.valid = (ctx.r3.u32 != 0);  // 0 = the wait ends (GPU hung): start fresh next time
}
