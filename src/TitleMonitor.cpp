//
// TitleMonitor.cpp
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
#include <regex>

#include "TitleWait.h"
#include "HelperFunctions.h"
#include "TitleMonitor.h"
#include "Screenshot.h"
#include "TitleWaitConfig.h"
#include "ProcessMonitor.h"

bool movedWindow = false;

BOOL CALLBACK EnumTopLevelDesktopWindowsProc(HWND topLevelWindow, LPARAM lparam)
{
	DWORD gwlStyle = GetWindowLong(topLevelWindow, GWL_STYLE);

	// Don't consider invisible windows (command line option?)
	if ((gwlStyle & WS_VISIBLE) == 0)
		return TRUE;

	// See if Window is in one of our spawned processes
	if (!config->searchAll) {

		DWORD windowProcessId;
		DWORD windowThreadId = GetWindowThreadProcessId(
			topLevelWindow,
			&windowProcessId
		);
		WindowsVerify(L"GetWindowThreadProcessId",
			windowThreadId != 0 and windowProcessId != 0
		);

		BOOL windowIsInSpawnedProcess = false;
		{
			if (WaitForSingleObject(processListMutex, INFINITE) != WAIT_OBJECT_0)
				WindowsVerify(L"WaitForSingleObject", FALSE);
			for (int index = 0; index < numProcesses; index++)
			{
				if (windowProcessId == processIds[index])
					windowIsInSpawnedProcess = TRUE;
			}
			WindowsVerify(L"ReleaseMutex", ReleaseMutex(processListMutex));
		}

		if (!windowIsInSpawnedProcess)
			return TRUE;

		// Move window in running process
		// Not sure if this is the best place to put this.  Maybe its own thread?
		if (config->shouldMoveWindow() and !movedWindow) {

			if ((gwlStyle & (WS_POPUP | WS_CHILD)) == WS_OVERLAPPED) {
				
				// Note: SetWindowPos does not work cross process reliably
				// SetWindowPlacement appears to work, however
				if (config->verbose) {
					TCHAR windowClassName[MAX_PATH];
					int childWindowClassNameLength =
						GetClassName(topLevelWindow, windowClassName, MAX_PATH);
					debugInfo(
						L"SetWindowPlacement => handle 0x%x with class %s",
						topLevelWindow,
						windowClassName
					);
				}

				{
					WINDOWPLACEMENT windowPlacement;
					WindowsVerify(L"GetWindowPlacement",
						GetWindowPlacement(topLevelWindow, &windowPlacement)
					);
					if (config->x != CW_USEDEFAULT) {
						windowPlacement.rcNormalPosition.left = config->x;
					}
					if (config->y != CW_USEDEFAULT) {
						windowPlacement.rcNormalPosition.top = config->y;
					}
					if (config->width != CW_USEDEFAULT) {
						windowPlacement.rcNormalPosition.right =
							windowPlacement.rcNormalPosition.left + config->width;
					}
					if (config->height != CW_USEDEFAULT) {
						windowPlacement.rcNormalPosition.bottom =
							windowPlacement.rcNormalPosition.top + config->height;
					}
					windowPlacement.flags = 0;
					windowPlacement.showCmd = SW_SHOWNA; 
					WindowsVerify(L"SetWindowPlacement", SetWindowPlacement(
						topLevelWindow, &windowPlacement)
					);
				
					movedWindow = true;
				}
			}
		}
	}

	// Fetch Window title and look for match
	//
	// Note: GetWindowText and GetWindowTextLength fails on some kinds of
	// windows, e.g. explorer shells.  This is despite claims it can be used
	// for getting window titles on other processes (just not controls)
	// Sending the message works around the problem:
	//
	//    http://msdn.microsoft.com/en-us/library/ms633520.aspx
	{
		int textLength = (int)SendMessage(
			topLevelWindow,
			WM_GETTEXTLENGTH,
			static_cast<WPARAM>(0),
			static_cast<LPARAM>(0)
		);
		LPWSTR titleGlobal = (LPWSTR)VirtualAlloc(
			NULL, static_cast<DWORD>(textLength + 1),
			MEM_COMMIT,
			PAGE_READWRITE
		);
		SendMessage(
			topLevelWindow,
			WM_GETTEXT,
			static_cast<WPARAM>(textLength + 1),
			reinterpret_cast<LPARAM>(titleGlobal)
		);
		std::wstring title(titleGlobal);
		WindowsVerify(L"VirtualFree", VirtualFree(
			titleGlobal,
			0,
			MEM_RELEASE
		));

		// Now do the last check, on the title string...
		// the original intent of this overblown program
		if (!title.empty() and !config->regex.empty()) {
			std::wsmatch what;
			if (std::regex_search(title, what, std::wregex(config->regex))) {
				if (not config->regexSnapshot.empty()) {
					Verify(L"Screen Capture Failed",
						TakeScreenshotToFile(config->regexSnapshot.c_str())
					);
				}

				if (config->closeOnMatch) {
					debugInfo(
							L"WM_SYSCOMMAND/SC_CLOSE => 0x%x with title: %s",
							topLevelWindow,
							title.c_str()
						);
							
					// This works better than WM_CLOSE... but is it the best?
					// Should we optionally try and kill the process?
					WindowsVerify(L"PostMessage", 
						PostMessage(
							topLevelWindow,
							WM_SYSCOMMAND,
							SC_CLOSE,
							MAKELPARAM(0,0)
						)
					);
				}

				// End the search...
				return FALSE;
			}
		}

		// Keep searching...
		return TRUE;
	}

	Verify(L"Unreachable code", FALSE);
	return FALSE;
}


