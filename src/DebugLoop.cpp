//
// DebugLoop.cpp
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

#include <vector>

#include "TitleWait.h"
#include "DebugLoop.h"
#include "HelperFunctions.h"
#include "ProcessMonitor.h"
#include "TitleWaitConfig.h"
#include "NTProcessInfo.h"


// NOTE: The screen capturing code is TitleWait's only dependency on
// GDI32.  Perhaps these functions should be loaded lazily and only if 
// a screen capture is being done?
#include "Screenshot.h"


// See if a previous image is still running of the process we're debugging
// Might want to keep the thread/process in a state of suspended animation
BOOL PreviousStillRunning(std::wstring & exeImageName, DWORD processId) 
{
	// Why all this rigamorole when there's:
	//
	//      DebugEv->u.CreateProcessInfo.lpImageName?
	//
	// Ok, here's why:
	//
	// "This member is strictly optional. Debuggers must be prepared to handle
	// the case where lpImageName is NULL or *lpImageName (in the address space
	// of the process being debugged) is NULL. Specifically, the system does
	// not provide an image name for a create process event, and will not
	// likely pass an image name for the first DLL event. The system also
	// does not provide this information in the case of debug events that
	// originate from a call to the DebugActiveProcess function.

	DWORD dwPIDCount;
	static smPROCESSINFO lpi[MAX_PI] = {0};

	// Get processes list
	debugInfo(L"Beginning process enumeration");
	dwPIDCount = EnumProcesses2Array(lpi);
	debugInfo(L"Ending process enumeration");

	smPROCESSINFO* outerExecutiveProcessInfo = NULL;
	// remember we run a "CMD" via system()
	smPROCESSINFO* cmdShellOuterProcessInfo = NULL;
	// remember we run a "CMD /C" via system() 
	smPROCESSINFO* cmdShellInnerProcessInfo = NULL;
	smPROCESSINFO* nestedExecutiveProcessInfo = NULL;
	smPROCESSINFO* mainDebuggeeProcessInfo = NULL;

	DWORD outerExecutiveProcessId = GetCurrentProcessId();
	DWORD cmdShellOuterProcessId = 0;
	DWORD cmdShellInnerProcessId = 0;
	DWORD nestedExecutiveProcessId = 0;
	DWORD mainDebuggeeProcessId = 0;
	{ 
		if (WaitForSingleObject(processListMutex, INFINITE) != WAIT_OBJECT_0)
			WindowsVerify(L"WaitForSingleObject", FALSE);
		if (numProcesses > 0)
			cmdShellOuterProcessId = processIds[0];
		if (numProcesses > 1)
			cmdShellInnerProcessId = processIds[1];
		if (numProcesses > 2)
			nestedExecutiveProcessId = processIds[2];
		if (numProcesses > 3)
			mainDebuggeeProcessId = processIds[3];
		WindowsVerify(L"ReleaseMutex", ReleaseMutex(processListMutex));
	}

	// little sanity check here...
	// How would we receive these debug events if the shell isn't running?!
	Verify(L"Nested executive command process couldn't be found.",
		cmdShellOuterProcessId != 0
	);

	if (mainDebuggeeProcessId == 0) {
		return FALSE; // not ready to do the process test yet.
	}
	
	if (processId != 0 and mainDebuggeeProcessId != processId) {
		return FALSE; // don't suspend threads in wrong process!
	}

	for (int index = 0; index < dwPIDCount; index++) {
		if (lpi[index].dwPID == outerExecutiveProcessId)
			outerExecutiveProcessInfo = &lpi[index];
		if (lpi[index].dwPID == cmdShellOuterProcessId)
			cmdShellOuterProcessInfo = &lpi[index];
		if (lpi[index].dwPID == cmdShellInnerProcessId)
			cmdShellInnerProcessInfo = &lpi[index];
		if (lpi[index].dwPID == nestedExecutiveProcessId)
			nestedExecutiveProcessInfo = &lpi[index];
		if (lpi[index].dwPID == mainDebuggeeProcessId)
			mainDebuggeeProcessInfo = &lpi[index];
	}
	
	debugInfo(L"GetNtProcessInfo(%s)", mainDebuggeeProcessInfo->szImgPath);

	for (int index = 0; index < dwPIDCount; index++) {

		if (lpi[index].dwPID == outerExecutiveProcessId)
				continue;
		if (lpi[index].dwPID == cmdShellOuterProcessId)
				continue;
		if (lpi[index].dwPID == cmdShellInnerProcessId)
				continue;
		if (lpi[index].dwPID == nestedExecutiveProcessId)
				continue;
		if (lpi[index].dwPID == mainDebuggeeProcessId)
				continue;

		// windows filenames are case insensitive
		debugInfo(L"Process Image: %s", lpi[index].szImgPath);
		if (!lstrcmpi(mainDebuggeeProcessInfo->szImgPath, lpi[index].szImgPath)) {
			exeImageName = std::wstring(mainDebuggeeProcessInfo->szImgPath);
			return TRUE;
		}
	}

	return FALSE;
}


