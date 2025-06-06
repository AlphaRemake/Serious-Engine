/* Copyright (c) 2002-2012 Croteam Ltd. 
   Copyright (c) 2024 Dreamy Cecil
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

#include "StdH.h"

#include <Engine/Base/Timer.h>
#include <Engine/Base/Input.h>
#include <Engine/Base/Translation.h>
#include <Engine/Math/Functions.h>
#include <Engine/Graphics/ViewPort.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/Synchronization.h>

#include <Engine/Base/ErrorReporting.h>

extern INDEX inp_iKeyboardReadingMethod;
extern INDEX inp_bAllowPrescan;

#if SE1_PREFER_SDL
  // [Cecil] For synchronizing SDL events
  static CTCriticalSection inp_csSDLInput;
#endif

/*

NOTE: Three different types of key codes are used here:
  1) kid - engine internal type - defined in KeyNames.h
  2) scancode - raw PC keyboard scancodes as returned in keydown/keyup messages
  3) virtkey - virtual key codes used by Windows or SDL

*/

// name that is not translated (international)
#define INTNAME(str) str, ""
// name that is translated
#define TRANAME(str) str, "ETRS" str

// basic key conversion table
struct KeyConversion {
  EInputKeyID kc_eKID;
  INDEX kc_iVirtKey;

  // [Cecil] NOTE: Some keys are offsetted by 256 to account for the "extended" flag that's being set
  // near scancodes when receiving key messages on Windows, otherwise it's impossible to distinguish
  // certain keys, e.g. Left Alt/Right Alt or Enter/Num Enter. As a result, the received scancode is
  // determined with a 0x1FF mask instead of just 0xFF. These scancodes are useless with SDL because
  // SDL is capable of distinguishing the keys when converting between its virtual keys and scancodes.
#if !SE1_PREFER_SDL
  INDEX kc_iScanCode;
#else
  INDEX kc_iUnusedWindowsScanCode;
#endif

  const char *kc_strName;
  const char *kc_strNameTrans;
};

