// shellExec.cpp : Defines the entry point for the console application.

#include "stdafx.h"

// -----------------------------------------------------------------------------------------------

typedef std::pair<std::wstring, std::wstring>			mPairPath_App;
typedef std::multimap<std::wstring, mPairPath_App>		appMap;

appMap mMap;
bool useDefault = false;
static HANDLE console = nullptr;

#define clGreen		FOREGROUND_GREEN|								  FOREGROUND_INTENSITY
#define clWhite		FOREGROUND_RED	|FOREGROUND_GREEN|FOREGROUND_BLUE
#define clYellow	FOREGROUND_GREEN|FOREGROUND_RED	 |				  FOREGROUND_INTENSITY

inline std::ostream&  green	(std::ostream &s)  { SetConsoleTextAttribute(console, clGreen );	return s; }
inline std::ostream&  white	(std::ostream &s)  { SetConsoleTextAttribute(console, clWhite );	return s; }
inline std::ostream&  yellow(std::ostream &s)  { SetConsoleTextAttribute(console, clYellow);	return s; }
inline std::wostream& green	(std::wostream &s) { SetConsoleTextAttribute(console, clGreen );	return s; }
inline std::wostream& white	(std::wostream &s) { SetConsoleTextAttribute(console, clWhite );	return s; }
inline std::wostream& yellow(std::wostream &s) { SetConsoleTextAttribute(console, clYellow);	return s; }

// -----------------------------------------------------------------------------------------------

void doPrint(const char *str, const char *param = "", bool isYellow = false)
{
	if( isYellow )
		std::cout << yellow;
	else
		std::cout << green;
	
	std::cout << " --> " << str << param << white << std::endl;
}
// -----------------------------------------------------------------------------------------------

void doPrint(const wchar_t *str, const wchar_t *param = L"", bool isYellow = false)
{
	if( isYellow )
		std::wcout << yellow;
	else
		std::wcout << green;
	
	std::wcout << " --> " << str << param << white << std::endl;
}
// -----------------------------------------------------------------------------------------------

void aaa(std::ostream &a)
{
	return;
}

std::wstring getName(const std::wstring &file)
{
	size_t pos = file.rfind('\\');

	return file.substr(pos + 1, file.length());
}
// -----------------------------------------------------------------------------------------------

std::wstring getExtension(const std::wstring &file)
{
	std::wstring ext = L"";

	size_t pos1 = file.rfind('\\');
	size_t pos2 = file.rfind('.');

	if( pos2 != std::string::npos && pos1 < pos2)
		ext = file.substr(pos2, file.length());

	return ext;
}
// -----------------------------------------------------------------------------------------------

bool pathCheck(const std::wstring &file, const wchar_t *substr, const wchar_t *&program, const wchar_t *progName)
{
	bool res = file.find(substr) != std::string::npos;

	if( res )
		program = progName;

	return res;
}
// -----------------------------------------------------------------------------------------------

void shellExecFile(const std::wstring &file, const wchar_t *program = nullptr)
{
	HINSTANCE err = program ? ShellExecuteW(NULL, 0, program, file.c_str(), NULL, SW_SHOW) : ShellExecuteW(NULL, 0, file.c_str(), 0, 0 , SW_SHOW);

	if( int(err) < 32 )
	{
		doPrint("Error opening the file");
	}
}
// -----------------------------------------------------------------------------------------------

