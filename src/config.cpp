#include <windows.h>

#include "common.h"
#include "config.h"

const TCHAR sError[] = _T("error");
const TCHAR sListWnd[] = _T("listwnd");
const TCHAR sPostWnd[] = _T("postwnd");
const TCHAR sRepostWnd[] = _T("repostwnd");
const TCHAR sFindWnd[] = _T("findwnd");
const TCHAR sSetWnd[] = _T("setwnd");
const TCHAR sListProc[] = _T("listproc");
const TCHAR sKillProc[] = _T("killproc");
const TCHAR sStartProc[] = _T("startproc");
const TCHAR sLogMsg[] = _T("logmsg");
const TCHAR sRegMsg[] = _T("regmsg");
const TCHAR sWaitMsg[] = _T("waitmsg");
const TCHAR sExit[] = _T("exit");
const TCHAR sInput[] = _T("input");
const TCHAR sStop[] = _T("stop");
const TCHAR sWait[] = _T("wait");
const TCHAR sSave[] = _T("save");
const TCHAR sTime[] = _T("time");
const TCHAR sInclude[] = _T("include");

const TCHAR aLabel[] = _T("label");
const TCHAR aWait[] = _T("wait");
const TCHAR aTimeout[] = _T("timeout");
const TCHAR aPrint[] = _T("print");
const TCHAR aWnd[] = _T("wnd");
const TCHAR aProc[] = _T("proc");
const TCHAR aPath[] = _T("path");
const TCHAR aMsg[] = _T("msg");
const TCHAR aWparam[] = _T("wparam");
const TCHAR aLparam[] = _T("lparam");
const TCHAR aUseSend[] = _T("usesend");
const TCHAR aAction[] = _T("action");
const TCHAR aFlags[] = _T("flags");
const TCHAR aHandler[] = _T("handler");
const TCHAR aOnSuccess[] = _T("onsuccess");
const TCHAR aOnError[] = _T("onerror");
const TCHAR aJump[] = _T("jump");
const TCHAR aStop[] = _T("stop");
const TCHAR aKeyDown[] = _T("keydown");
const TCHAR aKeyUp[] = _T("keyup");
const TCHAR aSleep[] = _T("sleep");
const TCHAR aCheck[] = _T("check");
const TCHAR aName[] = _T("name");
const TCHAR aValue[] = _T("value");
const TCHAR aSet[] = _T("set");
const TCHAR aFlush[] = _T("flush");
const TCHAR aMShift[] = _T("mshift");
const TCHAR aTZ[] = _T("tz");

const TCHAR sX[] = _T("x");
const TCHAR sY[] = _T("y");
const TCHAR sWidth[] = _T("width");
const TCHAR sHeight[] = _T("height");
const TCHAR sSplash[] = _T("splash");
const TCHAR sMinimize[] = _T("minimize");
const TCHAR sHide[] = _T("hide");
const TCHAR sClass[] = _T("wclass");
const TCHAR sTitle[] = _T("wtitle");
const TCHAR sSeparator[] = _T("separator");
const TCHAR sQuote[] = _T("quote");
const TCHAR sSecondNoActivate[] = _T("secondnoactivate");
const TCHAR sSecondNoExit[] = _T("secondnoexit");
const TCHAR sBenchmark[] = _T("benchmark");
const TCHAR sEmptyAsNull[] = _T("emptyAsNull");
const TCHAR sImmediateJump[] = _T("immediatejump");

const TCHAR vYes[] = _T("yes");
const TCHAR vNo[] = _T("no");

Config::Config(): m_sections(NULL), m_count(0), m_allocated(0), m_current(-1),
	m_file(NULL), m_line(0), m_seek(FALSE), m_include(FALSE)
{
	m_settings.module_path = NULL;
	m_settings.state_path = NULL;
	m_settings.x = 0;
	m_settings.y = 0;
	m_settings.width = 420;
	m_settings.height = 450;
	m_settings.splash = NULL;
	m_settings.minimize = FALSE;
	m_settings.hide = FALSE;
	m_settings.wclass = NULL;
	m_settings.wtitle = NULL;
	m_settings.separator = (TCHAR *)malloc(2 * sizeof(TCHAR));
	*m_settings.separator = _T(';');
	m_settings.quote = (TCHAR *)malloc(2 * sizeof(TCHAR));
	*m_settings.quote = _T('\'');
	m_settings.second_noactivate = FALSE;
	m_settings.second_noexit = FALSE;
	m_settings.benchmark = FALSE;
	m_settings.empty_as_null = TRUE;
	m_settings.immediate_jump = FALSE;
}