static const KeyConversion _akcKeys[] = {
  // reserved for 'no-key-pressed'
  { KID_NONE, -1, -1, TRANAME("None")},

  // numbers row
  { KID_1               , SE1K_1,       2, INTNAME("1")},
  { KID_2               , SE1K_2,       3, INTNAME("2")},
  { KID_3               , SE1K_3,       4, INTNAME("3")},
  { KID_4               , SE1K_4,       5, INTNAME("4")},
  { KID_5               , SE1K_5,       6, INTNAME("5")},
  { KID_6               , SE1K_6,       7, INTNAME("6")},
  { KID_7               , SE1K_7,       8, INTNAME("7")},
  { KID_8               , SE1K_8,       9, INTNAME("8")},
  { KID_9               , SE1K_9,      10, INTNAME("9")},
  { KID_0               , SE1K_0,      11, INTNAME("0")},
  { KID_MINUS           , SE1K_MINUS,  12, INTNAME("-")},
  { KID_EQUALS          , SE1K_EQUALS, 13, INTNAME("=")},

  // 1st alpha row
  { KID_Q               , SE1K_q,            16, INTNAME("Q")},
  { KID_W               , SE1K_w,            17, INTNAME("W")},
  { KID_E               , SE1K_e,            18, INTNAME("E")},
  { KID_R               , SE1K_r,            19, INTNAME("R")},
  { KID_T               , SE1K_t,            20, INTNAME("T")},
  { KID_Y               , SE1K_y,            21, INTNAME("Y")},
  { KID_U               , SE1K_u,            22, INTNAME("U")},
  { KID_I               , SE1K_i,            23, INTNAME("I")},
  { KID_O               , SE1K_o,            24, INTNAME("O")},
  { KID_P               , SE1K_p,            25, INTNAME("P")},
  { KID_LBRACKET        , SE1K_LEFTBRACKET,  26, INTNAME("[")},
  { KID_RBRACKET        , SE1K_RIGHTBRACKET, 27, INTNAME("]")},
  { KID_BACKSLASH       , SE1K_BACKSLASH,    43, INTNAME("\\")},

  // 2nd alpha row
  { KID_A               , SE1K_a,         30, INTNAME("A")},
  { KID_S               , SE1K_s,         31, INTNAME("S")},
  { KID_D               , SE1K_d,         32, INTNAME("D")},
  { KID_F               , SE1K_f,         33, INTNAME("F")},
  { KID_G               , SE1K_g,         34, INTNAME("G")},
  { KID_H               , SE1K_h,         35, INTNAME("H")},
  { KID_J               , SE1K_j,         36, INTNAME("J")},
  { KID_K               , SE1K_k,         37, INTNAME("K")},
  { KID_L               , SE1K_l,         38, INTNAME("L")},
  { KID_SEMICOLON       , SE1K_SEMICOLON, 39, INTNAME(";")},
  { KID_APOSTROPHE      , SE1K_QUOTE,     40, INTNAME("'")},

  // 3rd alpha row
  { KID_Z               , SE1K_z,      44, INTNAME("Z")},
  { KID_X               , SE1K_x,      45, INTNAME("X")},
  { KID_C               , SE1K_c,      46, INTNAME("C")},
  { KID_V               , SE1K_v,      47, INTNAME("V")},
  { KID_B               , SE1K_b,      48, INTNAME("B")},
  { KID_N               , SE1K_n,      49, INTNAME("N")},
  { KID_M               , SE1K_m,      50, INTNAME("M")},
  { KID_COMMA           , SE1K_COMMA,  51, INTNAME(",")},
  { KID_PERIOD          , SE1K_PERIOD, 52, INTNAME(".")},
  { KID_SLASH           , SE1K_SLASH,  53, INTNAME("/")},

  // row with F-keys
  { KID_F1              , SE1K_F1,   59, INTNAME("F1")},
  { KID_F2              , SE1K_F2,   60, INTNAME("F2")},
  { KID_F3              , SE1K_F3,   61, INTNAME("F3")},
  { KID_F4              , SE1K_F4,   62, INTNAME("F4")},
  { KID_F5              , SE1K_F5,   63, INTNAME("F5")},
  { KID_F6              , SE1K_F6,   64, INTNAME("F6")},
  { KID_F7              , SE1K_F7,   65, INTNAME("F7")},
  { KID_F8              , SE1K_F8,   66, INTNAME("F8")},
  { KID_F9              , SE1K_F9,   67, INTNAME("F9")},
  { KID_F10             , SE1K_F10,  68, INTNAME("F10")},
  { KID_F11             , SE1K_F11,  87, INTNAME("F11")},
  { KID_F12             , SE1K_F12,  88, INTNAME("F12")},

  // extra keys
  { KID_ESCAPE          , SE1K_ESCAPE,     1, TRANAME("Escape")},
  { KID_TILDE           , -1,             41, TRANAME("Tilde")},
  { KID_BACKSPACE       , SE1K_BACKSPACE, 14, TRANAME("Backspace")},
  { KID_TAB             , SE1K_TAB,       15, TRANAME("Tab")},
  { KID_CAPSLOCK        , SE1K_CAPSLOCK,  58, TRANAME("Caps Lock")},
  { KID_ENTER           , SE1K_RETURN,    28, TRANAME("Enter")},
  { KID_SPACE           , SE1K_SPACE,     57, TRANAME("Space")},

  // modifier keys
  { KID_LSHIFT          , SE1K_LSHIFT, 42,     TRANAME("Left Shift")},
  { KID_RSHIFT          , SE1K_RSHIFT, 54,     TRANAME("Right Shift")},
  { KID_LCONTROL        , SE1K_LCTRL,  29,     TRANAME("Left Control")},
  { KID_RCONTROL        , SE1K_RCTRL,  29+256, TRANAME("Right Control")},
  { KID_LALT            , SE1K_LALT,   56,     TRANAME("Left Alt")},
  { KID_RALT            , SE1K_RALT,   56+256, TRANAME("Right Alt")},

  // navigation keys
  { KID_ARROWUP         , SE1K_UP,           72+256, TRANAME("Arrow Up")},
  { KID_ARROWDOWN       , SE1K_DOWN,         80+256, TRANAME("Arrow Down")},
  { KID_ARROWLEFT       , SE1K_LEFT,         75+256, TRANAME("Arrow Left")},
  { KID_ARROWRIGHT      , SE1K_RIGHT,        77+256, TRANAME("Arrow Right")},
  { KID_INSERT          , SE1K_INSERT,       82+256, TRANAME("Insert")},
  { KID_DELETE          , SE1K_DELETE,       83+256, TRANAME("Delete")},
  { KID_HOME            , SE1K_HOME,         71+256, TRANAME("Home")},
  { KID_END             , SE1K_END,          79+256, TRANAME("End")},
  { KID_PAGEUP          , SE1K_PAGEUP,       73+256, TRANAME("Page Up")},
  { KID_PAGEDOWN        , SE1K_PAGEDOWN,     81+256, TRANAME("Page Down")},
  { KID_PRINTSCR        , SE1K_PRINTSCREEN,  55+256, TRANAME("Print Screen")},
  { KID_SCROLLLOCK      , SE1K_SCROLLLOCK,   70,     TRANAME("Scroll Lock")},
  { KID_PAUSE           , SE1K_PAUSE,        69,     TRANAME("Pause")},

  // numpad numbers
  { KID_NUM0            , SE1K_KP_0,      82, INTNAME("Num 0")},
  { KID_NUM1            , SE1K_KP_1,      79, INTNAME("Num 1")},
  { KID_NUM2            , SE1K_KP_2,      80, INTNAME("Num 2")},
  { KID_NUM3            , SE1K_KP_3,      81, INTNAME("Num 3")},
  { KID_NUM4            , SE1K_KP_4,      75, INTNAME("Num 4")},
  { KID_NUM5            , SE1K_KP_5,      76, INTNAME("Num 5")},
  { KID_NUM6            , SE1K_KP_6,      77, INTNAME("Num 6")},
  { KID_NUM7            , SE1K_KP_7,      71, INTNAME("Num 7")},
  { KID_NUM8            , SE1K_KP_8,      72, INTNAME("Num 8")},
  { KID_NUM9            , SE1K_KP_9,      73, INTNAME("Num 9")},
  { KID_NUMDECIMAL      , SE1K_KP_PERIOD, 83, INTNAME("Num .")},

  // numpad gray keys
  { KID_NUMLOCK         , SE1K_NUMLOCKCLEAR, 69+256, INTNAME("Num Lock")},
  { KID_NUMSLASH        , SE1K_KP_DIVIDE,    53+256, INTNAME("Num /")},
  { KID_NUMMULTIPLY     , SE1K_KP_MULTIPLY,  55,     INTNAME("Num *")},
  { KID_NUMMINUS        , SE1K_KP_MINUS,     74,     INTNAME("Num -")},
  { KID_NUMPLUS         , SE1K_KP_PLUS,      78,     INTNAME("Num +")},
  { KID_NUMENTER        , SE1K_KP_ENTER,     28+256, TRANAME("Num Enter")},

  // mouse buttons
  { KID_MOUSE1          , SE1K_LBUTTON,  -1, TRANAME("Mouse Button 1")},
  { KID_MOUSE2          , SE1K_RBUTTON,  -1, TRANAME("Mouse Button 2")},
  { KID_MOUSE3          , SE1K_MBUTTON,  -1, TRANAME("Mouse Button 3")},
  { KID_MOUSE4          , SE1K_XBUTTON1, -1, TRANAME("Mouse Button 4")},
  { KID_MOUSE5          , SE1K_XBUTTON2, -1, TRANAME("Mouse Button 5")},
  { KID_MOUSEWHEELUP    , -1, -1, TRANAME("Mouse Wheel Up")},
  { KID_MOUSEWHEELDOWN  , -1, -1, TRANAME("Mouse Wheel Down")},
};

