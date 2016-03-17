#ifndef __COMMON_H__
#define __COMMON_H__

static TCHAR APP_NAME[] = _T("Lisa Reloader");

static void (* exception_callback)();
static BOOL exception_showed = FALSE;
#define EXCEPTION_MSG_SIZE 1024
static TCHAR exception_msg[EXCEPTION_MSG_SIZE];


#define TR_START \
__try\
{

#define TR_END \
}\
__except (EXCEPTION_EXECUTE_HANDLER)\
{\
	if (exception_callback != NULL)\
		exception_callback();\
	if (!exception_showed)\
	{\
		exception_showed = TRUE;\
		_sntprintf(exception_msg, EXCEPTION_MSG_SIZE - 1, _T("Exception (0x%08X) at:\n\t%S:%d"), GetExceptionCode(), __FILE__, __LINE__);\
		exception_msg[EXCEPTION_MSG_SIZE - 1] = 0;\
		MessageBox(GetActiveWindow(), exception_msg, APP_NAME, NULL);\
		exit(1);\
	}\
}

#define TR_END0 TR_END \
return 0;



#include "stringlist.h"
#define MSG_BUF_SIZE 256
void AddLogMsg(StringList& messages, LPCTSTR format, ...);

#endif //__COMMON_H__