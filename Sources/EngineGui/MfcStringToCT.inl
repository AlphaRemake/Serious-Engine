/* Copyright (c) 2025 Dreamy Cecil
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

#ifndef SE_INCL_MFCSTRINGTOCT_INL
#define SE_INCL_MFCSTRINGTOCT_INL

#ifdef PRAGMA_ONCE
  #pragma once
#endif

// Convert MFC's CString to engine's CTString
inline CTString MfcStringToCT(const CString &str) {
#if !defined(UNICODE)
  // Return as is
  return (LPCSTR)str;

#elif _MSC_VER > 1200
  // Convert through CStringA
  return CStringA(str).GetString();

#else
  // Convert through Windows methods
  const wchar_t *pstrWide = str;
  INDEX ct = str.GetLength();

  // Create null-terminated ANSI string
  char *pstrANSI = new char[ct + 1];
  pstrANSI[ct] = 0;

  // Convert Unicode string to system locale (ANSI code page)
  WideCharToMultiByte(CP_ACP, 0, pstrWide, -1, pstrANSI, ct, NULL, NULL);

  CTString strOut(pstrANSI);
  delete[] pstrANSI;

  return strOut;
#endif
};

#endif // include-once check
