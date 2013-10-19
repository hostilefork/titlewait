//
// ProgramOptions.h
// Copyright (c) 2008 HostileFork.com
//
// Lame program option reading routines.  When I started this
// project in 2008, I really wanted to keep it lightweight and
// not rely on boost.  I also wanted to make sure it supported
// Unicode, which simpler libraries did not.  As the program grew
// more complex, and began to need many more options (and quite
// possibly config files) I concluded avoiding boost was myopic.
//
// But I did a small modification to read a more or less
// boost-compatible format, so this could be upgraded easily if
// anyone had an interest.  Therefore the options look like:
//
//     --optionname="stuff in quotes, usually"
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

#include <vector>
#include <string>
#include "windows.h"
#include "HelperFunctions.h"

enum OPTION {
	optionFirst = 0,
	optionHelp = 0,
	optionRegex,
	optionClose,
	optionAll,
	optionVerbose,
	optionFrequency,
	optionTimeout,
	optionCrashsnapshot,
	optionTitlesnapshot,
	optionProgram,
	optionArgs,
	optionDefer,
	optionX,
	optionY,
	optionWidth,
	optionHeight,
	optionShutdownevent,
	optionMax
};

extern std::wstring optionNames[optionMax];

//
// Program configuration.
// Where possible they should match the string in the command line
//
struct CONFIG
{
	bool help; // help invocation

	std::wstring regex; // title regular expression
	
	bool close; // close the window once found?

	bool verbose; // send debug information to stderr?
	DWORD frequency; // poll time in seconds
	DWORD timeout; // timeout interval in seconds, zero means no timeout

	std::wstring crashsnapshot; // path to bitmap to capture
	std::wstring titlesnapshot;

	// Options if we are running as a debugger
	std::wstring program; // full path of program to run
	std::wstring args; // command line arguments
	bool defer;
	DWORD x;
	DWORD y;
	DWORD width;
	DWORD height;

	// search all windows for the title regex, not just those in the
	// spawned processes
	bool all; 

	// not a user option -- this is how TitleWait works around child process termination issues!
	HANDLE shutdownevent;

public:
	// Default values.
	CONFIG () :
		help (false),
		close (false),
		verbose (true),
		frequency (3),
		timeout (0),
		crashsnapshot (),
		titlesnapshot (),
		defer (false),
		x (CW_USEDEFAULT),
		y (CW_USEDEFAULT),
		width (CW_USEDEFAULT),
		height (CW_USEDEFAULT),
		all (false)
	{
	}

	bool ProcessCommandLineArgs(int numberOfArgs, LPWSTR commandLineArgs[]);

	bool shouldRepositionWindow() const {
		return (x != CW_USEDEFAULT) or (y != CW_USEDEFAULT)
			or (width != CW_USEDEFAULT) or (height != CW_USEDEFAULT);
	}
};

extern const CONFIG & config;
extern CONFIG configWritable;