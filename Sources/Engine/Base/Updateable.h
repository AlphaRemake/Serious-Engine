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

#ifndef SE_INCL_UPDATEABLE_H
#define SE_INCL_UPDATEABLE_H
#ifdef PRAGMA_ONCE
  #pragma once
#endif

// Object that can be updated to reflect changes in some changeable object(s)
// [Cecil] Unified logic of old CUpdateable & CUpdateableRT classes behind a template
template<bool bRealTime>
class TUpdateable {
  private:
    TICK ud_tckLastUpdateTime; // last time this object has been updated

  public:
    // Constructor
    TUpdateable(void);
    // Get time when last updated
    TICK LastUpdateTick(void) const;
    // Mark that the object has been updated
    void MarkUpdated(void);
    // Mark that the object has become invalid in spite of its time stamp
    void Invalidate(void);
};

// [Cecil] Define template methods
#include <Engine/Base/Updateable.inl>

#endif  /* include-once check. */
