//
// ProcessMonitor.cpp
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

#include "ProcessMonitor.h"
#include "HelperFunctions.h"
#include "TitleWaitConfig.h"

HANDLE lastProcessExitedEvent = NULL;
HANDLE processListMutex = NULL;
int numProcesses = 0;
DWORD processIds[MAX_PATH];

// Using approach from Code Project:
// wait INFINITE on spawned processes with a spawned thread...
//
//     http://www.codeproject.com/KB/threads/asyncprocnotify.aspx
HANDLE processMonThreads[MAX_PATH];


DWORD WINAPI ProcessMonitorMain( LPVOID lpParam ) {
	HANDLE processHandle = lpParam;
	DWORD processId = GetProcessId(processHandle);

	debugInfo(L"Adding monitor for process id 0x%x", processId);

	if (WaitForSingleObject(processHandle, INFINITE) != WAIT_OBJECT_0) {
		WindowsVerify(L"WaitForSingleObject", FALSE);
	}

	if (WaitForSingleObject(processListMutex, INFINITE) != WAIT_OBJECT_0) {
		WindowsVerify(L"WaitForSingleObject", FALSE);
	}

	debugInfo(L"Removing monitor for process id 0x%x", processId);

	for (int index = 0; index < numProcesses; index++) {
		if (processIds[index] == processId) {
			if (index == numProcesses-1)
				numProcesses--;
			else {
				processIds[index] = processIds[numProcesses-1];
				processMonThreads[index] = processMonThreads[numProcesses-1];
				numProcesses--;
			}

			if (numProcesses == 1) {
				// down to just the nested guy (another titlewait)...
				// when he dies, the debugger finally dies...
				
				debugInfo(
					L"Last Child Process Exited, id 0x%x",
					GetProcessId(processHandle)
				);
				WindowsVerify(L"SetEvent", SetEvent(lastProcessExitedEvent));

				// We don't have to finish the nested executive gracefully,
				// though I always like graceful solutions.  Can I figure out
				// how to signal back to the inherited handles?
				debugInfo(L"Killing nested executive with ExitProcess.");
				ExitProcess(1);
			}
			WindowsVerify(L"ReleaseMutex", ReleaseMutex(processListMutex));

			return 0;
		}
	}

	WindowsVerify(L"ReleaseMutex", ReleaseMutex(processListMutex));
	ExitProgramOnWindowsError(L"TitleWait::ProcessMonitorMain::Thread Not Found", 0);
	return 0;
}