#include <windows.h>
#include "resource.h"

#include "stringlist.h"
#include "common.h"
#include "config.h"
#include "winceapi.h"

#define MAX_LOADSTRING	512

// Timers definition
#define TIMER_SECTION_START	1
#define TIMER_KILLWND_IT	11
#define TIMER_EXIT			101

#define TIMEOUT				300

// Global Variables:
HINSTANCE			hInst;			// The current instance
HWND				hWnd;			// Main wnd
HBITMAP				hSplash = NULL;
BOOL				g_Show = TRUE;

TCHAR g_CmdLine[1024];
TCHAR g_Title[MAX_LOADSTRING];
TCHAR g_WindowClass[MAX_LOADSTRING];


StringList			messages;		// Messages
BOOL				except = FALSE;	// Flag indicates that exception catched
Config *			config = NULL;
Config::Section *	section = NULL;	// Current section

// Draw global vars
int g_Page = 0;
int g_Pages = 1;
int g_Lines = 0;
int g_Size = 0;

// Log section
Config::Section *	g_LogSection = NULL;

// PostWnd global vars
unsigned long		g_StartTime = 0;
unsigned long		g_PostMsg = 0;
unsigned long		g_PostWParam = 0;
unsigned long		g_PostLParam = 0;
const unsigned long	g_PostItemStrSize = 64;
struct PostWndItem
{
	TCHAR className[g_PostItemStrSize];
	TCHAR windowName[g_PostItemStrSize];
	TCHAR procName[g_PostItemStrSize];
	BOOL closed;
	BOOL wndclosed;
};
PostWndItem *		g_PostWndList = NULL;	// Wnd list for PostWnd section
unsigned long		g_PostWndCount = 0;		// Wnd list size

// Wait messages vars
BOOL				g_WaitMsg = FALSE;
struct MsgHandler
{
	unsigned long msg;
	unsigned long wparam;
	unsigned long lparam;
	LPCTSTR label;
};
unsigned long		g_HandlerCount = 0;
const unsigned long	g_HandlerMaxCount = 100;
MsgHandler			g_Handlers[g_HandlerMaxCount];
unsigned long		g_LastMsg;
unsigned long		g_LastWParam;
unsigned long		g_LastLParam;

// State global variables
struct StateVariable
{
	TCHAR * name;
	TCHAR * value;
	size_t size;
};
unsigned long		g_VariableCount = 0;
const unsigned long	g_VariableMaxCount = 100;
StateVariable		g_Variables[g_VariableMaxCount];

// Immediate jump
UINT g_immediateJumpId = 0;
BOOL g_inImmediateLoop = FALSE;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass	(HINSTANCE, LPTSTR);
BOOL				InitInstance	(HINSTANCE, int);
LRESULT CALLBACK	WndProc			(HWND, UINT, WPARAM, LPARAM);

LPCTSTR getFilledValue(Config::Argument& arg);
LPCTSTR getFilledValue2(Config::Argument& arg);
LPCTSTR getFilledValue3(Config::Argument& arg);

void KillTimers()
{
	KillTimer(hWnd, TIMER_SECTION_START);
	KillTimer(hWnd, TIMER_KILLWND_IT);
	KillTimer(hWnd, TIMER_EXIT);
	g_LogSection = NULL;
	g_WaitMsg = FALSE;
}

void KillCallback()
{
	KillTimers();
	except = true;
}

int WINAPI WinMain(	HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPTSTR    lpCmdLine,
					int       nCmdShow)
{
	TR_START

	config = Config::getInstance();
	_tcsncpy(g_CmdLine, lpCmdLine, sizeof(g_CmdLine) / sizeof(TCHAR));
	lpCmdLine[sizeof(g_CmdLine) - 1] = 0;

	exception_callback = &KillCallback;
	initWinceApi();

	if (!InitInstance (hInstance, nCmdShow)) {
		return FALSE;
	}
	
	DWORD start = GetTickCount();

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (config->settings().benchmark)
	{
		TCHAR buf[MSG_BUF_SIZE];
		_sntprintf(buf, MSG_BUF_SIZE - 1, _T("WndProc run for %d msecs"), GetTickCount() - start);
		buf[MSG_BUF_SIZE - 1] = 0;
		MessageBox(NULL, buf, APP_NAME, MB_TOPMOST | MB_SETFOREGROUND);
	}

	freeWinceApi();

	return msg.wParam;

	TR_END0
}

