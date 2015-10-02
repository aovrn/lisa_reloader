#include <windows.h>

#include "winceapi.h"
#include "common.h"

_CreateToolhelp32Snapshot CreateToolhelp32Snapshot;
_CloseToolhelp32Snapshot CloseToolhelp32Snapshot;
_Process32First Process32First;
_Process32Next Process32Next;

HINSTANCE hToolhelpDLL = NULL;

void initWinceApi()
{
	TR_START

	CreateToolhelp32Snapshot = NULL;
	CloseToolhelp32Snapshot = NULL;
	Process32First = NULL;
	Process32Next = NULL;

	hToolhelpDLL = LoadLibrary(_T("toolhelp.dll"));
	if(hToolhelpDLL == NULL) 
	{
		TCHAR buf[1024];
		_sntprintf(buf, sizeof(buf), _T("LoadLibrary: 0x%08X; arg: toolhelp.dll"), GetLastError());
		MessageBox(GetActiveWindow(), buf, APP_NAME, NULL);
		exit(1);
	};
		
	CreateToolhelp32Snapshot = (_CreateToolhelp32Snapshot)GetProcAddress(hToolhelpDLL, _T("CreateToolhelp32Snapshot"));
	if(CreateToolhelp32Snapshot == NULL)
	{
		TCHAR buf[1024];
		_sntprintf(buf, sizeof(buf), _T("GetProcAddress: 0x%08X; arg: CreateToolhelp32Snapshot"), GetLastError());
		MessageBox(GetActiveWindow(), buf, APP_NAME, NULL);
		exit(1);
	}

	CloseToolhelp32Snapshot = (_CloseToolhelp32Snapshot)GetProcAddress(hToolhelpDLL, _T("CloseToolhelp32Snapshot"));
	if(CloseToolhelp32Snapshot == NULL)
	{
		TCHAR buf[1024];
		_sntprintf(buf, sizeof(buf), _T("GetProcAddress: 0x%08X; arg: CloseToolhelp32Snapshot"), GetLastError());
		MessageBox(GetActiveWindow(), buf, APP_NAME, NULL);
		exit(1);
	}

	Process32First = (_Process32First)GetProcAddress(hToolhelpDLL, _T("Process32First"));
	if(Process32First == NULL)
	{
		TCHAR buf[1024];
		_sntprintf(buf, sizeof(buf), _T("GetProcAddress: 0x%08X; arg: Process32First"), GetLastError());
		MessageBox(GetActiveWindow(), buf, APP_NAME, NULL);
		exit(1);
	}

	Process32Next = (_Process32Next)GetProcAddress(hToolhelpDLL, _T("Process32Next"));
	if(Process32Next == NULL)
	{
		TCHAR buf[1024];
		_sntprintf(buf, sizeof(buf), _T("GetProcAddress: 0x%08X; arg: Process32Next"), GetLastError());
		MessageBox(GetActiveWindow(), buf, APP_NAME, NULL);
		exit(1);
	}
	
	TR_END
}

void freeWinceApi()
{
	TR_START

	FreeLibrary(hToolhelpDLL);
	hToolhelpDLL = NULL;
	
	CreateToolhelp32Snapshot = NULL;
	CloseToolhelp32Snapshot = NULL;
	Process32First = NULL;
	Process32Next = NULL;
	
	TR_END
}