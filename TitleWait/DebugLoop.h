//
// DebugLoop.h
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

#include <string>
#include "windows.h"

struct DEBUGLOOPARGS {
	std::wstring commandLine;
	STARTUPINFO* startupInfo;
	LPDWORD millisecondsLeft;
	HANDLE showMessageEvent; // we tell GUI when we notice another process
	HANDLE messageRetryEvent; // GUI thread tells us when they said Retry.  We must poll to get this.
	std::wstring executableImageName;
};

DWORD WINAPI DebugLoopMain(LPVOID lpParam); // returns a MAINRETURN