static const size_t _ctKeyArray = ARRAYCOUNT(_akcKeys);

// autogenerated fast conversion tables
static INDEX _aiScanToKid[SDL_SCANCODE_COUNT];

// make fast conversion tables from the general table
static void MakeConversionTables(void) {
  INDEX i;

  // Clear conversion table
  for (i = 0; i < ARRAYCOUNT(_aiScanToKid); i++) {
    _aiScanToKid[i] = -1;
  }

  for (i = 0; i < _ctKeyArray; i++) {
    const KeyConversion &kc = _akcKeys[i];

    const EInputKeyID eKID  = kc.kc_eKID;
    const INDEX iVirt = kc.kc_iVirtKey;

    if (iVirt >= 0) {
    #if SE1_PREFER_SDL
      // [Cecil] SDL: Get scancode from virtual keycode
      INDEX iScan = SDL_GetScancodeFromKey(iVirt, NULL);

      // [Cecil] Fix Right Alt on Linux
      if (iVirt == SE1K_RALT && iScan == SDL_SCANCODE_SYSREQ) iScan = SDL_SCANCODE_RALT;

    #else
      // [Cecil] NOTE: Cannot use MapVirtualKey() here; see comment near kc_iScanCode field
      INDEX iScan = kc.kc_iScanCode;
    #endif

      _aiScanToKid[iScan] = eKID;
    }
  }

  // In case some keycode couldn't be mapped to a scancode
  _aiScanToKid[SDL_SCANCODE_UNKNOWN] = -1;
};

// which keys are pressed, as recorded by message interception (by KIDs)
static UBYTE _abKeysPressed[256];

// [Cecil] Accumulated wheel scroll in a certain direction (Z)
extern FLOAT _fGlobalMouseScroll;

#if SE1_PREFER_SDL

// [Cecil] SDL: Set key state according to the key event
static void SetKeyFromEvent(const SDL_Event *pEvent, const UBYTE ubDown) {
  const INDEX iKID = _aiScanToKid[pEvent->key.scancode];

  if (iKID != -1) {
    _abKeysPressed[iKID] = ubDown;
  }
};

