// ds2 - DLC package installation, see ds2_dlc.h.

#include "ds2_dlc.h"

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xam/content_manager.h>
#include <rex/system/xam/install_progress.h>
#include <rex/system/xcontent.h>
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/imgui_drawer.h>
#include <rex/ui/souls_style.h>

#include <imgui.h>
#include <imgui_internal.h>  // ImRect

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <initializer_list>
#include <memory>
#include <thread>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

REXCVAR_DEFINE_STRING(ds2_install_dlc, "", "DS2",
                      "STFS content packages to install at startup if missing, separated by ';' "
                      "(e.g. Compatibility Pack 4 or Calibration -GAME_0000). Relative paths are taken from the executable folder. "
                      "Empty: every file of <executable folder>/dlc/ is installed.")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

REXCVAR_DEFINE_BOOL(ds2_resolution_prompt, true, "DS2",
                    "v107: ask for the internal resolution (720p / 540p) on the start screen; off = ds2_render_scale");
REXCVAR_DEFINE_STRING(ds2_calibration_file, "", "DS2",
                      "RC92: the game's online calibration (the regulation the 360 downloads into the save, file "
                      "USER_DATA15 of the -GAME_0000 save, e.g. Calibration 1.15 of the release). At start it is copied "
                      "into every profile's -GAME_0000 save whose USER_DATA15 differs; the old one is kept once as "
                      "USER_DATA15.before_calibration. Relative to the executable folder. Empty: off.")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

namespace ds2::dlc {

namespace {

bool ReadAll(const std::filesystem::path& p, std::vector<char>& out) {
  FILE* f = std::fopen(rex::path_to_utf8(p).c_str(), "rb");
  if (!f) return false;
  out.clear();
  char buf[65536];
  size_t n;
  while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) out.insert(out.end(), buf, buf + n);
  std::fclose(f);
  return true;
}

bool WriteAll(const std::filesystem::path& p, const std::vector<char>& data) {
  const auto tmp = std::filesystem::path(rex::path_to_utf8(p) + ".tmp");
  FILE* f = std::fopen(rex::path_to_utf8(tmp).c_str(), "wb");
  if (!f) return false;
  const bool ok = std::fwrite(data.data(), 1, data.size(), f) == data.size();
  if (std::fclose(f) != 0 || !ok) return false;
  std::error_code ec;
  std::filesystem::remove(p, ec);
  std::filesystem::rename(tmp, p, ec);
  return !ec;
}

// The calibration is a zlib-compressed regulation BND4 behind two big-endian sizes (compressed, uncompressed).
void ImportCalibration(const std::filesystem::path& exe_dir) {
  std::string name = REXCVAR_GET(ds2_calibration_file);
  if (name.empty()) return;
  std::filesystem::path src(name);
  if (src.is_relative()) src = exe_dir / src;
  std::vector<char> calib;
  if (!ReadAll(src, calib) || calib.size() < 16 || uint8_t(calib[8]) != 0x78) {
    REXLOG_WARN("[ds2 calibration] {}: missing or not a calibration file", rex::path_to_utf8(src));
    return;
  }
  std::error_code ec;
  int copied = 0, same = 0;
  for (const auto& profile : std::filesystem::directory_iterator(exe_dir / "user", ec)) {
    if (ec) break;
    const auto save = profile.path() / "465307E4" / "00000001" / "-GAME_0000";
    std::error_code ec2;
    if (!std::filesystem::is_directory(save, ec2)) continue;
    const auto target = save / "USER_DATA15";
    std::vector<char> current;
    if (ReadAll(target, current) && current == calib) {
      ++same;
      continue;
    }
    const auto backup = save / "USER_DATA15.before_calibration";
    if (!current.empty() && !std::filesystem::exists(backup, ec2)) WriteAll(backup, current);
    if (WriteAll(target, calib)) {
      ++copied;
      REXLOG_INFO("[ds2 calibration] {} -> {} ({} bytes; old one kept as {})", rex::path_to_utf8(src),
                  rex::path_to_utf8(target), calib.size(), rex::path_to_utf8(backup));
    } else {
      REXLOG_WARN("[ds2 calibration] could not write {}", rex::path_to_utf8(target));
    }
  }
  REXLOG_INFO("[ds2 calibration] {}: {} save(s) updated, {} already had it", rex::path_to_utf8(src), copied, same);
}

}  // namespace

std::atomic<uint32_t> g_package_index{0};
std::atomic<uint32_t> g_package_count{0};

void InstallConfigured(rex::Runtime* runtime) {
  using rex::X_RESULT;
  if (!runtime || !runtime->kernel_state()) {
    return;
  }
  const auto exe_dir = rex::filesystem::GetExecutableFolder();
  if (!exe_dir.empty()) ImportCalibration(exe_dir);
  std::string list = REXCVAR_GET(ds2_install_dlc);
  if (list.empty() && !exe_dir.empty()) {
    // Portable layout (sdmc:/switch/ds2/dlc/ on the Switch): install whatever packages were copied there.
    std::error_code ec;
    for (const auto& e : std::filesystem::directory_iterator(exe_dir / "dlc", ec)) {
      if (ec) break;
      // Skip hidden files: macOS leaves "._<name>" AppleDouble copies next to every file it writes to a FAT card.
      if (e.path().filename().string().rfind('.', 0) == 0) continue;
      if (e.is_regular_file(ec)) list += (list.empty() ? "" : ";") + rex::path_to_utf8(e.path());
    }
  }
  if (list.empty()) {
    return;
  }
  auto* content = runtime->kernel_state()->content_manager();
  if (!content) {
    REXLOG_WARN("[ds2 dlc] no content manager");
    return;
  }
  {
    uint32_t n = 0;
    for (char c : list) n += c == ';';
    g_package_count.store(n + 1, std::memory_order_relaxed);
    g_package_index.store(0, std::memory_order_relaxed);
  }
  size_t start = 0;
  while (start <= list.size()) {
    size_t end = list.find(';', start);
    if (end == std::string::npos) end = list.size();
    const std::string item = list.substr(start, end - start);
    start = end + 1;
    if (item.empty()) continue;
    std::filesystem::path path(item);
    if (path.is_relative() && !exe_dir.empty()) path = exe_dir / path;
    const std::string name = path.filename().string();
    g_package_index.fetch_add(1, std::memory_order_relaxed);
    REXLOG_INFO("[ds2 dlc] installing {} (first boot only: the package is unpacked onto the SD card, which takes "
                "minutes on the console)", rex::path_to_utf8(path));
    rex::FlushLogging();
    const auto t0 = std::chrono::steady_clock::now();
    const auto result = content->InstallContent(path);
    if (result == X_ERROR_ALREADY_EXISTS) {
      REXLOG_INFO("[ds2 dlc] {} already installed", name);
    } else {
      REXLOG_INFO("[ds2 dlc] install {} -> {:08X} in {:.1f} s", rex::path_to_utf8(path), uint32_t(result),
                  std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
    }
  }
}

namespace {

// v101: progress of the first-boot unpacking, centred on the screen. The bar shows only while a package is really
// being extracted (an installed package is skipped in about a second without one).
class InstallDialog final : public rex::ui::ImGuiDialog {
 public:
  explicit InstallDialog(rex::ui::ImGuiDrawer* drawer) : ImGuiDialog(drawer) {}

 protected:
  void OnDraw(ImGuiIO& io) override {
    namespace souls = rex::ui::souls;
    auto& progress = rex::system::xam::install_progress();
    const uint64_t total = progress.bytes_total.load(std::memory_order_relaxed);
    if (!total) return;
    const uint64_t done = std::min(progress.bytes_done.load(std::memory_order_relaxed), total);
    const float fraction = float(double(done) / double(total));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.6f, 0.0f), ImGuiCond_Always);
    souls::PushWindowStyle();
    const ImGuiWindowFlags flags = souls::kWindowFlags | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
                                   ImGuiWindowFlags_NoInputs;
    if (ImGui::Begin("##ds2_install", nullptr, flags)) {
      souls::DrawFrame();
      // v109: Dark Souls look (rex/ui/souls_style.h).
      souls::CenteredTitle("Installing Game Data", 1.0f, souls::kParchment);
      char line[64];
      std::snprintf(line, sizeof(line), "Package %u of %u", g_package_index.load(std::memory_order_relaxed),
                    g_package_count.load(std::memory_order_relaxed));
      souls::CenteredTitle(line, 0.6f, souls::kAsh);
      souls::Divider();
      char label[32];
      std::snprintf(label, sizeof(label), "%.1f%%", double(fraction) * 100.0);
      ImGui::PushStyleColor(ImGuiCol_PlotHistogram, souls::kEmber);
      ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.08f, 0.06f, 1.0f));
      ImGui::PushStyleColor(ImGuiCol_Border, souls::kBronze);
      ImGui::PushStyleColor(ImGuiCol_Text, souls::kParchment);
      ImGui::SetWindowFontScale(2.0f);
      ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f), label);
      ImGui::SetWindowFontScale(1.0f);
      ImGui::PopStyleColor(4);
      std::snprintf(line, sizeof(line), "%.1f / %.1f MB", double(done) / 1048576.0, double(total) / 1048576.0);
      souls::CenteredTitle(line, 0.55f, souls::kAsh);
    }
    ImGui::End();
    souls::PopWindowStyle();
  }
};

