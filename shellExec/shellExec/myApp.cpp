#include "stdafx.h"
#include "myApp.h"



#include <codecvt>
#include <cvt/wstring>
#include <iostream>
#include <locale>
#include <string>
#include <codecvt>

HANDLE myApp::console = nullptr;

const wchar_t *myApp::iniFile = 
	L"#\n"
	L"# Options section. May contain come additional options\n"
	L"[options]\n"
	L"# Set [use_default] to 1 to be able to run default system program for your file\n"
	L"use_default=0\n"
	L"max_cmd_line_len=8000\n\n"

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
	L"# [key] is a command that [app] is using for batch processing\n"
	L"# Usage in Far's associations: \"shellExec.exe\" /batch !/ !&\n"
	L"/batch\n"
	L"C:\\Program Files (x86)\\AIMP\\AIMP.exe?/insert\n"
	L"/batchFile\n"
	L"# Batch command: the same as /batch, but passes single tmp file with all selected file names\n"
	L"# Usage in Far's associations: \"shellExec.exe\" /batchFile !/ !@!\n"
	L"C:\\Program Files (x86)\\AIMP\\AIMP.exe?/insert\n";

// -----------------------------------------------------------------------------------------------

myApp::myApp()
{
	useDefault = false;
	maxCmdLineLen = 2000u;
	console = GetStdHandle(STD_OUTPUT_HANDLE);
}
// -----------------------------------------------------------------------------------------------

myApp::~myApp()
{
}
// -----------------------------------------------------------------------------------------------

bool myApp::doUseDefault()
{
	return useDefault;
}
// -----------------------------------------------------------------------------------------------

appMapRange myApp::getRange(std::wstring &ext, std::wstring &action)
{
	return mMap.equal_range(ext + action);
}
// -----------------------------------------------------------------------------------------------

void myApp::doPrint(const char *str, const char *param, bool isYellow)
{
	if( isYellow )
		std::cout << yellow;
	else
		std::cout << green;

	std::cout << " --> " << str << param << white << std::endl;
}
// -----------------------------------------------------------------------------------------------

void myApp::doPrint(const wchar_t *str, const wchar_t *param, bool isYellow)
{
	if( isYellow )
		std::wcout << yellow;
	else
		std::wcout << green;
	
	std::wcout << " --> " << str << param << white << std::endl;
}
// -----------------------------------------------------------------------------------------------

std::wstring myApp::getName(const std::wstring &file)
{
	size_t pos = file.rfind('\\');

	return file.substr(pos + 1, file.length());
}
// -----------------------------------------------------------------------------------------------

std::wstring myApp::getExtension(const std::wstring &file)
{
	std::wstring ext = L"";

	size_t pos1 = file.rfind('\\');
	size_t pos2 = file.rfind('.');

	if( pos2 != std::string::npos && pos1 < pos2)
		ext = file.substr(pos2, file.length());

	return ext;
}
// -----------------------------------------------------------------------------------------------

void myApp::parseExtensions(const std::wstring &str, std::vector<std::wstring> &vec)
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

bool myApp::pathCheck(const std::wstring &file, const wchar_t *substr, const wchar_t *&program, const wchar_t *progName)
{
	bool res = file.find(substr) != std::string::npos;

	if( res )
		program = progName;

	return res;
}
// -----------------------------------------------------------------------------------------------

void myApp::shellExecFile(const std::wstring &file, const wchar_t *program)
{
	HINSTANCE err = program ? 
		ShellExecuteW(NULL, 0, program, file.c_str(), NULL, SW_SHOW)
		:
		ShellExecuteW(NULL, 0, file.c_str(), 0, 0 , SW_SHOW);

	if( int(err) < 32 )
	{
		doPrint("Error opening the file");
	}
}
// -----------------------------------------------------------------------------------------------

