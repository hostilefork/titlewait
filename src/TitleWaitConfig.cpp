//
// TitleWaitConfig.cpp
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

#include <iostream>
#include <string>
#include <sstream>

#include "TitleWaitConfig.h"
#include "HelperFunctions.h"

std::wstring TitleWaitConfig::optionNames[OptionMax] = {
    L"help",
    L"program",
    L"args",
    L"regex",
    L"frequency",
    L"regexsnapshot",
    L"closeonmatch",
    L"searchall",
    L"timeout",
    L"timeoutsnapshot",
    L"crashsnapshot",
    L"defer",
    L"x",
    L"y",
    L"width",
    L"height",
    L"verbose",
    L"shutdownevent"
};


std::wstring TitleWaitConfig::optionDescriptions[OptionMax] = {
    L"Show this help information",
    L"Path to the program to run (required)",
    L"Arguments to the program",
    L"The regular expression you want to search the title for",
    L"How often to check the regex on the title (in seconds, 1 is default)",
    L"Path to screen snapshot to take if expression matches title (.BMP)",
    L"Send a window close message to the application if regex matches",
    L"Search windows of all processes for regex, not just those this spawns",
    L"Number of seconds the program run before closing it due to timeout",
    L"Path to screen snapshot to take if timeout period elapses (.BMP)",
    L"Path to screen snapshot to take if program crashes (.BMP)",
    L"If the program is already running, wait on execution of new instance",
    L"X position to ask program window to move to",
    L"Y position to ask program window to move to",
    L"Requested width of program window",
    L"Requested height of program window",
    L"Verbose debugging mode",
    L"Internal use only, passed to nested executive for debug control"
};


// Using what boost tolerates for boolean true and false...
// http://stackoverflow.com/questions/15629771/
//
bool GetBoolOption(
    std::wstring const & optionName,
    std::wstring const & value,
    bool & result)
{
    if (value == L"true" || value == L"on" || value == L"yes" || value == L"1") {
        result = true;
        return true;
    }
    if (value == L"false" || value == L"off" || value == L"no" || value == L"0") {
        result = false;
        return true;
    }
    return false;
}

bool GetDwordOption(
    std::wstring const & optionName,
    std::wstring const & value,
    DWORD & result)
{
    std::wistringstream ss (value);
    if (ss >> result) {
        return true;
    }
    return false;
}

bool GetPointerOption(
    std::wstring const & optionName,
    std::wstring const & value,
    void* & result)
{
    std::wistringstream ss (value);
    if (ss >> result) {
        return true;
    }
    return false;
}

bool GetStringOption(
    std::wstring const & optionName,
    std::wstring const & value,
    std::wstring & result)
{
    // Can we just trust that quotes have been handled properly?
    // It seems like Windows lets us assume so.  :-/
    //
    if (true) {
        result = value;
        return true;
    }
    return false;
}

