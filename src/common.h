static TCHAR APP_NAME[] = _T("Lisa Reloader");

static void (* exception_callback)();
static BOOL exception_showed = FALSE;
static TCHAR exception_msg[1024];


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
		_sntprintf(exception_msg, 1024, _T("Exception (0x%08X) at:\n\t%S:%d"), GetExceptionCode(), __FILE__, __LINE__);\
		MessageBox(GetActiveWindow(), exception_msg, APP_NAME, NULL);\
		exit(1);\
	}\
}

#define TR_END0 TR_END \
return 0;
