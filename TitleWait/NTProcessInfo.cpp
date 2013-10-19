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
// Functions to access NtQueryInformationProcess in NTDLL.DLL
//
// Copyright � 2007 Steven Moore (OrionScorpion).  All Rights Reserved.
//
//***********************************************************************/
#include "NTProcessInfo.h"
#include "HelperFunctions.h"

pfnNtQueryInformationProcess gNtQueryInformationProcess = NULL;
PGETPROCESSIMAGEFILENAME getProcessImageFileNameFunction = NULL;

// Enable a privilege for a process token
// The privilege must already be assigned
// This function only enables not assigns
BOOL sm_EnableTokenPrivilege(LPCTSTR pszPrivilege)
{
	HANDLE hToken		 = 0;
	TOKEN_PRIVILEGES tkp = {0}; 

	// Get a token for this process. 
	if (!OpenProcessToken(GetCurrentProcess(),
						  TOKEN_ADJUST_PRIVILEGES |
						  TOKEN_QUERY, &hToken))
	{
        return FALSE;
	}

	// Get the LUID for the privilege. 
	if(LookupPrivilegeValue(NULL, pszPrivilege,
						    &tkp.Privileges[0].Luid)) 
	{
        tkp.PrivilegeCount = 1;  // one privilege to set    
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		// Set the privilege for this process. 
		AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
							  (PTOKEN_PRIVILEGES)NULL, 0); 

		if (GetLastError() != ERROR_SUCCESS)
			return FALSE;
		
		return TRUE;
	}

	return FALSE;
}

// Load NTDLL Library and get entry address
// for NtQueryInformationProcess
HMODULE sm_LoadNTDLLFunctions()
{
	InactiveCode() {
		// Can't use GetModuleFileNameEx
		// does not work 
		// http://winprogger.com/?p=26
		HMODULE psapiDll = LoadLibrary(L"psapi.dll");
		if (psapiDll != NULL)
			getProcessImageFileNameFunction = (PGETPROCESSIMAGEFILENAME) GetProcAddress(
				psapiDll,
				"GetProcessImageFileNameW"
		);
		if (getProcessImageFileNameFunction == NULL)
			MessageBox(NULL, L"GetProcessImageFileNameW", L"GetProcessImageFileNameW", MB_OK | MB_ICONEXCLAMATION);
	}

	HMODULE hNtDll = LoadLibrary(L"ntdll.dll");
	if(hNtDll == NULL) return NULL;

	gNtQueryInformationProcess = (pfnNtQueryInformationProcess)GetProcAddress(hNtDll,
														"NtQueryInformationProcess");
	if(gNtQueryInformationProcess == NULL) {
		FreeLibrary(hNtDll);
		return NULL;
	}
	return hNtDll;
}

// Unloads the NTDLL.DLL and resets the
// global gNtQueryInformationProcess variable
void sm_FreeNTDLLFunctions(HMODULE hNtDll)
{
	if(hNtDll)
		FreeLibrary(hNtDll);
	gNtQueryInformationProcess = NULL;

	InactiveCode() { /*
		if (psapiDll != NULL)
		WindowsVerify(L"FreeLibrary", FreeLibrary(psapiDll));
		*/ 
	}
}