bool TitleWaitConfig::ProcessCommandLineArgs(
    int numArgs, LPWSTR programArgs[]
) {
    this->numArgs = numArgs;
    this->programArgs = programArgs;

    bool result = true;

    help = (numArgs <= 1);

    for (int argIndex = 1; argIndex < numArgs; argIndex++) {
        bool optionMatched = false;
        bool validValue = true;

        std::wstring arg (programArgs[argIndex]);
        for (int optInt = 0; optInt < OptionMax; optInt++) {
            Option option = static_cast<Option>(optInt);

            std::wstringstream decoratedOption;
            decoratedOption << L"--" << optionNames[optInt];
            if (option != HelpOption)
                decoratedOption << "=";

            // We test for the zero position; very beginning of option
            //
            if (arg.find(decoratedOption.str().c_str()) != 0) {
                continue;
            }

            optionMatched = true;

            // Substring from past the equals sign to end of string
            // This means we may get quotes around string arguments
            //
            std::wstring value (
                arg.substr(decoratedOption.str().length(), std::string::npos)
            );

            switch (option) {
              case HelpOption:
                help = true;
                validValue = true;
                break;

              case RegexOption:
                validValue = GetStringOption(
                    optionNames[optInt],
                    value,
                    /*&*/ regex
                );
                break;

              case SearchAllOption:
                validValue = GetBoolOption(
                    optionNames[optInt],
                    value,
                    /*&*/ searchAll
                );
                break;

              case VerboseOption:
                validValue = GetBoolOption(
                    optionNames[optInt],
                    value,
                    /*&*/ verbose
                );
                break;

              case FrequencyOption:
                validValue = GetDwordOption(
                    optionNames[optInt],
                    value,
                    /*&*/ frequency
                );
                break;

              case TimeoutOption:
                validValue = GetDwordOption(
                    optionNames[optInt],
                    value,
                    /*&*/ timeout
                );
                break;

              case CrashSnapshotOption:
                validValue = GetStringOption(
                    optionNames[optInt],
                    value,
                    /*&*/ crashSnapshot
                );
                break;

              case RegexSnapshotOption:
                validValue = GetStringOption(
                    optionNames[optInt],
                    value,
                    /*&*/ regexSnapshot
                );
                break;

              case CloseOnMatchOption:
                validValue = GetBoolOption(
                    optionNames[optInt],
                    value,
                    /*&*/ closeOnMatch
                );
                break;

              case TimeoutSnapshotOption:
                validValue = GetStringOption(
                    optionNames[optInt],
                    value,
                    /*&*/ timeoutSnapshot
                );
                break;

              case ProgramOption:
                validValue = GetStringOption(
                    optionNames[optInt],
                    value,
                    /*&*/ program
                );
                break;

              case ArgsOption:
                validValue = GetStringOption(
                    optionNames[optInt],
                    value,
                    /*&*/ args
                );
                break;

              case DeferOption:
                validValue = GetBoolOption(
                    optionNames[optInt],
                    value,
                    /*&*/ defer
                );
                break;

              case XOption:
                validValue = GetDwordOption(
                    optionNames[optInt],
                    value,
                    /*&*/ x
                );
                break;

              case YOption:
                validValue = GetDwordOption(
                    optionNames[optInt],
                    value,
                    /*&*/ y
                );
                break;

              case WidthOption:
                validValue = GetDwordOption(
                    optionNames[optInt],
                    value,
                    /*&*/ width
                );
                break;

              case HeightOption:
                validValue = GetDwordOption(
                    optionNames[optInt],
                    value,
                    /*&*/ height
                );
                break;

              case ShutdownEventOption: {
                void* shutdownEventPtr;
                validValue = GetPointerOption(
                    optionNames[optInt],
                    value,
                    /*&*/ shutdownEventPtr
                );
                shutdownEvent = static_cast<HANDLE>(shutdownEventPtr);
                break;
            }

              default:
                Verify(L"Unreachable code", FALSE);
            }

            if (not validValue) {
                std::wcerr <<
                    L"Invalid option value for " <<
                    optionNames[optInt];
                std::wcerr << L": " << value << L'\n';
                result = false;
            }

            break;
        }

        if (not optionMatched) {
            std::wcerr << L"Unknown command line argument: " << arg << L'\n';
            result = false;
        }
    }

    debugInfo(L"Done processing command line args for %s", GetCommandLineW());

    return result;
}


std::wstring TitleWaitConfig::RegenerateCommandLine() const
{
    std::wstringstream commandLine;

    DWORD moduleNameSize;
    TCHAR moduleName[MAX_PATH];
    moduleNameSize = GetModuleFileName(
        NULL, moduleName, sizeof(moduleName)
    );
    Verify(L"GetmoduleName returned string longer than MAX_PATH",
        moduleNameSize < sizeof(moduleName)
    );

    commandLine << L'"' << moduleName << L'"';

    for (int argIndex = 1; argIndex < numArgs; argIndex++) {
        commandLine << L' ';

        // must escape any single quotes with \"
        //
        for (int index = 0; index < wcslen(programArgs[argIndex]); index++) {
            if (programArgs[argIndex][index] == L'"') {
                commandLine << L'\\';
                commandLine << L'"';
            } else if (programArgs[argIndex][index] == L'=') {
                commandLine << L'=';
                commandLine << L'"';
            } else {
                commandLine << programArgs[argIndex][index];
            }
        }
        commandLine << L'"';
    }

    return commandLine.str();
}