std::unique_ptr<InstallDialog> g_dialog;
std::thread g_install_thread;

// v107: start screen with the internal resolution of the scene (ds2_render_scale): two buttons in the centre of the
// screen, driven with the controller (D-pad + A) or the touch screen.
// v116: the choices of the start screen, kept on the SD card for the next start.
struct StartPrefs {
  bool valid = false;
  double scale = 1.0;
  int fps = 60;
  int language = 0;  // 0 = not saved
  bool sharpen = true;
};
constexpr const char* kStartPrefsPath = "sdmc:/switch/ds2/start_screen.cfg";

StartPrefs LoadStartPrefs() {
  StartPrefs p;
  FILE* f = std::fopen(kStartPrefsPath, "r");
  if (!f) return p;
  char line[64];
  while (std::fgets(line, sizeof(line), f)) {
    double scale;
    int v;
    if (std::sscanf(line, "scale=%lf", &scale) == 1) {
      p.scale = scale < 1.0 ? 0.75 : 1.0;
      p.valid = true;
    } else if (std::sscanf(line, "fps=%d", &v) == 1) {
      p.fps = v == 30 ? 30 : 60;
    } else if (std::sscanf(line, "lang=%d", &v) == 1) {
      p.language = v;
    } else if (std::sscanf(line, "sharpen=%d", &v) == 1) {
      p.sharpen = v != 0;
    }
  }
  std::fclose(f);
  return p;
}

void SaveStartPrefs(const StartPrefs& p) {
  FILE* f = std::fopen(kStartPrefsPath, "w");
  if (!f) return;
  std::fprintf(f, "scale=%.2f\nfps=%d\nlang=%d\nsharpen=%d\n", p.scale, p.fps, p.language, p.sharpen ? 1 : 0);
  std::fclose(f);
}

#ifndef DS2_VERSION_STRING
#define DS2_VERSION_STRING ""
#endif

// v109: the start screen (v107 resolution, v113 frame rate lock, v114 language; v116: remembered choices with a
// countdown, ember background, version, button glyphs, sharpening for 540p).
class ResolutionDialog final : public rex::ui::ImGuiDialog {
 public:
  ResolutionDialog(rex::ui::ImGuiDrawer* drawer, std::function<void(const StartPrefs&)> on_choice)
      : ImGuiDialog(drawer), on_choice_(std::move(on_choice)) {
    const StartPrefs saved = LoadStartPrefs();
    REXLOG_INFO("ds2: start screen: remembered choice {} (scale {:.2f}, {} FPS lock, language {}, sharpening {})",
                saved.valid ? "found" : "none", saved.scale, saved.fps, saved.language, saved.sharpen);
    toml_language_ = TomlLanguage();
    language_ = (toml_language_ == 1 || OtherLanguageName(toml_language_)) ? toml_language_ : 12;
    if (saved.language == 1 || saved.language == 12 || OtherLanguageName(saved.language)) {
      language_ = saved.language;
      if (OtherLanguageName(saved.language)) toml_language_ = saved.language;
    }
    fps_ = saved.fps;
    sharpen_ = saved.sharpen;
    remembered_ = saved.valid;
    scale_ = saved.valid ? saved.scale : 1.0;
    focus_ = scale_ >= 1.0 ? kCard720 : kCard540;  // v131: one A starts with the remembered choice
  }