// Gets information on process with NtQueryInformationProcess
BOOL sm_GetNtProcessInfo(const DWORD dwPID, smPROCESSINFO *ppi)
{
	BOOL  bReturnStatus						= TRUE;
	DWORD dwSize							= 0;
	DWORD dwSizeNeeded						= 0;
	DWORD dwBytesRead						= 0;
	DWORD dwBufferSize						= 0;
	HANDLE hHeap							= 0;
	WCHAR *pwszBuffer						= NULL;

	smPROCESSINFO spi						= {0};
	smPPROCESS_BASIC_INFORMATION pbi		= NULL;

	smPEB peb								= {0};
	smPEB_LDR_DATA peb_ldr					= {0};
	smRTL_USER_PROCESS_PARAMETERS peb_upp	= {0};

	ZeroMemory(&spi, sizeof(spi));
	ZeroMemory(&peb, sizeof(peb));
	ZeroMemory(&peb_ldr, sizeof(peb_ldr));
	ZeroMemory(&peb_upp, sizeof(peb_upp));

	spi.dwPID = dwPID;

	// Attempt to access process
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | 
								  PROCESS_VM_READ, FALSE, dwPID);
	if(hProcess == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	// Modification
	InactiveCode() {
		// returns "the handle is invalid" ?
		DWORD moduleNameLength;
		moduleNameLength = getProcessImageFileNameFunction(hProcess, ppi->szImgPath, MAX_PATH);
		if (moduleNameLength == 0)
			WindowsVerify(L"GetProcessImageFileName", FALSE);
		return TRUE;
	}

	// Try to allocate buffer 
	hHeap = GetProcessHeap();

	dwSize = sizeof(smPROCESS_BASIC_INFORMATION);

	pbi = (smPPROCESS_BASIC_INFORMATION)HeapAlloc(hHeap,
												  HEAP_ZERO_MEMORY,
												  dwSize);
	// Did we successfully allocate memory
	if(!pbi) {
		CloseHandle(hProcess);
		return FALSE;
	}

	// Attempt to get basic info on process
	NTSTATUS dwStatus = gNtQueryInformationProcess(hProcess,
												  ProcessBasicInformation,
												  pbi,
												  dwSize,
												  &dwSizeNeeded);

	// If we had error and buffer was too small, try again
	// with larger buffer size (dwSizeNeeded)
	if(dwStatus >= 0 and dwSize < dwSizeNeeded)
	{
		if(pbi)
			HeapFree(hHeap, 0, pbi);
		pbi = (smPPROCESS_BASIC_INFORMATION)HeapAlloc(hHeap,
													  HEAP_ZERO_MEMORY,
                                                      dwSizeNeeded);
		if(!pbi) {
			CloseHandle(hProcess);
			return FALSE;
		}

		dwStatus = gNtQueryInformationProcess(hProcess,
											 ProcessBasicInformation,
											 pbi, dwSizeNeeded, &dwSizeNeeded);
	}

	// Did we successfully get basic info on process
	if(dwStatus >= 0)
	{
		// Basic Info
//        spi.dwPID			 = (DWORD)pbi->UniqueProcessId;
		spi.dwParentPID		 = (DWORD)pbi->InheritedFromUniqueProcessId;
		spi.dwBasePriority	 = (LONG)pbi->BasePriority;
		spi.dwExitStatus	 = (NTSTATUS)pbi->ExitStatus;
		spi.dwPEBBaseAddress = (DWORD)pbi->PebBaseAddress;
		spi.dwAffinityMask	 = (DWORD)pbi->AffinityMask;

		// Read Process Environment Block (PEB)
		if(pbi->PebBaseAddress)
		{
			if(ReadProcessMemory(hProcess, pbi->PebBaseAddress, &peb, sizeof(peb), &dwBytesRead))
			{
				spi.dwSessionID	   = (DWORD)peb.SessionId;
				spi.cBeingDebugged = (BYTE)peb.BeingDebugged;

				InactiveCode() {
					// Here we could access PEB_LDR_DATA, i.e., module list for process
					dwBytesRead = 0;
					if(ReadProcessMemory(hProcess,
										 pbi->PebBaseAddress->Ldr,
										 &peb_ldr,
										 sizeof(peb_ldr),
										 &dwBytesRead))
					{
					// get ldr
					}
				}

				// if PEB read, try to read Process Parameters
				dwBytesRead = 0;
				if(ReadProcessMemory(hProcess,
									 peb.ProcessParameters,
									 &peb_upp,
									 sizeof(smRTL_USER_PROCESS_PARAMETERS),
									 &dwBytesRead))
				{
					// We got Process Parameters, is CommandLine filled in
					if(peb_upp.CommandLine.Length > 0) {
						// Yes, try to read CommandLine
						pwszBuffer = (WCHAR *)HeapAlloc(hHeap,
														HEAP_ZERO_MEMORY,
														peb_upp.CommandLine.Length);
						// If memory was allocated, continue
						if(pwszBuffer)
						{
							if(ReadProcessMemory(hProcess,
												 peb_upp.CommandLine.Buffer,
												 pwszBuffer,
												 peb_upp.CommandLine.Length,
												 &dwBytesRead))
							{
								// if commandline is larger than our variable, truncate
								if(peb_upp.CommandLine.Length >= sizeof(spi.szCmdLine)) 
									dwBufferSize = sizeof(spi.szCmdLine) - sizeof(TCHAR);
								else
									dwBufferSize = peb_upp.CommandLine.Length;
							
								// Copy CommandLine to our structure variable
#if defined(UNICODE) or (_UNICODE)
								// Since core NT functions operate in Unicode
								// there is no conversion if application is
								// compiled for Unicode
								StringCbCopyN(spi.szCmdLine, sizeof(spi.szCmdLine),
											  pwszBuffer, dwBufferSize);
#else
								// Since core NT functions operate in Unicode
								// we must convert to Ansi since our application
								// is not compiled for Unicode
								WideCharToMultiByte(CP_ACP, 0, pwszBuffer,
													(int)(dwBufferSize / sizeof(WCHAR)),
													spi.szCmdLine, sizeof(spi.szCmdLine),
													NULL, NULL);
#endif
							}
							if(!HeapFree(hHeap, 0, pwszBuffer)) {
								// failed to free memory
								bReturnStatus = FALSE;
								goto gnpiFreeMemFailed;
							}
						}
					}	// Read CommandLine in Process Parameters

					// We got Process Parameters, is ImagePath filled in
					if(peb_upp.ImagePathName.Length > 0) {
						// Yes, try to read ImagePath
						dwBytesRead = 0;
						pwszBuffer = (WCHAR *)HeapAlloc(hHeap,
														HEAP_ZERO_MEMORY,
														peb_upp.ImagePathName.Length);
						if(pwszBuffer)
						{
                            if(ReadProcessMemory(hProcess,
												 peb_upp.ImagePathName.Buffer,
												 pwszBuffer,
												 peb_upp.ImagePathName.Length,
												 &dwBytesRead))
							{
								// if ImagePath is larger than our variable, truncate
								if(peb_upp.ImagePathName.Length >= sizeof(spi.szImgPath)) 
									dwBufferSize = sizeof(spi.szImgPath) - sizeof(TCHAR);
								else
									dwBufferSize = peb_upp.ImagePathName.Length;

								// Copy ImagePath to our structure
#if defined(UNICODE) or (_UNICODE)
								StringCbCopyN(spi.szImgPath, sizeof(spi.szImgPath),
											  pwszBuffer, dwBufferSize);
#else
								WideCharToMultiByte(CP_ACP, 0, pwszBuffer,
													(int)(dwBufferSize / sizeof(WCHAR)),
													spi.szImgPath, sizeof(spi.szImgPath),
													NULL, NULL);
#endif
							}
							if(!HeapFree(hHeap, 0, pwszBuffer)) {
								// failed to free memory
								bReturnStatus = FALSE;
								goto gnpiFreeMemFailed;
							}
						}
					}	// Read ImagePath in Process Parameters
				}	// Read Process Parameters
			}	// Read PEB 
		}	// Check for PEB

		// System process for WinXP and later is PID 4 and we cannot access
		// PEB, but we know it is aka ntoskrnl.exe so we will manually define it.
		// ntkrnlpa.exe if Physical Address Extension (PAE)
		// ntkrnlmp.exe if Symmetric MultiProcessing (SMP)
		// Actual filename is ntoskrnl.exe, but other name will be in
		// Original Filename field of version block.
		if(spi.dwPID == 4) {
			ExpandEnvironmentStrings(L"%SystemRoot%\\System32\\ntoskrnl.exe",
									 spi.szImgPath, sizeof(spi.szImgPath));
		}
	}	// Read Basic Info

gnpiFreeMemFailed:

	// Free memory if allocated
	if(pbi != NULL)
		if(!HeapFree(hHeap, 0, pbi)) {
			// failed to free memory
		}

	CloseHandle(hProcess);

	// Return filled in structure to caller
	*ppi = spi;

	return bReturnStatus;
}

// Code from demo to enumerage processes to an array

// Get current running processes
DWORD EnumProcesses2Array(smPROCESSINFO lpi[MAX_PI])
{
	DWORD dwPIDs[MAX_PI] = {0};
	DWORD dwArraySize	 = MAX_PI * sizeof(DWORD);
	DWORD dwSizeNeeded	 = 0;
	DWORD dwPIDCount	 = 0;

	//== only to have better chance to read processes =====
	if(!sm_EnableTokenPrivilege(SE_DEBUG_NAME)) {
	//	return 0;
	}

	// Get a list of Process IDs of current running processes
	if(EnumProcesses((DWORD*)&dwPIDs, dwArraySize, &dwSizeNeeded))
	{
			// Get detail info on each process
            dwPIDCount = dwSizeNeeded / sizeof(DWORD);
			for(DWORD p = 0; p < MAX_PI and p < dwPIDCount; p++)
			{
				if(sm_GetNtProcessInfo(dwPIDs[p], &lpi[p]))
				{
					// Do something else upon success
				}
			}
	}

	// Return either PID count or MAX_PI whichever is smaller
	return (DWORD)(dwPIDCount > MAX_PI) ? MAX_PI : dwPIDCount;
}