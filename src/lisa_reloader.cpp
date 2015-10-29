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

#define TIMEOUT				100

// Global Variables:
HINSTANCE			hInst;			// The current instance
HWND				hWnd;			// Main wnd
HBITMAP				hSplash = NULL;
BOOL				g_Show = TRUE;

TCHAR g_CmdLine[1024];

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
struct PostWndItem
{
	TCHAR className[64];
	TCHAR windowName[64];
	BOOL closed;
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
	TCHAR * label;
};
unsigned long		g_HandlerCount = 0;
const unsigned long	g_HandlerMaxCount = 100;
MsgHandler			g_Handlers[g_HandlerMaxCount];

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

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass	(HINSTANCE, LPTSTR);
BOOL				InitInstance	(HINSTANCE, int);
LRESULT CALLBACK	WndProc			(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About			(HWND, UINT, WPARAM, LPARAM);

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

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
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

	TCHAR szTitle[MAX_LOADSTRING];
	TCHAR szWindowClass[MAX_LOADSTRING];

	hInst = hInstance;
	if (config->settings().wclass != NULL)
	{
		_tcsncpy(szWindowClass, config->settings().wclass, MAX_LOADSTRING);
		szWindowClass[MAX_LOADSTRING - 1] = 0;
	}
	else
	{
		LoadString(hInstance, IDS_APP_CLASS, szWindowClass, MAX_LOADSTRING);
	}

	MyRegisterClass(hInstance, szWindowClass);

	if (config->settings().wtitle != NULL)
	{
		_tcsncpy(szTitle, config->settings().wtitle, MAX_LOADSTRING);
		szTitle[MAX_LOADSTRING - 1] = 0;
	}
	else
	{
		LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	}

	// Avoids previous instances
	hWnd = FindWindow(szWindowClass, szTitle);
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

	hWnd = CreateWindow(szWindowClass, szTitle, nCmdShow != SW_HIDE ? WS_VISIBLE : 0,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	if (nCmdShow != SW_HIDE )
	{
		ShowWindow(hWnd, nCmdShow);
	}
	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0,
		config->settings().width, config->settings().height,
		!g_Show ? SWP_NOACTIVATE | SWP_NOZORDER : SWP_SHOWWINDOW);
	UpdateWindow(hWnd);

	return TRUE;

	TR_END0
}

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
			messages.AddLast(_T("last section in chain; stopped"));
		}
		else if (timeout > 0)
		{
			TCHAR buf[64];
			_sntprintf(buf, sizeof(buf), _T("waiting for %d msecs"), timeout);
			messages.AddLast(buf);
		}
	}

	invalidate();

	if (id == TIMER_KILLWND_IT || section == NULL || !section->stop)
	{
		SetTimer(hWnd, id, timeout, NULL);
	}

	TR_END
}

BOOL jumpToSection(BOOL success, BOOL gonext = TRUE)
{
	TR_START
	
	static TCHAR buf[256];
	TCHAR * handler = NULL;

	if (success)
	{
		for (unsigned long i = 0; i < section->argc; ++i)
		{
			if (section->args[i].type == Config::earg::onsuccess)
			{
				handler = section->args[i].szValue;
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
				handler = section->args[i].szValue;
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
				handler = section->args[i].szValue;
				break;
			}
		}
	}
	
	if (handler != NULL)
	{
		if(config->seekToSection(handler))
		{
			_sntprintf(buf, sizeof(buf), _T("jump to '%s'"), handler);
			messages.AddLast(buf);
			unsigned long wait = section->wait;
			section = NULL;
			setTimer(TIMER_SECTION_START, wait);
			return TRUE;
		}
		else
		{
			_sntprintf(buf, sizeof(buf), _T("jump error to '%s'"), handler);
			messages.AddLast(buf);
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
LPTSTR getStateVariable(LPTSTR name, LPTSTR defval)
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
		TCHAR buf[64];

		if (g_VariableCount == g_VariableMaxCount)
		{
			_sntprintf(buf, sizeof(buf), _T("error: maximum state variables count is %d"), g_VariableMaxCount);
			messages.AddLast(buf);
			break;
		}
		
		if (config->settings().state_path == NULL)
		{
			messages.AddLast(_T("state path is not set"));
			break;
		}

		TCHAR path[MAX_PATH + 1];
		
		if (_tcslen(config->settings().state_path) + _tcslen(name) + 6 > MAX_PATH)
		{
			messages.AddLast(_T("state variable name is too big"));
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
				_sntprintf(buf, sizeof(buf), _T("ftell error: 0x%08X"), GetLastError());
				messages.AddLast(buf);
				messages.AddLast(path);
			}
			fseek(f, 0, SEEK_SET);
			
			g_Variables[g_VariableCount].value = (TCHAR *)malloc(len + sizeof(TCHAR));
			g_Variables[g_VariableCount].size = len + sizeof(TCHAR);

			if (fread(g_Variables[g_VariableCount].value, sizeof(TCHAR), len / sizeof(TCHAR), f) != (size_t)len / sizeof(TCHAR))
			{
				_sntprintf(buf, sizeof(buf), _T("fread error: 0x%08X"), GetLastError());
				messages.AddLast(buf);
				messages.AddLast(path);

				free(g_Variables[g_VariableCount].value);
			}
			else
			{				
				g_Variables[g_VariableCount].value[len / sizeof(TCHAR)] = 0;

				_sntprintf(buf, sizeof(buf), _T("loaded '%s': '%s'"), name, g_Variables[g_VariableCount].value);
				messages.AddLast(buf);

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
			_sntprintf(buf, sizeof(buf), _T("_tfopen error: 0x%08X"), GetLastError());
			messages.AddLast(buf);
			messages.AddLast(path);
		}

		break;
	}

	return defval;

	TR_END0
}

void setStateVariable(LPTSTR name, LPTSTR value)
{
	TR_START
	
	TCHAR buf[64];

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
			
			_sntprintf(buf, sizeof(buf), _T("set '%s': %s'"), name, value);
			messages.AddLast(buf);
			return;
		}
	}

	if (g_VariableCount == g_VariableMaxCount)
	{
		_sntprintf(buf, sizeof(buf), _T("error: maximum state variables count is %d"), g_VariableMaxCount);
		messages.AddLast(buf);
		return;
	}
	
	g_Variables[g_VariableCount].value = (TCHAR *)malloc(size);
	g_Variables[g_VariableCount].size = size;
	memcpy(g_Variables[g_VariableCount].value, value, size);
	
	size = (_tcslen(name) + 1) * sizeof(TCHAR);
	g_Variables[g_VariableCount].name = (TCHAR *)malloc(size);
	memcpy(g_Variables[g_VariableCount].name, name, size);
	
	++g_VariableCount;
	
	_sntprintf(buf, sizeof(buf), _T("set '%s': %s'"), name, value);
	messages.AddLast(buf);

	TR_END
}

