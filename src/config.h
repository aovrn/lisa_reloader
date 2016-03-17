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
		onsuccess,
		onerror,
		jump,
		stop,
		keydown,
		keyup,
		sleep,
		check,
		name,
		value,
		set,
		flush,
		mshift,
		tz
	};

	enum esec
	{
		error,
		listwnd,
		postwnd,
		repostwnd,
		findwnd,
		setwnd,
		listproc,
		killproc,
		startproc,
		logmsg,
		regmsg,
		waitmsg,
		exitapp,
		input,
		sstop,
		swait,
		save,
		time,
		include
	};

	struct Argument
	{
		earg type;
		LPTSTR szValue;
		LPTSTR szValue2;
		LPTSTR szValue3;
		LPTSTR szValueFilled;
		LPTSTR szValueFilled2;
		LPTSTR szValueFilled3;
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
		LPTSTR module_path;
		LPTSTR state_path;
		unsigned long x;
		unsigned long y;
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
		BOOL benchmark;
		BOOL empty_as_null;
		BOOL immediate_jump;
	};

	static Config * getInstance();

	Section * currentSection();
	Section * nextSection();
	BOOL seekToSection(LPCTSTR label);
	Settings& settings();

protected:
	Config();
	~Config();

	void loadConfig();
	void loadConfig(LPCTSTR path);
	void addSection(LPCTSTR name, unsigned long wait, unsigned long argc = 10);
	void addArgument(Section * section, LPCTSTR name, LPCTSTR value);
	void addSetting(LPCTSTR name, LPCTSTR value);
	void addError(LPCTSTR format, ...);
	BOOL readYesNo(LPCTSTR value, BOOL * arg);
	BOOL readULong(LPCTSTR value, unsigned long * arg);
	BOOL readStrings(LPCTSTR value, LPTSTR * arg1, LPTSTR * arg2 = NULL, LPTSTR * arg3 = NULL);
	TCHAR * trimQuotes(LPTSTR value);

private:
	Section *		m_sections;
	unsigned long	m_count;
	unsigned long	m_allocated;
	long			m_current;
	LPCTSTR			m_file;
	unsigned long	m_line;
	Settings		m_settings;
	BOOL			m_seek;
	BOOL			m_include;

};

#endif //__CONFIG_H__