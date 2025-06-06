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

#ifndef SE_INCL_DYNAMICSTACKARRAY_H
#define SE_INCL_DYNAMICSTACKARRAY_H

#ifdef PRAGMA_ONCE
  #pragma once
#endif

#include <Engine/Templates/DynamicArray.h>

// Template class for stack-like array with dynamic allocation of objects
template<class Type>
class CDynamicStackArray : public CDynamicArray<Type> {
  public:
    INDEX da_ctUsed; // Number of used objects in array
    INDEX da_ctAllocationStep; // How many elements to allocate when stack overflows

  public:
    // Default constructor
    inline CDynamicStackArray(void);

    // Copy constructor
    inline CDynamicStackArray(const CDynamicStackArray<Type> &arOriginal);

    // Destructor
    inline ~CDynamicStackArray(void);

    // Set how many elements to allocate when stack overflows
    inline void SetAllocationStep(INDEX ctStep);

    // Destroy all objects and reset the stack to the initial (empty) state
    inline void Clear(void);

  public:
    // Add new object on top of the stack
    inline Type &Push(void);

    // Add a number of new objects on top of the stack
    inline Type *Push(INDEX ct);

    // Remove all objects from the stack, but keep stack space
    inline void PopAll(void);

    // Remove last object from the stack
    inline void Pop(void);

    // Destroy a given object
    void Delete(Type *ptObject);

  public:
    // Random access operator
    inline Type &operator[](INDEX iObject);

    // Constant random access operator
    inline const Type &operator[](INDEX iObject) const;

    // Get number of used elements in the stack
    INDEX Count(void) const;

    // Get index of a object from its pointer
    INDEX Index(Type *ptObject);

    // Get array of pointers to elements (used for sorting elements by sorting pointers)
    Type **GetArrayOfPointers(void);

    // Assignment operator
    CDynamicStackArray<Type> &operator=(const CDynamicStackArray<Type> &arOriginal);
};

// [Cecil] Inline definition
#include <Engine/Templates/DynamicStackArray.cpp>

#endif // include-once check
