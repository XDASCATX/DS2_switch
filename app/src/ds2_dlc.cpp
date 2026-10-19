// ds2 - DLC package installation, see ds2_dlc.h.

#include "ds2_dlc.h"

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/runtime.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xam/content_manager.h>
#include <rex/system/xcontent.h>

#include <filesystem>
#include <string>
#include <string_view>

REXCVAR_DEFINE_STRING(ds2_install_dlc, "", "DS2",
                      "STFS DLC packages to install at startup if missing, separated by ';' "
                      "(e.g. the Compatibility Pack 4 file D4B91B6B...)")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

namespace ds2::dlc {

void InstallConfigured(rex::Runtime* runtime) {
  const std::string list = REXCVAR_GET(ds2_install_dlc);
  if (list.empty() || !runtime || !runtime->kernel_state()) {
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
    const std::filesystem::path path(item);
    const std::string name = path.filename().string();
    bool present = false;
    for (const auto& c : installed) {
      if (c.file_name() == name) present = true;
    }
    if (present) {
      REXLOG_INFO("[ds2 dlc] {} already installed", name);
      continue;
    }
    const auto result = content->InstallContent(path);
    REXLOG_INFO("[ds2 dlc] install {} -> {:08X}", item, uint32_t(result));
  }
}

}  // namespace ds2::dlc
