//
// HelperFunctions.cpp
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

#include <stdlib.h>
#include <stdio.h>
#include <sstream>

#include "TitleWait.h"
#include "HelperFunctions.h"
#include "TitleWaitConfig.h" // for main return codes?

// from http://alter.org.ua/docs/win/args/
// Copyright (c) 2002-2013 by Alter aka Alexander A. Telyatnikov
// License: "You can use, modify and redistribute published materials
// for non-commecial purposes keeping Copyright information."

PCHAR* CommandLineToArgvA(PCHAR CmdLine, int* _argc)
{
    PCHAR* argv;
    PCHAR _argv;
    size_t len;
    ULONG argc;
    CHAR a;
    size_t i;
    size_t j;

    BOOLEAN  in_QM;
    BOOLEAN  in_TEXT;
    BOOLEAN  in_SPACE;

    len = strlen(CmdLine);
    i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

    argv = (PCHAR*)GlobalAlloc(GMEM_FIXED, i + (len+2)*sizeof(CHAR));

    _argv = (PCHAR)(((PUCHAR)argv)+i);

    argc = 0;
    argv[argc] = _argv;
    in_QM = FALSE;
    in_TEXT = FALSE;
    in_SPACE = TRUE;
    i = 0;
    j = 0;

    while(a = CmdLine[i]) {
        if(in_QM) {
            if(a == '\"') {
                if (i==0 or CmdLine[i-1] != '\\') {
                    in_QM = FALSE;
                } else {
                    // patch to handle escaped quotes --
                    // we copied the backslash, overwrite with quote
                    _argv[j] = a;
                    j++;
                }
            } else {
                _argv[j] = a;
                j++;
            }
        } else {
            switch(a) {
            case '\"':
                if (i==0 or CmdLine[i-1] != '\\')
                    in_QM = TRUE;
                in_TEXT = TRUE;
                if (in_SPACE) {
                    argv[argc] = _argv+j;
                    argc++;
                }
                in_SPACE = FALSE;
                break;
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                if (in_TEXT) {
                    _argv[j] = '\0';
                    j++;
                }
                in_TEXT = FALSE;
                in_SPACE = TRUE;
                break;
            default:
                in_TEXT = TRUE;
                if(in_SPACE) {
                    argv[argc] = _argv+j;
                    argc++;
                }
                _argv[j] = a;
                j++;
                in_SPACE = FALSE;
                break;
            }
        }
        i++;
    }
    _argv[j] = '\0';
    argv[argc] = NULL;

    (*_argc) = argc;
    return argv;
}

// Convert a std::wstring to a std::string
//
// https://stackoverflow.com/a/69410299
//
std::string wide_string_to_string(const std::wstring& wide_string)
{
    if (wide_string.empty())
        return "";

    const auto size_needed = WideCharToMultiByte(
        CP_UTF8,
        0,
        &wide_string.at(0),
        (int)wide_string.size(),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (size_needed <= 0)
        throw std::runtime_error(
            "WideCharToMultiByte() failed: " + std::to_string(size_needed)
        );

    std::string result(size_needed, 0);
    WideCharToMultiByte(
        CP_UTF8,
        0,
        &wide_string.at(0),
        (int)wide_string.size(),
        &result.at(0),
        size_needed,
        nullptr,
        nullptr
    );

    return result;
}


// Print a windows error
void WindowsErrorToStderr(LPWSTR functionName, DWORD errorCode, UINT lineNumber)
{
    const DWORD size = 100+1;
    WCHAR buffer[size];
    if (FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        errorCode,
        0,
        (LPWSTR) &buffer,
        size,
        NULL))
    {
        fwprintf(
            stderr,
            L"error: 0x%x on line %d in %s - %s",
            errorCode,
            lineNumber,
            functionName,
            buffer
        );
    } else {
        DWORD formatMessageErrorCode = GetLastError();
        fwprintf(
            stderr,
            L"error: 0x%x on line %d in %s (FormatMessage failed with 0x%x)\n",
            errorCode,
            lineNumber,
            functionName,
            formatMessageErrorCode
        );
    }
    ExitProcess(TitleWait::InternalErrorReturn);
}


void ExitProgramOnWindowsError_Core(
    LPWSTR functionName, DWORD errorCode, UINT lineNumber
) {
    fwprintf(
        stderr,
        L"Report @ https://github.com/hostilefork/titlewait/issues\n"
    );
    WindowsErrorToStderr(functionName, errorCode, lineNumber);
}


// If there is an error with Windows, fail noisily.
void WindowsVerify_Core(
    LPWSTR functionName, BOOL windowsReturn, UINT lineNumber
) {
    if (!windowsReturn) {
        ExitProgramOnWindowsError_Core(functionName, GetLastError(), lineNumber);
    }
}