Config::~Config()
{
	TR_START

	for (unsigned long i = 0; i < m_count; ++i)
	{
		for (unsigned long j = 0; j < m_sections[i].argc; ++j)
		{
			if (m_sections[i].args[j].szValue != NULL)
			{
				free(m_sections[i].args[j].szValue);
			}
			if (m_sections[i].args[j].szValue2 != NULL)
			{
				free(m_sections[i].args[j].szValue2);
			}
			if (m_sections[i].args[j].szValue3 != NULL)
			{
				free(m_sections[i].args[j].szValue3);
			}
		}
		
		free(m_sections[i].args);
		free(m_sections[i].name);
		free(m_sections[i].label);
	}

	free(m_sections);

	TR_END
}

Config * Config::getInstance()
{
	static Config * config = NULL;

	TR_START

	if (config == NULL)
	{
		config = new Config();
		config->loadConfig();
	}

	TR_END

	return config;
}

Config::Section * Config::currentSection()
{
	TR_START

	if (m_current < 0 || (size_t)m_current >= m_count)
	{
		return NULL;
	}

	return &m_sections[m_current];
	
	TR_END0
}

Config::Section * Config::nextSection()
{
	if (m_seek)
	{
		m_seek = FALSE;
	}
	else
	{
		++m_current;
	}
	return currentSection();
}

BOOL Config::seekToSection(LPCTSTR label)
{
	TR_START

	BOOL result = FALSE;

	for (unsigned long it = 0; it < m_count; ++it)
	{
		if (m_sections[it].label != NULL && _tcscmp(label, m_sections[it].label) == 0)
		{
			m_current = it;
			m_seek = result = TRUE;
			break;
		}
	}

	return result;

	TR_END0
}

Config::Settings& Config::settings()
{
	return m_settings;
}

void Config::addSection(LPCTSTR name, unsigned long wait, unsigned long argc)
{
	TR_START
		
	if (name == NULL || _tcslen(name) == 0)
	{
		addError(_T("%s:%d: Empty section name"), m_file, m_line);
		return;
	}

	if (m_sections == NULL)
	{
		m_count = 0;
		m_allocated = 5;
		m_sections = (Section *)malloc(sizeof(Section) * m_allocated);

	}
		
	if (m_count == m_allocated)
	{
		m_allocated += 5;
		m_sections = (Section *)realloc(m_sections, sizeof(Section) * m_allocated);
	}

	Section& s = m_sections[m_count];
	m_include = FALSE;

	if (_tcsnicmp(name, sError, 255) == 0)
	{
		if (m_count != 0)
		{
			addError(_T("%s:%d: Section '%s' cannot be defined"), m_file, m_line, name);
			return;
		}
		s.type = esec::error;
	}
	else if (_tcsnicmp(name, sListWnd, 255) == 0)
	{
		s.type = esec::listwnd;
	}
	else if (_tcsnicmp(name, sPostWnd, 255) == 0)
	{
		s.type = esec::postwnd;
	}
	else if (_tcsnicmp(name, sRepostWnd, 255) == 0)
	{
		s.type = esec::repostwnd;
	}
	else if (_tcsnicmp(name, sFindWnd, 255) == 0)
	{
		s.type = esec::findwnd;
	}
	else if (_tcsnicmp(name, sSetWnd, 255) == 0)
	{
		s.type = esec::setwnd;
	}
	else if (_tcsnicmp(name, sListProc, 255) == 0)
	{
		s.type = esec::listproc;
	}
	else if (_tcsnicmp(name, sKillProc, 255) == 0)
	{
		s.type = esec::killproc;
	}
	else if (_tcsnicmp(name, sStartProc, 255) == 0)
	{
		s.type = esec::startproc;
	}
	else if (_tcsnicmp(name, sLogMsg, 255) == 0)
	{
		s.type = esec::logmsg;
	}
	else if (_tcsnicmp(name, sRegMsg, 255) == 0)
	{
		s.type = esec::regmsg;
	}
	else if (_tcsnicmp(name, sWaitMsg, 255) == 0)
	{
		s.type = esec::waitmsg;
	}
	else if (_tcsnicmp(name, sExit, 255) == 0)
	{
		s.type = esec::exitapp;
	}
	else if (_tcsnicmp(name, sInput, 255) == 0)
	{
		s.type = esec::input;
	}
	else if (_tcsnicmp(name, sStop, 255) == 0)
	{
		s.type = esec::sstop;
	}
	else if (_tcsnicmp(name, sWait, 255) == 0)
	{
		s.type = esec::swait;
	}
	else if (_tcsnicmp(name, sSave, 255) == 0)
	{
		s.type = esec::save;
	}
	else if (_tcsnicmp(name, sTime, 255) == 0)
	{
		s.type = esec::time;
	}
	else if (_tcsnicmp(name, sInclude, 255) == 0)
	{
		s.type = esec::include;
		m_include = TRUE;
	}
	else
	{
		addError(_T("%s:%d: Unsupported section '%s'"), m_file, m_line, name);
		return;
	}

	s.wait = wait;
	s.timeout = 0;
	s.argc = 0;
	s.allocated_argc = argc;
	if (argc > 0)
		s.args = (Argument *)malloc(sizeof(Argument) * argc);
	else
		s.args = NULL;
	readStrings(name, &s.name);
	s.label = NULL;
	s.stop = FALSE;

	++m_count;

	TR_END
}

