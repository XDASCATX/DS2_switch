// ds2 - host application class. Starts from the `rexglue init` template; game-specific hooks go here.

#pragma once

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/logging.h>
#include <rex/rex_app.h>

#include "ds2_capture.h"
#include "ds2_dlc.h"
#include "ds2_input_script.h"
#include "ds2_watchdog.h"

#include <filesystem>
#include <string>
#include <system_error>

class Ds2App : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<Ds2App>(new Ds2App(ctx, "ds2", PPCImageConfig));
  }

 protected:
  // Portable layout, as nfsmw-nx does (app/src/nfsmw_app.h): with no --game_data_root / --user_data_root, look
  // next to the executable. On the Switch that is the NRO folder, sdmc:/switch/ds2/:
  //   game_root/   the game folder (Default.xex, GameData*.bdt, ...)
  //   user/        saves and installed content (created by the runtime)
  // Runs before ds2.toml is read, so only the command line overrides it (SDK ordering, sdk/src/ui/rex_app.cpp).
  void OnConfigurePaths(rex::PathConfig& paths) override {
    const auto dir = rex::filesystem::GetExecutableFolder();
    if (dir.empty()) return;
    std::error_code ec;
    if (paths.game_data_root.empty() && std::filesystem::is_directory(dir / "game_root", ec))
      paths.game_data_root = dir / "game_root";
    if (rex::cvar::GetFlagByName("user_data_root").empty()) {
      paths.user_data_root = dir / "user";
      if (rex::cvar::GetFlagByName("cache_root").empty()) paths.cache_root = dir / "user" / "cache";
    }
  }

  // Without a GPU plugin the runtime drops the game's graphics ("no GPU emulation loaded"); default to xenos
  // unless the command line or ds2.toml chose otherwise.
  void OnPostInitLogging() override {
    SetIfDefault("gpu_plugin", "xenos");
    ds2::watchdog::ApplyPlatformSwitches();
  }

  static void SetIfDefault(const char* name, const char* value) {
    if (rex::cvar::GetFlagInfo(name) == nullptr || rex::cvar::HasNonDefaultValue(name)) return;
    if (rex::cvar::SetFlagByName(name, value)) REXLOG_INFO("ds2: default {} = {}", name, value);
  }

  // The SDK starts game:\default.xex and checks it on the host file system before the (case-insensitive) VFS
  // sees it (sdk/src/ui/rex_app.cpp). The DS2 disc has it as Default.xex, so on a case-sensitive host
  // (Linux, Switch SD card) the lowercase name is not found.
  void OnLoadXexImage(std::string& xex_image) override { xex_image = "game:\\Default.xex"; }

  // Automated tests: scripted virtual pad (off unless --ds2_input_script is given), see ds2_input_script.h.
  void OnPreSetup(rex::RuntimeConfig& config) override { ds2::input_script::Wrap(config); }

  // DLC packages (--ds2_install_dlc), installed once the XEX is loaded so the title id is known.
  void OnPostSetup() override { ds2::dlc::InstallConfigured(runtime()); }

  // Diagnostic frame capture (off unless --ds2_capture_dir is given), see ds2_capture.h.
  void OnPostLaunchModule(rex::system::XThread* thread) override {
    (void)thread;
    ds2::capture::Start(runtime());
    ds2::watchdog::Start(runtime());
  }
  void OnShutdown() override {
    ds2::watchdog::Stop();
    ds2::capture::Stop();
  }
};
