#ifndef _MY_APP_H_
#define _MY_APP_H_
#pragma once

#define clGreen		FOREGROUND_GREEN|								  FOREGROUND_INTENSITY
#define clWhite		FOREGROUND_RED	|FOREGROUND_GREEN|FOREGROUND_BLUE
#define clYellow	FOREGROUND_GREEN|FOREGROUND_RED	 |				  FOREGROUND_INTENSITY

typedef std::pair<std::wstring, std::wstring>						appPair;
typedef std::multimap<std::wstring, appPair>						appMap;
typedef std::pair<appMap::const_iterator, appMap::const_iterator>	appMapRange;

// -----------------------------------------------------------------------------------------------

class myApp
{
	public:
		myApp();
		~myApp();

		void process(int, _TCHAR* []);
		void doPrint(const char	   *, const char    * =  "", bool = false);
		void doPrint(const wchar_t *, const wchar_t * = L"", bool = false);

	private:
		void processExec			(int, _TCHAR* [], const wchar_t *, const wchar_t *, std::wstring &);
		void processBatch			(int, _TCHAR* [], const wchar_t *, const wchar_t *, std::wstring &);
		void processBatchFile		(int, _TCHAR* [], const wchar_t *, const wchar_t *, std::wstring &);

		bool doUseDefault			();
		appMapRange getRange		(std::wstring &, std::wstring &);

		std::wstring getName		(const std::wstring &);
		std::wstring getExtension	(const std::wstring &);
		void parseExtensions		(const std::wstring &, std::vector<std::wstring> &);
		bool pathCheck				(const std::wstring &, const wchar_t *, const wchar_t *&, const wchar_t *);
		void shellExecFile			(const std::wstring &, const wchar_t * = nullptr);
		bool createIni				(const wchar_t *);					// Create default .ini-file
		bool readFromIni			(const wchar_t *);					// Read data from the .ini-file

		static std::ostream  &  green	(std::ostream  &s) { SetConsoleTextAttribute(console, clGreen ); return s; }
		static std::ostream  &  white	(std::ostream  &s) { SetConsoleTextAttribute(console, clWhite ); return s; }
		static std::ostream  &  yellow	(std::ostream  &s) { SetConsoleTextAttribute(console, clYellow); return s; }
		static std::wostream &	green	(std::wostream &s) { SetConsoleTextAttribute(console, clGreen ); return s; }
		static std::wostream &	white	(std::wostream &s) { SetConsoleTextAttribute(console, clWhite ); return s; }
		static std::wostream &	yellow	(std::wostream &s) { SetConsoleTextAttribute(console, clYellow); return s; }

	private:
		appMap					mMap;
		bool					useDefault;
		size_t					maxCmdLineLen;
		static	HANDLE			console;
		static	const wchar_t*	iniFile;
};
// -----------------------------------------------------------------------------------------------

#endif
