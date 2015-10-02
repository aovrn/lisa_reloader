#include <windows.h>
#include "resource.h"

#include "stringlist.h"
#include "common.h"
#include "config.h"
#include "winceapi.h"

#define MAX_LOADSTRING	6000

// Timers definition
#define TIMER_SECTION_START	1
#define TIMER_KILLWND_IT	11
#define TIMER_EXIT			101

#define TIMEOUT				100

// Global Variables:
HINSTANCE			hInst;			// The current instance
HWND				hWnd;			// Main wnd
HBITMAP				hSplash = NULL;

TCHAR g_CmdLine[1024];

StringList			messages;		// Messages
BOOL				except = FALSE;	// Flag indicates that exception catched
Config *			config = NULL;
Config::Section *	section = NULL;	// Current section

int g_Page = 0;
int g_Pages = 1;
int g_Lines = 0;
int g_Size = 0;

BOOL				g_LogMsg = FALSE;	

unsigned long		g_StartTime = 0;

unsigned long		g_PostMsg = 0;
unsigned long		g_PostWParam = 0;
unsigned long		g_PostLParam = 0;
TCHAR *				g_PostWndList = NULL;	// Wnd list for PostWnd section
unsigned long		g_PostWndItemSize = 64;
unsigned long		g_PostWndCount = 0;		// Wnd list size

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass	(HINSTANCE, LPTSTR);
BOOL				InitInstance	(HINSTANCE, int);
LRESULT CALLBACK	WndProc			(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About			(HWND, UINT, WPARAM, LPARAM);

void KillCallback()
{
	KillTimer(hWnd, TIMER_SECTION_START);
	KillTimer(hWnd, TIMER_KILLWND_IT);
	KillTimer(hWnd, TIMER_EXIT);
	except = true;
}

int WINAPI WinMain(	HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPTSTR    lpCmdLine,
					int       nCmdShow)
{
	TR_START

	config = Config::getInstance();
	_tcsncpy(g_CmdLine, lpCmdLine, 1024);

	exception_callback = &KillCallback;
	initWinceApi();

	if (!InitInstance (hInstance, nCmdShow)) {
		return FALSE;
	}

	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0,
		config->settings().width, config->settings().height, SWP_SHOWWINDOW);

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
		_tcsncpy(szWindowClass, config->settings().wclass, MAX_LOADSTRING - 1);
	}
	else
	{
		LoadString(hInstance, IDS_APP_CLASS, szWindowClass, MAX_LOADSTRING);
	}

	MyRegisterClass(hInstance, szWindowClass);

	if (config->settings().wtitle != NULL)
	{
		_tcsncpy(szTitle, config->settings().wtitle, MAX_LOADSTRING - 1);
	}
	else
	{
		LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	}

	// Avoids previous instances
	hWnd = FindWindow(szWindowClass, szTitle);
	if (hWnd)
	{
		SetForegroundWindow((HWND)(((DWORD)hWnd) | 0x01));    
		return 0;
	}

	hWnd = CreateWindow(szWindowClass, szTitle, WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;

	TR_END0
}

void invalidate()
{
	if (hSplash == NULL)
	{
		InvalidateRgn(hWnd, NULL, TRUE);
	}
}