ATOM MyRegisterClass(HINSTANCE hInstance, LPTSTR szWindowClass)
{
	WNDCLASS	wc;

    wc.style			= CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc		= (WNDPROC) WndProc;
    wc.cbClsExtra		= 0;
    wc.cbWndExtra		= 0;
    wc.hInstance		= hInstance;
    wc.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
    wc.hCursor			= 0;
    wc.hbrBackground	= (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName		= 0;
    wc.lpszClassName	= szWindowClass;

	return RegisterClass(&wc);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	TR_START

	hInst = hInstance;
	if (config->settings().wclass != NULL)
	{
		_tcsncpy(g_WindowClass, config->settings().wclass, MAX_LOADSTRING);
		g_WindowClass[MAX_LOADSTRING - 1] = 0;
	}
	else
	{
		LoadString(hInstance, IDS_APP_CLASS, g_WindowClass, MAX_LOADSTRING);
	}

	MyRegisterClass(hInstance, g_WindowClass);

	if (config->settings().wtitle != NULL)
	{
		_tcsncpy(g_Title, config->settings().wtitle, MAX_LOADSTRING);
		g_Title[MAX_LOADSTRING - 1] = 0;
	}
	else
	{
		LoadString(hInstance, IDS_APP_TITLE, g_Title, MAX_LOADSTRING);
	}

	// Avoids previous instances
	hWnd = FindWindow(g_WindowClass, g_Title);
	if (hWnd && !config->settings().second_noactivate)
	{
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
	}
	if (hWnd && !config->settings().second_noexit)
	{
		return 0;
	}

	g_Show = !config->settings().minimize && !config->settings().hide;
	if (config->settings().minimize)
	{
		nCmdShow = SW_MINIMIZE;
	}
	else if (config->settings().hide)
	{
		nCmdShow = SW_HIDE;
	}

	hWnd = CreateWindow(g_WindowClass, g_Title, nCmdShow != SW_HIDE ? WS_VISIBLE : 0,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	if (nCmdShow != SW_HIDE )
	{
		ShowWindow(hWnd, nCmdShow);
	}
	SetWindowPos(hWnd, HWND_TOPMOST,
		config->settings().x, config->settings().y,
		config->settings().width, config->settings().height,
		!g_Show ? SWP_NOACTIVATE | SWP_NOZORDER : SWP_SHOWWINDOW);
	UpdateWindow(hWnd);

	return TRUE;

	TR_END0
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void invalidate()
{
	if (hSplash == NULL && g_Show &&
		g_Pages - g_Page == 1 && messages.Size() != g_Size)
	{
		InvalidateRgn(hWnd, NULL, TRUE);
	}
}

void setTimer(UINT id, UINT timeout, BOOL showwait = TRUE)
{
	TR_START

	if (id != TIMER_KILLWND_IT)
	{
		if (section != NULL && section->stop)
		{
			AddLogMsg(messages, _T("last section in chain; stopped"));
			
		}
		else if (timeout > 0)
		{
			AddLogMsg(messages, _T("waiting for %d msecs"), timeout);
		}
	}

	invalidate();

	if (id == TIMER_KILLWND_IT || section == NULL || !section->stop)
	{
		if (config->settings().immediate_jump &&
			(section == NULL || section->type != Config::esec::error))
		{
			if (timeout > 0)
				Sleep(timeout);
			g_immediateJumpId = id;
		}
		else
		{
			SetTimer(hWnd, id, timeout, NULL);
		}
		
	}

	TR_END
}

BOOL jumpToSection(BOOL success, BOOL gonext = TRUE)
{
	TR_START
	
	LPCTSTR handler = NULL;

	if (success)
	{
		for (unsigned long i = 0; i < section->argc; ++i)
		{
			if (section->args[i].type == Config::earg::onsuccess)
			{
				handler = getFilledValue(section->args[i]);
				break;
			}
		}
	}
	else
	{
		for (unsigned long i = 0; i < section->argc; ++i)
		{
			if (section->args[i].type == Config::earg::onerror)
			{
				handler = getFilledValue(section->args[i]);
				break;
			}
		}
	}
	
	if (handler == NULL)
	{
		for (unsigned long i = 0; i < section->argc; ++i)
		{
			if (section->args[i].type == Config::earg::jump)
			{
				handler = getFilledValue(section->args[i]);
				break;
			}
		}
	}
	
	if (handler != NULL)
	{
		if(config->seekToSection(handler))
		{
			AddLogMsg(messages, _T("jump to '%s'"), handler);
			unsigned long wait = section->wait;
			section = NULL;
			setTimer(TIMER_SECTION_START, wait);
			return TRUE;
		}
		else
		{
			AddLogMsg(messages, _T("jump error to '%s'"), handler);
			invalidate();
		}
	}
	else if (gonext)
	{
		setTimer(TIMER_SECTION_START, section->wait);
		return TRUE;
	}
		
	return FALSE;

	TR_END0
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// VARIABLE FORMAT: ${key:defval}
LPCTSTR getStateVariable(LPCTSTR name, LPCTSTR defval)
{
	TR_START

	for (unsigned long it = 0; it < g_VariableCount; ++it)
	{
		if (_tcsicmp(g_Variables[it].name, name) == 0)
		{
			return g_Variables[it].value;
		}
	}

	while (1)
	{
		if (g_VariableCount == g_VariableMaxCount)
		{
			AddLogMsg(messages, _T("error: maximum state variables count is %d"), g_VariableMaxCount);
			break;
		}
		
		if (config->settings().state_path == NULL)
		{
			AddLogMsg(messages, _T("error: state path is not set"));
			break;
		}

		TCHAR path[MAX_PATH + 1];
		
		if (_tcslen(config->settings().state_path) + _tcslen(name) + 6 > MAX_PATH)
		{
			AddLogMsg(messages, _T("error: state variable name is too big"));
			break;
		}
		
		_tcscpy(path, config->settings().state_path);
		_tcscat(path, name);
		_tcscat(path, _T(".state"));
		
		FILE * f = _tfopen(path, _T("r"));
		if(f)
		{
			fseek(f, 0, SEEK_END);
			long len = ftell(f);
			if (len == -1)
			{
				AddLogMsg(messages, _T("ftell error: 0x%08X"), GetLastError());
				AddLogMsg(messages, path);
			}
			fseek(f, 0, SEEK_SET);
			
			g_Variables[g_VariableCount].value = (TCHAR *)malloc(len + sizeof(TCHAR));
			g_Variables[g_VariableCount].size = len + sizeof(TCHAR);

			if (fread(g_Variables[g_VariableCount].value, sizeof(TCHAR), len / sizeof(TCHAR), f) != (size_t)len / sizeof(TCHAR))
			{
				AddLogMsg(messages, _T("fread error: 0x%08X"), GetLastError());
				AddLogMsg(messages, path);

				free(g_Variables[g_VariableCount].value);
			}
			else
			{				
				g_Variables[g_VariableCount].value[len / sizeof(TCHAR)] = 0;

				AddLogMsg(messages, _T("loaded '%s': '%s'"), name, g_Variables[g_VariableCount].value);

				size_t size = (_tcslen(name) + 1) * sizeof(TCHAR);
				g_Variables[g_VariableCount].name = (TCHAR *)malloc(size);
				memcpy(g_Variables[g_VariableCount].name, name, size);
				++g_VariableCount;
				
				fclose(f);
				return g_Variables[g_VariableCount - 1].value;
			}
			fclose(f);
		}
		else
		{
			AddLogMsg(messages, _T("_tfopen error: 0x%08X"), GetLastError());
			AddLogMsg(messages, path);
		}

		break;
	}

	return defval;

	TR_END0
}

void setStateVariable(LPCTSTR name, LPCTSTR value)
{
	TR_START

	size_t size = (_tcslen(value) + 1) * sizeof(TCHAR);

	for (unsigned long it = 0; it < g_VariableCount; ++it)
	{
		if (_tcsicmp(g_Variables[it].name, name) == 0)
		{
			if (g_Variables[it].size < size)
			{
				free(g_Variables[it].value);
				g_Variables[it].value = (TCHAR *)malloc(size);
				g_Variables[it].size = size;
			}
			memcpy(g_Variables[it].value, value, size);
			
			AddLogMsg(messages, _T("set '%s': %s'"), name, value);
			return;
		}
	}

	if (g_VariableCount == g_VariableMaxCount)
	{
		AddLogMsg(messages, _T("error: maximum state variables count is %d"), g_VariableMaxCount);
		return;
	}
	
	g_Variables[g_VariableCount].value = (TCHAR *)malloc(size);
	g_Variables[g_VariableCount].size = size;
	memcpy(g_Variables[g_VariableCount].value, value, size);
	
	size = (_tcslen(name) + 1) * sizeof(TCHAR);
	g_Variables[g_VariableCount].name = (TCHAR *)malloc(size);
	memcpy(g_Variables[g_VariableCount].name, name, size);
	
	++g_VariableCount;
	
	AddLogMsg(messages, _T("set '%s': %s'"), name, value);

	TR_END
}

void saveStateVariable(LPCTSTR name)
{
	TR_START
	
	LPTSTR value = NULL;

	for (unsigned long it = 0; it < g_VariableCount; ++it)
	{
		if (_tcsicmp(g_Variables[it].name, name) == 0)
		{
			value = g_Variables[it].value;
			break;
		}
	}

	if (value == NULL)
	{
		AddLogMsg(messages, _T("error: state variable '%s' not found"), name);
		return;
	}	

	while (1)
	{
		TCHAR path[MAX_PATH + 1];
		
		if (value == NULL)
		{
			AddLogMsg(messages, _T("error: variable value is empty"));
			break;
		}
		if (config->settings().state_path == NULL)
		{
			AddLogMsg(messages, _T("error: state path is not set"));
			break;
		}
		
		if (_tcslen(config->settings().state_path) + _tcslen(name) + 6 > MAX_PATH)
		{
			AddLogMsg(messages, _T("error: state variable name is too big"));
			break;
		}
		
		_tcscpy(path, config->settings().state_path);
		_tcscat(path, name);
		_tcscat(path, _T(".state"));
		
		FILE * f = _tfopen(path, _T("w"));
		if(f)
		{
			size_t len = _tcslen(value);
			if (fwrite(value, sizeof(TCHAR), len, f) != len)
			{
				AddLogMsg(messages, _T("fwrite error: 0x%08X"), GetLastError());
				AddLogMsg(messages, path);
			}
			else
			{
				AddLogMsg(messages, _T("saved '%s': %s'"), name, value);
			}
			fclose(f);
		}
		else
		{
			AddLogMsg(messages, _T("_tfopen error: 0x%08X"), GetLastError());
			AddLogMsg(messages, path);
		}

		break;
	}

	TR_END
}

LPCTSTR fillStringVariables(LPTSTR buf, size_t count, LPCTSTR str)
{
	TR_START

	LPCTSTR ret = str;

	if (str == NULL || _tcsstr(str, _T("${")) == NULL)
	{
		return ret;
	}

	size_t size = (_tcslen(str) + 1) * sizeof(TCHAR);
	TCHAR * tmp = (TCHAR *)malloc(size);
	memcpy(tmp, str, size);
	*buf = 0;

	TCHAR * pos1, * pos2, * pos3;
	for (pos1 = tmp, pos2 = _tcsstr(tmp, _T("${"));
		pos2 != NULL && _tcslen(pos2) > 3; )
	{
		if ((pos3 = _tcsstr(pos2, _T("}"))) == NULL)
		{
			pos2 = _tcsstr(pos2 + 2, _T("${"));
			continue;
		}

		ret = buf;

		*pos2 = 0;
		_tcsncat(buf, pos1, count - _tcslen(buf) - 1);

		if (count - _tcslen(buf) - 1 <= 0)
		{
			break;
		}

		pos1 = pos2 + 2;
		pos3 = _tcsstr(pos1, _T("}"));
		*pos3 = 0;
		if ((pos2 = _tcsstr(pos1, _T(":"))) != NULL)
		{
			*pos2 = 0;
			++pos2;
		}
		else
		{
			static TCHAR err[] = _T("ERROR_NO_DEFAULT");
			pos2 = err;
		}

		_tcsncat(buf, getStateVariable(pos1, pos2), count - _tcslen(buf) - 1);
		
		if (count - _tcslen(buf) - 1 <= 0)
		{
			break;
		}

		pos1 = pos3 + 1;
		pos2 = _tcsstr(pos1, _T("${"));
	}
		
	if (ret == buf)
	{
		if (count - _tcslen(buf) - 1 > 0 && _tcslen(pos1) > 0)
		{
			_tcsncat(buf, pos1, count - _tcslen(buf) - 1);
		}
		buf[count - 1] = 0;
	}

	free(tmp);

	//AddLogMsg(messages, _T("string: %s"), ret);

	return ret;

	TR_END0
}

const size_t g_fillBufferSize = 4096;
TCHAR g_fillBuffer[g_fillBufferSize];
LPCTSTR getFilledValue(LPCTSTR value, LPTSTR& valueFilled)
{
	TR_START

	LPCTSTR ret = fillStringVariables(g_fillBuffer, g_fillBufferSize, value);

	if (ret != value)
	{
		if (valueFilled == NULL || _tcscmp(g_fillBuffer, valueFilled) != 0)
		{
			size_t len = (_tcslen(g_fillBuffer) + 1) * sizeof(TCHAR);
			if (valueFilled == NULL || _tcslen(g_fillBuffer) != _tcslen(valueFilled))
			{
				if (valueFilled != NULL)
				{
					free(valueFilled);
					//AddLogMsg(messages, _T("string: free buffer"));
				}
				valueFilled = (TCHAR *)malloc(len);
				//AddLogMsg(messages, _T("string: allocated new buffer"));
			}
			memcpy(valueFilled, g_fillBuffer, len);
		}
		ret = valueFilled;
	}

	return ret;
	
	TR_END0
}

LPCTSTR getFilledValue(Config::Argument& arg)
{
	return getFilledValue(arg.szValue, arg.szValueFilled);
}

LPCTSTR getFilledValue2(Config::Argument& arg)
{
	return getFilledValue(arg.szValue2, arg.szValueFilled2);
}

LPCTSTR getFilledValue3(Config::Argument& arg)
{
	return getFilledValue(arg.szValue3, arg.szValueFilled3);
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void listWnds()
{
	TR_START

	AddLogMsg(messages, _T(""));
	AddLogMsg(messages, _T("[ListWnd]"));

	HWND nextWindow = GetForegroundWindow();
	TCHAR lpWindowTitle[64];
	TCHAR lpClassName[64];

	while(nextWindow)
	{
		GetWindowText(nextWindow, lpWindowTitle, 64);
		GetClassName(nextWindow, lpClassName, 64);

		AddLogMsg(messages, _T("'%s' '%s'"), lpClassName, lpWindowTitle);
		
		nextWindow = GetWindow(nextWindow, GW_HWNDNEXT);
	}

	TR_END
}

HWND g_HWND[500];
DWORD g_Pid[500];
DWORD g_WndIt;

BOOL CALLBACK EnumWindowsProcMy(HWND hwnd,LPARAM lParam)
{
	TR_START

	if (g_WndIt == 500)
	{
		return FALSE;
	}

	DWORD lpdwProcessId;
	GetWindowThreadProcessId(hwnd, &lpdwProcessId);
	g_HWND[g_WndIt] = hwnd;
	g_Pid[g_WndIt++] = lpdwProcessId;

	return TRUE;
	
	TR_END0
}

void showProcs(PPROCESSENTRY32 * list, LPCTSTR proc)
{
	TR_START
	
	TCHAR lpWindowTitle[64];
	TCHAR lpClassName[64];

	while (1)
	{
		if (list == NULL)
		{
			if (proc != NULL)
			{
				memset(g_HWND, 0, sizeof(g_HWND));
				memset(g_Pid, 0, sizeof(g_Pid));
				g_WndIt = 0;
				if (!EnumWindows(EnumWindowsProcMy, NULL))
				{
					AddLogMsg(messages, _T("EnumWindows: 0x%08X"), GetLastError());
					break;
				}
			}
		}

		HANDLE hProcessSnap;
		PROCESSENTRY32 pe32;
		
		hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if(hProcessSnap == INVALID_HANDLE_VALUE)
		{
			AddLogMsg(messages, _T("CreateToolhelp32Snapshot: 0x%08X"), GetLastError());
			break;
		}
		
		pe32.dwSize = sizeof(PROCESSENTRY32);
		
		if(!Process32First(hProcessSnap, &pe32))
		{
			AddLogMsg(messages, _T("Process32First: 0x%08X"), GetLastError());
			CloseToolhelp32Snapshot(hProcessSnap);
			break;
		}

		if (list != NULL)
		{
			size_t len = 1;
			do
			{
				++len;
			}
			while(Process32Next(hProcessSnap, &pe32));
			len = sizeof(tagPROCESSENTRY32) * len;
			*list = (PPROCESSENTRY32)malloc(len);
			memset(*list, 0, len);

			if(!Process32First(hProcessSnap, &pe32))
			{
				AddLogMsg(messages, _T("Process32First: 0x%08X"), GetLastError());
				CloseToolhelp32Snapshot(hProcessSnap);
				break;
			}
		}

		int it = 0;
		do
		{
			if (proc != NULL && (_tcsicmp(pe32.szExeFile, proc) != 0) && (_tcsicmp(_T("all"), proc) != 0))
			{
				continue;
			}
			if (list == NULL)
			{
				AddLogMsg(messages, _T("0x%08X '%s'"), pe32.th32ProcessID, pe32.szExeFile);

				if (proc != NULL)
				{
					for (DWORD it = 0; it < g_WndIt; ++it)
					{
						if (pe32.th32ProcessID == g_Pid[it])
						{
							if(GetClassName(g_HWND[it], lpClassName, 64) == 0)
							{
								AddLogMsg(messages, _T("    GetClassName: 0x%08X"), GetLastError());
							}
							if((GetWindowText(g_HWND[it], lpWindowTitle, 64) == 0) && GetLastError() != 0)
							{
								AddLogMsg(messages, _T("    GetWindowText: 0x%08X"), GetLastError());
							}

							AddLogMsg(messages, _T("    0x%08X '%s' '%s'"), g_HWND[it], lpClassName, lpWindowTitle);
						}
					}
				}
			}
			else
			{
				(*list)[it] = pe32;
				++it;
			}
		}
		while(Process32Next(hProcessSnap, &pe32));

		if (GetLastError() != ERROR_NO_MORE_FILES)
		{
			AddLogMsg(messages, _T("Process32Next: 0x%08X"), GetLastError());
		}
		
		CloseToolhelp32Snapshot( hProcessSnap );
		break;
	}

	return;

	TR_END
}

HWND findWindow(PPROCESSENTRY32 * list,
		LPCTSTR lpClassNameArg, LPCTSTR lpWindowNameArg, LPCTSTR lpProcNameArg,
		BOOL logNull = TRUE)
{
	TR_START
		
	LPCTSTR lpClassName = (*lpClassNameArg == 0 && config->settings().empty_as_null ? NULL : lpClassNameArg);
	LPCTSTR lpWindowName = (*lpWindowNameArg == 0 && config->settings().empty_as_null ? NULL : lpWindowNameArg);
	LPCTSTR lpProcName = (*lpProcNameArg == 0 ? NULL : lpProcNameArg);

	HWND hWnd = NULL;

	if (lpClassName == NULL && lpWindowName == NULL)
	{
		AddLogMsg(messages, _T("wClass and wTitle are empty"));
		return hWnd;
	}

	if (lpProcName != NULL)
	{
		if (*list == NULL)
		{
			memset(g_HWND, 0, sizeof(g_HWND));
			memset(g_Pid, 0, sizeof(g_Pid));
			g_WndIt = 0;
			if (!EnumWindows(EnumWindowsProcMy, NULL))
			{
				AddLogMsg(messages, _T("EnumWindows: 0x%08X"), GetLastError());
				return hWnd;
			}
			showProcs(list, NULL);
		}

		TCHAR lpEnumWindowName[64];
		TCHAR lpEnumClassName[64];
		for (DWORD it1 = 0; it1 < g_WndIt; ++it1)
		{
			if(GetClassName(g_HWND[it1], lpEnumClassName, 64) == 0)
			{
				AddLogMsg(messages, _T("GetClassName: 0x%08X"), GetLastError());
				lpEnumClassName[0] = 0;
			}
			if((GetWindowText(g_HWND[it1], lpEnumWindowName, 64) == 0) && GetLastError() != 0)
			{
				AddLogMsg(messages, _T("GetWindowText: 0x%08X"), GetLastError());
				lpEnumWindowName[0] = 0;
			}
			if ((lpWindowName != NULL && _tcsicmp(lpEnumWindowName, lpWindowName) != 0) ||
				(lpClassName != NULL && _tcsicmp(lpEnumClassName, lpClassName) != 0))
			{
				continue;
			}
			for (unsigned long it2 = 0; ; ++it2)
			{
				if ((*list)[it2].szExeFile[0] == 0)
				{
					break;
				}
				if (_tcsicmp((*list)[it2].szExeFile, lpProcName) == 0)
				{
					if ((*list)[it2].th32ProcessID == g_Pid[it1])
					{
						hWnd = g_HWND[it1];
						break;
					}
				}
			}
			if (hWnd != NULL)
			{
				break;
			}
		}
	}
	else
	{
		hWnd = FindWindow(lpClassName, lpWindowName);
	}

	if (logNull && hWnd == NULL)
	{
		if (lpProcName == NULL)
		{
			AddLogMsg(messages, _T("FindWindow: 0x%08X '%s' '%s'"), GetLastError(),
				lpClassName == NULL ? _T("") : lpClassName,
				lpWindowName == NULL ? _T("") : lpWindowName);
		}
		else
		{
			AddLogMsg(messages, _T("not found '%s' '%s' '%s'"),
				lpClassName == NULL ? _T("") : lpClassName,
				lpWindowName == NULL ? _T("") : lpWindowName,
				lpProcName);
		}
	}

	return hWnd;

	TR_END0
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (except)
	{
		return 0;
	}

	TR_START

	static int xPos = -1;
	static int yPos = -1;

	static int clickCount = 0;
	static int clickTime = 0;
	
	if (g_LogSection != NULL)
	{
		TR_START

		// Process [LogMsg] section
		if (g_LogSection->type != Config::esec::logmsg)
		{
			AddLogMsg(messages, _T("logmsg error: incorrect section"));
			g_LogSection = NULL;
			invalidate();
		}
		BOOL bSkip = FALSE;
		for (unsigned long it = 0; it < g_LogSection->argc; ++it)
		{
			if (g_LogSection->args[it].type == Config::earg::msg && g_LogSection->args[it].ulValue == message)
			{
				bSkip = TRUE;
				break;
			}
			if (g_LogSection->args[it].type == Config::earg::wparam && g_LogSection->args[it].ulValue == wParam)
			{
				bSkip = TRUE;
				break;
			}
			if (g_LogSection->args[it].type == Config::earg::lparam && (LPARAM)g_LogSection->args[it].ulValue == lParam)
			{
				bSkip = TRUE;
				break;
			}
		}
		
		if (!bSkip)
		{
			switch (message)
			{
			case WM_WINDOWPOSCHANGED:
				{
					LPWINDOWPOS lpwp = LPWINDOWPOS(lParam);
					AddLogMsg(messages, _T("[msg] 0x%08x WM_WINDOWPOSCHANGED"), message, wParam, lParam);
					AddLogMsg(messages, _T("    ins: 0x%x; x: %d; y: %d; cx: %d; cy: %d; flags: 0x%08x"),
						lpwp->hwndInsertAfter, lpwp->x, lpwp->y, lpwp->cx, lpwp->cy, lpwp->flags);
				}
				break;
			default:
				AddLogMsg(messages, _T("[msg] 0x%08x 0x%08x 0x%08x"), message, wParam, lParam);
				break;
			}
			invalidate();
		}

		TR_END
	}

	switch (message)
	{
		case WM_LBUTTONDOWN:
			TR_START

			// Process exit
			int tc = GetTickCount();
			if (clickTime == 0 || tc - clickTime > 500)
			{
				clickTime = tc;
				clickCount = 1;
				++g_Page;
				InvalidateRgn(hWnd, NULL, TRUE);
			}
			else if (clickCount < 2)
			{
				clickTime = tc;
				++clickCount;
			}
			else
			{
				clickTime = 0;
				if (hSplash != NULL)
				{
					hSplash = NULL;
					invalidate();
				}
				else
				{
					KillTimers();
					PostMessage(hWnd, WM_CLOSE, 0, 0);
				}
			}

			xPos = -1;
			yPos = -1;

			TR_END
			break;
		case WM_MOUSEMOVE:
			TR_START

			if (wParam == MK_LBUTTON)
			{
				// Process move window
				int newXPos = LOWORD(lParam);
				int newYPos = HIWORD(lParam);
				if (xPos != -1 && yPos != -1)
				{
					int deltaX = newXPos - xPos;
					int deltaY = newYPos - yPos;
					if (abs(deltaX) < 10 && abs(deltaY) < 10)
					{
						break;
					}
					
					RECT rc;
					GetWindowRect(hWnd, &rc);
					SetWindowPos(hWnd, HWND_TOPMOST, rc.left + deltaX, rc.top + deltaY,
						config->settings().width, config->settings().height, SWP_SHOWWINDOW);
					GetWindowRect(hWnd, &rc);
				}
				else
				{
					xPos = newXPos;
					yPos = newYPos;
				}
			}
			
			TR_END
			break;
		case WM_CREATE:
			TR_START
			
			AddLogMsg(messages, _T("cmd: '%s'"), g_CmdLine);
			AddLogMsg(messages, _T("wnd: '%s, %s'"), g_WindowClass, g_Title);

			if (config->settings().splash != NULL)
			{
				hSplash = (HBITMAP)SHLoadDIBitmap(config->settings().splash);
				if (hSplash == NULL)
				{
					AddLogMsg(messages, _T("LoadImage: 0x%08X '%s'"), GetLastError(), config->settings().splash);
				}
			}
			SetTimer(hWnd, TIMER_SECTION_START, 0, NULL);

			TR_END
			break;
		case WM_PAINT:
		{
			TR_START
			
			RECT rt1 = {0};
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);

			if (hSplash == NULL)
			{
				GetClientRect(hWnd, &rt1);
				FillRect(hdc, &rt1, (HBRUSH)(COLOR_WINDOWTEXT));
				RECT rt = {rt1.left + 1, rt1.top + 1, rt1.right - 1, rt1.bottom -1 };
				FillRect(hdc, &rt, (HBRUSH)(COLOR_WINDOW));
				SetBkMode(hdc, TRANSPARENT);
				++rt.top;
				++rt.left;

				int lh = 0;
				static TCHAR title[] = _T("===== Lisa Reloader =====");
			
				lh = DrawText(hdc, title, _tcslen(title), &rt, DT_LEFT | DT_TOP);
				g_Lines = (config->settings().height - 4) / lh - 1;
	
				g_Size = messages.Size();
				g_Pages = (g_Size / g_Lines) + 1;

				if (g_Page >= g_Pages)
				{
					g_Page = 0;
				}
			
				TCHAR buf[64];
				_sntprintf(buf, 63, _T("page [%d/%d]"), g_Page + 1, g_Pages);
				buf[63] = 0;
				DrawText(hdc, buf, _tcslen(buf), &rt, DT_RIGHT | DT_TOP);

				rt.top += lh;
			
				for (int it = g_Page * g_Lines; it < g_Size && it < (g_Page + 1) * g_Lines; ++it)
				{
					DrawText(hdc, messages[it], _tcslen(messages[it]), &rt, DT_LEFT | DT_TOP);
					rt.top += lh;
				}
			}
			else
			{
				HDC hdcMem = CreateCompatibleDC(hdc);
				HGDIOBJ oldBitmap = SelectObject(hdcMem, hSplash);
				BITMAP bitmap;
				GetObject(hSplash, sizeof(bitmap), &bitmap);
				BitBlt(hdc, 0, 0, bitmap.bmWidth, bitmap.bmHeight, hdcMem, 0, 0, SRCCOPY);
				SelectObject(hdcMem, oldBitmap);
				DeleteDC(hdcMem);
			}

			EndPaint(hWnd, &ps);
			
			TR_END
			break;
		}
		case WM_TIMER:
		{
			TR_START

			KillTimer(hWnd, wParam);
			if (wParam == TIMER_SECTION_START)
			{
				TR_START
				// PROCESS NEW SECTION
				section = config->nextSection();
				if (section == NULL)
				{
					SetTimer(hWnd, TIMER_EXIT, 0, NULL);
					break;
				}
				switch (section->type)
				{
				case Config::esec::error:
					TR_START
					// Process [Error] section
					if (section->argc == 0)
					{
						SetTimer(hWnd, TIMER_SECTION_START, 0, NULL);
						break;
					}

					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[Error]"));
					for (unsigned long it = 0; it < section->argc; ++it)
					{
						if (section->args[it].type == Config::earg::print)
						{
							AddLogMsg(messages, section->args[it].szValue);
						}
					}
					AddLogMsg(messages, _T(""));

					setTimer(TIMER_EXIT, section->wait);
					TR_END
					break;
				case Config::esec::listwnd:
					TR_START
					// Process [Error] section
					listWnds();
					jumpToSection(TRUE);
					TR_END
					break;
				case Config::esec::listproc:
					TR_START
					// Process [ListProc] section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[ListProc]"));
		
					LPCTSTR proc = NULL;
					for (unsigned long it = 0; it < section->argc; ++it)
					{
						if (section->args[it].type == Config::earg::proc)
						{
							proc = getFilledValue(section->args[it]);
							break;
						}
					}
					showProcs(NULL, proc);
					jumpToSection(TRUE);
					TR_END
					break;
				case Config::esec::postwnd:
					TR_START
					// Process [PostWnd] section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[PostWnd]"));
					BOOL result = TRUE;
					
					g_PostMsg = 0;
					g_PostWParam = 0;
					g_PostLParam = 0;
					g_StartTime = GetTickCount();
					g_PostWndCount = 0;
					BOOL bUseSend = FALSE;
					for (unsigned long i = 0; i < section->argc; ++i)
					{
						if (section->args[i].type == Config::earg::wnd)
						{
							++g_PostWndCount;
						}
						else if (section->args[i].type == Config::earg::msg)
						{
							g_PostMsg = section->args[i].ulValue;
						}
						else if (section->args[i].type == Config::earg::wparam)
						{
							g_PostWParam = section->args[i].ulValue;
						}
						else if (section->args[i].type == Config::earg::lparam)
						{
							g_PostLParam = section->args[i].ulValue;
						}
						else if (section->args[i].type == Config::earg::usesend)
						{
							bUseSend = section->args[i].bValue;
						}
					}

					if (section->timeout > 0 && g_PostWndCount > 0)
					{
						g_PostWndList = (PostWndItem *)malloc(sizeof(PostWndItem) * g_PostWndCount);
						memset(g_PostWndList, 0, sizeof(PostWndItem) * g_PostWndCount);
					}
					
					AddLogMsg(messages, _T("0x%08x 0x%08x 0x%08x"), g_PostMsg, g_PostWParam, g_PostLParam);
					
					PPROCESSENTRY32 list = NULL;
					for (unsigned long j = 0, it = 0; j < section->argc; ++j)
					{
						if (section->args[j].type == Config::earg::wnd)
						{
							LPCTSTR lpClassName = getFilledValue(section->args[j]);
							LPCTSTR lpWindowName = getFilledValue2(section->args[j]);
							LPCTSTR lpProcName = getFilledValue3(section->args[j]);
							
							HWND hWnd = findWindow(&list, lpClassName, lpWindowName, lpProcName);
							if (hWnd == NULL)
							{
								result = FALSE;
								continue;
							}

							BOOL bPostRes = !bUseSend ? PostMessage(hWnd, g_PostMsg, g_PostWParam, g_PostLParam) : 0;
							LRESULT lSendRes = bUseSend ? SendMessage(hWnd, g_PostMsg, g_PostWParam, g_PostLParam) : 0;
													
							if (bPostRes || bUseSend)
							{
								if (section->timeout > 0)
								{
									if (lpClassName != NULL)
									{
										_tcsncpy(g_PostWndList[it].className, lpClassName, g_PostItemStrSize);
										g_PostWndList[it].className[g_PostItemStrSize - 1] = 0;
									}
									if (lpWindowName != NULL)
									{
										_tcsncpy(g_PostWndList[it].windowName, lpWindowName, g_PostItemStrSize);
										g_PostWndList[it].windowName[g_PostItemStrSize - 1] = 0;
									}
									if (lpProcName != NULL)
									{
										_tcsncpy(g_PostWndList[it].procName, lpProcName, g_PostItemStrSize);
										g_PostWndList[it].procName[g_PostItemStrSize - 1] = 0;
									}
									++it;
								}
								else if (bPostRes)
								{
									AddLogMsg(messages, _T("posted to '%s' '%s'"), lpClassName, lpWindowName);
								}

								if (bUseSend)
								{
									AddLogMsg(messages, _T("sent to '%s' '%s'; lresult: 0x%08x"), lpClassName, lpWindowName, lSendRes);
								}
							}
							else
							{
								AddLogMsg(messages, _T("PostMessage: 0x%08X '%s' '%s'"), GetLastError(), lpClassName, lpWindowName);
								result = FALSE;
							}
						}
					}

					if (list != NULL)
					{
						free(list);
					}

					if (jumpToSection(result, FALSE))
					{
						break;
					}

					if (section->timeout > 0)
					{
						setTimer(TIMER_KILLWND_IT, TIMEOUT);
					}
					else
					{
						setTimer(TIMER_SECTION_START, section->wait);
					}
					TR_END
					break;
				case Config::esec::repostwnd:
					TR_START
					// Process [RepostWnd] section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[RepostWnd]"));
					AddLogMsg(messages, _T("0x%08x 0x%08x 0x%08x"), g_LastMsg, g_LastWParam, g_LastLParam);
					BOOL result = TRUE;
					
					PPROCESSENTRY32 list = NULL;
					for (unsigned long j = 0, it = 0; j < section->argc; ++j)
					{
						if (section->args[j].type == Config::earg::wnd)
						{
							LPCTSTR lpClassName = getFilledValue(section->args[j]);
							LPCTSTR lpWindowName = getFilledValue2(section->args[j]);
							LPCTSTR lpProcName = getFilledValue3(section->args[j]);
							
							HWND hWnd = findWindow(&list, lpClassName, lpWindowName, lpProcName);
							if (hWnd == NULL)
							{
								result = FALSE;
								continue;
							}
													
							if (PostMessage(hWnd, g_LastMsg, g_LastWParam, g_LastLParam))
							{
								AddLogMsg(messages, _T("posted to '%s' '%s'"), lpClassName, lpWindowName);
							}
							else
							{
								AddLogMsg(messages, _T("PostMessage: 0x%08X '%s' '%s'"), GetLastError(), lpClassName, lpWindowName);
								result = FALSE;
							}
						}
					}

					if (list != NULL)
					{
						free(list);
					}

					jumpToSection(result);
					TR_END
					break;
				case Config::esec::findwnd:
					TR_START
					// Process [FindWnd] section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[FindWnd]"));
					BOOL result = TRUE;
					
					BOOL bCheck = FALSE;
					BOOL bCheckForeground = FALSE;
					for (unsigned long i = 0; i < section->argc; ++i)
					{
						if (section->args[i].type == Config::earg::check)
						{
							bCheck = TRUE;
							bCheckForeground = _tcsicmp(getFilledValue(section->args[i]), _T("foreground")) == 0;
						}
					}
					
					if (bCheck && !bCheckForeground)
					{
						AddLogMsg(messages, _T("error: invalid check"));
						AddLogMsg(messages, _T("use 'foreground' check"));
						result = FALSE;
					}
					else
					{
						PPROCESSENTRY32 list = NULL;
						for (unsigned long j = 0; j < section->argc; ++j)
						{
							if (section->args[j].type == Config::earg::wnd)
							{
								LPCTSTR lpClassName = getFilledValue(section->args[j]);
								LPCTSTR lpWindowName = getFilledValue2(section->args[j]);
								LPCTSTR lpProcName = getFilledValue3(section->args[j]);
							
								HWND hWnd = findWindow(&list, lpClassName, lpWindowName, lpProcName);
								if (hWnd == NULL)
								{
									result = FALSE;
									continue;
								}

								if (bCheckForeground && hWnd != GetForegroundWindow())
								{
									AddLogMsg(messages, _T("not foreground: '%s' '%s'"), lpClassName, lpWindowName);
									result = FALSE;
									continue;
								}

								AddLogMsg(messages, _T("found '%s' '%s'"), lpClassName, lpWindowName);
							}
						}
						if (list != NULL)
						{
							free(list);
						}
					}

					jumpToSection(result);
					TR_END
					break;
				case Config::esec::setwnd:
					TR_START
					// Process [SetWnd] section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[SetWnd]"));
					BOOL result = TRUE;

					LPCTSTR action = NULL;
					unsigned long flags = 0;
					
					for (unsigned long i = 0; i < section->argc; ++i)
					{
						if (section->args[i].type == Config::earg::action)
						{
							action = getFilledValue(section->args[i]);
						}
						else if (section->args[i].type == Config::earg::flags)
						{
							flags = section->args[i].ulValue;
						}
					}
					
					AddLogMsg(messages, _T("action: '%s'; flags 0x%08X"), action, flags);
					
					PPROCESSENTRY32 list = NULL;
					for (unsigned long j = 0; j < section->argc; ++j)
					{
						if (section->args[j].type == Config::earg::wnd)
						{
							LPCTSTR lpClassName = getFilledValue(section->args[j]);
							LPCTSTR lpWindowName = getFilledValue2(section->args[j]);
							LPCTSTR lpProcName = getFilledValue3(section->args[j]);
							
							HWND hWnd = findWindow(&list, lpClassName, lpWindowName, lpProcName);
							if (hWnd == NULL)
							{
								result = FALSE;
								continue;
							}
							
							if (_tcsicmp(action, _T("show")) == 0)
							{
								ShowWindow(hWnd, flags);
							}
							else if (_tcsicmp(action, _T("foreground")) == 0)
							{
								if (!SetForegroundWindow(hWnd))
								{
									AddLogMsg(messages, _T("SetForegroundWindow: 0x%08X '%s' '%s'"), GetLastError(), lpClassName, lpWindowName);
									result = FALSE;
									continue;
								}
							}
							else if (_tcsicmp(action, _T("show_ex")) == 0)
							{
								ShowWindow(hWnd, SW_SHOWNORMAL);
								if (!SetForegroundWindow(hWnd))
								{
									AddLogMsg(messages, _T("SetForegroundWindow: 0x%08X '%s' '%s'"), GetLastError(), lpClassName, lpWindowName);
									result = FALSE;
									continue;
								}
							}
							else
							{
								AddLogMsg(messages, _T("error: invalid action"));
								AddLogMsg(messages, _T("use 'show', 'foreground' or 'show_ex' actions"));
								result = FALSE;
								break;
							}

							AddLogMsg(messages, _T("set '%s' '%s'"), lpClassName, lpWindowName);
						}
					}
					if (list != NULL)
					{
						free(list);
					}

					jumpToSection(result);
					TR_END
					break;
				case Config::esec::killproc:
					TR_START
					// Process [KillProc] section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[KillProc]"));
					BOOL result = TRUE;

					PPROCESSENTRY32 list = NULL;
					showProcs(&list, NULL);

					for (unsigned long i = 0; i < section->argc; ++i)
					{
						if (section->args[i].type == Config::earg::proc)
						{
							LPCTSTR lpProc = getFilledValue(section->args[i]);

							PPROCESSENTRY32 proc = NULL;
							for (unsigned long j = 0; ; ++j)
							{
								if (list[j].szExeFile[0] == 0)
								{
									break;
								}
								
								if (_tcsicmp(list[j].szExeFile, lpProc) == 0)
								{
									proc = list + j;

									HANDLE hProcess = OpenProcess(0, FALSE, proc->th32ProcessID);
									if (hProcess == NULL)
									{
										AddLogMsg(messages, _T("OpenProcess 0x%08X '%s'"), GetLastError(), proc->szExeFile);
										result = FALSE;
									}
									else
									{
										if (TerminateProcess(hProcess, 0))
										{
											AddLogMsg(messages, _T("killed '%s'"), proc->szExeFile);
										}
										else
										{
											AddLogMsg(messages, _T("TerminateProcess 0x%08X '%s'"), GetLastError(), proc->szExeFile);
											result = FALSE;
										}
										CloseHandle(hProcess);
									}
								}

							}
							if (proc == NULL)
							{
								AddLogMsg(messages, _T("not found '%s'"), lpProc);
								result = FALSE;
							}
						}
					}
					free(list);
					jumpToSection(result);
					TR_END
					break;
				case Config::esec::startproc:
					TR_START
					// Process [StartProc] section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[StartProc]"));
					BOOL result = TRUE;

					for (unsigned long it = 0; it < section->argc; ++it)
					{
						if (section->args[it].type == Config::earg::path)
						{
							LPCTSTR path = getFilledValue(section->args[it]);
							LPCTSTR args = getFilledValue2(section->args[it]);
							if (args != NULL && _tcsicmp(_T("$args"), args) == 0)
							{
								args = g_CmdLine;
							}

							PROCESS_INFORMATION processInformation;
							if (!CreateProcess(path, *args == 0 ? NULL : args,
								NULL, NULL, FALSE, 0, NULL, NULL, NULL,
								&processInformation))
							{
								AddLogMsg(messages, _T("error 0x%08X '%s'"), GetLastError(), path);
								result = FALSE;
							}
							else
							{
								AddLogMsg(messages, _T("started '%s'; args '%s'"), path, args);
							}
						}
					}

					jumpToSection(result);
					TR_END
					break;
				case Config::esec::logmsg:
					TR_START
					// Process [LogMsg] section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[LogMsg]"));
					g_LogSection = section;
					jumpToSection(TRUE);
					TR_END
					break;
				case Config::esec::regmsg:
					TR_START
					// Process [RegMsg] section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[RegMsg]"));

					if (g_HandlerCount == g_HandlerMaxCount)
					{
						AddLogMsg(messages, _T("error: maximum message handlers is %d"), g_HandlerMaxCount);
					}
					else
					{
						MsgHandler h;
						h.msg = 0;
						h.wparam = 0xFFFFFFFF;
						h.lparam = 0xFFFFFFFF;
						h.label = NULL;
						for (unsigned long i = 0; i < section->argc; ++i)
						{
							if (section->args[i].type == Config::earg::msg)
							{
								h.msg = section->args[i].ulValue;
							}
							else if (section->args[i].type == Config::earg::wparam)
							{
								h.wparam = section->args[i].ulValue;
							}
							else if (section->args[i].type == Config::earg::lparam)
							{
								h.lparam = section->args[i].ulValue;
							}
							else if (section->args[i].type == Config::earg::handler)
							{
								h.label = getFilledValue(section->args[i]);
							}
						}
						if (h.msg == 0 || h.label == NULL)
						{
							AddLogMsg(messages, _T("error register handler: invalid args"));
						}
						else
						{
							g_Handlers[g_HandlerCount] = h;
							++g_HandlerCount;

							AddLogMsg(messages, _T("handler[%d]: '%s'"), g_HandlerCount, h.label);
							AddLogMsg(messages, _T("0x%08x 0x%08x 0x%08x"), h.msg, h.wparam, h.lparam);
						}
					}
					jumpToSection(TRUE);
					TR_END
					break;
				case Config::esec::waitmsg:
					TR_START
					// Process [WaitMsg] section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[WaitMsg]"));
					g_WaitMsg = TRUE;
					jumpToSection(TRUE);
					TR_END
					break;
				case Config::esec::exitapp:
					TR_START
					// Process [ExitApp] section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[Exit]"));
					setTimer(TIMER_EXIT, section->wait);
					TR_END
					break;

				case Config::esec::sstop:
					TR_START
					// Process [Stop] section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[Stop]"));
					section->stop = TRUE;
					jumpToSection(TRUE);
					TR_END
					break;

				case Config::esec::swait:
					TR_START
					// Process [Wait] section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[Wait]"));
					jumpToSection(TRUE);
					TR_END
					break;

				case Config::esec::include:
					TR_START
					// Process [Include] section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[Include]"));
					for (unsigned long it = 0; it < section->argc; ++it)
					{
						if (section->args[it].type == Config::earg::path)
						{
							AddLogMsg(messages, _T("path: '%s'"), section->args[it].szValue);
						}
					}
					jumpToSection(TRUE);
					TR_END
					break;

				case Config::esec::save:
					TR_START
					// Process [Save] section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[Save]"));

					LPCTSTR value = NULL;
					BOOL bFlush = FALSE;
					for (unsigned long i = 0; i < section->argc; ++i)
					{
						if (section->args[i].type == Config::earg::value)
						{
							value = getFilledValue(section->args[i]);
						}
						else if (section->args[i].type == Config::earg::flush)
						{
							bFlush = section->args[i].bValue;
						}
					}
					
					if (bFlush && config->settings().state_path == NULL)
					{
						AddLogMsg(messages, _T("variables state path is not set"));
					}
					else
					{
						for (unsigned long j = 0; j < section->argc; ++j)
						{
							if (section->args[j].type == Config::earg::name)
							{
								if (!bFlush && value == NULL)
								{
									AddLogMsg(messages, _T("value is empty"));
									continue;
								}

								LPCTSTR name = getFilledValue(section->args[j]);
								
								if (value != NULL)
								{
									setStateVariable(name, value);
								}

								if (bFlush)
								{
									saveStateVariable(name);
								}
							}
							else if (section->args[j].type == Config::earg::set)
							{
								LPCTSTR name = getFilledValue(section->args[j]);
								setStateVariable(name, getFilledValue2(section->args[j]));

								if (bFlush)
								{
									saveStateVariable(name);
								}
							}
						}
					}

					jumpToSection(TRUE);
					TR_END
					break;

				case Config::esec::input:
					TR_START
					// Process [Input] section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[Input]"));
						
					for (unsigned long i = 0; i < section->argc; ++i)
					{
						if (section->args[i].type == Config::earg::keydown)
						{
							AddLogMsg(messages, _T("keydown 0x%02x"), (unsigned char)section->args[i].ulValue);
							keybd_event((unsigned char)section->args[i].ulValue, 0, 0, 0);
						}
						else if (section->args[i].type == Config::earg::keyup)
						{
							AddLogMsg(messages, _T("keyup 0x%02x"), (unsigned char)section->args[i].ulValue);
							keybd_event((unsigned char)section->args[i].ulValue, 0, KEYEVENTF_KEYUP, 0);
						}
						else if (section->args[i].type == Config::earg::sleep)
						{
							if (section->args[i].ulValue > 1000)
							{
								AddLogMsg(messages, _T("sleep interval is too big, use different section"));
							}
							else
							{
								AddLogMsg(messages, _T("sleep for %u"), section->args[i].ulValue);
								Sleep(section->args[i].ulValue);
							}
						}
					}

					jumpToSection(TRUE);
					TR_END
					break;
					
				case Config::esec::time:
					TR_START
					// Process [Time] section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("[Time]"));

					long mshift = 0;
					LPCTSTR tz = NULL;
					for (unsigned long i = 0; i < section->argc; ++i)
					{
						if (section->args[i].type == Config::earg::mshift)
						{
							mshift = section->args[i].ulValue;
						}
						else if (section->args[i].type == Config::earg::tz)
						{
							tz = getFilledValue(section->args[i]);
						}
					}

					if (mshift != 0)
					{
						TR_START

						SYSTEMTIME st;
						FILETIME ft;

						GetLocalTime(&st);

						if (!SystemTimeToFileTime(&st, &ft))
						{
							AddLogMsg(messages, _T("SystemTimeToFileTime: 0x%08X"), GetLastError());
						}
						else
						{
							ULONGLONG ull = (((ULONGLONG)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
							ull += mshift * (ULONGLONG)10000000 * 60;
							ft.dwLowDateTime = (DWORD)(ull & 0xFFFFFFFF);
							ft.dwHighDateTime = (DWORD)(ull >> 32);
							
							if (!FileTimeToSystemTime(&ft, &st))
							{
								AddLogMsg(messages, _T("FileTimeToSystemTime: 0x%08X"), GetLastError());
							}
							else if (!SetLocalTime(&st))
							{
								AddLogMsg(messages, _T("SetLocalTime: 0x%08X"), GetLastError());
							}
							else
							{
								AddLogMsg(messages, _T("time shifted by %d minutes"), mshift);
							}
						}

						TR_END
					}

					LONG ret;
					if (tz != NULL)
					{
						TR_START

						TIME_ZONE_INFORMATION tzi;
						if ((ret = GetTimeZoneInformationByName(&tzi, tz)) != 0)
						{
							AddLogMsg(messages, _T("GetTimeZoneInformationByName: ret %d; err 0x%08X"), ret, GetLastError());
						}
						else if (!SetTimeZoneInformation(&tzi))
						{
							AddLogMsg(messages, _T("SetTimeZoneInformation: 0x%08X"), GetLastError());
						}
						else
						{
							AddLogMsg(messages, _T("timezone '%s' set"), tz);
						}

						TR_END
					}

					jumpToSection(TRUE);
					TR_END
					break;

				default:
					TR_START
					// Process unknown section
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T(""));
					AddLogMsg(messages, _T("Unknown section... Exiting..."));
					SetTimer(hWnd, TIMER_EXIT, 1000, NULL);
					TR_END
					break;
				}
				TR_END
			}
			else if (wParam == TIMER_KILLWND_IT)
			{
				TR_START
				// Step 2 of [PostWnd]
				BOOL result = TRUE;

				if (section == NULL)
				{
					AddLogMsg(messages, _T("Step 2 of [PostWnd] error: section is NULL..."));
					break;
				}
				BOOL timeout = section->timeout > 0 && GetTickCount() - g_StartTime > section->timeout;
				
				unsigned int closed = 0;
				PPROCESSENTRY32 list = NULL;
				for (unsigned long it = 0; it < g_PostWndCount; ++it)
				{
					if (g_PostWndList[it].closed)
					{
						++closed;
						continue;
					}

					if (*g_PostWndList[it].className == 0 && *g_PostWndList[it].windowName == 0)
					{
						++closed;
						continue;
					}

					HWND hWnd = findWindow(&list, g_PostWndList[it].className, g_PostWndList[it].windowName, g_PostWndList[it].procName, FALSE);
					if (hWnd == NULL)
					{
						if (!g_PostWndList[it].wndclosed)
						{
							AddLogMsg(messages, _T("closed '%s' '%s'"), g_PostWndList[it].className, g_PostWndList[it].windowName);
							g_PostWndList[it].wndclosed = TRUE;
						}

						if (g_PostWndList[it].procName[0] != 0 && list != NULL)
						{
							BOOL terminated = TRUE;
							for (unsigned long it2 = 0; ; ++it2)
							{
								if (list[it2].szExeFile[0] == 0)
								{
									break;
								}
								if (_tcsicmp(list[it2].szExeFile, g_PostWndList[it].procName) == 0)
								{
									terminated = FALSE;
									break;
								}
							}
							if (terminated)
							{
								AddLogMsg(messages, _T("terminated '%s'"), g_PostWndList[it].procName);
								g_PostWndList[it].closed = TRUE;
								++closed;
								continue;
							}
						}
						else
						{
							g_PostWndList[it].closed = TRUE;
							++closed;
							continue;
						}
					}
					
					if (timeout)
					{
						AddLogMsg(messages, _T("timeout '%s' '%s' '%s'"), g_PostWndList[it].className, g_PostWndList[it].windowName, g_PostWndList[it].procName);
					}
				}

				if (list != NULL)
				{
					free(list);
				}

				if ((closed == g_PostWndCount) || timeout)
				{
					free(g_PostWndList);
					g_PostWndList = NULL;
					g_PostWndCount = 0;
					if (timeout)
						result = FALSE;
					jumpToSection(result);
				}
				else
				{
					setTimer(TIMER_KILLWND_IT, TIMEOUT);
				}
				TR_END
			}
			else if (wParam == TIMER_EXIT)
			{
				TR_START
				KillTimers();
				PostMessage(hWnd, WM_CLOSE, 0, 0);
				TR_END
			}

			TR_END
			break;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
		{
			if (g_WaitMsg)
			{
				TR_START
				// Handle registered mesasges
				BOOL found = false;
				for (unsigned long it = 0; it < g_HandlerCount; ++it)
				{
					if (g_Handlers[it].msg == message &&
						(g_Handlers[it].wparam == 0xFFFFFFFF || g_Handlers[it].wparam == wParam) &&
						(g_Handlers[it].lparam == 0xFFFFFFFF || g_Handlers[it].lparam == (unsigned long)lParam))
					{
						AddLogMsg(messages, _T(""));
						AddLogMsg(messages, _T("[WaitMsg]"));
						AddLogMsg(messages, _T("got: 0x%08x 0x%08x 0x%08x"), message, wParam, lParam);

						if(config->seekToSection(g_Handlers[it].label))
						{
							g_LastMsg = message;
							g_LastWParam = wParam;
							g_LastLParam = lParam;
							AddLogMsg(messages, _T("jump to '%s'"), g_Handlers[it].label);
							section = NULL;
							KillTimer(hWnd, TIMER_SECTION_START);
							KillTimer(hWnd, TIMER_KILLWND_IT);
							setTimer(TIMER_SECTION_START, 0);
						}
						else
						{
							AddLogMsg(messages, _T("jump error to '%s'"), g_Handlers[it].label);
							invalidate();
						}
						found = TRUE;
						break;
					}
				}
				if (found)
				{
					break;
				}
				TR_END
			}
			if (message == WM_WINDOWPOSCHANGED && (((LPWINDOWPOS)lParam)->flags & SWP_SHOWWINDOW))
			{
				g_Show = TRUE;
			}

			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
		
	if (g_immediateJumpId != 0 && !g_inImmediateLoop)
	{
		g_inImmediateLoop = TRUE;
		while (g_immediateJumpId != 0)
		{
			UINT id = g_immediateJumpId;
			g_immediateJumpId = 0;
			WndProc(hWnd, WM_TIMER, id, NULL);
		}
		g_inImmediateLoop = FALSE;
	}

   return 0;

   TR_END0
}