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
		handler,
		stop,
		keydown,
		keyup,
		sleep
	};

	enum esec
	{
		error,
		listwnd,
		listproc,
		postwnd,
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
		unsigned long ulValue;
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