// Timer callback, check titles and move window if applicable
VOID CALLBACK CheckTitleProc(
	HWND messageWindow, UINT uMsg, UINT_PTR idEvent, DWORD dwTime
) {
	BOOL continueSearch = EnumWindows(EnumTopLevelDesktopWindowsProc, 0);
	if (!continueSearch) {
			WindowsVoid(PostQuitMessage(0));
			debugInfo(L"PostQuitMessage() to timer thread");
	}
}


// Title Monitor gets its own thread
// This way we can do debug event processing in main thread
DWORD WINAPI TitleMonitorThreadProc(LPVOID lpParam)
{
	LPWSTR invisibleWindowTitle = (LPWSTR)lpParam;

	// Create an invisible window to handle our timer calls
	HWND messageWindow = CreateWindow(
		L"#32770",
		invisibleWindowTitle,
		~WS_VISIBLE,
		0,
		0,
		100,
		100,
		NULL,
		NULL,
		NULL,
		0
	);
	
	// Call it before the delay...
	CheckTitleProc(messageWindow, WM_TIMER, 0, 0);

	{
		// Initialize a timer that will be called every N seconds
		// to see if window has the title regex we are looking for
		UINT_PTR timerId = SetTimer(
				messageWindow, // hwnd
				0, // timer identifier 
				config->frequency*1000, // convert seconds to milliseconds 
				(TIMERPROC) CheckTitleProc // timer callback
			);
		
		// Must dispatch messages in order to get timer notifications
		MSG msg;
		while (GetMessage(
			&msg, // message structure 
			NULL, // handle to window to receive the message 
			0, // lowest message to examine 
			0 // highest message to examine 
		)) { 
			TranslateMessage(&msg); // translates virtual-key codes 
			DispatchMessage(&msg);  // dispatches message to window 
		} 

		WindowsVerify(L"KillTimer", KillTimer(messageWindow, 0));
	}

	WindowsVerify(L"DestroyWindow", DestroyWindow(messageWindow));

	debugInfo(L"Terminating title monitor thread");

	// WaitOnSingleObject for this thread handle will block until value returned

	return 0;
}
