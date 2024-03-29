//
// TitleWait.cpp
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

#include <string>
#include <iostream>
#include <sstream>
#include <regex>

#include "windows.h"

#include "TitleWait.h"
#include "HelperFunctions.h"
#include "TitleWaitConfig.h"
#include "DebugLoop.h"
#include "TitleMonitor.h"
#include "ProcessMonitor.h"
#include "Screenshot.h"


// Nasty global variable, but Windows isn't particularly good about making
// it easy to pass 64-bit compatible pointers around in callbacks, ever since
// the trick of poking a 32-bit value into GWL_USER went away...
//
TitleWaitConfig const * config = NULL;


std::wstring TitleWait::returnDescriptions[TitleWait::ReturnMax] = {
    L"Success",
    L"Bug! Report to https://github.com/hostilefork/titlewait/issues",
    L"Invoked with bad command line arguments",
    L"No program to run was supplied with --program",
    L"A timeout was specified and program didn't exit before timeout",
    L"Attempt to close the window normally failed, had to terminate",
    L"The spawned process crashed",
    L"The spawned process closed itself before match or timeout",
    L"Running with --defer option and user canceled instead of waiting"
};


TitleWait::MainReturn TitleWait::doMain()
{
    if (config->help) {
        std::wcout << L"\n";

        std::wcout << L"TitleWait (c) 2008-2014 HostileFork.com\n";
        std::wcout << L"See http://titlewait.hostilefork.com\n\n";

        std::wcout << L"Options are like --verbose=1 or --program=\"foo.exe\"\n";
        std::wcout << L"For all options that are strings, enclose in quotes.\n";
        std::wcout << L"To embed quotes in those strings, use \\\"\n";
        std::wcout << L"For booleans, use on/true/yes/1 or off/false/no/0\n";

        std::wcout << L"\n";

        std::wcout << L"Tip: Long command line? Use ^ to join lines\n";

        std::wcout << L"\n";

        std::wcout << L"OPTION LIST:\n";

        std::wcout << L"\n";

        for (int optInt = 0; optInt < TitleWaitConfig::OptionMax; optInt++) {
            TitleWaitConfig::Option option =
                static_cast<TitleWaitConfig::Option>(optInt);

            std::wstringstream decoratedOption;
            decoratedOption << L"  --" << TitleWaitConfig::optionNames[optInt];

            std::wcout << decoratedOption.str() << L'\n';

            std::wcout << L"    ";

            std::wcout << TitleWaitConfig::optionDescriptions[optInt] << "\n";

            std::wcout << L"\n";
        }

        std::wcout << L"RETURN CODE LIST:\n";

        std::wcout << L"\n";

        for (int retInt = 0; retInt < ReturnMax; retInt++) {
            MainReturn mainReturn =
                static_cast<MainReturn>(retInt);

            std::wstringstream decoratedReturn;
            decoratedReturn << L"  " << retInt << L" => " << returnDescriptions[retInt];

            std::wcout << decoratedReturn.str() << L'\n';
        }

        return SuccessReturn;
    }

    if (config->program.empty()) {
        std::wcerr << L"No program specified via --program.  Nothing to do!\n";
        return NoProgramReturn;
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

    MainReturn ret = InternalErrorReturn;
    DWORD msecLeft =
        config->timeout == 0
        ? INFINITE
        : config->timeout * 1000;

    if (config->shutdownEvent) {

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

        // We have unicode strings, but system() doesn't take wide strings

        std::string programUtf8 = wide_string_to_string(config->program);

        commandLine << '"';
        commandLine << programUtf8;
        commandLine << '"';
        commandLine << ' ';

        std::string argsUtf8 = wide_string_to_string(config->args);

        commandLine << argsUtf8;

        commandLine << ' ';
        commandLine << '"';

        // http://www.devx.com/tips/Tip/15144
        //
        // The system() function (declared in <cstdlib>) launches another
        // program from the current program.  As opposed to what most
        // users think, it doesn't return the exit code of the launched
        // process.  Instead, it returns the exit code of the shell that
        // launches the process in question. Consider the following example:
        //
        //        int systemResult = system("winword.exe");
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

        debugInfoA(
            "system(%s) returned with result %d",
            commandLine.str().c_str(),
            systemResult
        );

        HANDLE neverEvent = CreateEvent(
            NULL,  // default security attributes
            TRUE,  // manual-reset event
            FALSE,  // initial state is nonsignaled
            NULL  // object name
        );

        debugInfo(L"Waiting indefinitely, must kill this thread");
        WindowsVerify(L"WaitForSingleObject",
            WaitForSingleObject(neverEvent, INFINITE) == WAIT_OBJECT_0
        );

        // Exitcode of second process started is what we detect in parent...

        return ClosedReturn;
    }

    // Create the thread to begin execution on its own.

    DWORD titleMonitorThreadId;
    HANDLE titleMonitorThread = CreateThread(
        NULL,  // default security attributes
        0,  // use default stack size
        TitleMonitorThreadProc,  // thread function name
        GetCommandLineW(),  // argument to thread function
        0,  // use default creation flags
        &titleMonitorThreadId
    );
    if (titleMonitorThread == NULL)
        WindowsVerify(L"CreateThread", FALSE);

    if (not config->program.empty()) {

        // DEBUG LAUNCHER
        //
        // The outer process which spawns the nested executive

        processListMutex = CreateMutex(
            NULL,  // default security attributes
            FALSE,  // initially not owned
            NULL  // unnamed mutex
        );

        lastProcessExitedEvent = CreateEvent(
            NULL,  // default security attributes
            TRUE,  // manual-reset event
            FALSE,  // initial state is nonsignaled
            NULL  // object name
        );

        std::wstringstream commandLine;

        // Passing to the constructor seems to not put the insertion position
        // at end.  Some documents suggest you could with ios_base::ate, but
        // the wording is dodgy about being "implementation dependent".  Eh?
        //
        commandLine << config->RegenerateCommandLine();

        // We'll be running the same command again, only add shutdownevent
        // Simulate %p by casting to void:
        //
        //     http://stackoverflow.com/a/5657144/211160
        //
        commandLine <<
            L" --shutdownevent=" <<
            static_cast<void*>(lastProcessExitedEvent);

        // see: http://www.catch22.net/tuts/undoc01.asp
        //
        STARTUPINFO startupInfo;
        ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
        startupInfo.cb = sizeof(STARTUPINFO);

        startupInfo.wShowWindow = config->shouldMoveWindow() ? SW_HIDE : SW_SHOW;

      #ifdef CREATE_PROCESS_WITH_DESKTOP
        //
        // Wondered if NULL for lpDesktop was causing problems in the
        // startupInfo.  Turned out to not be the problem.  Might want to
        // give user power to specify which desktop though
        //
        HDESK desktopForThread = GetThreadDesktop(GetCurrentThreadId());
        DWORD desktopNameLength;
        if (not GetUserObjectInformation(
            desktopForThread,
            UOI_NAME,
            NULL,
            0,
            &desktopNameLength
        )){
            DWORD lastError = GetLastError();
            // we expect the error for passing in a buffer that's too small
            if (lastError != 0x7a) {
                ExitProgramOnWindowsError(L"GetUserObjectInformation",
                    lastError
                );
            }
        }
        LPWSTR desktopName = (LPWSTR)VirtualAlloc(
            (LPVOID)NULL,
            (DWORD)(desktopNameLength + 1),
            MEM_COMMIT,
            PAGE_READWRITE
        );
        WindowsVerify(L"GetUserObjectInformation",
            GetUserObjectInformation(
                desktopForThread,
                UOI_NAME,
                desktopName,
                desktopNameLength + 1,
                NULL
            )
        );
        startupInfo.lpDesktop = desktopName;

        /* (...process create...) */

        VirtualFree(desktopName, 0, MEM_RELEASE);
      #endif

      #ifdef CREATE_PROCESS_AS_USER
        //
        // Tried CreateProcessAsUser and link with advapi32.lib.
        // That didn't help launch issue one bit!
        //
        HANDLE tokenHandle;
        WindowsVerify(L"OpenProcessToken",
            OpenProcessToken(
                GetCurrentProcess(),
                TOKEN_ALL_ACCESS,
                &tokenHandle
            )
        );
        /*if (CreateProcessAsUser(tokenHandle, ... */
      #endif

      #ifdef USE_STARTUPINFO_FOR_WINDOW_POSITION
        //
        // You would think this would work, as it's been documented to.
        // But it does not work.
        //
        // Causes the process to freeze up, you have to then run ResumeThread
        // on it... even then, the window will pick its own coordinates
        //
        startupInfo.dwX = config->x;
        startupInfo.dwY = config->y;
        startupInfo.dwXSize = config->width;
        startupInfo.dwYSize = config->height;

        /* (...process create...) */
      #endif

        startupInfo.hStdInput = stdin;
        startupInfo.hStdOutput = stdout;
        startupInfo.hStdError = stderr;

        // Argument to thread function
        // We shouldn't read from this until thread is terminated!!
        //
        DebugArgs debugArgs;
        debugArgs.startupInfo = &startupInfo;
        debugArgs.commandLine = commandLine.str();
        debugArgs.msecLeft = &msecLeft;
        debugArgs.deferEvent = CreateEvent(
            NULL,
            TRUE,
            FALSE,
            L"deferEvent"
        );
        debugArgs.retryEvent = CreateEvent(
            NULL,
            TRUE,
            FALSE,
            L"retryEvent"
        );
        debugArgs.exeImageName = L"";

        // Create the debugger thread.  Unfortunately we can't put message
        // boxes and such up in a debug thread without sometimes hanging the UI
        //
        DWORD debugThreadId;
        HANDLE debugThread = CreateThread(
            NULL,  // default security attributes
            0,  // use default stack size
            DebugLoopMain,  // thread function name
            &debugArgs,
            0,  // use default creation flags
            &debugThreadId
        );
        if (debugThread == NULL)
            WindowsVerify(L"CreateThread", FALSE);

        // Dirt simple GUI thread right now, we wait for a signal or thread
        // termination...then read ret

        HANDLE waitOnObjects[2] = {debugThread, debugArgs.deferEvent};
        BOOL debugLoopRunning = TRUE;
        while (debugLoopRunning) {
            switch(WaitForMultipleObjects(2, waitOnObjects, FALSE, INFINITE)) {
              case WAIT_OBJECT_0:
                debugLoopRunning = FALSE;
                break;

              case WAIT_OBJECT_0 + 1: {
                WindowsVerify(L"ResetEvent",
                    ResetEvent(debugArgs.deferEvent)
                );
                std::wstringstream message;
                message << L"Spawning process " << debugArgs.exeImageName
                    << L" with at least one running instance already. "
                    << L"Since you are using --defer, this message will "
                    << L"stay the other instance closes.  Press Retry when "
                    << L"the other processes are closed or cancel to quit.";

                int id = MessageBox(
                    NULL,
                    message.str().c_str(),
                    L"TitleWait - Defer Handling",
                    MB_RETRYCANCEL | MB_ICONEXCLAMATION
                );
                WindowsVerify(L"MessageBox", id != 0);
                if (id == IDCANCEL) {
                    ExitProcess(DeferCancelledReturn);
                } else {
                    WindowsVerify(L"SetEvent",
                        SetEvent(debugArgs.retryEvent)
                    );
                }
                break; }

              default:
                WindowsVerify(L"WaitForMultipleObjects", FALSE);
            }
        }

        DWORD dwReturnCode;
        WindowsVerify(L"GetExitCodeThread",
            GetExitCodeThread(debugThread, &dwReturnCode)
        );
        ret = (MainReturn)dwReturnCode;

        WindowsVerify(L"CloseHandle", CloseHandle(debugThread));
        WindowsVerify(L"CloseHandle", CloseHandle(debugArgs.deferEvent));
        WindowsVerify(L"CloseHandle", CloseHandle(debugArgs.retryEvent));
    }

    if ((ret == ClosedReturn) and config->searchAll) {

        // We are searching all windows, the program was closed, and the
        // title matching condition has not been reached.  For as long
        // as any timeout period remains, we will wait.

        // Assume user will start process to make the title on their own

        switch(WaitForSingleObject(titleMonitorThread, msecLeft)) {
          case WAIT_OBJECT_0:
            ret = SuccessReturn;  // success, thread died of its own accord...
            break;

          case WAIT_TIMEOUT:
            if (not config->timeoutSnapshot.empty()) {
                Verify(L"Screen Capture Failed",
                    TakeScreenshotToFile(config->timeoutSnapshot.c_str())
                );
            }
            ret = TimeoutReturn;
            break;

          default:
            WindowsVerify(L"WaitForSingleObject", FALSE);
            ret = InternalErrorReturn;  // Never executed
        }
    }

    // Cleanly tie up the monitor thread.
    // It may be in a finished state, thus waiting on it returns immediately
    //
    if (WaitForSingleObject(titleMonitorThread, 0) != WAIT_OBJECT_0) {
        WindowsVerify(L"PostThreadMessage",
            PostThreadMessage(titleMonitorThreadId, WM_QUIT, 0, 0)
        );
    }
    if (WaitForSingleObject(titleMonitorThread, INFINITE) != WAIT_OBJECT_0) {
        WindowsVerify(L"WaitForSingleObject", FALSE);
    }

    WindowsVerify(L"CloseHandle", CloseHandle(titleMonitorThread));

    if (config->verbose) {
        debugInfo(L"Return Code was %d\n", ret);
    }

    WindowsVerify(L"CloseHandle", CloseHandle(processListMutex));
    processListMutex = NULL;

    WindowsVerify(L"CloseHandle", CloseHandle(lastProcessExitedEvent));
    lastProcessExitedEvent = NULL;

    return ret;
}
