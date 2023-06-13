// From: http://www.codeproject.com/KB/threads/GetNtProcessInfo.aspx

// All Code Project samples are released under the "Code Project Open
// License: http://www.codeproject.com/info/cpol10.aspx

// The main points subject to the terms of the License are:
// * Source Code and Executable Files can be used in commercial applications;
// * Source Code and Executable Files can be redistributed; and
// * Source Code can be modified to create derivative works.
// No claim of suitability, guarantee, or any warranty whatsoever is
// provided. The software is provided "as-is".

//***********************************************************************/
// Header definitions to access NtQueryInformationProcess in NTDLL.DLL
//
// Copyright © 2007 Steven Moore (OrionScorpion).  All Rights Reserved.
//
// NOTES: PEB_LDR_DATA, RTL_USER_PROCESS_PARAMETERS and PEB struct are
//        defined in Winternl.h and Ntddk.h.  The specs below are from
//        Microsoft MSDN web site as of Jul 2007.  I locally specified
//        them below since they can change in future versions and may
//        not reflect current winternl.h or ntddk.h
//***********************************************************************/

#ifndef _ORIONSCORPION_NTPROCESSINFO_H_
#define _ORIONSCORPION_NTPROCESSINFO_H_

#pragma once

#include <windows.h>
#include <winternl.h>
#include <psapi.h>

#define STRSAFE_LIB
#include <strsafe.h>

#pragma comment(lib, "strsafe.lib")
#pragma comment(lib, "rpcrt4.lib")
#pragma comment(lib, "psapi.lib")

// !!! TitleWait modification: add this pragma.  Without it, building
// with VS2022 will give linker errors regarding __vsnprintf.
// 
// https://stackoverflow.com/a/49399046
//
#pragma comment(lib, "legacy_stdio_definitions.lib")

// !!! TitleWait modification: add this #if.  It seems winternl.h
// was updated to define this enumeration for _WIN32_WINNT >= 0x0500
// so trust use of that definition of PROCESSINFOCLASS if available.
//
#if (_WIN32_WINNT < 0x0500)

typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation,
    ProcessQuotaLimits,
    ProcessIoCounters,
    ProcessVmCounters,
    ProcessTimes,
    ProcessBasePriority,
    ProcessRaisePriority,
    ProcessDebugPort,
    ProcessExceptionPort,
    ProcessAccessToken,
    ProcessLdtInformation,
    ProcessLdtSize,
    ProcessDefaultHardErrorMode,
    ProcessIoPortHandlers,          // Note: this is kernel mode only
    ProcessPooledUsageAndLimits,
    ProcessWorkingSetWatch,
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup,
    ProcessPriorityClass,
    ProcessWx86Information,
    ProcessHandleCount,
    ProcessAffinityMask,
    ProcessPriorityBoost,
    ProcessDeviceMap,
    ProcessSessionInformation,
    ProcessForegroundInformation,
    ProcessWow64Information,
    MaxProcessInfoClass
    } PROCESSINFOCLASS;

#endif  // !!! TitleWait modification

#ifndef NTSTATUS
#define LONG NTSTATUS
#endif

// Unicode path usually prefix with '\\?\'
#define MAX_UNICODE_PATH	32767L

// Used in PEB struct
typedef ULONG smPPS_POST_PROCESS_INIT_ROUTINE;

// Used in PEB struct
typedef struct _smPEB_LDR_DATA {
	BYTE Reserved1[8];
	PVOID Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
} smPEB_LDR_DATA, *smPPEB_LDR_DATA;

// Used in PEB struct
typedef struct _smRTL_USER_PROCESS_PARAMETERS {
	BYTE Reserved1[16];
	PVOID Reserved2[10];
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
} smRTL_USER_PROCESS_PARAMETERS, *smPRTL_USER_PROCESS_PARAMETERS;

// Used in PROCESS_BASIC_INFORMATION struct
typedef struct _smPEB {
	BYTE Reserved1[2];
	BYTE BeingDebugged;
	BYTE Reserved2[1];
	PVOID Reserved3[2];
	smPPEB_LDR_DATA Ldr;
	smPRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	BYTE Reserved4[104];
	PVOID Reserved5[52];
	smPPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
	BYTE Reserved6[128];
	PVOID Reserved7[1];
	ULONG SessionId;
} smPEB, *smPPEB;

// Used with NtQueryInformationProcess
typedef struct _smPROCESS_BASIC_INFORMATION {
    LONG ExitStatus;
    smPPEB PebBaseAddress;
    ULONG_PTR AffinityMask;
    LONG BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} smPROCESS_BASIC_INFORMATION, *smPPROCESS_BASIC_INFORMATION;

// NtQueryInformationProcess in NTDLL.DLL
typedef NTSTATUS (NTAPI *pfnNtQueryInformationProcess)(
	IN	HANDLE ProcessHandle,
    IN	PROCESSINFOCLASS ProcessInformationClass,
    OUT	PVOID ProcessInformation,
    IN	ULONG ProcessInformationLength,
    OUT	PULONG ReturnLength	OPTIONAL
    );

extern pfnNtQueryInformationProcess gNtQueryInformationProcess; // need to define once.

typedef struct _smPROCESSINFO
{
	DWORD	dwPID;
	DWORD	dwParentPID;
	DWORD	dwSessionID;
	DWORD	dwPEBBaseAddress;
	DWORD	dwAffinityMask;
	LONG	dwBasePriority;
	LONG	dwExitStatus;
	BYTE	cBeingDebugged;
	TCHAR	szImgPath[MAX_UNICODE_PATH];
	TCHAR	szCmdLine[MAX_UNICODE_PATH];
} smPROCESSINFO;

HMODULE sm_LoadNTDLLFunctions(void);
void sm_FreeNTDLLFunctions(IN HMODULE hNtDll);
BOOL sm_EnableTokenPrivilege(IN LPCTSTR pszPrivilege);
BOOL sm_GetNtProcessInfo(IN const DWORD dwPID, OUT smPROCESSINFO *ppi);

// added from demo

#define MAX_PI 1024
DWORD EnumProcesses2Array(smPROCESSINFO lpi[MAX_PI]);

typedef DWORD (WINAPI *PGETPROCESSIMAGEFILENAME)(HANDLE, LPTSTR, DWORD);
extern PGETPROCESSIMAGEFILENAME getProcessImageFileNameFunction;

#endif	// _ORIONSCORPION_NTPROCESSINFO_H_