 protected:
  // v128: the start screen drawn 1:1 from the mock-up (artifact "DS2 Start Screen", 1280 x 720 px, Cinzel). Every
  // position and size below is in mock-up pixels and scaled by k_ = display width / 1280.
  void OnDraw(ImGuiIO& io) override {
    if (chosen_) return;
    time_ += io.DeltaTime;
    k_ = io.DisplaySize.x / 1280.0f;
    // Countdown: any button or touch stops it.
    // v118: only a new press stops it, after a short grace: the A that launched the port can still be held
    // when the screen appears (v117 stopped the countdown in its first frame), and stick noise is ignored.
    if (counting_ && time_ > 0.6f) {
      for (int k = ImGuiKey_GamepadStart; k <= ImGuiKey_GamepadR3; ++k) {
        if (ImGui::IsKeyPressed(ImGuiKey(k), false)) counting_ = false;
      }
      for (int k = ImGuiKey_GamepadLStickLeft; k <= ImGuiKey_GamepadRStickDown; ++k) {
        if (ImGui::IsKeyPressed(ImGuiKey(k), false) && ImGui::GetKeyData(ImGuiKey(k))->AnalogValue > 0.5f) {
          counting_ = false;
        }
      }
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) counting_ = false;
    }
    // v131: the screen's own focus and input instead of ImGui navigation. On the console ImGui's focus ended on the
    // 60 button whatever was requested (v128-v130), so A did not start the game. Here the focus starts on the
    // remembered card, the D-pad / left stick move it along a fixed map, A activates the focused control, and a
    // touch activates the control under the finger.
    double choice = 0.0;
    auto activate = [&](int item) {
      switch (item) {
        case kRus: language_ = 12; break;
        case kEng: language_ = 1; break;
        case kOther: language_ = toml_language_; break;
        case kCard720: choice = 1.0; break;
        case kCard540: choice = 0.75; break;
        case kSharpen: sharpen_ = !sharpen_; break;
        case kFps30: fps_ = 30; break;
        case kFps60: fps_ = 60; break;
        default: break;
      }
    };
    {
      // A = accept as in ImGui's menus (switch_imgui_input.h: by letter A is the right face button).
      const ImGuiKey accept = io.ConfigNavSwapGamepadButtons ? ImGuiKey_GamepadFaceRight : ImGuiKey_GamepadFaceDown;
      if (!ImGui::IsKeyDown(accept)) armed_ = true;  // the A that launched the port must be released first
      int dir = -1;
      if (ImGui::IsKeyPressed(ImGuiKey_GamepadDpadLeft, true) || ImGui::IsKeyPressed(ImGuiKey_GamepadLStickLeft, true)) dir = 0;
      if (ImGui::IsKeyPressed(ImGuiKey_GamepadDpadRight, true) || ImGui::IsKeyPressed(ImGuiKey_GamepadLStickRight, true)) dir = 1;
      if (ImGui::IsKeyPressed(ImGuiKey_GamepadDpadUp, true) || ImGui::IsKeyPressed(ImGuiKey_GamepadLStickUp, true)) dir = 2;
      if (ImGui::IsKeyPressed(ImGuiKey_GamepadDpadDown, true) || ImGui::IsKeyPressed(ImGuiKey_GamepadLStickDown, true)) dir = 3;
      if (dir >= 0) focus_ = Move(focus_, dir);
      if (armed_ && ImGui::IsKeyPressed(accept, false)) activate(focus_);
      if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const ImVec2 m = io.MousePos;
        for (int item = 0; item < kItems; ++item) {
          if (item == kOther && !OtherLanguageName(toml_language_)) continue;
          const ImVec4 r = Rect(item);
          if (m.x >= r.x * k_ && m.x < r.z * k_ && m.y >= r.y * k_ && m.y < r.w * k_) {
            focus_ = item;
            activate(item);
            break;  // the sharpening toggle comes before the 540p card it sits in
          }
        }
      }
    }
    DrawBackground(ImGui::GetBackgroundDrawList(), io.DisplaySize);

    // One window over the whole screen, only to draw in (no ImGui navigation).
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    if (ImGui::Begin("##ds2_resolution", nullptr,
                     rex::ui::souls::kWindowFlags | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus)) {
      ImDrawList* dl = ImGui::GetWindowDrawList();
      // Language (top left).
      Txt(dl, 28, 25, 13, 3, kLabel, "IN-GAME LANGUAGE");
      Pill(dl, "RUS", language_ == 12, focus_ == kRus, Rect(kRus), 14, 2);
      Pill(dl, "ENG", language_ == 1, focus_ == kEng, Rect(kEng), 14, 2);
      if (const char* other = OtherLanguageName(toml_language_)) {
        Pill(dl, other, language_ == toml_language_, focus_ == kOther, Rect(kOther), 14, 2);
      }
      // Title.
      Txt(dl, 640, 53, 38, 12, IM_COL32(239, 226, 198, 255), "DARK SOULS II", 1, false, IM_COL32(255, 130, 40, 12));
      Txt(dl, 640, 97, 13, 7, kGold, "SCHOLAR OF THE FIRST SIN", 1);
      dl->AddRectFilledMultiColor(P(450, 123.5f), P(640 - 12, 124.5f), IM_COL32(176, 138, 79, 0), kBronzeLine,
                                  kBronzeLine, IM_COL32(176, 138, 79, 0));
      dl->AddRectFilledMultiColor(P(640 + 12, 123.5f), P(830, 124.5f), kBronzeLine, IM_COL32(176, 138, 79, 0),
                                  IM_COL32(176, 138, 79, 0), kBronzeLine);
      dl->AddCircleFilled(P(640, 124), 9.0f * k_, IM_COL32(255, 122, 26, 50), 16);
      rex::ui::souls::Diamond(dl, P(640, 124), 5.6f * k_, IM_COL32(255, 138, 42, 255));
      Txt(dl, 640, 161, 15, 6, kMuted, "CHOOSE RESOLUTION", 1);

      // The two cards: the focused one is lit; while the focus is elsewhere, the remembered one.
      const bool card_focus = focus_ == kCard720 || focus_ == kCard540;
      const bool on720 = focus_ == kCard720 || (!card_focus && scale_ >= 1.0);
      const bool on540 = focus_ == kCard540 || (!card_focus && scale_ < 1.0);
      CardFace(dl, 220, on720);
      CardFace(dl, 760, on540);
      if (remembered_) Txt(dl, scale_ >= 1.0 ? 370 : 910, 218, 11, 4, IM_COL32(255, 198, 122, 255), "LAST USED", 1);
      Txt(dl, 370, 292, 76, 0, on720 ? kBigOn : kBigOff, "720p", 1, false, on720 ? kBigGlow : 0);
      Txt(dl, 370, 353, 15, 4, kSand, "NATIVE", 1);
      Txt(dl, 370, 382, 13, 2, kMuted, "Sharp picture", 1);
      Txt(dl, 910, 270, 76, 0, on540 ? kBigOn : kBigOff, "540p", 1, false, on540 ? kBigGlow : 0);
      Txt(dl, 910, 330, 15, 4, kSand, "MORE FPS", 1);
      Txt(dl, 910, 359, 13, 2, kMuted, "Softer picture", 1);
      Pill(dl, sharpen_ ? "SHARPENING: ON" : "SHARPENING: OFF", sharpen_, focus_ == kSharpen, Rect(kSharpen), 13, 2);
      DrawBonfire(dl, P(520, 190), k_, time_);

      // Frame rate lock.
      Txt(dl, 644, 510, 16, 4, kLabel, "FRAME RATE LOCK", 2);
      Pill(dl, "30", fps_ == 30, focus_ == kFps30, Rect(kFps30), 18, 1);
      Pill(dl, "60", fps_ == 60, focus_ == kFps60, Rect(kFps60), 18, 1);

      // Countdown with the remembered choice: text and a draining ember bar.
      const float left = counting_ ? kCountdown - time_ : 0.0f;
      if (counting_ && left <= 0.0f && choice == 0.0) choice = scale_;
      if (counting_) {
        char num[8];
        std::snprintf(num, sizeof(num), "%d", int(left) + 1);
        const char* a = "Auto start in ";
        const char* b = " s  \xC2\xB7  any button to stay";
        const float wa = Width(a, 14, 2), wn = Width(num, 18, 0) + 0.7f * k_, wb = Width(b, 14, 2);
        const float x0 = (640.0f * k_ - (wa + wn + wb) * 0.5f) / k_;
        Txt(dl, x0, 568, 14, 2, kSand, a);
        Txt(dl, x0 + wa / k_, 568, 18, 0, IM_COL32(255, 183, 92, 255), num, 0, true);
        Txt(dl, x0 + (wa + wn) / k_, 568, 14, 2, kSand, b);
        const float frac = std::clamp(left / kCountdown, 0.0f, 1.0f);
        dl->AddRectFilled(P(430, 590), P(850, 593), IM_COL32(42, 32, 22, 255));
        const float xe = 430.0f + 420.0f * frac;
        for (int i = 3; i >= 1; --i) {
          const float g = float(i) * 2.5f;
          dl->AddRectFilled(P(430 - g, 590 - g), P(xe + g, 593 + g), IM_COL32(255, 122, 26, 22));
        }
        dl->AddRectFilledMultiColor(P(430, 590), P(xe, 593), IM_COL32(138, 60, 12, 255), IM_COL32(255, 138, 42, 255),
                                    IM_COL32(255, 138, 42, 255), IM_COL32(138, 60, 12, 255));
      }
      DrawHints(dl);
      first_frame_ = false;
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    if (choice > 0.0) {
      chosen_ = true;
      StartPrefs p;
      p.valid = true;
      p.scale = choice;
      p.fps = fps_;
      p.language = language_;
      p.sharpen = sharpen_;
      SaveStartPrefs(p);
      on_choice_(p);
    }
  }

 private:
  static constexpr float kCountdown = 20.0f;  // v121: 20 s (was 10)
  static constexpr ImU32 kMuted = IM_COL32(168, 154, 133, 255);       // #a89a85
  static constexpr ImU32 kLabel = IM_COL32(196, 181, 155, 255);       // small labels: a little lighter (v130)
  static constexpr ImU32 kSand = IM_COL32(216, 196, 156, 255);        // #d8c49c
  static constexpr ImU32 kGold = IM_COL32(201, 164, 104, 255);        // #c9a468
  static constexpr ImU32 kBronzeLine = IM_COL32(176, 138, 79, 255);   // #b08a4f
  static constexpr ImU32 kBigOn = IM_COL32(255, 226, 176, 255);       // #ffe2b0
  static constexpr ImU32 kBigOff = IM_COL32(185, 169, 140, 255);      // #b9a98c
  static constexpr ImU32 kBigGlow = IM_COL32(255, 140, 40, 16);
  // ImGui sizes a font by ascent + descent; Cinzel's are 976 + 372 per 1000 em, so CSS font-size px x 1.348.
  static constexpr float kCinzelEm = 1.348f;

  ImVec2 P(float x, float y) const { return ImVec2(x * k_, y * k_); }
  ImVec2 S(float w, float h) const { return ImVec2(w * k_, h * k_); }

  static int Utf8Len(unsigned char c) { return c < 0x80 ? 1 : (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3 : 4; }

  // Width in screen pixels of `t` in Cinzel at `px` mock-up pixels with `sp` px of letter spacing.
  float Width(const char* t, float px, float sp) const {
    rex::ui::souls::PushTitleFont(px * kCinzelEm * k_ / rex::ui::souls::kTitleFontSize);
    float w = 0.0f;
    int n = 0;
    for (const char* c = t; *c;) {
      const int len = Utf8Len(static_cast<unsigned char>(*c));
      w += ImGui::CalcTextSize(c, c + len).x;
      c += len;
      ++n;
    }
    rex::ui::souls::PopTitleFont();
    return w + sp * k_ * float(n > 0 ? n - 1 : 0);
  }

  // Text in Cinzel with letter spacing, its vertical centre at `yc`; align 0 = left at x, 1 = centred on x,
  // 2 = right edge at x. `bold` draws it twice, a glow colour draws a soft halo under it.
  void Txt(ImDrawList* dl, float x, float yc, float px, float sp, ImU32 col, const char* t, int align = 0,
           bool bold = false, ImU32 glow = 0) const {
    const float w = Width(t, px, sp);
    rex::ui::souls::PushTitleFont(px * kCinzelEm * k_ / rex::ui::souls::kTitleFontSize);
    const float h = ImGui::CalcTextSize("A").y;
    float X = x * k_;
    if (align == 1) X -= w * 0.5f;
    if (align == 2) X -= w;
    const float Y = yc * k_ - h * 0.5f;
    auto run = [&](float ox, float oy, ImU32 c) {
      float cx = X + ox;
      for (const char* p = t; *p;) {
        const int len = Utf8Len(static_cast<unsigned char>(*p));
        dl->AddText(ImVec2(cx, Y + oy), c, p, p + len);
        cx += ImGui::CalcTextSize(p, p + len).x + sp * k_;
        p += len;
      }
    };
    if (glow) {
      for (float r : {7.0f, 4.5f, 2.5f}) {
        for (int i = 0; i < 8; ++i) {
          const float a = float(i) * 0.785398f;
          run(std::cos(a) * r * k_, std::sin(a) * r * k_, glow);
        }
      }
    }
    run(0.0f, 0.0f, col);
    // v130: Cinzel's hairlines vanish at small sizes on the console: text up to 16 px is drawn twice, offset.
    if (bold || px <= 16.0f) run((px <= 16.0f ? 0.6f : 0.7f) * k_, 0.0f, col);
    rex::ui::souls::PopTitleFont();
  }

  // A soft glow around a rectangle: stacked outlines fading out over `spread` px.
  void Glow(ImDrawList* dl, ImVec2 a, ImVec2 b, int r, int g, int bl, float alpha, float spread) const {
    // Each step is a band (four rects) one step wide, so the bands meet without overlapping.
    const int steps = std::max(4, int(spread * k_ / 1.5f));
    const float w = spread * k_ / float(steps);
    for (int i = 0; i < steps; ++i) {
      const float d0 = w * float(i), d1 = w * float(i + 1);
      const float f = 1.0f - (float(i) + 0.5f) / float(steps);
      const ImU32 c = IM_COL32(r, g, bl, int(255.0f * alpha * f * f));
      dl->AddRectFilled(ImVec2(a.x - d1, a.y - d1), ImVec2(b.x + d1, a.y - d0), c);
      dl->AddRectFilled(ImVec2(a.x - d1, b.y + d0), ImVec2(b.x + d1, b.y + d1), c);
      dl->AddRectFilled(ImVec2(a.x - d1, a.y - d0), ImVec2(a.x - d0, b.y + d0), c);
      dl->AddRectFilled(ImVec2(b.x + d0, a.y - d0), ImVec2(b.x + d1, b.y + d0), c);
    }
  }

  // A small button: lit (ember gradient, warm border, glow) when `on`, dark otherwise; a ring shows the focus.
  // The controls: what is focusable, where it is (x0, y0, x1, y1 in mock-up px) and where the D-pad leads.
  enum Item { kRus, kEng, kOther, kSharpen, kCard720, kCard540, kFps30, kFps60, kItems };
  static ImVec4 Rect(int item) {
    switch (item) {
      case kRus: return ImVec4(28, 42, 92, 86);
      case kEng: return ImVec4(100, 42, 164, 86);
      case kOther: return ImVec4(172, 42, 236, 86);
      case kSharpen: return ImVec4(800, 394, 1020, 438);
      case kCard720: return ImVec4(220, 190, 520, 460);
      case kCard540: return ImVec4(760, 190, 1060, 460);
      case kFps30: return ImVec4(666, 488, 738, 532);
      case kFps60: return ImVec4(752, 488, 824, 532);
      default: return ImVec4(0, 0, 0, 0);
    }
  }
  // dir: 0 left, 1 right, 2 up, 3 down.
  int Move(int item, int dir) const {
    const bool other = OtherLanguageName(toml_language_) != nullptr;
    switch (item) {
      case kRus: return dir == 1 ? kEng : dir == 3 ? kCard720 : item;
      case kEng: return dir == 0 ? kRus : (dir == 1 && other) ? kOther : dir == 3 ? kCard720 : item;
      case kOther: return dir == 0 ? kEng : dir == 3 ? kCard720 : item;
      case kCard720: return dir == 1 ? kCard540 : dir == 2 ? kRus : dir == 3 ? kFps30 : item;
      case kCard540: return dir == 0 ? kCard720 : dir == 2 ? kEng : dir == 3 ? kSharpen : item;
      case kSharpen: return dir == 0 ? kCard720 : dir == 2 ? kCard540 : dir == 3 ? kFps60 : item;
      case kFps30: return dir == 1 ? kFps60 : dir == 2 ? kCard720 : item;
      case kFps60: return dir == 0 ? kFps30 : dir == 2 ? kSharpen : item;
      default: return item;
    }
  }

  // A small button: lit (ember gradient, warm border, glow) when `on`, dark otherwise; a ring shows the focus.
  void Pill(ImDrawList* dl, const char* text, bool on, bool hot, ImVec4 r, float px, float sp) const {
    const ImVec2 a = P(r.x, r.y), b = P(r.z, r.w);
    if (on) {
      Glow(dl, a, b, 255, 120, 24, 0.4f, 16.0f);
      dl->AddRectFilledMultiColor(a, b, IM_COL32(58, 26, 7, 255), IM_COL32(58, 26, 7, 255), IM_COL32(138, 60, 12, 255),
                                  IM_COL32(138, 60, 12, 255));
      dl->AddRect(a, b, IM_COL32(231, 172, 95, 255), 0.0f, 0, 1.2f * k_);
    } else {
      dl->AddRectFilled(a, b, IM_COL32(14, 11, 8, 230));
      dl->AddRect(a, b, hot ? IM_COL32(176, 138, 79, 255) : IM_COL32(74, 58, 38, 255), 0.0f, 0, 1.2f * k_);
    }
    if (hot) {
      const float d = 4.0f * k_;
      dl->AddRect(ImVec2(a.x - d, a.y - d), ImVec2(b.x + d, b.y + d), IM_COL32(255, 198, 122, 210), 0.0f, 0, 1.5f * k_);
    }
    Txt(dl, (r.x + r.z) * 0.5f, (r.y + r.w) * 0.5f, px, sp,
        on ? IM_COL32(255, 226, 176, 255) : hot ? IM_COL32(230, 216, 189, 255) : kMuted, text, 1);
  }

  // A resolution card (300 x 270 at x, 190): lit = ember gradient with a pulsing glow, else dark with bronze ticks.
  void CardFace(ImDrawList* dl, float x, bool on) const {
    const ImVec2 a = P(x, 190), b = P(x + 300, 460);
    const float ym = (190.0f + 270.0f * 0.58f) * k_;
    if (on) {
      const float pulse = 0.5f + 0.5f * std::sin(time_ * 6.2831853f / 2.4f);
      Glow(dl, a, b, 255, 125, 27, 0.30f + 0.30f * pulse, 46.0f);
      dl->AddRectFilledMultiColor(a, ImVec2(b.x, ym), IM_COL32(22, 14, 8, 245), IM_COL32(22, 14, 8, 245),
                                  IM_COL32(64, 26, 6, 245), IM_COL32(64, 26, 6, 245));
      dl->AddRectFilledMultiColor(ImVec2(a.x, ym), b, IM_COL32(64, 26, 6, 245), IM_COL32(64, 26, 6, 245),
                                  IM_COL32(150, 62, 10, 245), IM_COL32(150, 62, 10, 245));
      const int inner = int(255.0f * (0.16f + 0.18f * pulse));
      dl->AddRectFilledMultiColor(ImVec2(a.x, b.y - 90.0f * k_), b, IM_COL32(255, 150, 50, 0), IM_COL32(255, 150, 50, 0),
                                  IM_COL32(255, 150, 50, inner), IM_COL32(255, 150, 50, inner));
      dl->AddRect(a, b, IM_COL32(231, 172, 95, 255), 0.0f, 0, 1.2f * k_);
    } else {
      dl->AddRectFilledMultiColor(a, b, IM_COL32(16, 12, 9, 240), IM_COL32(16, 12, 9, 240), IM_COL32(10, 8, 6, 240),
                                  IM_COL32(10, 8, 6, 240));
      dl->AddRect(a, b, IM_COL32(74, 58, 38, 255), 0.0f, 0, 1.2f * k_);
    }
    const ImU32 tick = on ? IM_COL32(255, 198, 122, 255) : IM_COL32(138, 108, 62, 255);
    const float t = 2.0f * k_, l = 16.0f * k_, in = 8.0f * k_;
    const ImVec2 c[4] = {ImVec2(a.x + in, a.y + in), ImVec2(b.x - in, a.y + in), ImVec2(a.x + in, b.y - in),
                         ImVec2(b.x - in, b.y - in)};
    const float sx[4] = {1, -1, 1, -1}, sy[4] = {1, 1, -1, -1};
    for (int i = 0; i < 4; ++i) {
      dl->AddRectFilled(ImVec2(std::min(c[i].x, c[i].x + sx[i] * l), std::min(c[i].y, c[i].y + sy[i] * t)),
                        ImVec2(std::max(c[i].x, c[i].x + sx[i] * l), std::max(c[i].y, c[i].y + sy[i] * t)), tick);
      dl->AddRectFilled(ImVec2(std::min(c[i].x, c[i].x + sx[i] * t), std::min(c[i].y, c[i].y + sy[i] * l)),
                        ImVec2(std::max(c[i].x, c[i].x + sx[i] * t), std::max(c[i].y, c[i].y + sy[i] * l)), tick);
    }
  }

  // Background: near black, the bonfire's warm halo, a darker vignette, embers rising from the fire, the author at
  // the bottom left and the version at the bottom right.
  void DrawBackground(ImDrawList* dl, ImVec2 d) const {
    dl->AddRectFilled(ImVec2(0, 0), d, IM_COL32(5, 4, 3, 255));
    // Halo and vignette as vertex-coloured grids (smooth radial gradients without banding).
    auto grid = [&](auto color_at) {
      constexpr int kNx = 40, kNy = 24;
      for (int j = 0; j < kNy; ++j) {
        for (int i = 0; i < kNx; ++i) {
          const float x0 = 1280.0f * float(i) / kNx, x1 = 1280.0f * float(i + 1) / kNx;
          const float y0 = 720.0f * float(j) / kNy, y1 = 720.0f * float(j + 1) / kNy;
          dl->AddRectFilledMultiColor(P(x0, y0), P(x1, y1), color_at(x0, y0), color_at(x1, y0), color_at(x1, y1),
                                      color_at(x0, y1));
        }
      }
    };
    // Halo: radial ellipse 560 x 400 at (640, 403): 26 % orange at the centre, 12 % at 46 %, nothing at 72 %.
    const float breathe = 0.85f + 0.15f * (0.5f + 0.5f * std::sin(time_ * 6.2831853f / 2.4f));
    grid([&](float x, float y) {
      const float f = std::sqrt(((x - 640.0f) / 560.0f) * ((x - 640.0f) / 560.0f) +
                                ((y - 403.0f) / 400.0f) * ((y - 403.0f) / 400.0f));
      float alpha = 0.0f, mix = 1.0f;
      if (f < 0.46f) {
        mix = f / 0.46f;
        alpha = 0.26f + (0.12f - 0.26f) * mix;
      } else if (f < 0.72f) {
        alpha = 0.12f * (0.72f - f) / 0.26f;
      }
      return IM_COL32(int(255 + (130 - 255) * mix), int(112 + (44 - 112) * mix), int(20 + (6 - 20) * mix),
                      int(255.0f * alpha * breathe));
    });
    dl->AddRectFilledMultiColor(P(0, 520), d, IM_COL32(90, 30, 4, 0), IM_COL32(90, 30, 4, 0), IM_COL32(90, 30, 4, 41),
                                IM_COL32(90, 30, 4, 41));
    // Vignette: radial ellipse 900 x 560, clear to 55 %, 78 % black at the edge.
    grid([&](float x, float y) {
      const float f = std::sqrt(((x - 640.0f) / 900.0f) * ((x - 640.0f) / 900.0f) +
                                ((y - 360.0f) / 560.0f) * ((y - 360.0f) / 560.0f));
      const float alpha = f < 0.55f ? 0.0f : 0.78f * std::min(1.0f, (f - 0.55f) / 0.45f);
      return IM_COL32(0, 0, 0, int(255.0f * alpha));
    });
    // Embers rising from the fire (the mock-up's ten): x, drift, period, delay, size.
    static constexpr float kEm[10][5] = {{598, -40, 5.2f, 0.0f, 4}, {622, 26, 4.1f, 0.7f, 3}, {640, -14, 6.0f, 1.5f, 4},
                                         {655, 48, 4.6f, 2.2f, 3},  {676, 18, 5.6f, 0.3f, 4}, {612, -62, 6.4f, 3.1f, 2},
                                         {664, 70, 6.8f, 3.8f, 2},  {634, 8, 3.8f, 2.7f, 3},  {588, -90, 7.2f, 1.1f, 2},
                                         {690, 96, 7.4f, 4.4f, 2}};
    for (const auto& e : kEm) {
      if (time_ < e[3]) continue;
      const float t = std::fmod(time_ - e[3], e[2]) / e[2];
      const float op = t < 0.12f ? t / 0.12f : (1.0f - t) / 0.88f;
      const float r = e[4] * 0.5f;
      const ImVec2 c = P(e[0] + r + e[1] * t, 470.0f - r - 430.0f * t);
      dl->AddCircleFilled(c, (r + 5.0f) * k_, IM_COL32(255, 122, 26, int(60.0f * op)), 12);
      dl->AddCircleFilled(c, (r + 2.0f) * k_, IM_COL32(255, 122, 26, int(110.0f * op)), 12);
      dl->AddCircleFilled(c, r * k_, IM_COL32(255, 183, 92, int(255.0f * op)), 12);
    }
    // Author (bottom left).
    Txt(dl, 28, 630, 13, 5, kLabel, "SWITCH PORT BY");
    dl->AddCircleFilled(P(31.5f, 659), 8.0f * k_, IM_COL32(255, 122, 26, 60), 12);
    rex::ui::souls::Diamond(dl, P(31.5f, 659), 5.0f * k_, IM_COL32(255, 138, 42, 255));
    Txt(dl, 45, 659, 24, 5, IM_COL32(232, 201, 140, 255), "XDASCATX", 0, true, IM_COL32(255, 130, 40, 14));
    Txt(dl, 28, 689, 14, 4, kGold, "XBOX 360 RECOMP");
    // Version (bottom right), in a box.
    if (DS2_VERSION_STRING[0]) {
      const float wl = Width("VERSION", 13, 4) / k_, wv = Width(DS2_VERSION_STRING, 26, 2) / k_ + 0.7f;
      const float x1 = 1252.0f, x0 = x1 - 16.0f - wv - 12.0f - wl - 16.0f;
      dl->AddRectFilled(P(x0, 645), P(x1, 698), IM_COL32(14, 11, 8, 217));
      dl->AddRect(P(x0, 645), P(x1, 698), IM_COL32(107, 82, 48, 255), 0.0f, 0, 1.2f * k_);
      Txt(dl, x0 + 16.0f, 671.5f, 13, 4, kLabel, "VERSION");
      Txt(dl, x1 - 16.0f, 671.5f, 26, 2, IM_COL32(255, 183, 92, 255), DS2_VERSION_STRING, 2, true,
          IM_COL32(255, 122, 26, 20));
    }
  }

  // "[D-pad] Select   (A) Start   or   [tap] Tap", the mock-up's outlined glyphs, centred at y 677.
  void DrawHints(ImDrawList* dl) const {
    const ImU32 ink = kGold;
    const float ws = Width("Select", 14, 2) / k_, wt = Width("Start", 14, 2) / k_;
    const float wo = Width("or", 14, 2) / k_, wp = Width("Tap", 14, 2) / k_;
    const float total = 26 + 10 + ws + 32 + 26 + 10 + wt + 24 + wo + 24 + 26 + 10 + wp;
    float x = 640.0f - total * 0.5f;
    const float y0 = 677.0f - 13.0f;
    auto G = [&](float gx, float gy, float ox) { return P(ox + gx, y0 + gy); };
    const float th = 1.6f * k_;
    // D-pad.
    {
      const float pts[12][2] = {{10, 2}, {16, 2}, {16, 10}, {24, 10}, {24, 16}, {16, 16},
                                {16, 24}, {10, 24}, {10, 16}, {2, 16},  {2, 10},  {10, 10}};
      for (const auto& p : pts) dl->PathLineTo(G(p[0], p[1], x));
      dl->PathStroke(ink, ImDrawFlags_Closed, th);
    }
    x += 26 + 10;
    Txt(dl, x, 677, 14, 2, kMuted, "Select");
    x += ws + 32;
    // A button.
    dl->AddCircle(G(13, 13, x), 11.0f * k_, ink, 32, th);
    dl->PathLineTo(G(8.5f, 18.5f, x));
    dl->PathLineTo(G(13, 7.5f, x));
    dl->PathLineTo(G(17.5f, 18.5f, x));
    dl->PathStroke(ink, 0, th);
    dl->AddLine(G(10.3f, 14.5f, x), G(15.7f, 14.5f, x), ink, th);
    x += 26 + 10;
    Txt(dl, x, 677, 14, 2, kMuted, "Start");
    x += wt + 24;
    Txt(dl, x, 677, 14, 2, IM_COL32(133, 119, 100, 255), "or");
    x += wo + 24;
    // Tap: a finger with two arcs.
    {
      const float pts[][2] = {{11, 23},   {11, 13.5f}, {11.6f, 12.1f}, {13, 11.5f},   {14.4f, 12.1f}, {15, 13.5f},
                              {15, 19},   {19.2f, 20.2f}, {20.3f, 21.0f}, {20.6f, 22.4f}, {20.3f, 24}};
      for (const auto& p : pts) dl->PathLineTo(G(p[0], p[1], x));
      dl->PathStroke(ink, 0, th);
      dl->PathArcTo(G(13, 12.27f, x), 5.4f * k_, -2.667f, -0.475f, 16);
      dl->PathStroke(ink, 0, 1.4f * k_);
      dl->PathArcTo(G(13, 12.2f, x), 8.4f * k_, -2.641f, -0.500f, 20);
      dl->PathStroke(ink, 0, 1.4f * k_);
    }
    x += 26 + 10;
    Txt(dl, x, 677, 14, 2, kMuted, "Tap");
  }

  // v128: a bonfire of bones with two skulls (the start screen's centre), in a 240 x 270 box at `o`, `s` pixels per
  // unit; the flames flicker with `t`.
  static void DrawBonfire(ImDrawList* dl, ImVec2 o, float s, float t) {
    constexpr float kPi = 3.14159265f;
    auto P = [&](float x, float y) { return ImVec2(o.x + x * s, o.y + y * s); };
    const ImU32 bone = IM_COL32(203, 189, 159, 255), bone_edge = IM_COL32(107, 95, 74, 255);
    const ImU32 hole = IM_COL32(20, 13, 8, 255);
    // Glow: the mock-up's radial gradient (centre 50 % / 78 % of the box, radius 55 %), kept inside the box.
    dl->PushClipRect(o, P(240.0f, 270.0f), true);
    {
      constexpr int kRings = 16;
      float prev = 0.0f;
      for (int i = kRings; i >= 1; --i) {
        const float f = float(i) / float(kRings);
        const float target = f < 0.5f ? 0.55f + (0.18f - 0.55f) * (f / 0.5f) : 0.18f * (1.0f - f) / 0.5f;
        const float mix = std::min(1.0f, f / 0.5f);
        const float add = std::max(0.0f, target - prev);
        prev = target;
        dl->AddEllipseFilled(P(120.0f, 210.6f), ImVec2(132.0f * f * s, 148.5f * f * s),
                             IM_COL32(255, int(181 + (106 - 181) * mix), int(74 + (18 - 74) * mix), int(255.0f * add)),
                             0.0f, 48);
      }
    }
    dl->PopClipRect();
    dl->AddEllipseFilled(P(120.0f, 254.0f), ImVec2(104.0f * s, 12.0f * s), IM_COL32(13, 9, 7, 255));
    auto Bone = [&](float cx, float cy, float len, float deg) {
      const float a = deg * kPi / 180.0f, c = std::cos(a), sn = std::sin(a);
      const float h = len * 0.5f - 4.5f;
      dl->AddLine(P(cx - c * h, cy - sn * h), P(cx + c * h, cy + sn * h), bone, 4.5f * s);
      for (int e = -1; e <= 1; e += 2) {
        const float ex = cx + c * h * float(e), ey = cy + sn * h * float(e);
        for (int side = -1; side <= 1; side += 2) {
          dl->AddCircleFilled(P(ex - sn * 3.2f * float(side), ey + c * 3.2f * float(side)), 4.6f * s, bone, 12);
        }
      }
    };
    auto Skull = [&](float cx, float cy, float k, float deg) {
      const float a = deg * kPi / 180.0f, c = std::cos(a), sn = std::sin(a);
      auto Q = [&](float x, float y) {
        x = (x - 20.0f) * k;
        y = (y - 22.0f) * k;
        return P(cx + x * c - y * sn, cy + x * sn + y * c);
      };
      dl->AddQuadFilled(Q(9, 27), Q(31, 27), Q(31, 42), Q(9, 42), bone);
      dl->AddQuad(Q(9, 27), Q(31, 27), Q(31, 42), Q(9, 42), bone_edge, 1.0f);
      dl->AddCircleFilled(Q(20, 19), 17.0f * k * s, bone, 28);
      dl->AddCircle(Q(20, 19), 17.0f * k * s, bone_edge, 28, 1.0f);
      dl->AddEllipseFilled(Q(13, 21), ImVec2(5.0f * k * s, 5.5f * k * s), hole, a);
      dl->AddEllipseFilled(Q(27, 21), ImVec2(5.0f * k * s, 5.5f * k * s), hole, a);
      dl->AddTriangleFilled(Q(20, 26), Q(17, 32), Q(23, 32), hole);
      for (float x : {14.0f, 18.0f, 22.0f, 26.0f}) dl->AddLine(Q(x, 36), Q(x, 42), bone_edge, 1.0f);
    };
    // Bones behind the fire, and the ribs.
    Bone(130.0f, 231.0f, 76.0f, -24.0f);
    Bone(110.0f, 231.0f, 76.0f, 22.0f);
    dl->PathLineTo(P(92, 236));
    dl->PathBezierCubicCurveTo(P(96, 212), P(144, 212), P(148, 236), 16);
    dl->PathStroke(bone, 0, 3.0f * s);
    dl->PathLineTo(P(100, 240));
    dl->PathBezierCubicCurveTo(P(104, 222), P(136, 222), P(140, 240), 16);
    dl->PathStroke(bone, 0, 3.0f * s);
    dl->AddEllipseFilled(P(120.0f, 234.0f), ImVec2(34.0f * s, 8.0f * s), IM_COL32(255, 106, 18, 100));
    // The flames: four layers, each flickering around its base.
    auto Flame = [&](std::initializer_list<float> v, ImU32 col, float sx, float sy) {
      const float* p = v.begin();
      const size_t n = v.size();
      auto F = [&](float x, float y) {
        x = 120.0f + (x - 120.0f) * sx;
        y = 240.0f + (y - 240.0f) * sy;
        return P(18.0f + 0.85f * x, 20.0f + 0.85f * y);
      };
      dl->PathLineTo(F(p[0], p[1]));
      for (size_t i = 2; i + 5 < n; i += 6) {
        dl->PathBezierCubicCurveTo(F(p[i], p[i + 1]), F(p[i + 2], p[i + 3]), F(p[i + 4], p[i + 5]), 14);
      }
      if (dl->_Path.Size > 3) dl->_Path.pop_back();  // the last point repeats the first
      dl->PathFillConcave(col);
    };
    const float a1 = 1.0f + 0.03f * std::sin(t * 4.1f), b1 = 1.0f + 0.05f * std::sin(t * 3.3f + 1.0f);
    const float a2 = 1.0f + 0.04f * std::sin(t * 5.7f + 2.0f), b2 = 1.0f + 0.06f * std::sin(t * 4.6f);
    const float a3 = 1.0f + 0.05f * std::sin(t * 6.3f + 0.5f), b3 = 1.0f + 0.07f * std::sin(t * 5.2f + 1.7f);
    Flame({70, 150, 78, 170, 74, 186, 82, 204, 70, 196, 60, 180, 70, 150}, IM_COL32(179, 50, 10, 255), a1, b1);
    Flame({172, 138, 166, 160, 174, 178, 164, 200, 180, 188, 184, 164, 172, 138}, IM_COL32(179, 50, 10, 255), a1, b1);
    Flame({120, 36,  150, 88,  186, 120, 176, 180, 170, 222, 146, 242, 120, 242, 94,  242, 68,  224, 64,  182,
           60,  140, 88,  128, 96,  96,  102, 120, 112, 124, 114, 110, 116, 85,  112, 60,  120, 36},
          IM_COL32(214, 72, 16, 255), a1, b1);
    Flame({122, 92,  142, 126, 160, 150, 154, 190, 150, 220, 136, 238, 120, 238, 102, 238, 86,  224, 84,  194,
           82,  166, 100, 156, 106, 134, 110, 150, 118, 150, 119, 138, 120, 122, 118, 108, 122, 92},
          IM_COL32(255, 128, 32, 255), a2, b2);
    Flame({121, 150, 132, 170, 140, 186, 136, 208, 133, 226, 127, 236, 120, 236, 112, 236, 104, 226, 103, 210,
           102, 192, 112, 184, 116, 170, 118, 178, 121, 176, 121, 150},
          IM_COL32(255, 210, 122, 255), a3, b3);
    // Bones and skulls in front.
    Bone(65.0f, 252.0f, 66.0f, 8.0f);
    Bone(175.0f, 254.0f, 66.0f, -10.0f);
    Bone(122.0f, 259.0f, 56.0f, 0.0f);
    Skull(68.0f, 236.0f, 1.0f, -12.0f);
    Skull(173.0f, 239.0f, 0.85f, 14.0f);
  }
  float k_ = 1.0f;
  int focus_ = kCard720;  // v131: the focused control (Item)
  bool armed_ = false;    // v131: A was seen released since the screen appeared

  // Xbox 360 language ids of the European disc other than English and Russian.
  static const char* OtherLanguageName(int id) {
    switch (id) {
      case 3: return "DEU";
      case 4: return "FRA";
      case 5: return "ESP";
      case 6: return "ITA";
      case 9: return "POR";
      case 11: return "POL";
      default: return nullptr;
    }
  }
  static int TomlLanguage() {
    try {
      return std::stoi(rex::cvar::GetFlagByName("user_language"));
    } catch (...) {
      return 12;
    }
  }

  std::function<void(const StartPrefs&)> on_choice_;
  int fps_ = 60;
  int toml_language_ = 12;
  int language_ = 12;
  bool sharpen_ = true;
  double scale_ = 1.0;   // remembered resolution (focus and countdown)
  bool remembered_ = false;
  bool counting_ = false;  // only with a remembered choice (StartCountdown)
  float time_ = 0.0f;
  bool chosen_ = false;
  bool first_frame_ = true;

 public:
  void StartCountdown() { counting_ = remembered_; }
};

