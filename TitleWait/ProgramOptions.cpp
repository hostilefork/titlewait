//
// ProgramOptions.cpp
// Copyright (c) 2008 HostileFork.com
//
// See comments in ProgramOptions.h
//
// This file is part of TitleWait
// See http://hostilefork.com/titlewait/
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

#include "ProgramOptions.h"
#include "HelperFunctions.h"

// Nasty global variables, but Windows isn't particularly good about making
// it easy to pass 64-bit compatible pointers around in callbacks, ever since
// the trick of poking a 32-bit value into GWL_USER went away...
CONFIG configWritable;

// Use this alias to access the program options if you don't need to change them
const CONFIG& config = configWritable;

std::wstring optionNames[optionMax] = {
	L"help",
	L"regex",
	L"close",
	L"all",
	L"verbose",
	L"frequency",
	L"timeout",
	L"crashsnapshot",
	L"titlesnapshot",
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

bool CONFIG::ProcessCommandLineArgs(int numberOfArgs, LPWSTR commandLineArgs[]) {

	bool result = true;

	configWritable.help = (numberOfArgs <= 1);

	for (int argIndex = 1; argIndex < numberOfArgs; argIndex++) {

		bool optionMatched = false;
		bool validValue = true;

		std::wstring arg (commandLineArgs[argIndex]);
		for (int optionInt = 0; optionInt < optionMax; optionInt++) {
			OPTION option = static_cast<OPTION>(optionInt);

			std::wstringstream decoratedOption;
			decoratedOption << L"--" << optionNames[optionInt];
			if (option != optionHelp) {
				decoratedOption << "=";
			}

			// We test for the zero position; very beginning of option
			if (arg.find(decoratedOption.str().c_str()) != 0) {
				continue;
			}

			optionMatched = true;

			// Substring from past the equals sign to end of string
			// This means we may get quotes around string arguments
			std::wstring value (arg.substr(decoratedOption.str().length(), std::string::npos));
			switch (option) {
			case optionHelp:
				configWritable.help = true;
				validValue = true;
				break;

			case optionRegex:
				validValue = GetStringOption(
					optionNames[optionInt],
					value,
					/*&*/ configWritable.regex
				);
				break;

			case optionClose:
				// NOTE: Can't set a timeOut without closing when found
				// require parameter to be passed in as close=TRUE
				validValue = GetBoolOption(
					optionNames[optionInt],
					value,
					/*&*/ configWritable.close
				);	
				break;

			case optionAll:
				validValue = GetBoolOption(
					optionNames[optionInt],
					value,
					/*&*/ configWritable.all
				);
				break;

			case optionVerbose:
				validValue = GetBoolOption(
					optionNames[optionInt],
					value,
					/*&*/ configWritable.verbose
				);
				break;

			case optionFrequency:
				validValue = GetDwordOption(
					optionNames[optionInt],
					value,
					/*&*/ configWritable.frequency
				);
				break;
	
			case optionTimeout:
				validValue = GetDwordOption(
					optionNames[optionInt],
					value,
					/*&*/ configWritable.timeout
				);
				break;

			case optionCrashsnapshot:
				validValue = GetStringOption(
					optionNames[optionInt],
					value,
					/*&*/ configWritable.crashsnapshot
				);
				break;

			case optionTitlesnapshot:
				validValue = GetStringOption(
					optionNames[optionInt],
					value,
					/*&*/ configWritable.titlesnapshot
				);
				break;

			case optionProgram:
				validValue = GetStringOption(
					optionNames[optionInt],
					value,
					/*&*/ configWritable.program
				);
				break;

			case optionArgs:
				validValue = GetStringOption(
					optionNames[optionInt],
					value,
					/*&*/ configWritable.args
				);
				break;

			case optionDefer:
				validValue = GetBoolOption(
					optionNames[optionInt],
					value,
					/*&*/ configWritable.defer
				);
				break;

			case optionX:
				validValue = GetDwordOption(
					optionNames[optionInt],
					value,
					/*&*/ configWritable.x
				);
				break;

			case optionY:
				validValue = GetDwordOption(
					optionNames[optionInt],
					value,
					/*&*/ configWritable.y
				);
				break;

			case optionWidth:
				validValue = GetDwordOption(
					optionNames[optionInt],
					value,
					/*&*/ configWritable.width
				);
				break;

			case optionHeight:
				validValue = GetDwordOption(
					optionNames[optionInt],
					value,
					/*&*/ configWritable.height
				);
				break;

			case optionShutdownevent:
				{
					void* shutdowneventPtr;
					validValue = GetPointerOption(
						optionNames[optionInt],
						value,
						/*&*/ shutdowneventPtr
					);
					configWritable.shutdownevent = static_cast<HANDLE>(shutdowneventPtr);
				}
				break;

			default:
				Verify(L"Unreachable code", FALSE);
			}

			if (not validValue) {
				std::wcerr << L"Invalid option value for " << optionNames[optionInt];
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