// [Cecil] SDL: Special method for polling SDL events that is synced with the engine's input system
// This method has the same prototype as SDL_PollEvent() and should always be used in its place
int SE_PollEventForInput(SDL_Event *pEvent) {
  // Make sure it's synchronized
  CTSingleLock slInput(&inp_csSDLInput, FALSE);
  if (!slInput.TryToLock()) return FALSE;

  int iRet = SDL_PollEvent(pEvent);
  if (!iRet) return iRet;

  switch (pEvent->type) {
    case SDL_EVENT_MOUSE_ADDED: {
      _pInput->OpenMouse(pEvent->mdevice.which);
    } break;

    case SDL_EVENT_MOUSE_REMOVED: {
      _pInput->CloseMouse(pEvent->mdevice.which);
    } break;

    case SDL_EVENT_MOUSE_MOTION: {
      CInput::MouseDevice_t *pMouse = _pInput->GetMouseByID(pEvent->motion.which);

      if (pMouse != NULL) {
        pMouse->vMotion[0] += pEvent->motion.xrel;
        pMouse->vMotion[1] += pEvent->motion.yrel;
      }
    } break;

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
      INDEX iKID = KID_NONE;

      switch (pEvent->button.button) {
        case SDL_BUTTON_LEFT: iKID = KID_MOUSE1; break;
        case SDL_BUTTON_RIGHT: iKID = KID_MOUSE2; break;
        case SDL_BUTTON_MIDDLE: iKID = KID_MOUSE3; break;
        case SDL_BUTTON_X1: iKID = KID_MOUSE4; break;
        case SDL_BUTTON_X2: iKID = KID_MOUSE5; break;
      }

      if (iKID == KID_NONE) break;

      // Press/release mouse button for a specific mouse device
      INDEX iMouse = _pInput->GetMouseSlotForDevice(pEvent->motion.which);
      if (iMouse == -1) break;

      if (pEvent->button.down) {
        _abKeysPressed[iKID] |= INPUTDEVICES_NUM(iMouse);
      } else {
        _abKeysPressed[iKID] &= ~INPUTDEVICES_NUM(iMouse);
      }
    } break;

    case SDL_EVENT_MOUSE_WHEEL: {
      const FLOAT fMotion = Sgn(pEvent->wheel.y);

      // Reset wheel scroll if it's suddenly in the opposite direction
      if (Sgn(_fGlobalMouseScroll) != fMotion) {
        _fGlobalMouseScroll = 0;
      }

      // Add to the global mouse scroll
      _fGlobalMouseScroll += fMotion;

      // Then do the same for a specific mouse device
      INDEX iMouse;
      CInput::MouseDevice_t *pMouse = _pInput->GetMouseByID(pEvent->motion.which, &iMouse);
      if (pMouse == NULL) break;

      FLOAT &fScroll = pMouse->fScroll;

      // Reset wheel scroll if it's suddenly in the opposite direction
      if (Sgn(fScroll) != Sgn(fMotion)) {
        fScroll = 0.0f;
      }

      fScroll += fMotion;

      // Determine which direction the wheel is scrolled, which is reset later in CInput::GetInput()
      if (fScroll > 0) {
        _abKeysPressed[KID_MOUSEWHEELUP] |= INPUTDEVICES_NUM(iMouse);
      } else {
        _abKeysPressed[KID_MOUSEWHEELUP] &= ~INPUTDEVICES_NUM(iMouse);
      }

      if (fScroll < 0) {
        _abKeysPressed[KID_MOUSEWHEELDOWN] |= INPUTDEVICES_NUM(iMouse);
      } else {
        _abKeysPressed[KID_MOUSEWHEELDOWN] &= ~INPUTDEVICES_NUM(iMouse);
      }
    } break;

    case SDL_EVENT_KEY_DOWN: SetKeyFromEvent(pEvent, INPUTDEVICES_ALL); break;
    case SDL_EVENT_KEY_UP: SetKeyFromEvent(pEvent, INPUTDEVICES_NONE); break;

    default: break;
  }

  return iRet;
};

#else

static HHOOK _hGetMsgHook = NULL;
static HHOOK _hSendMsgHook = NULL;

// Set key state according to the key message
static void SetKeyFromMsg(MSG *pMsg, const UBYTE ubDown) {
  // Get key ID from scan code
  const INDEX iScan = (pMsg->lParam >> 16) & 0x1FF; // Use extended bit too!
  const INDEX iKID = _aiScanToKid[iScan];

  if (iKID != -1) {
    //CPrintF("%s: %d\n", _pInput->inp_strButtonNames[iKID], ubDown);
    _abKeysPressed[iKID] = ubDown;
  }
};