// Read data from the .ini-file
int readFromIni(const wchar_t *exeFileName, appMap &map)
{
	int res = -1;

	std::wfstream file;
	std::string error("");
	std::wstring fileName(exeFileName);
	fileName.replace(fileName.length()-3, 3, L"ini");


	map.clear();


	// If the file exists, we read data from it. Otherwise, we create it.
	file.open(fileName, std::fstream::in | std::fstream::out | std::fstream::app);

	if( file.is_open() )
	{
		size_t len, pos, lineNo = 0;
		std::wstring line, ext, command, action;

		while( std::getline(file, line) && error.empty() )
		{
			lineNo++;
			len = line.length();

			// Skip commentary, short or empty lines
			if( len > 3 && line[0] != '#' && line[0] != ';' )
			{
				if( line[0] == '[' )
				{
					command.clear();
					 action.clear();
						ext.clear();

					// get extension
					if( line[1] == '.' && line[len-1] == ']' )
					{
						ext = line.substr(1, len-2);
						continue;
					}

					// get command
					if( line[len-1] == ']' )
					{
						command = line.substr(1, len-2);
						continue;
					}
				}

				// read [options] section
				if( command == L"options" )
				{
					if( line.find(L"use_default") != std::string::npos )
						useDefault = line[len-1] == '1';

					continue;
				}

				// read [.ext] section
				if( !ext.empty() )
				{
					// read action
					if( line[0] == '/' )
					{
						action = line;
					}
					else
					{
						// depending on current action, parse the line
						if( action == L"/exec" )
						{
							pos = line.find('?');

							if( pos != std::string::npos )
							{
								std::wstring first  = line.substr(0, pos);
								std::wstring second = line.substr(pos+1, len);

								mPairPath_App p(first, second);

								// { .jpg/exec, { c:\\, c:\\program files\\word.exe } }
								map.insert(std::pair<std::wstring, mPairPath_App>(ext+action, p));
							}

							continue;
						}
						
						if( action == L"/ins" )
						{
							pos = line.find('?');

							if( pos != std::string::npos )
							{
								std::wstring first  = line.substr(pos+1, len);
								std::wstring second = line.substr(0, pos);

								mPairPath_App p(first, second);

								// { .mp3/ins, { c:\\program files\\Aimp.exe, /insert } }
								map.insert(std::pair<std::wstring, mPairPath_App>(ext+action, p));
							}

							continue;
						}
					}
				}
			}
		}

		file.close();
	}
	else
	{
		error = "Could not open .ini file";
	}

	if( !error.empty() )
	{
		map.clear();
		std::cout << "\nERROR:\n";
		doPrint(error.c_str());
		res = -1;
	}

	return res;
}
// -----------------------------------------------------------------------------------------------

int _tmain(int argc, _TCHAR* argv[])
{
	if( argc > 2 )
	{
		console = GetStdHandle(STD_OUTPUT_HANDLE);

		if( readFromIni(argv[0], mMap) )
		{
			std::wstring Action(argv[1]);

			const wchar_t *prog = nullptr;
			const wchar_t *file = nullptr;

			// Open single file
			// "shellExec.exe" /exec "!/!.!"
			if( Action == L"/exec" )
			{
				file = argv[2];

				size_t len = wcslen(file);
				std::wstring sFile;
				sFile.resize(len);

				// Copy data and make it lowercase
				std::transform(file, file+len, sFile.begin(), ::tolower);

				std::wstring sName = getName(sFile);
				std::wstring sExt  = getExtension(sFile);

				if( sExt.empty() )
				{
					doPrint("Extension is empty, file won't be opened");
				}
				else
				{
					auto rng = mMap.equal_range(sExt + Action);
		
					for(auto iter = rng.first; iter != rng.second; ++iter)
					{
						const wchar_t *pPath = iter->second.first.c_str();
						const wchar_t *pProg = iter->second.second.c_str();

						if( pathCheck(sFile, pPath, prog, pProg) )
							break;
					}

					// Execution
					if( prog || useDefault )
					{
						shellExecFile(sFile, prog);
					}
					else
					{
						doPrint(L"Parameter [use_default] is 'false', and no program associated with this path and this file type: ", sExt.c_str());
					}
				}
			}

			// Send selected files to the app
			// "shellExec.exe" /ins !/ !&
			if( Action == L"/ins" )
			{
				const wchar_t *path = argv[2];
				size_t len_p = wcslen(path);

				// for each file in the list
				for(int i = 3; i < argc; i++)
				{
					file = argv[i];

					size_t len_f = wcslen(file);
					std::wstring sFile;
					sFile.resize(len_f + len_p);

					// Get full lowercase file path
					std::transform(path, path+len_p, sFile.begin(), ::tolower);
					std::transform(file, file+len_f, sFile.begin()+len_p, ::tolower);

					std::wstring sName = getName(sFile);
					std::wstring sExt  = getExtension(sFile);
					std::wstring sProg = L"";

					if( sExt.empty() )
					{
						doPrint("Extension is empty, file won't be opened");
					}
					else
					{
						auto rng = mMap.equal_range(sExt + Action);

						for(auto iter = rng.first; iter != rng.second; ++iter)
						{
							sProg = iter->second.second;
							sFile = iter->second.first + L" " + sFile;
							break;
						}

						// Execution
						if( !sProg.empty() )
						{
							doPrint(sProg.c_str(), sFile.c_str());
							shellExecFile(sFile, sProg.c_str());
						}
						else
						{
							doPrint(L"Unknown file type: ", sExt.c_str(), true);
						}
					}
				}
			}
		}
	}

	return 0;
}
// -----------------------------------------------------------------------------------------------
