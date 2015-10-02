
#ifndef _WINCE_API_H
#define _WINCE_API_H

#define TH32CS_SNAPPROCESS  0x00000002

typedef struct tagPROCESSENTRY32 {
    DWORD   dwSize;
    DWORD   cntUsage;
    DWORD   th32ProcessID;
    DWORD   th32DefaultHeapID;
    DWORD   th32ModuleID;
    DWORD   cntThreads;
    DWORD   th32ParentProcessID;
    LONG    pcPriClassBase;
    DWORD   dwFlags;
    TCHAR   szExeFile[MAX_PATH];
    DWORD	th32MemoryBase;
    DWORD	th32AccessKey;
} PROCESSENTRY32, *PPROCESSENTRY32, *LPPROCESSENTRY32;

typedef HANDLE (WINAPI * _CreateToolhelp32Snapshot)(DWORD dwFlags, DWORD th32ProcessID);
typedef BOOL (WINAPI * _CloseToolhelp32Snapshot)(HANDLE hSnapshot);
typedef BOOL (WINAPI * _Process32First)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
typedef BOOL (WINAPI * _Process32Next)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);

extern _CreateToolhelp32Snapshot CreateToolhelp32Snapshot;
extern _CloseToolhelp32Snapshot CloseToolhelp32Snapshot;
extern _Process32First Process32First;
extern _Process32Next Process32Next;

void initWinceApi();
void freeWinceApi();

#endif //_WINCE_API_H