static void CheckMessage(MSG *pMsg)
{
  if (pMsg->message == WM_LBUTTONUP) {
    _abKeysPressed[KID_MOUSE1] = INPUTDEVICES_NONE;
  } else if (pMsg->message == WM_LBUTTONDOWN || pMsg->message == WM_LBUTTONDBLCLK) {
    _abKeysPressed[KID_MOUSE1] = INPUTDEVICES_ALL;

  } else if (pMsg->message == WM_RBUTTONUP) {
    _abKeysPressed[KID_MOUSE2] = INPUTDEVICES_NONE;
  } else if (pMsg->message == WM_RBUTTONDOWN || pMsg->message == WM_RBUTTONDBLCLK) {
    _abKeysPressed[KID_MOUSE2] = INPUTDEVICES_ALL;

  } else if (pMsg->message == WM_MBUTTONUP) {
    _abKeysPressed[KID_MOUSE3] = INPUTDEVICES_NONE;
  } else if (pMsg->message == WM_MBUTTONDOWN || pMsg->message == WM_MBUTTONDBLCLK) {
    _abKeysPressed[KID_MOUSE3] = INPUTDEVICES_ALL;

  // [Cecil] Proper support for MB4 and MB5
  } else if (pMsg->message == WM_XBUTTONUP) {
    if (GET_XBUTTON_WPARAM(pMsg->wParam) & XBUTTON1) {
      _abKeysPressed[KID_MOUSE4] = INPUTDEVICES_NONE;
    }
    if (GET_XBUTTON_WPARAM(pMsg->wParam) & XBUTTON2) {
      _abKeysPressed[KID_MOUSE5] = INPUTDEVICES_NONE;
    }

  } else if (pMsg->message == WM_XBUTTONDOWN || pMsg->message == WM_XBUTTONDBLCLK) {
    if (GET_XBUTTON_WPARAM(pMsg->wParam) & XBUTTON1) {
      _abKeysPressed[KID_MOUSE4] = INPUTDEVICES_ALL;
    }
    if (GET_XBUTTON_WPARAM(pMsg->wParam) & XBUTTON2) {
      _abKeysPressed[KID_MOUSE5] = INPUTDEVICES_ALL;
    }

  } else if (pMsg->message == WM_KEYUP || pMsg->message == WM_SYSKEYUP) {
    SetKeyFromMsg(pMsg, INPUTDEVICES_NONE);

  } else if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) {
    SetKeyFromMsg(pMsg, INPUTDEVICES_ALL);
  }
};

// Procedure called when message is retreived
LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam) {
  MSG *pMsg = reinterpret_cast<MSG *>(lParam);
  CheckMessage(pMsg);

  LRESULT r = CallNextHookEx(_hGetMsgHook, nCode, wParam, lParam);

  if (wParam == PM_NOREMOVE) {
    return r;
  }

  if (pMsg->message == WM_MOUSEWHEEL) {
    const SWORD swDir = Sgn(SWORD(UWORD(HIWORD(pMsg->wParam))));

    // Reset wheel scroll if it's suddenly in the opposite direction
    if (Sgn(_fGlobalMouseScroll) != swDir) {
      _fGlobalMouseScroll = 0;
    }

    // Add to the global mouse scroll
    _fGlobalMouseScroll += swDir;

    // Determine which direction the wheel is scrolled, which is reset later in CInput::GetInput()
    _abKeysPressed[KID_MOUSEWHEELUP]   = (swDir > 0) ? INPUTDEVICES_ALL : INPUTDEVICES_NONE;
    _abKeysPressed[KID_MOUSEWHEELDOWN] = (swDir < 0) ? INPUTDEVICES_ALL : INPUTDEVICES_NONE;
  }

  return r;
};

// Procedure called when message is sent
LRESULT CALLBACK SendMsgProc(int nCode, WPARAM wParam, LPARAM lParam) {
  MSG *pMsg = reinterpret_cast<MSG *>(lParam);
  CheckMessage(pMsg);

  return CallNextHookEx(_hSendMsgHook, nCode, wParam, lParam);
};

#endif // !SE1_PREFER_SDL

// pointer to global input object
CInput *_pInput = NULL;

// [Cecil]
bool CInput::InputDeviceAction::IsActive(DOUBLE fThreshold) const {
  return Abs(ida_fReading) >= Clamp(fThreshold, 0.01, 1.0);
};

// deafult constructor
CInput::CInput(void)
{
  // disable control scaning
  inp_bInputEnabled = FALSE;
  inp_bPollJoysticks = FALSE;

#if SE1_PREFER_SDL
  inp_csSDLInput.cs_eIndex = EThreadMutexType::E_MTX_INPUT; // [Cecil]
#endif

  MakeConversionTables();
};

// Destructor
CInput::~CInput() {
  // [Cecil] Various cleanups
  ShutdownMice();
  ShutdownJoysticks();
};

/*
 * Sets names of keys on keyboard
 */
void CInput::SetKeyNames( void)
{
  // [Cecil] Reset names of all actions
  for (INDEX iResetAction = 0; iResetAction < GetMaxInputActions(); iResetAction++) {
    inp_aInputActions[iResetAction].ida_strNameInt = "None";
    inp_aInputActions[iResetAction].ida_strNameTra = TRANS("None");
  }

  // Set names of key IDs
  for (INDEX iKey = 0; iKey < _ctKeyArray; iKey++) {
    const KeyConversion &kc = _akcKeys[iKey];
    if (kc.kc_strName == NULL) continue;

    inp_aInputActions[kc.kc_eKID].ida_strNameInt = kc.kc_strName;
    CTString &strTranslated = inp_aInputActions[kc.kc_eKID].ida_strNameTra;

    if (strlen(kc.kc_strNameTrans) == 0) {
      strTranslated = kc.kc_strName;
    } else {
      strTranslated = TranslateConst(kc.kc_strNameTrans, 4);
    }
  }

  // Set names of mouse axes
  #define SET_AXIS_NAME(_Axis, _Name) \
    inp_aInputActions[KID_FIRST_AXIS + _Axis].ida_strNameInt = _Name; \
    inp_aInputActions[KID_FIRST_AXIS + _Axis].ida_strNameTra = TRANS(_Name);

  SET_AXIS_NAME(EIA_MOUSE_X, "mouse X");
  SET_AXIS_NAME(EIA_MOUSE_Y, "mouse Y");
  SET_AXIS_NAME(EIA_MOUSE_Z, "mouse Z");

  #undef SET_AXIS_NAME

  // [Cecil] Set joystick names separately
  AddJoystickAbbilities();
}