void Config::addArgument(Config::Section * section, LPCTSTR name, LPCTSTR value)
{
	TR_START

	if (section == NULL)
		throw STATUS_INVALID_PARAMETER_1;
	
	if (name == NULL || _tcslen(name) == 0)
	{
		addError(_T("%s:%d: Empty argument name"), m_file, m_line);
		return;
	}
	
	if (value == NULL || _tcslen(value) == 0)
	{
		addError(_T("%s:%d: Empty argument value"), m_file, m_line);
		return;
	}
	
	if (section->argc == section->allocated_argc)
	{
		section->allocated_argc += 10;
		section->args = (Argument *)realloc(section->args, sizeof(Argument) * section->allocated_argc);
	}

	Argument& arg = section->args[section->argc];
	arg.szValue = NULL;
	arg.szValue2 = NULL;
	arg.szValue3 = NULL;
	arg.szValueFilled = NULL;
	arg.szValueFilled2 = NULL;
	arg.szValueFilled3 = NULL;
	arg.ulValue = 0;
	arg.bValue = 0;

	BOOL bInvalidArg = FALSE;
	TCHAR * include = NULL;
	
	if (_tcsnicmp(name, aLabel, 255) == 0)
	{
		readStrings(value, &section->label);
		arg.type = earg::label;
	}
	else if (_tcsnicmp(name, aWait, 255) == 0)
	{
		if (!readULong(value, &arg.ulValue)) return;
		arg.type = earg::wait;
		section->wait = arg.ulValue;
	}
	else if (_tcsnicmp(name, aTimeout, 255) == 0)
	{
		bInvalidArg = section->type != esec::postwnd;
		if (!bInvalidArg)
		{
			if (!readULong(value, &arg.ulValue)) return;
			arg.type = earg::timeout;
			section->timeout = arg.ulValue;
		}
	}
	else if (_tcsnicmp(name, aPrint, 255) == 0)
	{
		bInvalidArg = section->type != esec::error;
		arg.type = earg::print;
	}
	else if (_tcsnicmp(name, aWnd, 255) == 0)
	{
		bInvalidArg = section->type != esec::postwnd && section->type != esec::findwnd &&
			section->type != esec::setwnd && section->type != esec::repostwnd;
		if (!bInvalidArg)
		{
			readStrings(value, &arg.szValue, &arg.szValue2, &arg.szValue3);
			arg.type = earg::wnd;
		}
	}
	else if (_tcsnicmp(name, aProc, 255) == 0)
	{
		bInvalidArg = section->type != esec::listproc && section->type != esec::killproc;
		arg.type = earg::proc;
	}
	else if (_tcsnicmp(name, aPath, 255) == 0)
	{
		bInvalidArg = section->type != esec::startproc && !m_include;
		if (!bInvalidArg)
		{
			readStrings(value, &arg.szValue, &arg.szValue2);
			arg.type = earg::path;
			if (m_include)
			{
				if (*arg.szValue != '\\')
				{
					if (_tcslen(m_settings.module_path) + _tcslen(arg.szValue) + 1 > MAX_PATH)
					{
						addError(_T("%s:%d: Include path is too big"), m_file, m_line);
						return;
					}
					include = (TCHAR *)malloc((MAX_PATH + 1) * sizeof(TCHAR));
					*include = 0;
					_tcscpy(include, m_settings.module_path);
					_tcscat(include, arg.szValue);
				}
				else
				{
					if (_tcslen(arg.szValue) + 1 > MAX_PATH)
					{
						addError(_T("%s:%d: Include path is too big"), m_file, m_line);
						return;
					}
					include = (TCHAR *)malloc((MAX_PATH + 1) * sizeof(TCHAR));
					*include = 0;
					_tcscpy(include, arg.szValue);
				}
			}
		}
	}
	else if (_tcsnicmp(name, aMsg, 255) == 0)
	{
		bInvalidArg = section->type != esec::postwnd && section->type != esec::logmsg &&
			section->type != esec::regmsg;
		if (!bInvalidArg)
		{
			if (!readULong(value, &arg.ulValue)) return;
			arg.type = earg::msg;
		}
	}
	else if (_tcsnicmp(name, aWparam, 255) == 0)
	{
		bInvalidArg = section->type != esec::postwnd && section->type != esec::logmsg &&
			section->type != esec::regmsg;
		if (!bInvalidArg)
		{
			if (!readULong(value, &arg.ulValue)) return;
			arg.type = earg::wparam;
		}
	}
	else if (_tcsnicmp(name, aLparam, 255) == 0)
	{
		bInvalidArg = section->type != esec::postwnd && section->type != esec::logmsg &&
			section->type != esec::regmsg;
		if (!bInvalidArg)
		{
			if (!readULong(value, &arg.ulValue)) return;
			arg.type = earg::lparam;
		}
	}
	else if (_tcsnicmp(name, aUseSend, 255) == 0)
	{
		bInvalidArg = section->type != esec::postwnd;
		if (!bInvalidArg)
		{
			if (!readYesNo(value, &arg.bValue)) return;
			arg.type = earg::usesend;
		}
	}
	else if (_tcsnicmp(name, aAction, 255) == 0)
	{
		bInvalidArg = section->type != esec::setwnd;
		arg.type = earg::action;
	}
	else if (_tcsnicmp(name, aFlags, 255) == 0)
	{
		bInvalidArg = section->type != esec::setwnd;
		if (!bInvalidArg)
		{
			if (!readULong(value, &arg.ulValue)) return;
			arg.type = earg::flags;
		}
	}
	else if (_tcsnicmp(name, aHandler, 255) == 0)
	{
		bInvalidArg = section->type != esec::regmsg;
		arg.type = earg::handler;
	}
	else if (_tcsnicmp(name, aOnSuccess, 255) == 0)
	{
		bInvalidArg = section->type != esec::postwnd && section->type != esec::findwnd &&
			section->type != esec::killproc && section->type != esec::startproc &&
			section->type != esec::setwnd && section->type != esec::repostwnd;
		arg.type = earg::onsuccess;
	}
	else if (_tcsnicmp(name, aOnError, 255) == 0)
	{
		bInvalidArg = section->type != esec::postwnd && section->type != esec::findwnd &&
			section->type != esec::killproc && section->type != esec::startproc &&
			section->type != esec::setwnd && section->type != esec::repostwnd;
		arg.type = earg::onerror;
	}
	else if (_tcsnicmp(name, aJump, 255) == 0)
	{
		arg.type = earg::jump;
	}
	else if (_tcsnicmp(name, aStop, 255) == 0)
	{
		if (!readYesNo(value, &arg.bValue)) return;
		arg.type = earg::stop;
		section->stop = arg.bValue;
	}
	else if (_tcsnicmp(name, aKeyDown, 255) == 0)
	{
		bInvalidArg = section->type != esec::input;
		if (!bInvalidArg)
		{
			if (!readULong(value, &arg.ulValue)) return;
			arg.type = earg::keydown;
		}
	}
	else if (_tcsnicmp(name, aKeyUp, 255) == 0)
	{
		bInvalidArg = section->type != esec::input;
		if (!bInvalidArg)
		{
			if (!readULong(value, &arg.ulValue)) return;
			arg.type = earg::keyup;
		}
	}
	else if (_tcsnicmp(name, aSleep, 255) == 0)
	{
		bInvalidArg = section->type != esec::input;
		if (!bInvalidArg)
		{
			if (!readULong(value, &arg.ulValue)) return;
			arg.type = earg::sleep;
		}
	}
	else if (_tcsnicmp(name, aCheck, 255) == 0)
	{
		bInvalidArg = section->type != esec::findwnd;
		arg.type = earg::check;
	}
	else if (_tcsnicmp(name, aName, 255) == 0)
	{
		bInvalidArg = section->type != esec::save;
		arg.type = earg::name;
	}
	else if (_tcsnicmp(name, aValue, 255) == 0)
	{
		bInvalidArg = section->type != esec::save;
		arg.type = earg::value;
	}
	else if (_tcsnicmp(name, aSet, 255) == 0)
	{
		bInvalidArg = section->type != esec::save;
		if (!bInvalidArg)
		{
			readStrings(value, &arg.szValue, &arg.szValue2);
			arg.type = earg::set;
		}
	}
	else if (_tcsnicmp(name, aFlush, 255) == 0)
	{
		bInvalidArg = section->type != esec::save;
		if (!bInvalidArg)
		{
			if (!readYesNo(value, &arg.bValue)) return;
			arg.type = earg::flush;
		}
	}
	else if (_tcsnicmp(name, aMShift, 255) == 0)
	{
		bInvalidArg = section->type != esec::time;
		if (!bInvalidArg)
		{
			if (!readULong(value, &arg.ulValue)) return;
			arg.type = earg::mshift;
		}
	}
	else if (_tcsnicmp(name, aTZ, 255) == 0)
	{
		bInvalidArg = section->type != esec::time;
		arg.type = earg::tz;
	}
	else
	{
		addError(_T("%s:%d: Unsupported argument '%s'"), m_file, m_line, name);
		return;
	}
	
	if (bInvalidArg)
	{
		addError(_T("%s:%d: Unsupported argument '%s'"), m_file, m_line, name);
		addError(_T("%s:%d:     for section '%s'"), m_file, m_line, section->name);
		return;
	}

	if (arg.szValue == NULL)
	{
		readStrings(value, &arg.szValue);
	}

	++(section->argc);

	if (include)
	{
		loadConfig(include);
		free(include);
	}

	TR_END
}