// Create default .ini-file
bool myApp::createIni(const wchar_t *exeFileName)
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
bool myApp::readFromIni(const wchar_t *exeFileName)
{
	bool res = true;

	std::wfstream file;
	std::string error("");
	std::wstring fileName(exeFileName);
	fileName.replace(fileName.length()-3, 3, L"ini");


	mMap.clear();


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

					if( line.find(L"max_cmd_line_len") != std::string::npos )
					{
						pos = line.find('=');
						line = line.substr(pos+1, line.length());
						size_t val = _wtoi(line.c_str());

						maxCmdLineLen = (val > 1000 && val < 10000) ? val : maxCmdLineLen;
					}

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

								appPair p(first, second);

								// Create object like that: { .jpg/exec, { c:\\, c:\\program files\\word.exe } }
								for(size_t i = 0; i < vExt.size(); i++)
									mMap.insert(std::pair<std::wstring, appPair>(vExt[i] + action, p));
							}

							continue;
						}
						
						if( action == L"/batch" || action == L"/batchFile" )
						{
							pos = line.find('?');

							if( pos != std::string::npos )
							{
								std::wstring first  = line.substr(pos+1, len);
								std::wstring second = line.substr(0, pos);

								appPair p(first, second);

								// Create object like that: { .mp3/batch, { c:\\program files\\Aimp.exe, /insert } }
								for(size_t i = 0; i < vExt.size(); i++)
									mMap.insert(std::pair<std::wstring, appPair>(vExt[i] + action, p));
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
		mMap.clear();
		std::cout << "\nERROR:\n";
		doPrint(error.c_str(), "", true);
		res = false;
	}

	return res;
}
// -----------------------------------------------------------------------------------------------

void myApp::process(int argc, _TCHAR* argv[])
{
	if( argc == 1 )
	{
		doPrint("Program is started without parameters");
		doPrint("Trying to create ini-file...");
		createIni(argv[0]);
	}

	if( argc > 2 )
	{
		if( readFromIni(argv[0]) )
		{
			const wchar_t *prog = nullptr;
			const wchar_t *file = nullptr;

			std::wstring Action(argv[1]);

			// Open single file ["shellExec.exe" /exec "!/!.!"]
			if( Action == L"/exec" )
			{
				processExec(argc, argv, prog, file, Action);
			}

			// Send selected files to the app ["shellExec.exe" /batch !/ !@!]
			if( Action == L"/batchFile" )
			{
				processBatchFile(argc, argv, prog, file, Action);
			}

			// Send selected files to the app ["shellExec.exe" /batch !/ !&]
			if( Action == L"/batch" )
			{
				processBatch(argc, argv, prog, file, Action);
			}
		}
	}

	return;
}
// -----------------------------------------------------------------------------------------------

void myApp::processExec(int argc, _TCHAR* argv[], const wchar_t* prog, const wchar_t* file, std::wstring &Action)
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
		auto rng = getRange(sExt, Action);
		
		for(auto iter = rng.first; iter != rng.second; ++iter)
		{
			const wchar_t *pPath = iter->second.first.c_str();
			const wchar_t *pProg = iter->second.second.c_str();

			if( pathCheck(sFile, pPath, prog, pProg) )
				break;
		}

		// Execution
		if( prog || doUseDefault() )
		{
			sFile = L"\"" + sFile + L"\"";
			shellExecFile(sFile, prog);
		}
		else
		{
			doPrint(L"Parameter [use_default] is 'false', and no program is associated with this path/file type: ", sExt.c_str(), true);
		}
	}

	return;
}
// -----------------------------------------------------------------------------------------------

void myApp::processBatch(int argc, _TCHAR* argv[], const wchar_t* prog, const wchar_t* file, std::wstring &Action)
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

		// Get full file path
		sFile.reserve(len_p + wcslen(file));
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
			auto rng = getRange(sExt, Action);
			auto iter = rng.first;

			if( iter == rng.second )
			{
				doPrint(L"Unsupported file type: ", file, true);
			}
			else
			{
				if( sAction.empty() )
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

	return;
}
// -----------------------------------------------------------------------------------------------

