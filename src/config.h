#ifndef __CONFIG_H__
#define __CONFIG_H__

class Config
{
public:
	enum earg
	{
		wait,
		timeout,
		print,
		wnd,
		proc,
		path,
		msg,
		wparam,
		lparam
	};

	enum esec
	{
		error,
		listwnd,
		listproc,
		postwnd,
		killproc,
		startproc,
		logmsg
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
	};

	struct Settings
	{
		unsigned long width;
		unsigned long height;
		LPTSTR splash;
		LPTSTR wclass;
		LPTSTR wtitle;
	};

	static Config * getInstance();

	Section * currentSection();
	Section * nextSection();
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

};

#endif //__CONFIG_H__