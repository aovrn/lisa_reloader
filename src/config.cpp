#include <windows.h>

#include "common.h"
#include "config.h"

const TCHAR sError[] = _T("error");
const TCHAR sListWnd[] = _T("listwnd");
const TCHAR sListProc[] = _T("listproc");
const TCHAR sPostWnd[] = _T("postwnd");
const TCHAR sKillProc[] = _T("killproc");
const TCHAR sStartProc[] = _T("startproc");
const TCHAR sLogMsg[] = _T("logmsg");

const TCHAR aWait[] = _T("wait");
const TCHAR aTimeout[] = _T("timeout");
const TCHAR aPrint[] = _T("print");
const TCHAR aWnd[] = _T("wnd");
const TCHAR aProc[] = _T("proc");
const TCHAR aPath[] = _T("path");
const TCHAR aMsg[] = _T("msg");
const TCHAR aWparam[] = _T("wparam");
const TCHAR aLparam[] = _T("lparam");

const TCHAR sWidth[] = _T("width");
const TCHAR sHeight[] = _T("height");
const TCHAR sSplash[] = _T("splash");
const TCHAR sClass[] = _T("wclass");
const TCHAR sTitle[] = _T("wtitle");

Config::Config(): m_sections(NULL), m_count(0), m_allocated(0), m_current(-1), m_line(0)
{
	m_settings.width = 320;
	m_settings.height = 450;
	m_settings.splash = NULL;
	m_settings.wclass = NULL;
	m_settings.wtitle = NULL;
}