void saveStateVariable(LPTSTR name)
{
	TR_START
	
	TCHAR buf[64];
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
		_sntprintf(buf, sizeof(buf), _T("error: state variable '%s' not found"), name);
		messages.AddLast(buf);
		return;
	}	

	while (1)
	{
		TCHAR path[MAX_PATH + 1];
		
		if (value == NULL)
		{
			messages.AddLast(_T("variable value is empty"));
			break;
		}
		if (config->settings().state_path == NULL)
		{
			messages.AddLast(_T("state path is not set"));
			break;
		}
		
		if (_tcslen(config->settings().state_path) + _tcslen(name) + 6 > MAX_PATH)
		{
			messages.AddLast(_T("state variable name is too big"));
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
				_sntprintf(buf, sizeof(buf), _T("fwrite error: 0x%08X"), GetLastError());
				messages.AddLast(buf);
				messages.AddLast(path);
			}
			else
			{
				_sntprintf(buf, sizeof(buf), _T("saved '%s': %s'"), name, value);
				messages.AddLast(buf);
			}
			fclose(f);
		}
		else
		{
			_sntprintf(buf, sizeof(buf), _T("_tfopen error: 0x%08X"), GetLastError());
			messages.AddLast(buf);
			messages.AddLast(path);
		}

		break;
	}

	TR_END
}