void Config::addSetting(LPCTSTR name, LPCTSTR value)
{
	TR_START
	
	if (name == NULL || _tcslen(name) == 0)
	{
		addError(_T("%s:%d: Empty argument name"), m_file, m_line);
		return;
	}
	
	if (value == NULL || _tcslen(value) == 0)
	{
		addError(_T("%s:%d: Empty argument value"), m_file, m_line);
		return;
	}

	if (_tcsnicmp(name, sX, 255) == 0)
	{
		if (!readULong(value, &m_settings.x)) return;
	}
	else if (_tcsnicmp(name, sY, 255) == 0)
	{
		if (!readULong(value, &m_settings.y)) return;
	}
	else if (_tcsnicmp(name, sWidth, 255) == 0)
	{
		if (!readULong(value, &m_settings.width)) return;
	}
	else if (_tcsnicmp(name, sHeight, 255) == 0)
	{
		if (!readULong(value, &m_settings.height)) return;
	}
	else if (_tcsnicmp(name, sSplash, 255) == 0)
	{
		readStrings(value, &m_settings.splash);
	}
	else if (_tcsnicmp(name, sMinimize, 255) == 0)
	{
		if (!readYesNo(value, &m_settings.minimize)) return;
	}
	else if (_tcsnicmp(name, sHide, 255) == 0)
	{
		if (!readYesNo(value, &m_settings.hide)) return;
	}
	else if (_tcsnicmp(name, sClass, 255) == 0)
	{
		readStrings(value, &m_settings.wclass);
	}
	else if (_tcsnicmp(name, sTitle, 255) == 0)
	{
		readStrings(value, &m_settings.wtitle);
	}
	else if (_tcsnicmp(name, sSeparator, 255) == 0)
	{
		readStrings(value, &m_settings.separator);
	}
	else if (_tcsnicmp(name, sQuote, 255) == 0)
	{
		readStrings(value, &m_settings.quote);
	}
	else if (_tcsnicmp(name, sSecondNoActivate, 255) == 0)
	{
		if (!readYesNo(value, &m_settings.second_noactivate)) return;
	}
	else if (_tcsnicmp(name, sSecondNoExit, 255) == 0)
	{
		if (!readYesNo(value, &m_settings.second_noexit)) return;
	}
	else if (_tcsnicmp(name, sBenchmark, 255) == 0)
	{
		if (!readYesNo(value, &m_settings.benchmark)) return;
	}
	else if (_tcsnicmp(name, sEmptyAsNull, 255) == 0)
	{
		if (!readYesNo(value, &m_settings.empty_as_null)) return;
	}
	else if (_tcsnicmp(name, sImmediateJump, 255) == 0)
	{
		if (!readYesNo(value, &m_settings.immediate_jump)) return;
	}
	else
	{
		addError(_T("%s:%d: Unsupported argument '%s'"), m_file, m_line, name);
		return;
	}

	TR_END
}