/*
 * Initializes all available devices and enumerates available controls
 */
void CInput::Initialize( void )
{
  CPutString(TRANS("Detecting input devices...\n"));

  // [Cecil] Various initializations
  StartupMice();
  StartupJoysticks();

  SetKeyNames();
  CPutString("\n");
}

/*
 * Enable direct input
 */
void CInput::EnableInput(OS::Window hwnd)
{
  // skip if already enabled
  if( inp_bInputEnabled) return;

#if SE1_PREFER_SDL
  // [Cecil] Remember mouse position relative to the window
  OS::GetMouseState(&inp_aOldMousePos[0], &inp_aOldMousePos[1], TRUE);

  // [Cecil] SDL: Hide mouse cursor and clear relative movement since last time
  SDL_SetWindowRelativeMouseMode(hwnd, true);
  SDL_GetRelativeMouseState(NULL, NULL);

#else
  HWND hwndCurrent = hwnd;

  // get window rectangle
  RECT rectClient;
  GetClientRect(hwndCurrent, &rectClient);
  POINT pt;
  pt.x = pt.y = 0;
  ClientToScreen(hwndCurrent, &pt);
  OffsetRect(&rectClient, pt.x, pt.y);

  // remember mouse pos
  OS::GetMouseState(&inp_aOldMousePos[0], &inp_aOldMousePos[1], FALSE);
  // set mouse clip region
  ClipCursor(&rectClient);
  // determine screen center position
  inp_slScreenCenterX = (rectClient.left + rectClient.right) / 2;
  inp_slScreenCenterY = (rectClient.top + rectClient.bottom) / 2;

  // clear mouse from screen
  while (OS::ShowCursor(FALSE) >= 0);
  // save system mouse settings
  SystemParametersInfo(SPI_GETMOUSE, 0, &inp_mscMouseSettings, 0);

  // set new mouse speed
  extern INDEX inp_bAllowMouseAcceleration;
  if (!inp_bAllowMouseAcceleration) {
    MouseSpeedControl mscNewSetting = { 0, 0, 0 };
    SystemParametersInfo(SPI_SETMOUSE, 0, &mscNewSetting, 0);
  }

  // set cursor position to screen center
  SetCursorPos(inp_slScreenCenterX, inp_slScreenCenterY);

  _hGetMsgHook  = SetWindowsHookEx(WH_GETMESSAGE, &GetMsgProc, NULL, GetCurrentThreadId());
  _hSendMsgHook = SetWindowsHookEx(WH_CALLWNDPROC, &SendMsgProc, NULL, GetCurrentThreadId());
#endif // !SE1_PREFER_SDL

  // clear button's buffer
  memset(_abKeysPressed, 0, sizeof(_abKeysPressed));

  // [Cecil] Clear mouse movements
#if SE1_PREFER_SDL
  FOREACHINSTATICARRAY(inp_aMice, MouseDevice_t, itMouse) {
    itMouse->ResetMotion();
  }
#endif
  _fGlobalMouseScroll = 0;

  // This can be enabled to pre-read the state of currently pressed keys
  // for snooping methods, since they only detect transitions.
  // That has effect of detecting key presses for keys that were held down before
  // enabling.
  // the entire thing is disabled because it caused last menu key to re-apply in game.
#if 0
  // for each Key
  {for (INDEX iKey = 0; iKey < _ctKeyArray; iKey++) {
    struct KeyConversion &kc = _akcKeys[iKey];
    // get codes
    INDEX iKID  = kc.kc_iKID;
    INDEX iScan = kc.kc_iScanCode;
    INDEX iVirt = kc.kc_iVirtKey;

    // if there is a valid virtkey
    if (iVirt>=0) {
      // transcribe if modifier
      if (iVirt == VK_LSHIFT) {
        iVirt = VK_SHIFT;
      }
      if (iVirt == VK_LCONTROL) {
        iVirt = VK_CONTROL;
      }
      if (iVirt == VK_LMENU) {
        iVirt = VK_MENU;
      }
      // is state is pressed
      if (OS::GetAsyncKeyState(iVirt) & 0x8000) {
        // mark it as pressed
        _abKeysPressed[iKID] = INPUTDEVICES_ALL;
      }
    }
  }}
#endif

  // remember current status
  inp_bInputEnabled = TRUE;
  inp_bPollJoysticks = FALSE;
}

