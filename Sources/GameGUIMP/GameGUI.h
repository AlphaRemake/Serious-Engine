/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include <Game/Game.h>

// [Cecil] Directly export from the library
#ifdef GAMEGUI_EXPORTS
  #define GAMEGUI_API SE1_API_EXPORT
#else
  #define GAMEGUI_API SE1_API_IMPORT

  #ifdef NDEBUG
    #pragma comment(lib, "GameGUIMP.lib")
  #else
    #pragma comment(lib, "GameGUIMPD.lib")
  #endif
#endif

// This class is exported from the GameGUI.dll
class GAMEGUI_API CGameGUI {
public:
  // functions called from World Editor
  static void OnInvokeConsole(void);
  static void OnPlayerSettings(void);
  static void OnAudioQuality(void);
  static void OnVideoQuality(void);
  static void OnSelectPlayerAndControls(void);
};
