//
// TitleWait.cpp
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
#include <iostream>
#include <sstream>
#include <regex>

#include "windows.h"

#include "TitleWait.h"
#include "HelperFunctions.h"
#include "ProgramOptions.h"
#include "DebugLoop.h"
#include "TitleMonitor.h"
#include "ProcessMonitor.h"

// No TCHAR legacy, assume WCHAR everywhere...
// http://stackoverflow.com/a/3002494

// Define wmain instead of _tmain, project defines _UNICODE
// http://stackoverflow.com/a/895894
int wmain(int numberOfArgs, WCHAR* commandLineArgs[])
{	
	if (!configWritable.ProcessCommandLineArgs(numberOfArgs, commandLineArgs)) {
		return mainReturnBadArguments;
	}

	if (config.help) {
		std::wcout << L"TitleWait (c) 2008 HostileFork.com\n";
		std::wcout << L"See http://hostilefork.com/titlewait/ for documentation and news\n\n";

		std::wcout << L"Option list:\n\n";

		for (int optionInt = 0; optionInt < optionMax; optionInt++) {
			OPTION option = static_cast<OPTION>(optionInt);

			std::wstringstream decoratedOption;
			decoratedOption << L"--" << optionNames[optionInt];

			std::wcout << decoratedOption.str() << L'\n';
		}
		return mainReturnSuccess;
	}

	if (config.program.empty()) {
		std::wcerr << L"No program specified via --program.  Nothing to do!\n";
		return mainReturnNoProgram;
	}

	// Sometimes if you are using a scripting tool to launch a traditional
	// windows app (instead of a console app) the tool will use CreateProcess.
	// There is, however, a strange bug in that:
	//
	// http://www.neowin.net/forum/topic/605165-wacked-out-createprocess-problem/
	//		
	// I was unable to deduce a better solution to the problem, so I decided
	// to integrate the solution into TitleWait.  Do note the caveats, on
	// debug-enabled programs this *will* start a debugger.  Use only if
	// other approaches are not working.

	MAINRETURN returnCode = mainReturnInternalError;
	DWORD millisecondsLeft =
		config.timeout == 0
		? INFINITE
		: config.timeout * 1000;

	if (config.shutdownevent != NULL) {

		// NESTED EXECUTIVE
		//
		// A debugger, once attached to a process, will end all the child 
		// processes when the parent process terminates.  In order to
		// work around this, I do not spawn the program you ask to run
		// directly--instead, I run another instance of TitleWait as a
		// proxy.  It spawns the user's command as a child process of it,
		// and then never exits--thus allowing the parent process to freely
		// tolerate closing of the user program and not kill programs
		// it spawned.
		//
		// Unfortunately, CreateProcess does not work for this.  system()
		// does.  So I use that.  However, it is quirky.  To get it to
		// handle spaces in the executable path, you need to say: 
		// 
		// cmd /c " "C:\Space In Directory Name\Program.exe" arg1 arg2 "

		std::stringstream commandLine;
		commandLine << "cmd /c" << ' ' << '"' << ' ';

		// We have unicode strings, but alas, we need ascii to call system()
		// http://stackoverflow.com/a/12097772
		// Use this until a better solution comes along...

		std::string programAscii (config.program.begin(), config.program.end());

		commandLine << '"';
		commandLine << programAscii;
		commandLine << '"';
		commandLine << ' ';

		std::string argsAscii (config.args.begin(), config.args.end());

		commandLine << argsAscii;

		commandLine << ' ';
		commandLine << '"';

		// http://www.devx.com/tips/Tip/15144
		// The system() function (declared in <cstdlib>) launches another
		// program from the current program.  As opposed to what most
		// users think, it doesn't return the exit code of the launched
		// process.  Instead, it returns the exit code of the shell that
		// launches the process in question. Consider the following example:
		//
		//		int systemResult = system("winword.exe");  
		// 
		// When the system() call returns, the value assigned to
		// systemResult is the exit code of the shell process that in turn
		// launches Word, not Word's own exit status. Thus, examining the
		// return code of system() is pretty useless in most cases. To
		// collect the exit code of a launched application, you have to
		// use an Interprocess Communication mechanism such as signals,
		// pipes etc.

		debugInfoA("system(%s) starting...", commandLine.str().c_str());

		int systemResult = system(commandLine.str().c_str());

		debugInfoA("system(%s) returned with result %d\n", commandLine.str().c_str(), systemResult);

		HANDLE neverEvent = CreateEvent( 
			NULL,  // default security attributes
			TRUE, // manual-reset event
			FALSE, // initial state is nonsignaled
			NULL // object name
			);

		debugInfo(L"Waiting indefinitely, must kill this thread");
		WindowsVerify(L"WaitForSingleObject", WaitForSingleObject(neverEvent, INFINITE) == WAIT_OBJECT_0);

		// Exit code of second process started is what we should detect in parent...
		return mainReturnRunClosed;
	}

	// Create the thread to begin execution on its own.
	DWORD titleMonitorThreadId;
	HANDLE titleMonitorThread = CreateThread( 
		NULL, // default security attributes
		0, // use default stack size  
		TitleMonitorThreadProc, // thread function name
		GetCommandLineW(), // argument to thread function 
		0, // use default creation flags 
		&titleMonitorThreadId
	);
	if (titleMonitorThread == NULL)
		WindowsVerify(L"CreateThread", FALSE);

	if (!config.program.empty()) {

		// DEBUG LAUNCHER
		//
		// The outer process which spawns the nested executive

		processListMutex = CreateMutex( 
			NULL, // default security attributes
			FALSE, // initially not owned
			NULL // unnamed mutex
		);

		lastProcessExitedEvent = CreateEvent( 
			NULL, // default security attributes
			TRUE, // manual-reset event
			FALSE, // initial state is nonsignaled
			NULL  // object name
		);

		std::wstringstream commandLine;

		DWORD moduleFileNameSize;
		TCHAR moduleFileName[MAX_PATH];
		moduleFileNameSize = GetModuleFileName(NULL, moduleFileName, sizeof(moduleFileName));
		Verify(L"GetModuleFilename returned string longer than MAX_PATH", moduleFileNameSize < sizeof(moduleFileName));

		commandLine << L'"' << moduleFileName << L'"';

		for (int index = 1; index < numberOfArgs; index++) {
			commandLine << L' ';

			// must escape any single quotes with \"
			for (int indexChar = 0; indexChar < wcslen(commandLineArgs[index]); indexChar++) {
				if (commandLineArgs[index][indexChar] == L'"') {
					commandLine << L'\\';
					commandLine << L'"';
				} else if (commandLineArgs[index][indexChar] == L'=') {
					commandLine << L'=';
					commandLine << L'"';
				} else {
					commandLine << commandLineArgs[index][indexChar];
				}
			}
			commandLine << L'"';
		}

		// We'll be running the same command again, only add one little thing...
		// Simulate %p by casting to void: http://stackoverflow.com/a/5657144/211160

		commandLine << L" --shutdownevent=" << static_cast<void*>(lastProcessExitedEvent);

		// see: http://www.catch22.net/tuts/undoc01.asp
		STARTUPINFO startupInfo;
		ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
		startupInfo.cb = sizeof(STARTUPINFO);

		startupInfo.wShowWindow = config.shouldRepositionWindow() ? SW_HIDE : SW_SHOW;

		InactiveCode() {
			// Wondered if NULL for lpDesktop was causing problems in the startupInfo.  Turned out 
			// to not be the problem.  Might want to give user power to specify which desktop though

			HDESK desktopForThread = GetThreadDesktop(GetCurrentThreadId());
			DWORD desktopNameLength;
			if (!GetUserObjectInformation(desktopForThread, UOI_NAME, NULL, 0, &desktopNameLength)) {
				DWORD lastError = GetLastError();
				if (lastError != 0x7a) // ignore errors here, because we get an error for passing in a buffer that's too small
					ExitProgramOnWindowsError(L"GetUserObjectInformation", lastError);
			}
			LPWSTR desktopName = (LPWSTR)VirtualAlloc((LPVOID) NULL, (DWORD) (desktopNameLength + 1), MEM_COMMIT, PAGE_READWRITE); 
			WindowsVerify(L"GetUserObjectInformation", GetUserObjectInformation(desktopForThread, UOI_NAME, desktopName, desktopNameLength + 1, NULL));
			startupInfo.lpDesktop = desktopName; 

			/* (...process create...) */

			VirtualFree(desktopName, 0, MEM_RELEASE);
		}

		InactiveCode() {
			// Tried CreateProcessAsUser and link with advapi32.lib.  That didn't help launch issue one bit.
			HANDLE tokenHandle;
			WindowsVerify(L"OpenProcessToken", OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &tokenHandle));
			/*if (CreateProcessAsUser(tokenHandle, ... */
		}
		
		InactiveCode() {	
			// You would think this would work, as it's been documented to... but it does not
			// Causes the process to freeze up, you have to then run ResumeThread on it
			// Yet even still, the window will pick its own coordinates
			startupInfo.dwX = config.x;
			startupInfo.dwY = config.y;
			startupInfo.dwXSize = config.width;
			startupInfo.dwYSize = config.height; 
			
			/* (...process create...) */
			}
		 
		startupInfo.hStdInput = stdin;
		startupInfo.hStdOutput = stdout;
		startupInfo.hStdError = stderr;

		DEBUGLOOPARGS debugLoopArgs;
		debugLoopArgs.startupInfo = &startupInfo;
		debugLoopArgs.commandLine = commandLine.str();
		debugLoopArgs.millisecondsLeft = &millisecondsLeft;
		debugLoopArgs.showMessageEvent = CreateEvent(NULL, TRUE, FALSE, L"showMessageEvent");
		debugLoopArgs.messageRetryEvent = CreateEvent(NULL, TRUE, FALSE, L"messageRetryEvent");
		debugLoopArgs.executableImageName = L"";
		
		// Create the debugger thread.  Unfortunately we can't put message boxes and such up
		// in a debug thread without sometimes hanging the UI.
		DWORD debugLoopThreadId;
		HANDLE debugLoopThread = CreateThread( 
			NULL, // default security attributes
			0, // use default stack size
			DebugLoopMain, // thread function name
			&debugLoopArgs, // argument to thread function, we shouldn't read this until thread is terminated!!
			0, // use default creation flags 
			&debugLoopThreadId
		);
		if (debugLoopThread == NULL)
			WindowsVerify(L"CreateThread", FALSE);

		// Dirt simple GUI thread right now, we wait for a signal or thread termination...then read returnCode

		HANDLE waitOnObjects[2] = {debugLoopThread, debugLoopArgs.showMessageEvent};
		BOOL debugLoopRunning = TRUE;
		while (debugLoopRunning) {
			switch(WaitForMultipleObjects(2, waitOnObjects, FALSE, INFINITE)) {
			case WAIT_OBJECT_0:
				debugLoopRunning = FALSE;
				break;
			case WAIT_OBJECT_0+1:
				{
					WindowsVerify(L"ResetEvent", ResetEvent(debugLoopArgs.showMessageEvent));
					std::wstringstream message;
					message << L"Spawning process " << debugLoopArgs.executableImageName
						<< L" that already has at least one running instance already. " 
						<< L"Because you are using defer, this message box will "
						<< L"stay until you close the other instance.  Press Retry when "
						<< L"you've closed the other processes or cancel to quit.";

					int id = MessageBox(NULL, message.str().c_str(), L"Lite Wait Title - defer", MB_RETRYCANCEL | MB_ICONEXCLAMATION);
					WindowsVerify(L"MessageBox", id != 0);
					if (id == IDCANCEL) {
						ExitProcess(mainReturnDeferCancel);
					} else {
						WindowsVerify(L"SetEvent", SetEvent(debugLoopArgs.messageRetryEvent));
					}
				}
				break;

			default:
				WindowsVerify(L"WaitForMultipleObjects", FALSE);
			}
		}

		DWORD dwReturnCode;
		WindowsVerify(L"GetExitCodeThread", GetExitCodeThread(debugLoopThread, &dwReturnCode));
		returnCode = (MAINRETURN)dwReturnCode;

		WindowsVerify(L"CloseHandle", CloseHandle(debugLoopThread));
		WindowsVerify(L"CloseHandle", CloseHandle(debugLoopArgs.showMessageEvent));
		WindowsVerify(L"CloseHandle", CloseHandle(debugLoopArgs.messageRetryEvent));

		WindowsVerify(L"CloseHandle", CloseHandle(processListMutex));
		processListMutex = NULL;

		WindowsVerify(L"CloseHandle", CloseHandle(lastProcessExitedEvent));
		lastProcessExitedEvent = NULL;
	}

	if (config.program.empty()
		or ((returnCode == mainReturnRunClosed) and (config.all))) {

		// Assume user will start relevant process to make the title on their own
		switch(WaitForSingleObject(titleMonitorThread, millisecondsLeft)) {
			case WAIT_OBJECT_0:
				// success, thread died of its own accord...
				returnCode = mainReturnSuccess;
				break;
			case WAIT_TIMEOUT:
				returnCode = mainReturnTimedOut;
				break;
			default:
				WindowsVerify(L"WaitForSingleObject", FALSE);
				returnCode = mainReturnInternalError; // Never executed
		}
	}

	if (config.verbose)
		std::wcout << L"Return Code was " << returnCode << L"\n";

	// Cleanly tie up the monitor thread.  It may be in a finished state, thus waiting on it returns immediately
	if (WaitForSingleObject(titleMonitorThread, 0) != WAIT_OBJECT_0)
		WindowsVerify(L"PostThreadMessage", PostThreadMessage(titleMonitorThreadId, WM_QUIT, 0, 0));
	if (WaitForSingleObject(titleMonitorThread, INFINITE) != WAIT_OBJECT_0)
		WindowsVerify(L"WaitForSingleObject", FALSE);

	WindowsVerify(L"CloseHandle", CloseHandle(titleMonitorThread));

	return returnCode;
}