void fillStringVariables(LPTSTR buf, size_t count, LPCTSTR str)
{
	TR_START

	if (_tcsstr(str, _T("${")) == NULL)
	{
		_tcsncpy(buf, str, count);
		buf[count - 1] = 0;
		return;
	}

	size_t size = (_tcslen(str) + 1) * sizeof(TCHAR);
	TCHAR * tmp = (TCHAR *)malloc(size);
	memcpy(tmp, str, size);
	*buf = 0;

	TCHAR * pos1, * pos2, * pos3;
	for (pos1 = tmp, pos2 = _tcsstr(tmp, _T("${"));
		pos2 != NULL && _tcslen(pos2) > 4; )
	{
		if ((pos3 = _tcsstr(pos2, _T(":"))) == NULL)
		{
			pos2 = _tcsstr(pos2 + 2, _T("${"));
			continue;
		}
		if ((pos3 = _tcsstr(pos3, _T("}"))) == NULL)
		{
			pos2 = _tcsstr(pos2 + 2, _T("${"));
			continue;
		}

		*pos2 = 0;
		_tcsncat(buf, pos1, count - _tcslen(buf) - 1);

		if (count - _tcslen(buf) - 1 <= 0)
		{
			break;
		}

		pos1 = pos2 + 2;
		pos2 = _tcsstr(pos1, _T(":"));
		*pos2 = 0;
		++pos2;
		pos3 = _tcsstr(pos2, _T("}"));
		*pos3 = 0;

		_tcsncat(buf, getStateVariable(pos1, pos2), count - _tcslen(buf) - 1);
		
		if (count - _tcslen(buf) - 1 <= 0)
		{
			break;
		}

		pos1 = pos3 + 1;
		pos2 = _tcsstr(pos1, _T("${"));
	}
		
	if (count - _tcslen(buf) - 1 > 0 && _tcslen(pos1) > 0)
	{
		_tcsncat(buf, pos1, count - _tcslen(buf));
	}

	free(tmp);

	{
		TCHAR msg[64];
		_sntprintf(msg, sizeof(msg), _T("string: %s"), buf);
		messages.AddLast(msg);;
	}

	TR_END
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void listWnds()
{
	TR_START

	messages.AddLast(_T(""));
	messages.AddLast(_T("[ListWnd]"));

	HWND nextWindow = GetForegroundWindow();
	TCHAR lpWindowTitle[64];
	TCHAR lpClassName[64];
	TCHAR buf[134];

	while(nextWindow)
	{
		GetWindowText(nextWindow, lpWindowTitle, 64);
		GetClassName(nextWindow, lpClassName, 64);

		_sntprintf(buf, sizeof(buf), _T("'%s' '%s'"), lpClassName, lpWindowTitle);
		messages.AddLast(buf);
		
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
	
	TCHAR buf[256];
	*buf = 0;
	TCHAR lpWindowTitle[64];
	TCHAR lpClassName[64];

	while (1)
	{
		if (list == NULL)
		{
			messages.AddLast(_T(""));
			messages.AddLast(_T("[ListProc]"));
		
			if (proc != NULL)
			{
				memset(g_HWND, 0, sizeof(g_HWND));
				memset(g_Pid, 0, sizeof(g_Pid));
				g_WndIt = 0;
				if (!EnumWindows(EnumWindowsProcMy, NULL))
				{
					_sntprintf(buf, sizeof(buf), _T("EnumWindows: 0x%08X"), GetLastError());
					break;
				}
			}
		}

		HANDLE hProcessSnap;
		PROCESSENTRY32 pe32;
		
		hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if(hProcessSnap == INVALID_HANDLE_VALUE)
		{
			_sntprintf(buf, sizeof(buf), _T("CreateToolhelp32Snapshot: 0x%08X"), GetLastError());
			break;
		}
		
		pe32.dwSize = sizeof(PROCESSENTRY32);
		
		if(!Process32First(hProcessSnap, &pe32))
		{
			_sntprintf(buf, sizeof(buf), _T("Process32First: 0x%08X"), GetLastError());
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
				_sntprintf(buf, sizeof(buf), _T("Process32First: 0x%08X"), GetLastError());
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
				_sntprintf(buf, sizeof(buf), _T("0x%08X '%s'"), pe32.th32ProcessID, pe32.szExeFile);
				messages.AddLast(buf);

				if (proc != NULL)
				{
					for (DWORD it = 0; it < g_WndIt; ++it)
					{
						if (pe32.th32ProcessID == g_Pid[it])
						{
							if(GetClassName(g_HWND[it], lpClassName, 64) == 0)
							{
								_sntprintf(buf, sizeof(buf), _T("    GetClassName: 0x%08X"), GetLastError());
								messages.AddLast(buf);
							}
							if((GetWindowText(g_HWND[it], lpWindowTitle, 64) == 0) && GetLastError() != 0)
							{
								_sntprintf(buf, sizeof(buf), _T("    GetWindowText: 0x%08X"), GetLastError());
								messages.AddLast(buf);
							}

							_sntprintf(buf, sizeof(buf), _T("    0x%08X '%s' '%s'"), g_HWND[it], lpClassName, lpWindowTitle);
							messages.AddLast(buf);
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

		*buf = 0;

		if (GetLastError() != ERROR_NO_MORE_FILES)
		{
			_sntprintf(buf, sizeof(buf), _T("Process32Next: 0x%08X"), GetLastError());
		}
		
		CloseToolhelp32Snapshot( hProcessSnap );
		break;
	}

	if (*buf != 0)
	{
		messages.AddLast(buf);
	}

	return;

	TR_END
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
	
	static TCHAR buf[256];
	
	if (g_LogSection != NULL)
	{
		TR_START

		// Process [LogMsg] section
		if (g_LogSection->type != Config::esec::logmsg)
		{
			messages.AddLast(_T("logmsg error: incorrect section"));
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
					_sntprintf(buf, sizeof(buf), _T("[msg] 0x%08x WM_WINDOWPOSCHANGED"), message, wParam, lParam);
					messages.AddLast(buf);
					_sntprintf(buf, sizeof(buf), _T("    ins: 0x%x; x: %d; y: %d; cx: %d; cy: %d; flags: 0x%08x"),
						lpwp->hwndInsertAfter, lpwp->x, lpwp->y, lpwp->cx, lpwp->cy, lpwp->flags);
				}
				break;
			default:
				_sntprintf(buf, sizeof(buf), _T("[msg] 0x%08x 0x%08x 0x%08x"), message, wParam, lParam);
				break;
			}
			messages.AddLast(buf);
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
			
			_sntprintf(buf, sizeof(buf), _T("cmd: '%s'"), g_CmdLine);
			messages.AddLast(buf);

			if (config->settings().splash != NULL)
			{
				hSplash = (HBITMAP)SHLoadDIBitmap(config->settings().splash);
				if (hSplash == NULL)
				{
					_sntprintf(buf, sizeof(buf), _T("LoadImage: 0x%08X '%s'"), GetLastError(), config->settings().splash);
					messages.AddLast(buf);
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
			
				_sntprintf(buf, sizeof(buf), _T("page [%d/%d]"), g_Page + 1, g_Pages);
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

					messages.AddLast(_T(""));
					messages.AddLast(_T("[Error]"));
					for (unsigned long it = 0; it < section->argc; ++it)
					{
						if (section->args[it].type == Config::earg::print)
						{
							messages.AddLast(section->args[it].szValue);
						}
					}
					messages.AddLast(_T(""));

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
					TCHAR * proc = NULL;
					for (unsigned long it = 0; it < section->argc; ++it)
					{
						if (section->args[it].type == Config::earg::proc)
						{
							proc = section->args[it].szValue;
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
					messages.AddLast(_T(""));
					messages.AddLast(_T("[PostWnd]"));
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

					if (section->timeout > 0)
					{
						g_PostWndList = (PostWndItem *)malloc(sizeof(PostWndItem) * g_PostWndCount);
						memset(g_PostWndList, 0, sizeof(PostWndItem) * g_PostWndCount);
					}
					
					_sntprintf(buf, sizeof(buf), _T("0x%08x 0x%08x 0x%08x"), g_PostMsg, g_PostWParam, g_PostLParam);
					messages.AddLast(buf);
					
					PPROCESSENTRY32 list = NULL;
					for (unsigned long j = 0, it = 0; j < section->argc; ++j)
					{
						if (section->args[j].type == Config::earg::wnd)
						{
							LPTSTR lpClassName = (section->args[j].szValue == NULL || *section->args[j].szValue == 0) ? NULL : section->args[j].szValue;
							LPTSTR lpWindowName = (section->args[j].szValue2 == NULL || *section->args[j].szValue2 == 0) ? NULL : section->args[j].szValue2;
							LPTSTR lpProcName = (section->args[j].szValue3 == NULL || *section->args[j].szValue3 == 0) ? NULL : section->args[j].szValue3;
						
							if (lpClassName == NULL && lpWindowName == NULL)
							{
								messages.AddLast(_T("wClass and wTitle are empty"));
								continue;
							}
							
							HWND hWnd = NULL;
							if (lpProcName != NULL)
							{
								if (list == NULL)
								{
									memset(g_HWND, 0, sizeof(g_HWND));
									memset(g_Pid, 0, sizeof(g_Pid));
									g_WndIt = 0;
									if (!EnumWindows(EnumWindowsProcMy, NULL))
									{
										_sntprintf(buf, sizeof(buf), _T("EnumWindows: 0x%08X"), GetLastError());
										break;
									}
									showProcs(&list, NULL);
								}
						
								TCHAR lpEnumWindowName[64];
								TCHAR lpEnumClassName[64];
								for (DWORD it1 = 0; it1 < g_WndIt; ++it1)
								{
									if(GetClassName(g_HWND[it1], lpEnumClassName, 64) == 0)
									{
										_sntprintf(buf, sizeof(buf), _T("GetClassName: 0x%08X"), GetLastError());
										messages.AddLast(buf);
									}
									if((GetWindowText(g_HWND[it1], lpEnumWindowName, 64) == 0) && GetLastError() != 0)
									{
										_sntprintf(buf, sizeof(buf), _T("GetWindowText: 0x%08X"), GetLastError());
										messages.AddLast(buf);
									}
									if ((lpWindowName != NULL && _tcsicmp(lpEnumWindowName, lpWindowName) != 0) ||
										(lpClassName != NULL && _tcsicmp(lpEnumClassName, lpClassName) != 0))
									{
										continue;
									}
									
									for (unsigned long it2 = 0; ; ++it2)
									{
										if (list[it2].szExeFile[0] == 0)
										{
											break;
										}
										
										if (_tcsicmp(list[it2].szExeFile, lpProcName) == 0)
										{
											if (list[it2].th32ProcessID == g_Pid[it1])
											{
												hWnd = g_HWND[it];
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

							if (hWnd == NULL)
							{
								if (lpProcName = NULL)
								{
									_sntprintf(buf, sizeof(buf), _T("FindWindow: 0x%08X '%s' '%s'"), GetLastError(), section->args[j].szValue, section->args[j].szValue2);
								}
								else
								{
									_sntprintf(buf, sizeof(buf), _T("not found '%s' '%s' '%s'"), section->args[j].szValue, section->args[j].szValue2, section->args[j].szValue3);
								}
								messages.AddLast(buf);
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
										_tcsncpy(g_PostWndList[it].className, lpClassName, sizeof(((PostWndItem *)0)->className) / sizeof(TCHAR));
										g_PostWndList[it].className[sizeof(((PostWndItem *)0)->className) - 1] = 0;
									}
									if (lpWindowName != NULL)
									{
										_tcsncpy(g_PostWndList[it].windowName, lpWindowName, sizeof(((PostWndItem *)0)->windowName) / sizeof(TCHAR));
										g_PostWndList[it].className[sizeof(((PostWndItem *)0)->windowName) - 1] = 0;
									}
								}
								else if (bPostRes)
								{
									_sntprintf(buf, sizeof(buf), _T("posted to '%s' '%s'"), section->args[j].szValue, section->args[j].szValue2);
									messages.AddLast(buf);
								}

								if (bUseSend)
								{
									_sntprintf(buf, sizeof(buf), _T("sent to '%s' '%s'; lresult: 0x%08x"), section->args[j].szValue, section->args[j].szValue2, lSendRes);
									messages.AddLast(buf);
								}
							}
							else
							{
								_sntprintf(buf, sizeof(buf), _T("PostMessage: 0x%08X '%s' '%s'"), GetLastError(), section->args[j].szValue, section->args[j].szValue2);
								messages.AddLast(buf);
								result = FALSE;
							}
						}
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
				case Config::esec::findwnd:
					TR_START
					// Process [FindWnd] section
					messages.AddLast(_T(""));
					messages.AddLast(_T("[FindWnd]"));
					BOOL result = TRUE;
					
					BOOL bCheck = FALSE;
					BOOL bCheckForeground = FALSE;
					for (unsigned long i = 0; i < section->argc; ++i)
					{
						if (section->args[i].type == Config::earg::check)
						{
							bCheck = TRUE;
							bCheckForeground = _tcsicmp(section->args[i].szValue, _T("foreground")) == 0;
						}
					}
					
					if (bCheck && !bCheckForeground)
					{
						messages.AddLast(_T("error: invalid check"));
						messages.AddLast(_T("use 'foreground' check"));
					}
					else
					{
						for (unsigned long j = 0; j < section->argc; ++j)
						{
							if (section->args[j].type == Config::earg::wnd)
							{
								LPTSTR lpClassName = (section->args[j].szValue == NULL || *section->args[j].szValue == 0) ? NULL : section->args[j].szValue;
								LPTSTR lpWindowName = (section->args[j].szValue2 == NULL || *section->args[j].szValue2 == 0) ? NULL : section->args[j].szValue2;
								if (lpClassName == NULL && lpWindowName == NULL)
								{
									messages.AddLast(_T("wClass and wTitle are empty"));
									continue;
								}
							
								HWND hWnd = FindWindow(lpClassName, lpWindowName);
								if (hWnd == NULL)
								{
									_sntprintf(buf, sizeof(buf), _T("FindWindow: 0x%08X '%s' '%s'"), GetLastError(), section->args[j].szValue, section->args[j].szValue2);
									messages.AddLast(buf);
									result = FALSE;
									continue;
								}

								if (bCheckForeground && hWnd != GetForegroundWindow())
								{
									_sntprintf(buf, sizeof(buf), _T("not foreground: '%s' '%s'"), section->args[j].szValue, section->args[j].szValue2);
									messages.AddLast(buf);
									result = FALSE;
									continue;
								}

								_sntprintf(buf, sizeof(buf), _T("found '%s' '%s'"), section->args[j].szValue, section->args[j].szValue2);
								messages.AddLast(buf);
							}
						}
					}

					jumpToSection(result);
					TR_END
					break;
				case Config::esec::setwnd:
					TR_START
					// Process [SetWnd] section
					messages.AddLast(_T(""));
					messages.AddLast(_T("[SetWnd]"));
					BOOL result = TRUE;

					TCHAR * action = NULL;
					unsigned long flags = 0;
					
					for (unsigned long i = 0; i < section->argc; ++i)
					{
						if (section->args[i].type == Config::earg::action)
						{
							action = section->args[i].szValue;
						}
						else if (section->args[i].type == Config::earg::flags)
						{
							flags = section->args[i].ulValue;
						}
					}
					
					_sntprintf(buf, sizeof(buf), _T("action: '%s'; flags 0x%08X"), action, flags);
					messages.AddLast(buf);
					
					for (unsigned long j = 0; j < section->argc; ++j)
					{
						if (section->args[j].type == Config::earg::wnd)
						{
							LPTSTR lpClassName = (section->args[j].szValue == NULL || *section->args[j].szValue == 0) ? NULL : section->args[j].szValue;
							LPTSTR lpWindowName = (section->args[j].szValue2 == NULL || *section->args[j].szValue2 == 0) ? NULL : section->args[j].szValue2;
							if (lpClassName == NULL && lpWindowName == NULL)
							{
								messages.AddLast(_T("wClass and wTitle are empty"));
								continue;
							}
							HWND hWnd = FindWindow(lpClassName, lpWindowName);
							if (hWnd == NULL)
							{
								_sntprintf(buf, sizeof(buf), _T("FindWindow: 0x%08X '%s' '%s'"), GetLastError(), section->args[j].szValue, section->args[j].szValue2);
								messages.AddLast(buf);
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
									_sntprintf(buf, sizeof(buf), _T("SetForegroundWindow: 0x%08X '%s' '%s'"), GetLastError(), section->args[j].szValue, section->args[j].szValue2);
									messages.AddLast(buf);
									result = FALSE;
									continue;
								}
							}
							else if (_tcsicmp(action, _T("show_ex")) == 0)
							{
								ShowWindow(hWnd, SW_SHOWNORMAL);
								if (!SetForegroundWindow(hWnd))
								{
									_sntprintf(buf, sizeof(buf), _T("SetForegroundWindow: 0x%08X '%s' '%s'"), GetLastError(), section->args[j].szValue, section->args[j].szValue2);
									messages.AddLast(buf);
									result = FALSE;
									continue;
								}
							}
							else
							{
								messages.AddLast(_T("error: invalid action"));
								messages.AddLast(_T("use 'show', 'foreground' or 'show_ex' actions"));
								break;
							}

							_sntprintf(buf, sizeof(buf), _T("set '%s' '%s'"), section->args[j].szValue, section->args[j].szValue2);
							messages.AddLast(buf);
						}
					}

					jumpToSection(result);
					TR_END
					break;
				case Config::esec::killproc:
					TR_START
					// Process [KillProc] section
					messages.AddLast(_T(""));
					messages.AddLast(_T("[KillProc]"));
					BOOL result = TRUE;

					PPROCESSENTRY32 list = NULL;
					showProcs(&list, NULL);

					for (unsigned long i = 0; i < section->argc; ++i)
					{
						if (section->args[i].type == Config::earg::proc)
						{
							PPROCESSENTRY32 proc = NULL;
							for (unsigned long j = 0; ; ++j)
							{
								if (list[j].szExeFile[0] == 0)
								{
									break;
								}
								
								if (_tcsicmp(list[j].szExeFile, section->args[i].szValue) == 0)
								{
									proc = list + j;

									HANDLE hProcess = OpenProcess(0, FALSE, proc->th32ProcessID);
									if (hProcess == NULL)
									{
										_sntprintf(buf, sizeof(buf), _T("OpenProcess 0x%08X '%s'"), GetLastError(), proc->szExeFile);
										result = FALSE;
									}
									else
									{
										if (TerminateProcess(hProcess, 0))
										{
											_sntprintf(buf, sizeof(buf), _T("killed '%s'"), proc->szExeFile);
										}
										else
										{
											_sntprintf(buf, sizeof(buf), _T("TerminateProcess 0x%08X '%s'"), GetLastError(), proc->szExeFile);
											result = FALSE;
										}
										CloseHandle(hProcess);
									}
									messages.AddLast(buf);
								}

							}
							if (proc == NULL)
							{
								_sntprintf(buf, sizeof(buf), _T("not found '%s'"), section->args[i].szValue);
								messages.AddLast(buf);
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
					messages.AddLast(_T(""));
					messages.AddLast(_T("[StartProc]"));
					BOOL result = TRUE;

					for (unsigned long it = 0; it < section->argc; ++it)
					{
						if (section->args[it].type == Config::earg::path)
						{
							TCHAR path[MAX_PATH];
							fillStringVariables(path, MAX_PATH, section->args[it].szValue);

							LPTSTR args = (section->args[it].szValue2 == NULL || *section->args[it].szValue2 == 0) ? NULL : section->args[it].szValue2;
							if (args != NULL && _tcsicmp(_T("$args"), args) == 0)
							{
								args = g_CmdLine;
							}

							PROCESS_INFORMATION processInformation;
							if (!CreateProcess(path, args,
								NULL, NULL, FALSE, 0, NULL, NULL, NULL,
								&processInformation ))
							{
								_sntprintf(buf, sizeof(buf), _T("error 0x%08X '%s'"), GetLastError(), path);
								result = FALSE;
							}
							else
							{
								_sntprintf(buf, sizeof(buf), _T("started '%s'; args '%s'"), path, section->args[it].szValue2 == NULL ? _T("") : section->args[it].szValue2);
							}
							messages.AddLast(buf);
						}
					}

					jumpToSection(result);
					TR_END
					break;
				case Config::esec::logmsg:
					TR_START
					// Process [LogMsg] section
					messages.AddLast(_T(""));
					messages.AddLast(_T("[LogMsg]"));
					g_LogSection = section;
					jumpToSection(TRUE);
					TR_END
					break;
				case Config::esec::regmsg:
					TR_START
					// Process [RegMsg] section
					messages.AddLast(_T(""));
					messages.AddLast(_T("[RegMsg]"));

					if (g_HandlerCount == g_HandlerMaxCount)
					{
						_sntprintf(buf, sizeof(buf), _T("error: maximum message handlers is %d"), g_HandlerMaxCount);
						messages.AddLast(buf);
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
								h.label = section->args[i].szValue;
							}
						}
						if (h.msg == 0 || h.label == NULL)
						{
							_sntprintf(buf, sizeof(buf), _T("error register handler: invalid args"));
							messages.AddLast(buf);
						}
						else
						{
							g_Handlers[g_HandlerCount] = h;
							++g_HandlerCount;

							_sntprintf(buf, sizeof(buf), _T("handler[%d]: '%s'"), g_HandlerCount, h.label);
							messages.AddLast(buf);
							_sntprintf(buf, sizeof(buf), _T("0x%08x 0x%08x 0x%08x"), h.msg, h.wparam, h.lparam);
							messages.AddLast(buf);
						}
					}
					jumpToSection(TRUE);
					TR_END
					break;
				case Config::esec::waitmsg:
					TR_START
					// Process [WaitMsg] section
					messages.AddLast(_T(""));
					messages.AddLast(_T("[WaitMsg]"));
					g_WaitMsg = TRUE;
					jumpToSection(TRUE);
					TR_END
					break;
				case Config::esec::exitapp:
					TR_START
					// Process [ExitApp] section
					messages.AddLast(_T(""));
					messages.AddLast(_T("[Exit]"));
					setTimer(TIMER_EXIT, section->wait);
					TR_END
					break;

				case Config::esec::sstop:
					TR_START
					// Process [Stop] section
					messages.AddLast(_T(""));
					messages.AddLast(_T("[Stop]"));
					section->stop = TRUE;
					jumpToSection(TRUE);
					TR_END
					break;

				case Config::esec::swait:
					TR_START
					// Process [Wait] section
					messages.AddLast(_T(""));
					messages.AddLast(_T("[Wait]"));
					jumpToSection(TRUE);
					TR_END
					break;

				case Config::esec::save:
					TR_START
					// Process [Save] section
					messages.AddLast(_T(""));
					messages.AddLast(_T("[Save]"));

					TCHAR * value = NULL;
					BOOL bFlush = FALSE;
					for (unsigned long i = 0; i < section->argc; ++i)
					{
						if (section->args[i].type == Config::earg::value)
						{
							value = section->args[i].szValue;
						}
						else if (section->args[i].type == Config::earg::flush)
						{
							bFlush = section->args[i].bValue;
						}
					}
					
					if (!bFlush && value == NULL)
					{
						messages.AddLast(_T("value is empty"));
					}
					else if (bFlush && config->settings().state_path == NULL)
					{
						messages.AddLast(_T("variables state path is not set"));
					}
					else
					{
						for (unsigned long j = 0; j < section->argc; ++j)
						{
							if (section->args[j].type == Config::earg::name)
							{
								TCHAR * name = section->args[j].szValue;
								if (value != NULL)
								{
									setStateVariable(name, value);
								}
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
					messages.AddLast(_T(""));
					messages.AddLast(_T("[Input]"));
						
					for (unsigned long i = 0; i < section->argc; ++i)
					{
						if (section->args[i].type == Config::earg::keydown)
						{
							_sntprintf(buf, sizeof(buf), _T("keydown 0x%02x"), (unsigned char)section->args[i].ulValue);
							messages.AddLast(buf);
							keybd_event((unsigned char)section->args[i].ulValue, 0, 0, 0);
						}
						else if (section->args[i].type == Config::earg::keyup)
						{
							_sntprintf(buf, sizeof(buf), _T("keyup 0x%02x"), (unsigned char)section->args[i].ulValue);
							messages.AddLast(buf);
							keybd_event((unsigned char)section->args[i].ulValue, 0, KEYEVENTF_KEYUP, 0);
						}
						else if (section->args[i].type == Config::earg::sleep)
						{
							if (section->args[i].ulValue > 1000)
							{
								messages.AddLast(_T("sleep interval is too big, use different section"));
							}
							else
							{
								_sntprintf(buf, sizeof(buf), _T("sleep for %u"), section->args[i].ulValue);
								messages.AddLast(buf);
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
					messages.AddLast(_T(""));
					messages.AddLast(_T("[Time]"));

					long mshift = 0;
					TCHAR * tz = NULL;
					for (unsigned long i = 0; i < section->argc; ++i)
					{
						if (section->args[i].type == Config::earg::mshift)
						{
							mshift = section->args[i].ulValue;
						}
						else if (section->args[i].type == Config::earg::tz)
						{
							tz = section->args[i].szValue;
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
							_sntprintf(buf, sizeof(buf), _T("SystemTimeToFileTime: 0x%08X"), GetLastError());
							messages.AddLast(buf);
						}
						else
						{
							ULONGLONG ull = (((ULONGLONG)ft.dwHighDateTime) << 32) + ft.dwLowDateTime;
							ull += mshift * (ULONGLONG)10000000 * 60;
							ft.dwLowDateTime = (DWORD)(ull & 0xFFFFFFFF);
							ft.dwHighDateTime = (DWORD)(ull >> 32);
							
							if (!FileTimeToSystemTime(&ft, &st))
							{
								_sntprintf(buf, sizeof(buf), _T("FileTimeToSystemTime: 0x%08X"), GetLastError());
								messages.AddLast(buf);
							}
							else if (!SetLocalTime(&st))
							{
								_sntprintf(buf, sizeof(buf), _T("SetLocalTime: 0x%08X"), GetLastError());
								messages.AddLast(buf);
							}
							else
							{
								_sntprintf(buf, sizeof(buf), _T("time shifted by %d minutes"), mshift);
								messages.AddLast(buf);
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
							_sntprintf(buf, sizeof(buf), _T("GetTimeZoneInformationByName: ret %d; err 0x%08X"), ret, GetLastError());
							messages.AddLast(buf);
						}
						else if (!SetTimeZoneInformation(&tzi))
						{
							_sntprintf(buf, sizeof(buf), _T("SetTimeZoneInformation: 0x%08X"), GetLastError());
							messages.AddLast(buf);
						}
						else
						{
							_sntprintf(buf, sizeof(buf), _T("timezone '%s' set"), tz);
							messages.AddLast(buf);
						}

						TR_END
					}

					jumpToSection(TRUE);
					TR_END
					break;

				default:
					TR_START
					// Process unknown section
					messages.AddLast(_T(""));
					messages.AddLast(_T(""));
					messages.AddLast(_T("Unknown section... Exiting..."));
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
					messages.AddLast(_T("Step 2 of [PostWnd] error: section is NULL..."));
					break;
				}
				BOOL timeout = section->timeout > 0 && GetTickCount() - g_StartTime > section->timeout;
				
				unsigned int closed = 0;
				for (unsigned long it = 0; it < g_PostWndCount; ++it)
				{
					if (g_PostWndList[it].closed)
					{
						++closed;
						continue;
					}

					LPTSTR lpClassName = (*g_PostWndList[it].className == 0) ? NULL : g_PostWndList[it].className;
					LPTSTR lpWindowName = (*g_PostWndList[it].windowName == 0) ? NULL : g_PostWndList[it].windowName;
					if (lpClassName == NULL && lpWindowName == NULL)
					{
						g_PostWndList[it].closed = TRUE;
						++closed;
						continue;
					}
					HWND hWnd = FindWindow(lpClassName, lpWindowName);
					if (hWnd == NULL)
					{
						_sntprintf(buf, sizeof(buf), _T("closed '%s' '%s'"), g_PostWndList[it].className, g_PostWndList[it].windowName);
						messages.AddLast(buf);
						g_PostWndList[it].closed = TRUE;
						++closed;
						continue;
					}
					else if (timeout)
					{
						_sntprintf(buf, sizeof(buf), _T("timeout '%s' '%s'"), g_PostWndList[it].className, g_PostWndList[it].windowName);
						messages.AddLast(buf);
					}
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
				for (unsigned long it = 0; it < g_HandlerCount; ++it)
				{
					if (g_Handlers[it].msg == message &&
						(g_Handlers[it].wparam == 0xFFFFFFFF || g_Handlers[it].wparam == wParam) &&
						(g_Handlers[it].lparam == 0xFFFFFFFF || g_Handlers[it].lparam == (unsigned long)lParam))
					{
						messages.AddLast(_T(""));
						messages.AddLast(_T("[WaitMsg]"));
						_sntprintf(buf, sizeof(buf), _T("got: 0x%08x 0x%08x 0x%08x"), message, wParam, lParam);
						messages.AddLast(buf);
						if(config->seekToSection(g_Handlers[it].label))
						{
							_sntprintf(buf, sizeof(buf), _T("jump to '%s'"), g_Handlers[it].label);
							messages.AddLast(buf);
							section = NULL;
							KillTimer(hWnd, TIMER_SECTION_START);
							KillTimer(hWnd, TIMER_KILLWND_IT);
							setTimer(TIMER_SECTION_START, 0);
						}
						else
						{
							_sntprintf(buf, sizeof(buf), _T("jump error to '%s'"), g_Handlers[it].label);
							messages.AddLast(buf);
							invalidate();
						}
						break;
					}
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

   return 0;

   TR_END0
}