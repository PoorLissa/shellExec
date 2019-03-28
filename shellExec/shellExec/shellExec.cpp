// shellExec.cpp : Defines the entry point for the console application.

#include "stdafx.h"

// -----------------------------------------------------------------------------------------------

const wchar_t *iniFile = 
	L"#\n"
	L"# Options section. May contain come additional options\n"
	L"[options]\n"
	L"# Set [use_default] to 1 to be able to run default system program for your file\n"
	L"use_default=0\n\n"

	L"# File type section. Contains options for the files of this type\n"
	L"[.bat,.ini]\n"
	L"# Exec command: Opens single file using selected application\n"
	L"# Format: [path]?[app]\n"
	L"# [path] may be full, partial or empty\n"
	L"# Empty [path] provides default behaviour for the file type\n"
	L"# [app] is a full path to .exe file that will be called for this file\n"
	L"# Usage in Far's associations: \"shellExec.exe\" /exec \"!/!.!\"\n"
	L"/exec\n"
	L"\\bbb\?winword.exe\n"
	L"\\ccc\\?c:\\Users\\xbbnt9i\\AppData\\Local\\Programs\\Opera\\launcher.exe\n"
	L"\\ddd\\?cmd.exe\n"
	L"?notepad.exe\n\n"

	L"# Another file type section\n"
	L"[.mp3,.flac]\n"
	L"# Batch command: passes all selected files to the application\n"
	L"# Format: [app]?[key]\n"
	L"# [app] is a full path to .exe file that will be called for these files\n"
	L"# [key] is a command than [app] is using for batch processing\n"
	L"# Usage in Far's associations: \"shellExec.exe\" /batch !/ !&\n"
	L"/batch\n"
	L"C:\\Program Files (x86)\\AIMP\\AIMP.exe?/insert\n";

typedef std::pair<std::wstring, std::wstring>			mPairPath_App;
typedef std::multimap<std::wstring, mPairPath_App>		appMap;

appMap	mMap;
bool	useDefault = false;
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

void parseExtensions(const std::wstring &str, std::vector<std::wstring> &vec)
{
	size_t beg = 0u, end;

	do {
	
		end = str.find(',', beg);
		vec.push_back(str.substr(beg, end-beg));
		beg = end + 1;

	}
	while(end != std::string::npos);

	return;
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

// Create default .ini-file
bool createIni(const wchar_t *exeFileName)
{
	bool res = false;

	std::wfstream file;
	std::wstring fileName(exeFileName);
	fileName.replace(fileName.length()-3, 3, L"ini");

	file.open(fileName, std::fstream::in);

	if( file.is_open() )
	{
		file.close();
		doPrint(L"The file already exists", L"", true);
		doPrint(L"If you want to recreate it, delete it manually and restart the program.", L"", true);
	}
	else
	{
		// If the file does not exist, we create it
		file.open(fileName, std::fstream::in | std::fstream::out | std::fstream::app);

		// Try to create default ini-file
		if( file.is_open() )
		{
			res = true;
			file.write(iniFile, wcslen(iniFile));
			file.close();
			doPrint("Default ini-file was created");
		}
		else
		{
			std::cout << "\nERROR:\n";
			doPrint("Could not create ini-file", "", true);
		}
	}

	return res;
}
// -----------------------------------------------------------------------------------------------

// Read data from the .ini-file
bool readFromIni(const wchar_t *exeFileName, appMap &map)
{
	bool res = true;

	std::wfstream file;
	std::string error("");
	std::wstring fileName(exeFileName);
	fileName.replace(fileName.length()-3, 3, L"ini");


	map.clear();


	// If the file exists, we read data from it
	file.open(fileName, std::fstream::in);

	if( file.is_open() )
	{
		size_t len, pos, lineNo = 0;
		std::wstring line, ext, command, action;
		std::vector<std::wstring> vExt;

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
					   vExt.clear();

					// get extension
					if( line[1] == '.' && line[len-1] == ']' )
					{
						ext = line.substr(1, len-2);
						parseExtensions(ext, vExt);
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
				if( !vExt.empty() )
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

								// Create object like that: { .jpg/exec, { c:\\, c:\\program files\\word.exe } }
								for(size_t i = 0; i < vExt.size(); i++)
									map.insert(std::pair<std::wstring, mPairPath_App>(vExt[i] + action, p));
							}

							continue;
						}
						
						if( action == L"/batch" )
						{
							pos = line.find('?');

							if( pos != std::string::npos )
							{
								std::wstring first  = line.substr(pos+1, len);
								std::wstring second = line.substr(0, pos);

								mPairPath_App p(first, second);

								// Create object like that: { .mp3/batch, { c:\\program files\\Aimp.exe, /insert } }
								for(size_t i = 0; i < vExt.size(); i++)
									map.insert(std::pair<std::wstring, mPairPath_App>(vExt[i] + action, p));
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
		error = "Could not open .ini file\nRun program without parameters to recreate it.";
	}

	if( !error.empty() )
	{
		map.clear();
		std::cout << "\nERROR:\n";
		doPrint(error.c_str(), "", true);
		res = false;
	}

	return res;
}
// -----------------------------------------------------------------------------------------------

int _tmain(int argc, _TCHAR* argv[])
{
	console = GetStdHandle(STD_OUTPUT_HANDLE);

	if( argc == 1 )
	{
		doPrint("Program is started without parameters");
		doPrint("Trying to create ini-file...");
		createIni(argv[0]);
	}

	if( argc > 2 )
	{
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
			// "shellExec.exe" /batch !/ !&
			if( Action == L"/batch" )
			{
				const wchar_t *path = argv[2];
				const int FirstFileNum = 3;
				size_t len_p = wcslen(path);

				std::wstring sParams;
				std::wstring sAction;
				std::wstring sProg;
				std::wstring sFile;
				std::wstring sExt;

				// Assume average fileName length is about 33 symbols
				sParams.reserve((argc-FirstFileNum) * ( len_p + 35));

				// for each file in the list
				for(int i = FirstFileNum; i < argc; i++)
				{
					file = argv[i];

					// Get full lowercase file path
					sFile.resize(len_p + wcslen(file));
					sFile  = path;
					sFile += file;

					sExt = getExtension(sFile);
					std::transform(sExt.c_str(), sExt.c_str() + sExt.length(), sExt.begin(), ::tolower);

					if( sExt.empty() )
					{
						doPrint(L"Extension is empty, file won't be opened: ", file, true);
					}
					else
					{
						auto rng = mMap.equal_range(sExt + Action);
						auto iter = rng.first;

						if( iter == rng.second )
						{
							doPrint(L"Unsupported file type: ", file, true);
						}
						else
						{
							if( i == FirstFileNum )
							{
								sAction = iter->second.first;
								sProg   = iter->second.second;
							}

							if( sAction == iter->second.first && sProg == iter->second.second )
							{
								// Don't need sExt anymore, so using it as a tmp string
								sExt = sProg + L" " + sAction + L" ";
								doPrint(sExt.c_str(), file);
								sParams += L" " + sFile;
							}
						}
					}
				}

				if( sParams.empty() )
				{
					doPrint(L"No files found", L"", true);
				}
				else
				{
					sParams = sAction + sParams;
					shellExecFile(sParams, sProg.c_str());
				}
			}
		}
	}

	return 0;
}
// -----------------------------------------------------------------------------------------------
