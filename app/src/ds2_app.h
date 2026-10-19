// ds2 - host application class. Starts from the `rexglue init` template; game-specific hooks go here.

#pragma once

#include <rex/rex_app.h>

#include "ds2_capture.h"
#include "ds2_dlc.h"
#include "ds2_input_script.h"

#include <string>

class Ds2App : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<Ds2App>(new Ds2App(ctx, "ds2", PPCImageConfig));
  }

 protected:
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
  }
  void OnShutdown() override { ds2::capture::Stop(); }
};