void setTimer(UINT id, UINT timeout, BOOL showwait = TRUE)
{
	TR_START

	if (id != TIMER_KILLWND_IT)
	{
		TCHAR buf[64];
		_sntprintf(buf, sizeof(buf), _T("waiting for %d msecs"), timeout);
		messages.AddLast(buf);
	}
	
	if (g_Pages - g_Page == 1 && messages.Size() != g_Size)
	{
		invalidate();
	}

	SetTimer(hWnd, id, timeout, NULL);

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

		_sntprintf(buf, sizeof(buf), _T("'%s' '%s'"), lpWindowTitle, lpWindowTitle);
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
			size_t len = 0;
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
	
	if (g_LogMsg)
	{
		BOOL bSkip = FALSE;
		for (unsigned long it = 0; it < section->argc; ++it)
		{
			if (section->args[it].type == Config::earg::msg && section->args[it].ulValue == message)
			{
				bSkip = TRUE;
				break;
			}
			if (section->args[it].type == Config::earg::wparam && section->args[it].ulValue == wParam)
			{
				bSkip = TRUE;
				break;
			}
			if (section->args[it].type == Config::earg::lparam && (LPARAM)section->args[it].ulValue == lParam)
			{
				bSkip = TRUE;
				break;
			}
		}
		
		if (!bSkip)
		{
			_sntprintf(buf, sizeof(buf), _T("got: 0x%08x 0x%08x 0x%08x"), message, wParam, lParam);
			messages.AddLast(buf);
			invalidate();
		}
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
				invalidate();
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
				//hSplash = (HBITMAP)LoadImage(NULL, config->settings().splash, IMAGE_BITMAP, 0, 0, 0);
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
				section = config->nextSection();
				if (section == NULL)
				{
					SetTimer(hWnd, TIMER_EXIT, 0, NULL);
					break;
				}
				switch (section->type)
				{
				case Config::esec::error:
				{
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
					break;
				}
				case Config::esec::listwnd:
					listWnds();
					setTimer(TIMER_SECTION_START, section->wait);
					break;
				case Config::esec::listproc:
				{
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
					setTimer(TIMER_SECTION_START, section->wait);
					break;
				}
				case Config::esec::postwnd:
				{
					messages.AddLast(_T(""));
					messages.AddLast(_T("[PostWnd]"));
					
					
					g_PostMsg = 0;
					g_PostWParam = 0;
					g_PostLParam = 0;
					g_StartTime = GetTickCount();
					g_PostWndCount = 0;
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
					}

					g_PostWndList = (TCHAR *)malloc(g_PostWndItemSize * g_PostWndCount * sizeof(TCHAR));
					memset(g_PostWndList, 0, g_PostWndItemSize * g_PostWndCount * sizeof(TCHAR));
					
					for (unsigned long j = 0, it; j < section->argc; ++j)
					{
						if (section->args[j].type == Config::earg::wnd)
						{
							HWND hWnd = FindWindow(section->args[j].szValue, NULL);
							if (hWnd == NULL)
							{
								_sntprintf(buf, sizeof(buf), _T("FindWindow: 0x%08X '%s'"), GetLastError(), section->args[j].szValue);
								messages.AddLast(buf);
								continue;
							}
							
							if (PostMessage(hWnd, g_PostMsg, g_PostWParam, g_PostLParam))
							{
								_tcsncpy(&g_PostWndList[g_PostWndItemSize * (it++)], section->args[j].szValue, g_PostWndItemSize - 1);
							}
							else
							{
								_sntprintf(buf, sizeof(buf), _T("PostMessage: 0x%08X '%s'"), GetLastError(), GetLastError(), section->args[j].szValue);
								messages.AddLast(buf);
							}
						}
					}

					setTimer(TIMER_KILLWND_IT, TIMEOUT);
					break;
				}
				case Config::esec::killproc:
				{
					messages.AddLast(_T(""));
					messages.AddLast(_T("[KillProc]"));

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
							}
						}
					}
					free(list);
					setTimer(TIMER_SECTION_START, section->wait);
					break;
				}
				case Config::esec::startproc:
				{
					messages.AddLast(_T(""));
					messages.AddLast(_T("[StartProc]"));

					for (unsigned long it = 0; it < section->argc; ++it)
					{
						if (section->args[it].type == Config::earg::path)
						{
							PROCESS_INFORMATION processInformation;
							if (!CreateProcess(section->args[it].szValue,
								NULL, NULL, NULL, FALSE, 0, NULL, NULL, NULL,
								&processInformation ))
							{
								_sntprintf(buf, sizeof(buf), _T("error 0x%08X '%s'"), GetLastError(), section->args[it].szValue);
							}
							else
							{
								_sntprintf(buf, sizeof(buf), _T("started '%s'"), section->args[it].szValue);
							}
							messages.AddLast(buf);
						}
					}

					setTimer(TIMER_SECTION_START, section->wait);
					break;
				}
				case Config::esec::logmsg:
					messages.AddLast(_T(""));
					messages.AddLast(_T("[LogMsg]"));
					g_LogMsg = TRUE;
					invalidate();
					break;

				default:
					messages.AddLast(_T(""));
					messages.AddLast(_T(""));
					messages.AddLast(_T("Unknown section... Exiting..."));
					SetTimer(hWnd, TIMER_EXIT, 1000, NULL);
					break;
				}
			}
			else if (wParam == TIMER_KILLWND_IT)
			{
				BOOL timeout = section->timeout > 0 && GetTickCount() - g_StartTime > section->timeout;
				
				unsigned int closed = 0;
				for (unsigned long it = 0; it < g_PostWndCount; ++it)
				{
					if (g_PostWndList[g_PostWndItemSize * it] == 0)
					{
						++closed;
						continue;
					}

					HWND hWnd = FindWindow(&g_PostWndList[g_PostWndItemSize * it], NULL);
					if (hWnd == NULL)
					{
						_sntprintf(buf, sizeof(buf), _T("closed '%s'"), &g_PostWndList[g_PostWndItemSize * it]);
						messages.AddLast(buf);
						g_PostWndList[g_PostWndItemSize * it] = 0;
						++closed;
						continue;
					}
					else if (timeout)
					{
						_sntprintf(buf, sizeof(buf), _T("timeout '%s'"), &g_PostWndList[g_PostWndItemSize * it]);
						messages.AddLast(buf);
					}
				}

				if ((closed == g_PostWndCount) || timeout)
				{
					free(g_PostWndList);
					g_PostWndList = NULL;
					g_PostWndCount = 0;
					setTimer(TIMER_SECTION_START, section->wait);
				}
				else
				{
					setTimer(TIMER_KILLWND_IT, TIMEOUT);
				}
			}
			else if (wParam == TIMER_EXIT)
			{
				PostMessage(hWnd, WM_CLOSE, 0, 0);
			}

			TR_END
			break;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
   }

   return 0;

   TR_END0
}