BOOL Config::readYesNo(LPCTSTR value, BOOL * arg)
{
	TR_START

	if(_tcsnicmp(value, vYes, 255) != 0 && _tcsnicmp(value, vNo, 255) != 0)
	{
		addError(_T("%s:%d: Unsupported value '%s'"), m_file, m_line, value);
		addError(_T("%s:%d:     it must be 'yes' or 'no'"), m_file, m_line);
		return FALSE;
	}
	
	*arg= _tcsnicmp(value, vYes, 255) == 0;

	return TRUE;
	TR_END0
}

BOOL Config::readULong(LPCTSTR value, unsigned long * arg)
{
	TR_START
	
	if(_stscanf(value, (_tcsncmp(_T("0x"), value, 2) == 0) ? _T("0x%x") : _T("%u"), arg) != 1)
	{
		addError(_T("%s:%d: Unsupported value '%s'"), m_file, m_line, value);
		addError(_T("%s:%d:     it must be unsigned integer"), m_file, m_line);
		return FALSE;
	}

	return TRUE;
	TR_END0
}

BOOL Config::readStrings(LPCTSTR value, LPTSTR * arg1, LPTSTR * arg2, LPTSTR * arg3)
{
	TR_START
		
	if (*arg1 != NULL)
	{
		free(*arg1);
	}

	TCHAR * val = (TCHAR *)malloc((_tcslen(value) + 1) * sizeof(TCHAR));
	_tcscpy(val, value);
	
	if (arg1 != NULL && arg2 != NULL)
	{
		TCHAR * pos = _tcsstr(val, m_settings.separator);
		if (pos != NULL)
		{
			*pos = 0;
			pos += _tcslen(m_settings.separator);
			
			LPTSTR it = val + _tcslen(val) - 1;
			while (it > val && isspace(*it))
				--it;
			*(it + 1) = 0;
		}

		TCHAR * qval = trimQuotes(val);
		
		size_t len = (_tcslen(qval) + 1) * sizeof(TCHAR);
		*arg1 = (TCHAR *)malloc(len);
		_tcscpy(*arg1, qval);
		
		if (pos != NULL)
		{
			while (isspace(*pos))
				++pos;
			readStrings(pos, arg2, arg3);
		}
		else
		{
			readStrings(_T(""), arg2, arg3);
		}
	}
	else if (arg1 != NULL)
	{
		TCHAR * qval = trimQuotes(val);

		size_t len = (_tcslen(qval) + 1) * sizeof(TCHAR);
		*arg1 = (TCHAR *)malloc(len);
		_tcscpy(*arg1, qval);
	}

	free(val);

	return TRUE;
	TR_END0
}

