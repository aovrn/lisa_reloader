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

	TCHAR buf[MSG_BUF_SIZE];

	hToolhelpDLL = LoadLibrary(_T("toolhelp.dll"));
	if(hToolhelpDLL == NULL) 
	{
		_sntprintf(buf, MSG_BUF_SIZE - 1, _T("LoadLibrary: 0x%08X; arg: toolhelp.dll"), GetLastError());
		buf[MSG_BUF_SIZE - 1] = 0;
		MessageBox(GetActiveWindow(), buf, APP_NAME, NULL);
		exit(1);
	};
		
	CreateToolhelp32Snapshot = (_CreateToolhelp32Snapshot)GetProcAddress(hToolhelpDLL, _T("CreateToolhelp32Snapshot"));
	if(CreateToolhelp32Snapshot == NULL)
	{
		TCHAR buf[1024];
		_sntprintf(buf, MSG_BUF_SIZE - 1, _T("GetProcAddress: 0x%08X; arg: CreateToolhelp32Snapshot"), GetLastError());
		buf[MSG_BUF_SIZE - 1] = 0;
		MessageBox(GetActiveWindow(), buf, APP_NAME, NULL);
		exit(1);
	}

	CloseToolhelp32Snapshot = (_CloseToolhelp32Snapshot)GetProcAddress(hToolhelpDLL, _T("CloseToolhelp32Snapshot"));
	if(CloseToolhelp32Snapshot == NULL)
	{
		TCHAR buf[1024];
		_sntprintf(buf, MSG_BUF_SIZE - 1, _T("GetProcAddress: 0x%08X; arg: CloseToolhelp32Snapshot"), GetLastError());
		buf[MSG_BUF_SIZE - 1] = 0;
		MessageBox(GetActiveWindow(), buf, APP_NAME, NULL);
		exit(1);
	}

	Process32First = (_Process32First)GetProcAddress(hToolhelpDLL, _T("Process32First"));
	if(Process32First == NULL)
	{
		TCHAR buf[1024];
		_sntprintf(buf, MSG_BUF_SIZE - 1, _T("GetProcAddress: 0x%08X; arg: Process32First"), GetLastError());
		buf[MSG_BUF_SIZE - 1] = 0;
		MessageBox(GetActiveWindow(), buf, APP_NAME, NULL);
		exit(1);
	}

	Process32Next = (_Process32Next)GetProcAddress(hToolhelpDLL, _T("Process32Next"));
	if(Process32Next == NULL)
	{
		TCHAR buf[1024];
		_sntprintf(buf, MSG_BUF_SIZE - 1, _T("GetProcAddress: 0x%08X; arg: Process32Next"), GetLastError());
		buf[MSG_BUF_SIZE - 1] = 0;
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

typedef struct _REG_TZI_FORMAT
{
	LONG Bias;
	LONG StandardBias;
	LONG DaylightBias;
	SYSTEMTIME StandardDate;
	SYSTEMTIME DaylightDate;
} REG_TZI_FORMAT;

//#define REG_TIME_ZONES _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\")
#define REG_TIME_ZONES _T("Time Zones\\")

LONG GetTimeZoneInformationByName(LPTIME_ZONE_INFORMATION lptzi, LPCTSTR name)
{
	TR_START

	LONG ret = 0;

	HKEY hkey_tz;
	DWORD dw;
	REG_TZI_FORMAT regtzi;

	size_t len1 = _tcslen(REG_TIME_ZONES);
	size_t len2 = len1 + (sizeof(((TIME_ZONE_INFORMATION*)0)->StandardName) /
		sizeof(((TIME_ZONE_INFORMATION*)0)->StandardName[0]));
	TCHAR * tzsubkey = (TCHAR *)malloc(len2 * sizeof(TCHAR));

	_tcsncpy(tzsubkey, REG_TIME_ZONES, len1);
	_tcsncpy(tzsubkey + len1, name, len2  - len1);
	if (tzsubkey[len2 - 1] != 0)
	{
		// timezone name too long
		free(tzsubkey);
		return 1;
	}

	if (ERROR_SUCCESS != (dw = RegOpenKeyEx(HKEY_LOCAL_MACHINE, tzsubkey, 0, 0, &hkey_tz)))
	{
		// failed to open registry key
		free(tzsubkey);
		return 2;
	}

	#define X(param, type, var, err) \
		do if ((dw = sizeof(var)), (ERROR_SUCCESS != (dw = RegQueryValueExW(hkey_tz, param, NULL, NULL, (LPBYTE)&var, &dw)))) \
		{ \
			ret = err; \
			goto ennd; \
		} while(0)

	X(L"TZI", REG_BINARY, regtzi, 3);
	X(L"Std", REG_SZ, lptzi->StandardName, 4);
	X(L"Dlt", REG_SZ, lptzi->DaylightName, 5);
	#undef X

	lptzi->Bias = regtzi.Bias;
	lptzi->DaylightBias = regtzi.DaylightBias;
	lptzi->DaylightDate = regtzi.DaylightDate;
	lptzi->StandardBias = regtzi.StandardBias;
	lptzi->StandardDate = regtzi.StandardDate;

ennd:
	RegCloseKey(hkey_tz);
	free(tzsubkey);
	return ret;

	TR_END0
}