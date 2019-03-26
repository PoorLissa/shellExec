// shellExec.cpp : Defines the entry point for the console application.

#include "stdafx.h"


// -----------------------------------------------------------------------------------------------

typedef std::pair<std::wstring, std::wstring>			mPairPath_App;
typedef std::multimap<std::wstring, mPairPath_App>		appMap;

appMap mMap;
bool useDefault = false;

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
		std::cout << "-- Error opening the file: " << err << std::endl;
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
		std::wstring line, ext;
		bool isOptions = false;

		while( std::getline(file, line) && error.empty() )
		{
			lineNo++;
			len = line.length();

			// Skip commentary and empty lines
			if( len > 3 && line[0] != '#' && line[0] != ';' )
			{
				// read options
				if( line == L"[options]" )
				{
					isOptions = true;
					continue;
				}

				if( isOptions )
				{
					if( line.find(L"use_default") != std::string::npos )
					{
						useDefault = line[len-1] == '1';
					}
				}

				// get extension
				if( line[0] == '[' && line[1] == '.' && line[len-1] == ']' )
				{
					isOptions = false;
					ext = line.substr(1, len-2);
					continue;
				}

				pos = line.find('?');

				if( pos != std::string::npos )
				{
					isOptions = false;
					std::wstring first  = line.substr(0, pos);
					std::wstring second = line.substr(pos+1, len);

					mPairPath_App p(first, second);

					map.insert(std::pair<std::wstring, mPairPath_App>(ext, p));
					continue;
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
		std::cout << "\nERROR: "<< error << std::endl;
		res = -1;
	}

	return res;
}
// -----------------------------------------------------------------------------------------------

int _tmain(int argc, _TCHAR* argv[])
{
	if( argc != 2 )
	{
		std::cout << "-- Wrong number of parameters!" << std::endl;
	}
	else
	{
		if( readFromIni(argv[0], mMap) )
		{
			const wchar_t *file = argv[1];
			const wchar_t *prog = nullptr;

			size_t len = wcslen(file);
			std::wstring sFile;
			sFile.resize(len);

			// Copy data and make it lowercase
			std::transform(file, file+len, sFile.begin(), ::tolower);

			std::wstring sName = getName(sFile);
			std::wstring sExt  = getExtension(sFile);

			if( sExt.empty() )
			{
				std::cout << "-- Extension is empty, file won't be opened" << std::endl;
			}
			else
			{
				auto rng = mMap.equal_range(sExt);
		
				for(auto iter = rng.first; iter != rng.second; ++iter)
				{
					std::wstring *pPath = &(iter->second.first);
					std::wstring *pProg = &(iter->second.second);

					if( pathCheck(sFile, pPath->c_str(), prog, pProg->c_str()) )
						break;
				}

				// Execution
				if( prog || useDefault )
				{
					shellExecFile(sFile, prog);
				}
				else
				{
					std::cout << "-- No program found and [use_default] parameter is set to '0'." << std::endl;
				}
			}
		}
	}

	return 0;
}
// -----------------------------------------------------------------------------------------------