std::unique_ptr<ResolutionDialog> g_resolution_dialog;

}  // namespace

void ChooseResolution(rex::ui::ImGuiDrawer* drawer, std::function<void(std::function<void()>)> post_to_ui,
                      std::function<void()> then) {
  if (!drawer || !post_to_ui || !REXCVAR_GET(ds2_resolution_prompt)) {
    then();
    return;
  }
  g_resolution_dialog = std::make_unique<ResolutionDialog>(drawer, [post_to_ui, then](const StartPrefs& p) {
    // From inside the dialog's OnDraw: the dialog is destroyed later, on the UI thread, before the game starts.
    post_to_ui([p, then]() {
      // v114: game language, read by the game through XamGetLanguage after it starts.
      rex::cvar::SetFlagByName("user_language", std::to_string(p.language));
      rex::cvar::SetFlagByName("ds2_render_scale", p.scale >= 1.0 ? "1.0" : "0.75");
      // v113: frame rate lock (ds2_fps_cap): 30, or 60. v116: sharpening of the 540p upscale.
      rex::cvar::SetFlagByName("ds2_fps_cap", p.fps == 30 ? "30" : "60");
      rex::cvar::SetFlagByName("ds2_sharpen", p.sharpen ? "true" : "false");
      REXLOG_INFO("ds2: start screen: {}, {} FPS lock, language {}, sharpening {}",
                  p.scale >= 1.0 ? "720p" : "540p", p.fps, p.language, p.sharpen ? "on" : "off");
      g_resolution_dialog.reset();
      then();
    });
  });
  g_resolution_dialog->StartCountdown();
}


void InstallWithProgress(rex::Runtime* runtime, rex::ui::ImGuiDrawer* drawer,
                         std::function<void(std::function<void()>)> post_to_ui, std::function<void()> then) {
  if (!drawer || !post_to_ui) {
    InstallConfigured(runtime);
    then();
    return;
  }
  g_dialog = std::make_unique<InstallDialog>(drawer);
  g_install_thread = std::thread([runtime, post_to_ui, then]() {
    InstallConfigured(runtime);
    post_to_ui([then]() {
      // The dialog must be gone before the game starts: while a dialog is registered the presenter paints
      // from the UI thread (see ReXApp::LaunchModule).
      g_dialog.reset();
      if (g_install_thread.joinable()) g_install_thread.join();
      then();
    });
  });
}

}  // namespace ds2::dlc