// Send selected files to the app ["shellExec.exe" /batch !/ !@!]
// Command line length can't be greater than some finite number (2k or 8k).
// If Far sends many files, sometimes we'll get error "Cmd line is too long".
// Happily, Far can copy all selected file names to tmp file and give us the file's name.
// Far will delete this file when the app exits.
void myApp::processBatchFile(int argc, _TCHAR* argv[], const wchar_t* prog, const wchar_t* tmpFile, std::wstring &Action)
{
	auto do_skip_BOM = [](std::fstream &f)
	{
		// The UTF-8 representation of the BOM is the(hexadecimal) byte sequence 0xEF, 0xBB, 0xBF
		char a = f.get();
		char b = f.get();
		char c = f.get();

		if (a != (char)(0xEF) || b != (char)(0xBB) || c != (char)(0xBF))
			f.seekg(0);
	};


	std::wstring err;
	std::vector<std::wstring> vec, vecErr;
	const wchar_t *path = nullptr;
	size_t len_p;

	if( argc != 4 )
	{
		err = L"Wrong number of parameters (" + Action + L")\nParams are:\n";

		for(int i = 0; i < argc; i++)
			err += std::wstring(argv[i]) + L"\n";
	}

	if( err.empty() )
	{
		path	= argv[2];
		tmpFile = argv[3];
		len_p	= wcslen(path);

		std::fstream	file;
		std::string		line;

		// If the file exists, we read data from it
		file.open(tmpFile, std::fstream::in);

		if( file.is_open() )
		{
			wchar_t buf[MAX_PATH];


			// Now that we are making Far Manager to save the file as UTF-8, we need to check and skip the BOM record
			do_skip_BOM(file);


			while( std::getline(file, line) )
			{
				if( line.length() > 3 )
				{
					// Without this cyrillic symbols won't work here
					UINT codePage = CP_UTF8;													// was CP_OEMCP for Far's !@! -- now we use !@U!

					if (MultiByteToWideChar(codePage, 0, line.c_str(), -1, buf, MAX_PATH))
					{
						vec.emplace_back(buf);
					}
				}
			}

			file.close();

			if( !vec.size() )
				err = L"No files found";
		}
		else
		{
			err  = L"Could not open the file with file names: ";
			err += tmpFile;
		}
	}

	if( err.empty() )
	{
		std::wstring sParams, sAction, sProg, sFile, sExt;

		char bufFile[MAX_PATH];

		const size_t lenMax = maxCmdLineLen;
		sParams.reserve(lenMax);

		// for each file in the list
		for(size_t i = 0u; i < vec.size(); i++)
		{
			const wchar_t *file = vec[i].c_str();

			sFile  = path;
			sFile += file;
			sExt  = getExtension(sFile);
			std::transform(sExt.c_str(), sExt.c_str() + sExt.length(), sExt.begin(), ::tolower);

			if( sExt.empty() )
			{
				vecErr.push_back(file);
				doPrint(L"Extension is empty, file won't be opened: ", sFile.c_str(), true);
			}
			else
			{
				auto rng = getRange(sExt, Action);
				auto iter = rng.first;

				if( iter == rng.second )
				{
					vecErr.push_back(file);
					doPrint(L"Unsupported file type: ", file, true);
				}
				else
				{
					if( sAction.empty() )
					{
						sAction = iter->second.first;
						sProg   = iter->second.second;
						sParams = sAction;
					}

					if( sAction == iter->second.first && sProg == iter->second.second )
					{
						// Don't need sExt anymore, so using it as a tmp string
						sExt = sProg + L" " + sAction + L" " + file;

						// Used only for the printing purposes, no actual data afected
						WideCharToMultiByte(CP_INSTALLED, 0, sExt.c_str(), -1, bufFile, MAX_PATH, NULL, NULL);

						doPrint(bufFile);

						size_t lenParams = sParams.length();
						size_t lenFile   = sFile.length() + 1;

						if( lenParams + lenFile >= lenMax )
						{
							char buf[32];

							// Executing
							shellExecFile(sParams, sProg.c_str());

							doPrint("");
							_itoa_s(lenMax, buf, 32, 10);
							doPrint("Parameters string reached its max length: ", buf);

							_itoa_s(vec.size() - i, buf, 32, 10);
							doPrint("Files left: ", buf);
							doPrint(L"Sending files to ", sProg.c_str());
							doPrint("");

							sParams = sAction;

							// Too fast consequent calls for ShellExecute may lead to unpredictable behaviour
							// Let's wait a little bit between the calls
							for(int i = 0; i < 20; i++)
							{
								std::cout << green << " ." << white;
								Sleep(50);
							}
						}

						sParams += L" " + sFile;
					}
				}
			}
		}

		// Executing one last time
		if( sParams.length() > Action.length() )
		{
			shellExecFile(sParams, sProg.c_str());
		}
	}

	// Some files were not processed (empty extension, not supported type)
	if( vecErr.size() )
	{
		char buf[32];
		_itoa_s(vecErr.size(), buf, 32, 10);

		doPrint(buf, " file(s) not processed:", true);

		for(size_t i = 0; i < vecErr.size(); i++)
			doPrint(L"\t", vecErr[i].c_str(), true);
	}

	// Fatal error was encountered
	if( !err.empty() )
	{
		size_t pos0(0u), pos1 = err.find('\n');

		doPrint(L"ERROR: ", err.substr(pos0, pos1 - pos0).c_str(), true);

		do {

			pos0 = pos1+1;
			pos1 = err.find('\n', pos1+1);

			doPrint(err.substr(pos0, pos1 - pos0).c_str(), L"", true);
			pos0 = pos1;
		}
		while(pos1 != std::string::npos);

		doPrint(L"Press Enter to Exit");

		getchar();
	}

	return;
}
// -----------------------------------------------------------------------------------------------
