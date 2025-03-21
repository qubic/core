// Minimal interface definitions for using SetJump() and LongJump() of edk2_mdepkg

// Based on code of BaseLib.h in edk2_mdepkg with the following license:
/*
Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*/

#pragma once

// Jump buffer used by SetJump() and LongJump(). Must be 8-byte aligned.
struct LongJumpBuffer
{
    unsigned char registerStorage[248];
};

static_assert(sizeof(LongJumpBuffer) == 248, "Unexpected struct size. Check with X64 version of BASE_LIBRARY_JUMP_BUFFER in BaseLib.h");


extern "C"
{

    /**
      Saves the current CPU context that can be restored with a call to LongJump()
      and returns 0.

      Saves the current CPU context in the buffer specified by JumpBuffer and
      returns 0. The initial call to SetJump() must always return 0. Subsequent
      calls to LongJump() cause a non-zero value to be returned by SetJump().

      If JumpBuffer is NULL, then ASSERT().
      For Itanium processors, if JumpBuffer is not aligned on a 16-byte boundary, then ASSERT().

      NOTE: The structure BASE_LIBRARY_JUMP_BUFFER is CPU architecture specific.
      The same structure must never be used for more than one CPU architecture context.
      For example, a BASE_LIBRARY_JUMP_BUFFER allocated by an IA-32 module must never be used from an x64 module.
      SetJump()/LongJump() is not currently supported for the EBC processor type.

      @param  JumpBuffer  A pointer to CPU context buffer.

      @retval 0 Indicates a return from SetJump().

    **/
    unsigned long long _cdecl SetJump(LongJumpBuffer* JumpBuffer);


    /**
      Restores the CPU context that was saved with SetJump().

      Restores the CPU context from the buffer specified by JumpBuffer. This
      function never returns to the caller. Instead is resumes execution based on
      the state of JumpBuffer.

      If JumpBuffer is NULL, then ASSERT().
      For Itanium processors, if JumpBuffer is not aligned on a 16-byte boundary, then ASSERT().
      If Value is 0, then ASSERT().

      @param  JumpBuffer  A pointer to CPU context buffer.
      @param  Value       The value to return when the SetJump() context is
                          restored and must be non-zero.

    **/
    void _cdecl LongJump(LongJumpBuffer* JumpBuffer, unsigned long long Value);

}