// [Cecil] Disable input inside one window instead of generally
void CInput::DisableInput(OS::Window hwnd)
{
  // skip if allready disabled
  if( !inp_bInputEnabled) return;

#if SE1_PREFER_SDL
  // [Cecil] SDL: Restore mouse position and show mouse cursor
  SDL_WarpMouseInWindow(hwnd, inp_aOldMousePos[0], inp_aOldMousePos[1]);
  SDL_SetWindowRelativeMouseMode(hwnd, false);

#else
  UnhookWindowsHookEx(_hGetMsgHook);
  UnhookWindowsHookEx(_hSendMsgHook);

  // set mouse clip region to entire screen
  ClipCursor(NULL);
  // restore mouse pos
  SetCursorPos(inp_aOldMousePos[0], inp_aOldMousePos[1]);

  // show mouse on screen
  while (OS::ShowCursor(TRUE) < 0);
  // set system mouse settings
  SystemParametersInfo(SPI_SETMOUSE, 0, &inp_mscMouseSettings, 0);
#endif // !SE1_PREFER_SDL

  // remember current status
  inp_bInputEnabled = FALSE;
  inp_bPollJoysticks = FALSE;
}

// [Cecil] Scan states of global input sources without distinguishing them between specific devices
void CInput::GetGlobalInput(BOOL bPreScan) {
  // Game input is disabled
  if (!inp_bInputEnabled) return;

  // Cannot pre-scan input every possible frame
  if (bPreScan && !inp_bAllowPrescan) return;

#if !SE1_PREFER_SDL
  // Read mouse axes
  ClearAxisInput(TRUE, FALSE);
  GetMouseInput(bPreScan, -1);

  // Set cursor position to the screen center
  FLOAT fMouseX, fMouseY;
  OS::GetMouseState(&fMouseX, &fMouseY, FALSE);

  if (FloatToInt(fMouseX) != inp_slScreenCenterX || FloatToInt(fMouseY) != inp_slScreenCenterY) {
    SetCursorPos(inp_slScreenCenterX, inp_slScreenCenterY);
  }
#endif
};

// [Cecil] Scan states of all input sources that are distinguished between specific devices
// Even if all devices are included, it does not include "global" devices from GetGlobalInput()
void CInput::GetInputFromDevices(BOOL bPreScan, ULONG ulDevices) {
  // Game input is disabled
  if (!inp_bInputEnabled) return;

  // Cannot pre-scan input every possible frame
  if (bPreScan && !inp_bAllowPrescan) return;

  // if not pre-scanning
  if (!bPreScan) {
    // [Cecil] Reset key readings
    ClearButtonInput(TRUE, TRUE, FALSE);

    #if SE1_PREFER_SDL
      // [Cecil] SDL: Get current keyboard and mouse states just once
      const bool *aKeyboardState = SDL_GetKeyboardState(NULL);
      const ULONG ulMouseState = SDL_GetMouseState(NULL, NULL);
    #endif

    // [Cecil] Determine key mask for buttons of the specified device
    const UBYTE ubKeyPressedMask = (ulDevices & 0xFF);

    // for each Key
    for (INDEX iKey = 0; iKey < _ctKeyArray; iKey++) {
      const KeyConversion &kc = _akcKeys[iKey];
      // get codes
      const EInputKeyID eKID  = kc.kc_eKID;
      INDEX iVirt = kc.kc_iVirtKey;

      InputDeviceAction &idaKey = inp_aInputActions[eKID];

      // [Cecil] FIXME: Asynchronous reading does not distinguish buttons between different keyboards and mice
      // if reading async keystate
      if (inp_iKeyboardReadingMethod == 0) {
        // if there is a valid virtkey
        if (iVirt >= 0) {
          BOOL bKeyPressed = FALSE;

        #if SE1_PREFER_SDL
          switch (iVirt) {
            // [Cecil] SDL: Use mouse state for mouse buttons
            case SE1K_LBUTTON: bKeyPressed = !!(ulMouseState & SDL_BUTTON_LMASK); break;
            case SE1K_RBUTTON: bKeyPressed = !!(ulMouseState & SDL_BUTTON_RMASK); break;
            case SE1K_MBUTTON: bKeyPressed = !!(ulMouseState & SDL_BUTTON_MMASK); break;
            case SE1K_XBUTTON1: bKeyPressed = !!(ulMouseState & SDL_BUTTON_X1MASK); break;
            case SE1K_XBUTTON2: bKeyPressed = !!(ulMouseState & SDL_BUTTON_X2MASK); break;

            default: bKeyPressed = aKeyboardState[SDL_GetScancodeFromKey(iVirt, NULL)];
          }

        #else
          // transcribe if modifier
          if (iVirt == VK_LSHIFT)   iVirt = VK_SHIFT;
          if (iVirt == VK_LCONTROL) iVirt = VK_CONTROL;
          if (iVirt == VK_LMENU)    iVirt = VK_MENU;

          bKeyPressed = !!(OS::GetKeyState(iVirt) & 0x8000);
        #endif

          // is state is pressed
          if (bKeyPressed) {
            // mark it as pressed
            idaKey.ida_fReading = 1;
          }
        }

      // if snooping messages
      } else if (_abKeysPressed[eKID] & ubKeyPressedMask) {
        // mark it as pressed
        idaKey.ida_fReading = 1;

        // [Cecil] Release mouse wheel after "pressing" it because wheel scrolling is always momentary
        // [Cecil] FIXME: If the same mouse device is used for multiple players in split screen, only the first one can use the scroll wheel
        if (eKID == KID_MOUSEWHEELUP)   _abKeysPressed[KID_MOUSEWHEELUP]   &= ~ubKeyPressedMask;
        if (eKID == KID_MOUSEWHEELDOWN) _abKeysPressed[KID_MOUSEWHEELDOWN] &= ~ubKeyPressedMask;
      }
    }
  }

  // [Cecil] NOTE: Turns out you still need to poll gamepads during pre-scanning but only for resetting everything
  // to 0 because non-pre-scanned values are still there and are being reused during pre-scanning, which is bad
  PollJoysticks(bPreScan, ulDevices);

#if SE1_PREFER_SDL
  // [Cecil] Read axes of specific mice
  ClearAxisInput(TRUE, FALSE);

  // Any of the mice
  if (ulDevices == INPUTDEVICES_ALL) {
    GetMouseInput(bPreScan, -1);
    return;
  }

  const INDEX ct = inp_aMice.Count();

  for (INDEX i = 0; i < ct; i++) {
    if (ulDevices & INPUTDEVICES_NUM(i)) {
      GetMouseInput(bPreScan, i);
    }
  }
#endif
};

