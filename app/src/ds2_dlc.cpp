// ds2 - DLC package installation, see ds2_dlc.h.

#include "ds2_dlc.h"

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xam/content_manager.h>
#include <rex/system/xcontent.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

REXCVAR_DEFINE_STRING(ds2_install_dlc, "", "DS2",
                      "STFS DLC packages to install at startup if missing, separated by ';' "
                      "(e.g. the Compatibility Pack 4 file D4B91B6B...). Relative paths are taken from the executable folder. "
                      "Empty: every file of <executable folder>/dlc/ is installed.")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

namespace ds2::dlc {

void InstallConfigured(rex::Runtime* runtime) {
  if (!runtime || !runtime->kernel_state()) {
    return;
  }
  const auto exe_dir = rex::filesystem::GetExecutableFolder();
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
  const auto installed = content->ListContent(1, 0, rex::system::XContentType::kMarketplaceContent);
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
    bool present = false;
    for (const auto& c : installed) {
      if (c.file_name() == name) present = true;
    }
    if (present) {
      REXLOG_INFO("[ds2 dlc] {} already installed", name);
      continue;
    }
    REXLOG_INFO("[ds2 dlc] installing {} (first boot only: the package is unpacked onto the SD card, which takes "
                "minutes on the console)", rex::path_to_utf8(path));
    rex::FlushLogging();
    const auto t0 = std::chrono::steady_clock::now();
    const auto result = content->InstallContent(path);
    REXLOG_INFO("[ds2 dlc] install {} -> {:08X} in {:.1f} s", rex::path_to_utf8(path), uint32_t(result),
                std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count());
  }
}

}  // namespace ds2::dlc
