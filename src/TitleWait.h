//
// TitleWait.h
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

#ifndef __TITLEWAIT_TITLEWAIT_H__
#define __TITLEWAIT_TITLEWAIT_H__

#include "windows.h"
#include "TitleWaitConfig.h"

extern TitleWaitConfig const * config;

class TitleWait
{
public:
    //
    // Possible exit codes for the program.  Callers may depend on these
    // numbers; so keep them invariant, and retire or add numbers as
    // new conditions arise.  Description strings are in TitleWait.cpp
    //
    // "on POSIX-compatible systems, exit statuses are restricted
    //  to values 0-255, the range of an unsigned 8-bit integer"
    //
    //     http://en.wikipedia.org/wiki/Exit_status
    //
    enum MainReturn {
        ReturnMin = 0,
        SuccessReturn = 0,
        InternalErrorReturn = 1,
        BadArgumentsReturn = 2,
        NoProgramReturn = 3,
        TimeoutReturn = 4,
        WindowDidntCloseReturn = 5,
        CrashedReturn = 6,
        ClosedReturn = 7,
        DeferCancelledReturn = 8,
        ReturnMax
    };

    static std::wstring returnDescriptions[ReturnMax];

public:
    MainReturn doMain();
};

#endif
