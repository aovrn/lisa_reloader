#include <windows.h>
#include "common.h"

void AddLogMsg(StringList& messages, LPCTSTR format, ...)
{
	static TCHAR msg_buf[MSG_BUF_SIZE];

	va_list args;
	va_start(args, format);
	_vsntprintf(msg_buf, MSG_BUF_SIZE - 1, format, args);
	msg_buf[MSG_BUF_SIZE - 1] = 0;
	messages.AddLast(msg_buf);
	va_end(args);
}
