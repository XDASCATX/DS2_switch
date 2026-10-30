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

#include <imgui.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>
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
    auto& progress = rex::system::xam::install_progress();
    const uint64_t total = progress.bytes_total.load(std::memory_order_relaxed);
    if (!total) return;
    const uint64_t done = std::min(progress.bytes_done.load(std::memory_order_relaxed), total);
    const float fraction = float(double(done) / double(total));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.6f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.85f);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                   ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs;
    if (ImGui::Begin("##ds2_install", nullptr, flags)) {
      ImGui::SetWindowFontScale(2.0f);
      ImGui::Text("Installing game data %u/%u", g_package_index.load(std::memory_order_relaxed),
                  g_package_count.load(std::memory_order_relaxed));
      char label[32];
      std::snprintf(label, sizeof(label), "%.1f%%", double(fraction) * 100.0);
      ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f), label);
      ImGui::SetWindowFontScale(1.4f);
      ImGui::Text("%.1f / %.1f MB", double(done) / 1048576.0, double(total) / 1048576.0);
    }
    ImGui::End();
  }
};

std::unique_ptr<InstallDialog> g_dialog;
std::thread g_install_thread;

}  // namespace

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