void Verify_Core(LPWSTR msg, BOOL expr, UINT lineNumber) {
    if (expr) {
        return;
    }

    // for thread safety put together as one call.
    std::wstringstream formatStream;
    formatStream << L"Terminating program on line %d: condition %s\n";
    formatStream << L"Report @ https://github.com/hostilefork/titlewait/issues\n";

    fwprintf(stderr, formatStream.str().c_str(), lineNumber, msg);
    ExitProcess(TitleWait::InternalErrorReturn);
}


int debugInfo(LPWSTR formatString, ...){
    if (config and config->verbose) {
        int retval=0;
        va_list ap;

        // for thread safety put together as one call.
        std::wstringstream formatStream;
        formatStream << L"verbose: " << formatString << "\n";
        va_start(ap, formatString);
        retval = vfwprintf(stderr, formatStream.str().c_str(), ap);
        va_end(ap);

        return retval;
    }
    return 0;
}


int debugInfoA(char* formatString, ...){
    if (config and config->verbose) {
        int retval=0;
        va_list ap;

        // for thread safety put together as one call
        std::stringstream formatStream;
        formatStream << "verbose: " << formatString << "\n";
        va_start(ap, formatString);
        retval = vfprintf(stderr, formatStream.str().c_str(), ap);
        va_end(ap);

        return retval;
    }
    return 0;
}


// http://gd.tuwien.ac.at/infosys/mail/vm/base64-encode.c
// (public domain)

UCHAR alphabet[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
    't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', '+', '/'
};


BOOL base64_encode(
    const UCHAR* input, DWORD input_length, UCHAR* output, DWORD* output_length
) {
    int cols = 0;
    int bits = 0;
    int char_count = 0;

    DWORD input_index = 0;
    DWORD output_index = 0;
    UCHAR c;
    while (input_index < input_length) {
        c = input[input_index++];
        bits += c;
        char_count++;
        if (char_count == 3) {
            output[output_index++] = alphabet[bits >> 18];
            output[output_index++] = alphabet[(bits >> 12) & 0x3f];
            output[output_index++] = alphabet[(bits >> 6) & 0x3f];
            output[output_index++] = alphabet[bits & 0x3f];
            cols += 4;
            if (cols == 72) {
                output[output_index++] = '\n';
                cols = 0;
            }
            bits = 0;
            char_count = 0;
        } else {
            bits <<= 8;
        }
    }

    if (char_count != 0) {
        bits <<= 16 - (8 * char_count);
        output[output_index++] = alphabet[bits >> 18];
        output[output_index++] = alphabet[(bits >> 12) & 0x3f];

        if (char_count == 1) {
            output[output_index++] = '=';
            output[output_index++] = '=';
        } else {
            output[output_index++] = alphabet[(bits >> 6) & 0x3f];
            output[output_index++] = '=';
        }

        if (cols > 0)
            output[output_index++] = '\n';
    }

    *output_length = output_index;
    return TRUE;
}


// http://gd.tuwien.ac.at/infosys/mail/vm/base64-decode.c
// (public domain)

BOOL base64_decode(
    const UCHAR* input, DWORD input_length, UCHAR* output, DWORD* output_length
) {
    static char inalphabet[256];
    static char decoder[256];

    for (int i = (sizeof alphabet) - 1; i >= 0 ; i--) {
        inalphabet[alphabet[i]] = 1;
        decoder[alphabet[i]] = i;
    }

    int bits = 0;
    int char_count = 0;
    int errors = 0;

    int input_index = 0;
    int output_index = 0;
    UCHAR c = 0;

    while (input_index < input_length) {
        c = input[input_index++];
        if (c == '=')
            break;
        if (c > 255 or ! inalphabet[c])
            continue;
        bits += decoder[c];
        char_count++;
        if (char_count == 4) {
            output[output_index++] = (bits >> 16);
            output[output_index++] = ((bits >> 8) & 0xff);
            output[output_index++] = (bits & 0xff);
            bits = 0;
            char_count = 0;
        } else {
            bits <<= 6;
        }
    }

    /* c == '=' */
    switch (char_count) {
    case 1:
        NotReached(L"base64 encoding incomplete: at least 2 bits missing");
        errors++;
        break;
    case 2:
        output[output_index++] = (bits >> 10);
        break;
    case 3:
        output[output_index++] = (bits >> 16);
        output[output_index++] = ((bits >> 8) & 0xff);
        break;
    }

    *output_length = output_index;
    return errors ? FALSE : TRUE;
}
