//
// ProcessMonitor.h
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

#ifndef __TITLEWAIT_PROCESSMONITOR_H__
#define __TITLEWAIT_PROCESSMONITOR_H__

#include "windows.h"

extern HANDLE lastProcessExitedEvent;
extern HANDLE processListMutex;
extern int numProcesses;
extern DWORD processIds[MAX_PATH];
extern HANDLE processMonThreads[MAX_PATH];

DWORD WINAPI ProcessMonitorMain(LPVOID lpParam);

#endif