// [Cecil] Clear states of all buttons (as if they are all released)
void CInput::ClearButtonInput(BOOL bKeyboards, BOOL bMice, BOOL bJoysticks) {
  if (bKeyboards) {
    for (INDEX i = KID_FIRST_KEYBOARD; i <= KID_LAST_KEYBOARD; i++) {
      inp_aInputActions[i].ida_fReading = 0;
    }
  }

  if (bMice) {
    for (INDEX i = KID_FIRST_MOUSE; i <= KID_LAST_MOUSE; i++) {
      inp_aInputActions[i].ida_fReading = 0;
    }
  }

  if (bJoysticks) {
    for (INDEX i = KID_FIRST_GAMEPAD; i <= KID_LAST_GAMEPAD; i++) {
      inp_aInputActions[i].ida_fReading = 0;
    }
  }
};

// [Cecil] Clear movements of all axes (as if they are still)
void CInput::ClearAxisInput(BOOL bMice, BOOL bJoysticks) {
  if (bMice) {
    for (INDEX i = 0; i < EIA_MAX_MOUSE; i++) {
      inp_aInputActions[KID_FIRST_AXIS + i].ida_fReading = 0;
    }
  }

  if (bJoysticks) {
    for (INDEX i = EIA_CONTROLLER_OFFSET; i < EIA_MAX_ALL; i++) {
      inp_aInputActions[KID_FIRST_AXIS + i].ida_fReading = 0;
    }
  }
};

// [Cecil] Clear all available input actions at once
void CInput::ClearInput(void) {
  for (INDEX i = 0; i < GetMaxInputActions(); i++) {
    inp_aInputActions[i].ida_fReading = 0;
  }
};

// Get given button's current state
BOOL CInput::GetButtonState(INDEX iButtonNo) const {
  const InputDeviceAction &ida = inp_aInputActions[iButtonNo];

  // [Cecil] When using mouse axes as "buttons", check if they've been moved further than some amount of pixels
  if (iButtonNo >= KID_FIRST_AXIS && iButtonNo < KID_FIRST_AXIS + EIA_MAX_MOUSE) {
    extern INDEX inp_iMouseMovePressThreshold;
    return Abs(ida.ida_fReading) >= ClampDn(inp_iMouseMovePressThreshold, 1);
  }

  // [Cecil] Set custom threshold for axes
  FLOAT fThreshold = 0.5f;

  if (iButtonNo >= KID_FIRST_AXIS && iButtonNo < KID_FIRST_AXIS + GetMaxInputAxes()) {
    extern FLOAT inp_fAxisPressThreshold;
    fThreshold = inp_fAxisPressThreshold;
  }

  return ida.IsActive(fThreshold);
};

// [Cecil] Find axis action index by its name
EInputAxis CInput::FindAxisByName(const CTString &strName) {
  if (strName == "None") return EIA_NONE;

  for (INDEX i = EIA_NONE; i < GetMaxInputAxes(); i++) {
    if (GetAxisName(i) == strName) return (EInputAxis)i;
  }

  return EIA_NONE;
};

// [Cecil] Find input action index by its name
INDEX CInput::FindActionByName(const CTString &strName) {
  if (strName == "None") return KID_NONE;

  for (INDEX i = 0; i < GetMaxInputActions(); i++) {
    if (GetButtonName(i) == strName) return i;
  }

  return KID_NONE;
};
