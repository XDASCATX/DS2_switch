// ds2 - installs Xbox 360 DLC packages (STFS: PIRS/LIVE/CON files) for the game at startup.
//
// DS2 SotFS refuses to leave the title screen without "DARK SOULS II Compatibility Pack 4" (content type 0x2). The
// SDK's content manager enumerates DLC as extracted folders under <user_data_root>/0000000000000000/<title>/00000002/
// and already has ContentManager::InstallContent (STFS -> folder + .header with the license mask), but nothing calls
// it. --ds2_install_dlc=<package>[;<package>...] calls it once per package that is not installed yet.

#pragma once

namespace rex {
class Runtime;
}

namespace ds2::dlc {

void InstallConfigured(rex::Runtime* runtime);

}  // namespace ds2::dlc
