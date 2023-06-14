//
// TitleWaitConfig.h
// Copyright (c) 2008 HostileFork.com
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

#ifndef __TITLEWAIT_TITLEWAITCONFIG_H__
#define __TITLEWAIT_TITLEWAITCONFIG_H__

#include <vector>
#include <string>
#include "windows.h"
#include "HelperFunctions.h"


//
// Lame program option reading routines.  When I started this project in 2008,
// I really wanted to keep it lightweight and not rely on boost.  I also wanted
// to make sure it supported Unicode, which simpler libraries did not.  As the
// program grew more complex, and began to need many more options (and quite
// possibly config files) I concluded avoiding boost was myopic.
//
// But I did a small modification to read a more or less boost-compatible
// format, so this could be upgraded easily if anyone had an interest.
// Therefore the options look like:
//
//     --optionname="stuff in quotes, usually"
//
struct TitleWaitConfig
{
    enum Option {
        OptionFirst = 0,
        HelpOption = 0,
        ProgramOption,
        ArgsOption,
        RegexOption,
        FrequencyOption,
        RegexSnapshotOption,
        CloseOnMatchOption,
        SearchAllOption,
        TimeoutOption,
        TimeoutSnapshotOption,
        CrashSnapshotOption,
        DeferOption,
        XOption,
        YOption,
        WidthOption,
        HeightOption,
        VerboseOption,
        ShutdownEventOption,
        OptionMax
    };

    static std::wstring optionNames[OptionMax];
    static std::wstring optionDescriptions[OptionMax];

    // First step was moving these into a class, second step would be providing
    // accessor functions... for the moment, clearer to just expose the values.

  public:
    bool help;  // help invocation

    std::wstring regex;  // title regular expression

    bool verbose;  // send debug information to stderr?
    DWORD frequency;  // poll time in seconds
    DWORD timeout;  // timeout interval in seconds, zero means no timeout

    std::wstring crashSnapshot;  // path to bitmap to capture
    std::wstring regexSnapshot;
    std::wstring timeoutSnapshot;

    std::wstring program;  // full path of program to run
    std::wstring args;  // command line arguments
    bool defer;
    DWORD x;
    DWORD y;
    DWORD width;
    DWORD height;

    // Search all windows for the title regex (not just those in the
    // spawned processes)
    //
    bool searchAll;

    bool closeOnMatch;

    // not a user option
    // (this is how TitleWait works around child process termination issues!)
    //
    HANDLE shutdownEvent;

    int numArgs;
    LPWSTR * programArgs;

  public:
    TitleWaitConfig () :
        help (false),
        regex (),
        verbose (false),
        frequency (3),
        timeout (0),
        crashSnapshot (),
        regexSnapshot (),
        closeOnMatch (false),
        timeoutSnapshot (),
        program (),
        args (),
        defer (false),
        x (CW_USEDEFAULT),
        y (CW_USEDEFAULT),
        width (CW_USEDEFAULT),
        height (CW_USEDEFAULT),
        searchAll (false),
        shutdownEvent (NULL),
        numArgs (0),
        programArgs (NULL)
    {
    }

    bool ProcessCommandLineArgs(int numArgs, LPWSTR programArgs[]);

    std::wstring RegenerateCommandLine() const;

    bool shouldMoveWindow() const {
        return (x != CW_USEDEFAULT) or (y != CW_USEDEFAULT)
            or (width != CW_USEDEFAULT) or (height != CW_USEDEFAULT);
    }
};

#endif
