//
// ProgramOptions.cpp
// Copyright (c) 2008 HostileFork.com
//
// See comments in TitleWaitConfig.h
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

// Nasty global variables, but Windows isn't particularly good about making
// it easy to pass 64-bit compatible pointers around in callbacks, ever since
// the trick of poking a 32-bit value into GWL_USER went away...
TitleWaitConfig configWritable;

// Use this to access the program options if you don't need to change them
TitleWaitConfig const * config = &configWritable;

std::wstring TitleWaitConfig::optionNames[OptionMax] = {
	L"help",
	L"regex",
	L"close",
	L"all",
	L"verbose",
	L"frequency",
	L"timeout",
	L"crashsnapshot",
	L"titlesnapshot",
	L"timeoutsnapshot",
	L"program",
	L"args",
	L"defer",
	L"x",
	L"y",
	L"width",
	L"height",
	L"shutdownevent"
};

// Using what boost tolerates for boolean true and false...
// http://stackoverflow.com/questions/15629771/
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
	if (true) {
		result = value;
		return true;
	}
	return false;
}

bool TitleWaitConfig::ProcessCommandLineArgs(
	int numberOfArgs, LPWSTR commandLineArgs[]
) {
	bool result = true;

	configWritable.help = (numberOfArgs <= 1);

	for (int argIndex = 1; argIndex < numberOfArgs; argIndex++) {

		bool optionMatched = false;
		bool validValue = true;

		std::wstring arg (commandLineArgs[argIndex]);
		for (int optInt = 0; optInt < OptionMax; optInt++) {
			Option option = static_cast<Option>(optInt);

			std::wstringstream decoratedOption;
			decoratedOption << L"--" << optionNames[optInt];
			if (option != HelpOption) {
				decoratedOption << "=";
			}

			// We test for the zero position; very beginning of option
			if (arg.find(decoratedOption.str().c_str()) != 0) {
				continue;
			}

			optionMatched = true;

			// Substring from past the equals sign to end of string
			// This means we may get quotes around string arguments
			std::wstring value (
				arg.substr(decoratedOption.str().length(), std::string::npos)
			);
			switch (option) {
			case HelpOption:
				configWritable.help = true;
				validValue = true;
				break;

			case RegexOption:
				validValue = GetStringOption(
					optionNames[optInt],
					value,
					/*&*/ configWritable.regex
				);
				break;

			case CloseOption:
				// NOTE: Can't set a timeOut without closing when found
				// require parameter to be passed in as close=TRUE
				validValue = GetBoolOption(
					optionNames[optInt],
					value,
					/*&*/ configWritable.close
				);	
				break;

			case AllOption:
				validValue = GetBoolOption(
					optionNames[optInt],
					value,
					/*&*/ configWritable.all
				);
				break;

			case VerboseOption:
				validValue = GetBoolOption(
					optionNames[optInt],
					value,
					/*&*/ configWritable.verbose
				);
				break;

			case FrequencyOption:
				validValue = GetDwordOption(
					optionNames[optInt],
					value,
					/*&*/ configWritable.frequency
				);
				break;
	
			case TimeoutOption:
				validValue = GetDwordOption(
					optionNames[optInt],
					value,
					/*&*/ configWritable.timeout
				);
				break;

			case CrashSnapshotOption:
				validValue = GetStringOption(
					optionNames[optInt],
					value,
					/*&*/ configWritable.crashsnapshot
				);
				break;

			case TitleSnapshotOption:
				validValue = GetStringOption(
					optionNames[optInt],
					value,
					/*&*/ configWritable.titlesnapshot
				);
				break;

			case TimeoutSnapshotOption:
				validValue = GetStringOption(
					optionNames[optInt],
					value,
					/*&*/ configWritable.timeoutsnapshot
				);
				break;

			case ProgramOption:
				validValue = GetStringOption(
					optionNames[optInt],
					value,
					/*&*/ configWritable.program
				);
				break;

			case ArgsOption:
				validValue = GetStringOption(
					optionNames[optInt],
					value,
					/*&*/ configWritable.args
				);
				break;

			case DeferOption:
				validValue = GetBoolOption(
					optionNames[optInt],
					value,
					/*&*/ configWritable.defer
				);
				break;

			case XOption:
				validValue = GetDwordOption(
					optionNames[optInt],
					value,
					/*&*/ configWritable.x
				);
				break;

			case YOption:
				validValue = GetDwordOption(
					optionNames[optInt],
					value,
					/*&*/ configWritable.y
				);
				break;

			case WidthOption:
				validValue = GetDwordOption(
					optionNames[optInt],
					value,
					/*&*/ configWritable.width
				);
				break;

			case HeightOption:
				validValue = GetDwordOption(
					optionNames[optInt],
					value,
					/*&*/ configWritable.height
				);
				break;

			case ShutdownEventOption: {
				void* shutdowneventPtr;
				validValue = GetPointerOption(
					optionNames[optInt],
					value,
					/*&*/ shutdowneventPtr
				);
				configWritable.shutdownevent = 
					static_cast<HANDLE>(shutdowneventPtr);
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
