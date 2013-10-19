//
// TitleWait.h
// Copyright (c) 2008 HostileFork.com
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


//
// Possible exit codes for the program.  Callers may depend on these
// numbers; so keep them invariant, and retire or add numbers as
// new conditions arise.
//
enum MAINRETURN {
	// Everything was fine
	mainReturnSuccess = 0,

	// Generic internal error in TitleWait
	mainReturnInternalError = 1,

	// User invoked with bad command line arguments
	mainReturnBadArguments = 2,

	// User didn't supply a program to run
	mainReturnNoProgram = 3,

	// a timeout was given and we terminated abnormally
	mainReturnTimedOut = 4,

	// for some reason, our attempt to close via SC_CLOSE failed
	mainReturnWindowDidntClose = 5,

	// the spawned process crashed
	mainReturnRunCrashed = 6,

	// the spawned process closed itself
	mainReturnRunClosed = 7,

	// canceled because user didn't want to wait for previous instance
	mainReturnDeferCancel = 8
};