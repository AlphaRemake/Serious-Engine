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

#ifndef SE_INCL_SOUNDAPI_SDL_H
#define SE_INCL_SOUNDAPI_SDL_H

#include <Engine/Sound/SoundAPI.h>

#if SE1_SDL

class CSoundAPI_SDL : public CAbstractSoundAPI {
  public:
    Uint8 m_ubSilence;
    volatile SLONG m_slBackBufferAlloc;
    Uint8 *m_pBackBuffer;
    volatile SLONG m_slBackBufferPos;
    volatile SLONG m_slBackBufferRemain;
    SDL_AudioDeviceID m_iAudioDevice;

  public:
    // Constructor
    CSoundAPI_SDL() : CAbstractSoundAPI() {
      m_ubSilence = 0;
      m_slBackBufferAlloc = 0;
      m_pBackBuffer = NULL;
      m_slBackBufferPos = 0;
      m_slBackBufferRemain = 0;
      m_iAudioDevice = 0;
    };

    virtual ESoundAPI GetType(void) {
      return E_SND_SDL;
    };

  public:
    virtual BOOL StartUp(BOOL bReport);
    virtual void ShutDown(void);

    virtual void CopyMixerBuffer(SLONG slMixedSize);
    virtual SLONG PrepareSoundBuffer(void);

    virtual void Mute(BOOL &bSetSoundMuted);
};

#endif // SE1_SDL

#endif // include-once check
