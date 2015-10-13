#ifndef __CONFIG_H__
#define __CONFIG_H__

class Config
{
public:
	enum earg
	{
		label,
		wait,
		timeout,
		print,
		wnd,
		proc,
		path,
		msg,
		wparam,
		lparam,
		usesend,
		action,
		flags,
		handler,
		onerror,
		stop,
		keydown,
		keyup,
		sleep,
		check
	};

	enum esec
	{
		error,
		listwnd,
		postwnd,
		findwnd,
		setwnd,
		listproc,
		killproc,
		startproc,
		logmsg,
		regmsg,
		waitmsg,
		exitapp,
		input
	};

	struct Argument
	{
		earg type;
		LPTSTR szValue;
		LPTSTR szValue2;
		unsigned long ulValue;
		BOOL bValue;
	};

	struct Section
	{
		esec type;
		unsigned long wait;    // msec
		unsigned long timeout; // msec
		Argument * args;
		unsigned long argc;
		unsigned long allocated_argc;
		LPTSTR name;
		LPTSTR label;
		BOOL stop;
	};

	struct Settings
	{
		unsigned long width;
		unsigned long height;
		LPTSTR splash;
		BOOL minimize;
		BOOL hide;
		LPTSTR wclass;
		LPTSTR wtitle;
		LPTSTR separator;
		LPTSTR quote;
		BOOL second_noactivate;
		BOOL second_noexit;
	};

	static Config * getInstance();

	Section * currentSection();
	Section * nextSection();
	BOOL seekToSection(LPCTSTR label);
	Settings& settings();

protected:
	Config();
	~Config();

	void testConfig();
	void loadConfig();
	void addSection(LPCTSTR name, unsigned long wait, unsigned long argc = 10);
	void addArgument(Section * section, LPCTSTR name, LPCTSTR value);
	void addSetting(LPCTSTR name, LPCTSTR value);
	void addError(LPCTSTR value);
	BOOL readYesNo(LPCTSTR value, BOOL * arg);
	BOOL readULong(LPCTSTR value, unsigned long * arg);
	BOOL readStrings(LPCTSTR value, LPTSTR * arg1, LPTSTR * arg2 = NULL);
	TCHAR * trimQuotes(LPTSTR value);

private:
	Section *		m_sections;
	unsigned long	m_count;
	unsigned long	m_allocated;
	long			m_current;
	unsigned long	m_line;
	TCHAR			m_buf[256];
	Settings		m_settings;
	BOOL			m_seek;

};

#endif //__CONFIG_H__