TCHAR * Config::trimQuotes(LPTSTR value)
{
	TR_START

	size_t lenv = _tcslen(value);
	size_t lenq = _tcslen(m_settings.quote);
	if (lenv >= 2 * lenq &&
		_tcsncmp(value, m_settings.quote, lenq) == 0 &&
		_tcsncmp(value + lenv - lenq, m_settings.quote, lenq) == 0)
	{
		value[lenv - lenq] = 0;
		value += lenq;
	}

	return value;

	TR_END0
}

void Config::addError(LPCTSTR format, ...)
{
	TR_START
		
	static TCHAR m_buf[MSG_BUF_SIZE];
		
	va_list args;
	va_start(args, format);
	_vsntprintf(m_buf, MSG_BUF_SIZE - 1, format, args);
	m_buf[MSG_BUF_SIZE - 1] = 0;
	addArgument(m_sections, aPrint, m_buf);
	va_end(args);

	TR_END
}

void Config::loadConfig()
{
	TR_START

	if (m_count != 0)
	{
		addError(_T("Already inited"));
		return;
	}

	addSection(sError, 30000);

	TCHAR fullName[MAX_PATH + 1];
	if (GetModuleFileName(NULL, fullName, MAX_PATH) == 0)
	{
		addError(_T("GetModuleFileName error: 0x%08X"), GetLastError());
		return;
	}
	if (_tcslen(fullName) < 5)
	{
		addError(_T("Incorrect exe path: %s"), fullName);
		return;
	}

	_tcscpy(fullName + _tcslen(fullName) - 4, _T(".cfg"));

	TCHAR * pos = _tcsrchr(fullName, _T('\\'));
	if (pos != NULL)
	{
		TCHAR bak = *pos;
		*pos = 0;
		if (_tcslen(fullName) < MAX_PATH - 7)
		{
			m_settings.state_path = (TCHAR *)malloc((MAX_PATH + 1) * sizeof(TCHAR));
			_tcscpy(m_settings.state_path, fullName);
			_tcscat(m_settings.state_path, _T("\\state\\"));
			m_settings.module_path = (TCHAR *)malloc((MAX_PATH + 1) * sizeof(TCHAR));
			_tcscpy(m_settings.module_path, fullName);
			_tcscat(m_settings.module_path, _T("\\"));
		}
		*pos = bak;
	}
	
	loadConfig(fullName);

	TR_END
}