// EnterDebugLoop was taken from:
// http://msdn.microsoft.com/en-us/library/ms681675(VS.85).aspx
//
// Note that we don't actually *want* to be a debugger, but CreateProcess has
// some bug or security dependency which is leading it to (in some situations)
// not show the window of the process it launches.  Somehow, calling a debug
// loop addresses this.  I've left the full code from the sample here in order
// to make it easier to set breakpoints to discover what's going wrong if
// there is a problem with the technique of working around CreateProcess's
// flaw with the debug API.
//
DWORD WINAPI DebugLoopMain(LPVOID lpParam) // returns a MainReturn
{
	DebugArgs & debugArgs = *static_cast<DebugArgs *>(lpParam);

	HMODULE ntDllModule = sm_LoadNTDLLFunctions();

	debugInfo(L"Got to DebugLoopMain thread code.");

	PROCESS_INFORMATION processInfo;
	processInfo.hProcess = NULL;
	processInfo.hThread = NULL;

	// Windows can modify the command line, so we have to make a copy
	std::vector<WCHAR> commandLineCopy (
		debugArgs.commandLine.begin(),
		debugArgs.commandLine.end()
	);
	commandLineCopy.push_back(L'\0');

	DWORD dwCreationFlags =
#ifdef USE_STARTUPINFO_FOR_WINDOW_POSITION
			// These options don't work :-(
			(config->runWindowPosition ? STARTF_USEPOSITION : 0) |
			(config->runWindowSize ? STARTF_USESIZE : 0) |
#endif
			(config->shutdownEvent ? 0 : DEBUG_PROCESS) |
			STARTF_USESHOWWINDOW;

	// REVIEW: for security should lpApplicationName be the same as the 
	// name in the command line?
	WindowsVerify(
		L"CreateProcess",
		CreateProcess(
			0, // lpApplicationName
			&commandLineCopy[0], // lpCommandLine (no .data() in C++98)
			0, // lpProcessAttributes
			0, // lpThreadAttributes
			config->shutdownEvent == NULL ? TRUE : FALSE, // bInheritHandles
			dwCreationFlags,
			0, // lpEnvironment
			0, // lpCurrentDirectory
			debugArgs.startupInfo, // lpStartupInfo
			&processInfo // lpProcessInformation
		)
	);

	Verify(L"Thread or process was null.", 
		processInfo.hThread and processInfo.hProcess
	);

	debugInfo(L"CreateProcess(%s) - Id = %d\n", 
		debugArgs.commandLine.c_str(), processInfo.dwProcessId
	);

	debugInfo(L"Entering debugger loop...\n");

	HANDLE suspendedThread = NULL;
	BOOL previouslyWaited = FALSE;

	DEBUG_EVENT Ev;
	const LPDEBUG_EVENT DebugEv = &Ev;

	while (TRUE) {

		// Wait for a debugging event to occur. The second parameter indicates
		// that the function does not return until a debugging event occurs. 

		BOOL gotDebugEvent = FALSE;
		BOOL handledEvent = TRUE;

		DWORD tickBefore = GetTickCount();
		if (WaitForDebugEvent(
			DebugEv,
			suspendedThread ? 1000 : *debugArgs.msecLeft
		)) {
			gotDebugEvent = TRUE;
		} else {
			DWORD lastError = GetLastError();
			if (lastError == 0x79) {
				// The semaphore timeout period has expired, that is ok
			} else {
				ExitProgramOnWindowsError(L"WaitForDebugEvent", lastError);
			}
		}

		if (suspendedThread) {
			switch(WaitForSingleObject(debugArgs.retryEvent, 0)) {
			case WAIT_OBJECT_0: {
				WindowsVerify(L"ResetEvent", ResetEvent(debugArgs.retryEvent));
				// need to poll processes again...
				std::wstring exeImageName;
				if (PreviousStillRunning(exeImageName, 0)) {
					debugInfo(L"Retry failed, show message box again.");
					WindowsVerify(L"SetEvent", SetEvent(debugArgs.deferEvent));
				} else {
					debugInfo(L"Resuming thread handle 0x%x", suspendedThread);
					WindowsVerify(L"ResumeThread",
						ResumeThread(suspendedThread) != -1
					);
					WindowsVerify(L"CloseHandle", CloseHandle(suspendedThread));
					suspendedThread = NULL;
					previouslyWaited = TRUE;
				}
				break;
			}
			case WAIT_TIMEOUT:
				// nope, do nothing, try again in a second...
				break;
			default:
				WindowsVerify(L"WaitForSingleObject", FALSE);
			}
		}

		DWORD tickAfter = GetTickCount();
		if (*debugArgs.msecLeft != INFINITE) {
			bool timedOut = false;
			if (tickAfter < tickBefore) {
				// happens once every 28 days, the tick count rolls over...
				// account for the rollover only
				if (tickBefore > *debugArgs.msecLeft) {
					timedOut = true;
				} else {
					*debugArgs.msecLeft = *debugArgs.msecLeft - tickBefore;
				}
			} else {
				if (tickAfter - tickBefore > *debugArgs.msecLeft) {
					timedOut = true;
				} else {
					*debugArgs.msecLeft =
						*debugArgs.msecLeft - (tickAfter - tickBefore);
				}
			}
			if (timedOut) {
				// Need to take the screenshot here, because if we return from
				// the DebugLoop the windows will be closed and the state lost
				// by the time we return to the caller
				if (not config->timeoutsnapshot.empty()) {
					Verify(L"Screen Capture Failed", 
						TakeScreenshotToFile(config->timeoutsnapshot.c_str())
					);
				}
				return TitleWait::TimeoutReturn;
			}
		}

		// Process the debugging event code. 
		if (gotDebugEvent) {
			switch (DebugEv->dwDebugEventCode) { 

			case EXCEPTION_DEBUG_EVENT: {
				// Process the exception code. When handling 
				// exceptions, remember to set the continuation 
				// status parameter (dwContinueStatus). This value 
				// is used by the ContinueDebugEvent function. 
	 
				switch(DebugEv->u.Exception.ExceptionRecord.ExceptionCode) { 

				case EXCEPTION_ACCESS_VIOLATION: {
					// First chance: Pass this on to the system. 
					// Last chance: Display an appropriate error.

					debugInfo(L"EXCEPTION_ACCESS_VIOLATION");

					if (not config->crashsnapshot.empty()) {
						Verify(L"Screen Capture Failed",
							TakeScreenshotToFile(config->crashsnapshot.c_str())
						);
					}
	 				debugInfo(L"Child Process Crashed - Quitting");
					return TitleWait::CrashedReturn;
				}
	 
				case EXCEPTION_BREAKPOINT: {
					// First chance: Display the current 
					// instruction and register values.
					debugInfo(L"EXCEPTION_BREAKPOINT");
	
					// No process functions during CREATE_PROCESS_DEBUG_EVENT
					// have to wait for a later time
					if (!config->defer or previouslyWaited or suspendedThread)
						break;

					if (PreviousStillRunning(
						debugArgs.exeImageName, DebugEv->dwProcessId
					)) {
						WindowsVerify(L"SetEvent", SetEvent(debugArgs.deferEvent));

						// don't block, keep processing debug events...
						// but suspend the thread!
						debugInfo(
							L"About to suspend thread handle 0x%x", 
							DebugEv->dwThreadId
						);
						suspendedThread = OpenThread(
							THREAD_SUSPEND_RESUME, FALSE, DebugEv->dwThreadId
						);
						WindowsVerify(L"OpenThread", suspendedThread != NULL);
						WindowsVerify(L"Suspend Thread",
							SuspendThread(suspendedThread) != -1
						);
						}
					}
					break;
	 
				case EXCEPTION_DATATYPE_MISALIGNMENT: 
					// First chance: Pass this on to the system. 
					// Last chance: Display an appropriate error.
					debugInfo(L"EXCEPTION_DATATYPE_MISALIGNMENT");
					break;
	 
				case EXCEPTION_SINGLE_STEP: 
					// First chance: Update the display of the 
					// current instruction and register values.
					debugInfo(L"EXCEPTION_SINGLE_STEP");
					break;
	 
				case DBG_CONTROL_C: 
					// First chance: Pass this on to the system. 
					// Last chance: Display an appropriate error. 
					debugInfo(L"DBG_CONTROL_C");
					break;
	 
				default:
					debugInfo(
						L"EXCEPTION_(0x%x) (unrecognized, passing through)",
						DebugEv->u.Exception.ExceptionRecord.ExceptionCode
					);
					// We have debugger behaviors for everything we
					// understand, but not random exceptions...pass on up
					// the chain and say we don't handle it.
					handledEvent = false;
					break;
				}
				break;
			}
	 
			case CREATE_THREAD_DEBUG_EVENT: {
				// As needed, examine or change the thread's registers 
				// with the GetThreadContext and SetThreadContext functions; 
				// and suspend and resume thread execution with the 
				// SuspendThread and ResumeThread functions. 

				debugInfo(
					L"CREATE_THREAD_DEBUG_EVENT - H: 0x%x - P: 0x%x",
					DebugEv->u.CreateThread.hThread,
					DebugEv->dwProcessId
				);
				break;
			}

			case CREATE_PROCESS_DEBUG_EVENT: {
				// As needed, examine or change the registers of the
				// process's initial thread with the GetThreadContext and
				// SetThreadContext functions; read from and write to the
				// process's virtual memory with the ReadProcessMemory and
				// WriteProcessMemory functions; and suspend and resume
				// thread execution with the SuspendThread and ResumeThread
				// functions. 

				SetLastError(0);
				DWORD processId = GetProcessId(
					DebugEv->u.CreateProcessInfo.hProcess
				);
				DWORD lastError = GetLastError();
				if (lastError != ERROR_SUCCESS) {
					ExitProgramOnWindowsError(L"GetProcessId", lastError);
				}

				debugInfo(
					L"CREATE_PROCESS_DEBUG_EVENT - P: 0x%x - T: 0x%x",
					processId,
					DebugEv->dwThreadId
				);

				DWORD processMonThreadId;
				HANDLE processMonThread = CreateThread( 
					NULL, // default security attributes
					0, // use default stack size  
					ProcessMonitorMain, // thread function name
					// argument to thread function
					DebugEv->u.CreateProcessInfo.hProcess, 
					CREATE_SUSPENDED, // create the thread as suspended
					&processMonThreadId);   // returns thread identifier 
				if (processMonThread == NULL) {
					WindowsVerify(L"CreateThread", FALSE);
				}

				// lock process list mutex and append tracking data for thread
				if (WaitForSingleObject(processListMutex, INFINITE) != WAIT_OBJECT_0) {
					WindowsVerify(L"WaitForSingleObject", FALSE);
				}

				processIds[numProcesses] = processId;
				processMonThreads[numProcesses] = processMonThread;
				numProcesses++;

				WindowsVerify(L"ReleaseMutex", ReleaseMutex(processListMutex));

				WindowsVerify(L"ResumeThread",
					ResumeThread(processMonThread) != -1
				);

				// "Be sure to close the handle to the process image
				// file with CloseHandle."  Ok, sure.
				WindowsVerify(L"CloseHandle",
					CloseHandle(DebugEv->u.CreateProcessInfo.hFile)
				);
				break;
			}
	 
			case EXIT_THREAD_DEBUG_EVENT:
				debugInfo(L"EXIT_THREAD_DEBUG_EVENT");
				break;
	 
			case EXIT_PROCESS_DEBUG_EVENT: {
				// Display the process's exit code.  We should only get this if
		 		// the main spawn has returned.  This should not happen, ever,
		 		// but if we catch the first spawn we can perhaps watch on it
		 		// and pipe its result back.
				debugInfo(
					L"EXIT_PROCESS_DEBUG_EVENT - ExitCode: 0x%x",
					DebugEv->u.ExitProcess.dwExitCode
				);
			
#ifdef QUIT_WHEN_MAIN_PROCESS_EXITS
				// we used to exit here, but now we wait for the LAST
				// process exiting signal.  We could in theory return
				// ClosedReturn if we want the behavior of quitting
				// just because the main process we spawned quit...but that
				// is not a good default when people are using things like
				// iexplore which launches a child.... so 
				// make that a non-default option?
				if (WaitForSingleObject(lastProcessExitedEvent, INFINITE) 
					!= WAIT_OBJECT_0
				) {
					WindowsVerify(L"WaitForSingleObject", FALSE);
				}

				if (processInfo.hThread) {
					WindowsVerify(L"CloseHandle",
						CloseHandle(processInfo.hThread)
					);
				}

				if (processInfo.hProcess) {
					WindowsVerify(L"CloseHandle",
						CloseHandle(processInfo.hProcess)
					);
				}

				sm_FreeNTDLLFunctions(ntDllModule);

				return ClosedReturn;
#endif
				break;
			}

			case LOAD_DLL_DEBUG_EVENT:
				// Read the debugging information included in the newly 
				// loaded DLL. Be sure to close the handle to the loaded DLL 
				// with CloseHandle.
				WindowsVerify(L"CloseHandle", 
					CloseHandle(DebugEv->u.LoadDll.hFile)
				);
#ifdef DEBUG_DLL_EVENTS
				// A lot of these go by, disable by default
				debugInfo(L"LOAD_DLL_DEBUG_EVENT");
#endif
				break;

			case UNLOAD_DLL_DEBUG_EVENT:
#ifdef DEBUG_DLL_EVENTS
			// A lot of these go by, disable by default
				debugInfo(L"UNLOAD_DLL_DEBUG_EVENT");
#endif
				break;
	 
			case OUTPUT_DEBUG_STRING_EVENT:
				// Client wants us to display the output debug string.
				// Pass through to WinDbg or display ourselves?
				// For now, say we don't handle it. 
				handledEvent = false;
				debugInfo(
					L"OUTPUT_DEBUG_STRING_EVENT - ExceptionCode 0x%x",
					DebugEv->u.Exception.ExceptionRecord.ExceptionCode
				);
				break;

			case RIP_EVENT:
				debugInfo(L"RIP_EVENT");
				break;
			} 

		// Resume executing the thread that reported the debugging event. 
		WindowsVerify(
			L"ContinueDebugEvent",
			ContinueDebugEvent(
				DebugEv->dwProcessId, 
				DebugEv->dwThreadId, 
				handledEvent ? DBG_CONTINUE : DBG_EXCEPTION_NOT_HANDLED
			)
		);
		}		
 	} // end while(TRUE) loop

	if (processInfo.hThread) {
		WindowsVerify(L"CloseHandle", CloseHandle(processInfo.hThread));
	}

	if (processInfo.hProcess) {
		WindowsVerify(L"CloseHandle", CloseHandle(processInfo.hProcess));
	}

	sm_FreeNTDLLFunctions(ntDllModule);

	// unreachable point
	return TitleWait::InternalErrorReturn;
}