Config::~Config()
{
	TR_START

	for (unsigned long i = 0; i < m_count; ++i)
	{
		for (unsigned long j = 0; j < m_sections[i].argc; ++j)
		{
			free(m_sections[i].args[j].szValue);
		}
		
		free(m_sections[i].args);
		free(m_sections[i].name);
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
	++m_current;
	return currentSection();
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
		_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Empty section name"), m_line);
		addError(m_buf);
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

	if (_tcsnicmp(name, sError, 255) == 0)
	{
		if (m_count != 0)
		{
			_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Section '%s' cannot be defined"), m_line, name);
			addError(m_buf);
			return;
		}
		s.type = esec::error;
	}
	else if (_tcsnicmp(name, sListWnd, 255) == 0)
	{
		s.type = esec::listwnd;
	}
	else if (_tcsnicmp(name, sListProc, 255) == 0)
	{
		s.type = esec::listproc;
	}
	else if (_tcsnicmp(name, sPostWnd, 255) == 0)
	{
		s.type = esec::postwnd;
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
	else
	{
		_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Unsupported section '%s'"), m_line, name);
		addError(m_buf);
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
	s.name = (TCHAR *)malloc((_tcslen(name) + 1) * sizeof(TCHAR));
	_tcscpy(s.name, name);

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
		_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Empty argument name"), m_line);
		addError(m_buf);
		return;
	}
	
	if (value == NULL || _tcslen(value) == 0)
	{
		_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Empty argument value"), m_line);
		addError(m_buf);
		return;
	}
	
	if (section->argc == section->allocated_argc)
	{
		section->allocated_argc += 10;
		section->args = (Argument *)realloc(section->args, sizeof(Argument) * section->allocated_argc);
	}

	Argument& arg = section->args[section->argc];
	BOOL bInvalidArg = FALSE;
	
	if (_tcsnicmp(name, aWait, 255) == 0)
	{
		if(_stscanf(value, (_tcsncmp(_T("0x"), value, 2) == 0) ? _T("0x%x") : _T("%u"), &arg.ulValue) != 1)
		{
			_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Unsupported value '%s'"), m_line, value);
			addError(m_buf);
			_sntprintf(m_buf, sizeof(m_buf), _T("Line %d:     it must be unsigned integer"), m_line, value);
			addError(m_buf);
			return;
		}
		arg.type = earg::wait;
		section->wait = arg.ulValue;
	}
	else if (_tcsnicmp(name, aTimeout, 255) == 0)
	{
		bInvalidArg = section->type != esec::postwnd;
		if (!bInvalidArg)
		{
			if(_stscanf(value, (_tcsncmp(_T("0x"), value, 2) == 0) ? _T("0x%x") : _T("%u"), &arg.ulValue) != 1)
			{
				_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Unsupported value '%s'"), m_line, value);
				addError(m_buf);
				_sntprintf(m_buf, sizeof(m_buf), _T("Line %d:     it must be unsigned integer"), m_line, value);
				addError(m_buf);
				return;
			}
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
		bInvalidArg = section->type != esec::postwnd;
		arg.type = earg::wnd;
	}
	else if (_tcsnicmp(name, aProc, 255) == 0)
	{
		bInvalidArg = section->type != esec::listproc && section->type != esec::killproc;
		arg.type = earg::proc;
	}
	else if (_tcsnicmp(name, aPath, 255) == 0)
	{
		bInvalidArg = section->type != esec::startproc;
		arg.type = earg::path;
	}
	else if (_tcsnicmp(name, aMsg, 255) == 0)
	{
		bInvalidArg = section->type != esec::postwnd && section->type != esec::logmsg;
		if (!bInvalidArg)
		{
			if(_stscanf(value, (_tcsncmp(_T("0x"), value, 2) == 0) ? _T("0x%x") : _T("%u"), &arg.ulValue) != 1)
			{
				_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Unsupported value '%s'"), m_line, value);
				addError(m_buf);
				_sntprintf(m_buf, sizeof(m_buf), _T("Line %d:     it must be unsigned integer"), m_line, value);
				addError(m_buf);
				return;
			}
			arg.type = earg::msg;
		}
	}
	else if (_tcsnicmp(name, aWparam, 255) == 0)
	{
		bInvalidArg = section->type != esec::postwnd && section->type != esec::logmsg;
		if (!bInvalidArg)
		{
			if(_stscanf(value, (_tcsncmp(_T("0x"), value, 2) == 0) ? _T("0x%x") : _T("%u"), &arg.ulValue) != 1)
			{
				_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Unsupported value '%s'"), m_line, value);
				addError(m_buf);
				_sntprintf(m_buf, sizeof(m_buf), _T("Line %d:     it must be unsigned integer"), m_line, value);
				addError(m_buf);
				return;
			}
			arg.type = earg::wparam;
		}
	}
	else if (_tcsnicmp(name, aLparam, 255) == 0)
	{
		bInvalidArg = section->type != esec::postwnd && section->type != esec::logmsg;
		if (!bInvalidArg)
		{
			if(_stscanf(value, (_tcsncmp(_T("0x"), value, 2) == 0) ? _T("0x%x") : _T("%u"), &arg.ulValue) != 1)
			{
				_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Unsupported value '%s'"), m_line, value);
				addError(m_buf);
				_sntprintf(m_buf, sizeof(m_buf), _T("Line %d:     it must be unsigned integer"), m_line, value);
				addError(m_buf);
				return;
			}
			arg.type = earg::lparam;
		}
	}
	else
	{
		_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Unsupported argument '%s'"), m_line, name);
		addError(m_buf);
		return;
	}
	
	if (bInvalidArg)
	{
		_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Unsupported argument '%s'"), m_line, name);
		addError(m_buf);
		_sntprintf(m_buf, sizeof(m_buf), _T("Line %d:     for section '%s'"), m_line, section->name);
		addError(m_buf);
		return;
	}

	size_t len = (_tcslen(value) + 1) * sizeof(TCHAR);
	arg.szValue = (TCHAR *)malloc(len);
	_tcscpy(arg.szValue, value);

	++(section->argc);

	TR_END
}

void Config::addSetting(LPCTSTR name, LPCTSTR value)
{
	TR_START
	
	if (name == NULL || _tcslen(name) == 0)
	{
		_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Empty argument name"), m_line);
		addError(m_buf);
		return;
	}
	
	if (value == NULL || _tcslen(value) == 0)
	{
		_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Empty argument value"), m_line);
		addError(m_buf);
		return;
	}
	
	if (_tcsnicmp(name, sWidth, 255) == 0)
	{
		if(_stscanf(value, (_tcsncmp(_T("0x"), value, 2) == 0) ? _T("0x%x") : _T("%u"), &m_settings.width) != 1)
		{
			_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Unsupported value '%s'"), m_line, value);
			addError(m_buf);
			_sntprintf(m_buf, sizeof(m_buf), _T("Line %d:     it must be unsigned integer"), m_line, value);
			addError(m_buf);

			m_settings.width = 320;
			return;
		}
	}
	else if (_tcsnicmp(name, sHeight, 255) == 0)
	{
		if(_stscanf(value, (_tcsncmp(_T("0x"), value, 2) == 0) ? _T("0x%x") : _T("%u"), &m_settings.height) != 1)
		{
			_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Unsupported value '%s'"), m_line, value);
			addError(m_buf);
			_sntprintf(m_buf, sizeof(m_buf), _T("Line %d:     it must be unsigned integer"), m_line, value);
			addError(m_buf);

			m_settings.height = 450;
			return;
		}
	}
	else if (_tcsnicmp(name, sSplash, 255) == 0)
	{
		size_t len = (_tcslen(value) + 1) * sizeof(TCHAR);
		if (m_settings.splash != NULL)
		{
			free(m_settings.splash);
		}
		m_settings.splash = (TCHAR *)malloc(len);
		_tcscpy(m_settings.splash, value);
	}
	else if (_tcsnicmp(name, sClass, 255) == 0)
	{
		size_t len = (_tcslen(value) + 1) * sizeof(TCHAR);
		if (m_settings.wclass != NULL)
		{
			free(m_settings.wclass);
		}
		m_settings.wclass = (TCHAR *)malloc(len);
		_tcscpy(m_settings.wclass, value);
	}
	else if (_tcsnicmp(name, sTitle, 255) == 0)
	{
		size_t len = (_tcslen(value) + 1) * sizeof(TCHAR);
		if (m_settings.wtitle != NULL)
		{
			free(m_settings.wtitle);
		}
		m_settings.wtitle = (TCHAR *)malloc(len);
		_tcscpy(m_settings.wtitle, value);
	}
	else
	{
		_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Unsupported argument '%s'"), m_line, name);
		addError(m_buf);
		return;
	}

	TR_END
}

void Config::addError(LPCTSTR value)
{
	TR_START

	addArgument(m_sections, aPrint, value);

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

	//testConfig;
	//return;

	TCHAR fullName[MAX_PATH + 1];
	if (GetModuleFileName(NULL, fullName, MAX_PATH) == 0)
	{
		_sntprintf(m_buf, sizeof(m_buf), _T("GetModuleFileName error: 0x%08X"), GetLastError());
		addError(m_buf);
		return;
	}
	if (_tcslen(fullName) < 5)
	{
		_sntprintf(m_buf, sizeof(m_buf), _T("Incorrect exe path: %s"), fullName);
		addError(m_buf);
		return;
	}
	
	/*LPTSTR slash = _tcsrchr(fullName,'\\');
	if(slash)
	{
		*slash = 0;
	}

	_tcsncat(fullName, _T("\\LisaReloader.cfg"), MAX_PATH - _tcslen(fullName));*/

	_tcscpy(fullName + _tcslen(fullName) - 4, _T(".cfg"));

	FILE * f = _tfopen(fullName, _T("rt"));
	if(f)
	{
		TCHAR line[1024] = {0};
		LPTSTR key;
		LPTSTR value;
		LPTSTR it;

		while(_fgetts(line, sizeof(line), f))
		{
			++m_line;

			if(*line == _T(';'))
				continue;

			if(*line == _T('#'))
				continue;

			key = line;
			while (isspace(*key))
				++key;

			if (*key == 0)
				continue;

			it = key + _tcslen(key) - 1;
			while (it > key && isspace(*it))
				--it;
			*(it + 1) = 0;

			if (*key == _T('[') && *(key + _tcslen(key) - 1) == _T(']'))
			{
				*(key + _tcslen(key) - 1) = 0;
				addSection(key + 1, 100);
			}
			else
			{
				value = _tcschr(key, '=');
				if(value == NULL || key == value)
				{
					_sntprintf(m_buf, sizeof(m_buf), _T("Line %d: Incorrect line '%s'"), m_line, line);
					addError(m_buf);
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
		_sntprintf(m_buf, sizeof(m_buf), _T("_tfopen error: 0x%08X"), GetLastError());
		addError(m_buf);
		addError(fullName);
	}

	if (m_count == 1 && m_sections->argc == 0)
	{
		addError(_T("Configuration file is empty."));
		addError(fullName);
	}

	TR_END
}

void Config::testConfig()
{
	TR_START

	addSection(sListProc, 10000);
	addArgument(&m_sections[m_count - 1], aProc, _T("all"));
	addSection(sListWnd, 60000);
	//addSection(sKillTask, 10000);
	//addArgument(&m_sections[m_count - 1], aTask, _T("Storage Card"));
	//addSection(sStartProc, 10000);
	//addArgument(&m_sections[m_count - 1], aPath, _T("\\Windows\\Control.exe"));
	//addArgument(&m_sections[m_count - 1], aPath, _T("\\Windows\\Control1.exe"));
	
	//addSection(sListTask, 10000);
	//addSection(sListProc, 1000);
	//addSection(sListProc, 1000);
	//addSection(sListProc, 1000);
	//addSection(sKillProc, 10000);
	//addArgument(&m_sections[m_count - 1], aProc, _T("explorer.exe"));
	//return;
	

	addSection(sListProc, 1000);
	
	addSection(sPostWnd, 1000);
	addArgument(&m_sections[m_count - 1], aWnd, _T("SANTA"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("MONA"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("MILHOUSE"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("BART"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("HOMER"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("BLUE"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("PHAETHON"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("SELMA"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("CLANCY"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("WRAPPER"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("MARGE"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("JACQUELINE"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("LGENavi"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("WRAITH"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("OBSERVER"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("POSITION"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("MAGGIE"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("MURPHY"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("RALPH"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("SNOWBALL"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("KRONOS"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("PHOENIX"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("JUPITER"));
	addArgument(&m_sections[m_count - 1], aWnd, _T("VULCAN"));

	addSection(sPostWnd, 1000);
	addArgument(&m_sections[m_count - 1], aWnd, _T("LISA"));
	
	addSection(sListProc, 1000);

	addSection(sStartProc, 6000);
	addArgument(&m_sections[m_count - 1], aPath, _T("\\Storage Card\\System\\Lisa1.exe"));

	addSection(sListProc, 6000);


	return;

	addSection(sKillProc, 1000);
	addArgument(&m_sections[m_count - 1], aProc, _T("Patty.exe"));
	addSection(sKillProc, 1000);
	addArgument(&m_sections[m_count - 1], aProc, _T("blue.exe"));
	addSection(sKillProc, 1000);
	addArgument(&m_sections[m_count - 1], aProc, _T("Wrapper_WCe.exe"));
	addSection(sKillProc, 1000);
	addArgument(&m_sections[m_count - 1], aProc, _T("Selma.exe"));
	addSection(sKillProc, 1000);
	addArgument(&m_sections[m_count - 1], aProc, _T("WRAITH.exe"));
	addSection(sKillProc, 1000);
	addArgument(&m_sections[m_count - 1], aProc, _T("observer.exe"));
	addSection(sKillProc, 1000);
	addArgument(&m_sections[m_count - 1], aProc, _T("yn_position.exe"));
	addSection(sKillProc, 1000);
	addArgument(&m_sections[m_count - 1], aProc, _T("phaethon10.exe"));
	addSection(sKillProc, 1000);
	addArgument(&m_sections[m_count - 1], aProc, _T("eXpedi_CE.exe"));
	addSection(sKillProc, 1000);
	addArgument(&m_sections[m_count - 1], aProc, _T("Homer.exe"));
	addSection(sKillProc, 1000);
	addArgument(&m_sections[m_count - 1], aProc, _T("Santa.exe"));
	addSection(sKillProc, 1000);
	addArgument(&m_sections[m_count - 1], aProc, _T("Mona10.exe"));
	addSection(sKillProc, 1000);
	addArgument(&m_sections[m_count - 1], aProc, _T("Marge.exe"));
	addSection(sKillProc, 1000);
	addArgument(&m_sections[m_count - 1], aProc, _T("Maggie.exe"));
	addSection(sKillProc, 1000);
	addArgument(&m_sections[m_count - 1], aProc, _T("Bart.exe"));
	addSection(sKillProc, 1000);
	addArgument(&m_sections[m_count - 1], aProc, _T("MilHouse.exe"));
	addSection(sKillProc, 1000);
	addArgument(&m_sections[m_count - 1], aProc, _T("Clancy.exe"));
	addSection(sKillProc, 10000);
	addArgument(&m_sections[m_count - 1], aProc, _T("Lisa.exe"));

	addSection(sListProc, 10000);
	addSection(sStartProc, 60000);
	addArgument(&m_sections[m_count - 1], aPath, _T("\\Storage Card\\System\\Lisa1.exe"));
	addSection(sListProc, 60000);

	return;
	
	TR_END
}