void Config::loadConfig(LPCTSTR path)
{
	TR_START

	unsigned long old_line = m_line;
	m_line = 0;

	LPCTSTR old_file = m_file;
	m_file = _tcsrchr(path, _T('\\'));
	m_file = (m_file == NULL ? path : m_file + 1);

	FILE * f = _tfopen(path, _T("rt"));
	if(f)
	{
		TCHAR line[1024] = {0};
		LPTSTR key;
		LPTSTR value;
		LPTSTR it;

		while(_fgetts(line, 1024, f))
		{
			++m_line;

			key = line;
			while (isspace(*key))
				++key;

			if (*key == 0)
				continue;

			if(*key == _T(';'))
				continue;

			if(*key == _T('#'))
				continue;

			it = key + _tcslen(key) - 1;
			while (it > key && isspace(*it))
				--it;
			*(it + 1) = 0;

			if (*key == _T('[') && *(key + _tcslen(key) - 1) == _T(']'))
			{
				*(key + _tcslen(key) - 1) = 0;
				addSection(key + 1, 0);
			}
			else
			{
				value = _tcschr(key, '=');
				if(value == NULL || key == value)
				{
					addError(_T("%s:%d: Incorrect line '%s'"), m_file, m_line, line);
					continue;
				}
				
				*(value++) = 0;
				
				it = key + _tcslen(key) - 1;
				while (it > key && isspace(*it))
					--it;
				*(it + 1) = 0;
				
				while (isspace(*value))
					++value;
				
				if (m_count > 1)
				{
					addArgument(&m_sections[m_count - 1], key, value);
				}
				else
				{
					addSetting(key, value);
				}
			}
		}

		fclose(f);
	}
	else
	{
		addError(_T("_tfopen error: 0x%08X"), GetLastError());
		addError(path);
	}
	
	m_file = old_file;
	m_line = old_line;

	TR_END
}
