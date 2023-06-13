//
// HelperFunctions.h
// Copyright (c) 2008-2014 HostileFork.com
//
// This file is part of TitleWait
// See http://titlewait.hostilefork.com
//
// TitleWait is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// TitleWait is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with TitleWait.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef __TITLEWAIT_HELPERFUNCTIONS_H__
#define __TITLEWAIT_HELPERFUNCTIONS_H__

#include <string>
#include <cstdarg>
#include "windows.h"

// Microsoft never really cares for standards, do they?
// http://stackoverflow.com/a/3448308
#include <ciso646>

PCHAR* CommandLineToArgvA(PCHAR CmdLine, int* _argc);

void Verify_Core(LPWSTR msg, BOOL expr, UINT lineNumber);
#define Verify(msg, expr) Verify_Core((msg), (expr), __LINE__)
#define NotReached(msg) Verify_Core((msg), FALSE, __LINE__)

void WindowsVerify_Core(
    LPWSTR functionName, BOOL windowsReturnBoolean, UINT lineNumber
);
#define WindowsVerify(functionName,errorCode) \
    WindowsVerify_Core((functionName), (errorCode), __LINE__)

// Helpful for noting which windows functions have been checked in the API
// docs that return void.  (Annotates you don't need WindowsVerify on them)
#define WindowsVoid(X) X

void ExitProgramOnWindowsError_Core(
    LPWSTR functionName, DWORD errorCode, UINT lineNumber
);
#define ExitProgramOnWindowsError(functionName, errorCode) \
    ExitProgramOnWindowsError_Core((functionName),(errorCode), __LINE__)


int debugInfo(LPWSTR formatString, ...);
int debugInfoA(char* formatString, ...); // try not to use...

// Base 64 encoding is generally a useful thing to have around
BOOL base64_encode(
    const UCHAR* input, DWORD input_length, UCHAR* output, DWORD* output_length
);
BOOL base64_decode(
    const UCHAR* input, DWORD input_length, UCHAR* output, DWORD* output_length
);

#endif
