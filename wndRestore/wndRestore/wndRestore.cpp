// ==================================================================================
// === Returns some open windows to their custom preset positions defined by user ===
// ==================================================================================

#include "stdafx.h"
// -----------------------------------------------------------------------------------------------------------------------

typedef
	#ifndef UNICODE 
		std::string String;
	#else 
		std::wstring String;
	#endif

// -----------------------------------------------------------------------------------------------------------------------

bool onlyFindWindows = false;
bool cmdFlag;
int  cmdCounter = -1, foundCounter;

std::vector<int		   > wndPos;
std::vector<std::string> wndName;

// -----------------------------------------------------------------------------------------------------------------------

// Read data from the .ini-file
int readFromFile(const char *fileName, std::vector<int> &pos, std::vector<std::string> &name)
{
	int res = -1;

	enum { NAME, COORDINATES };

	std::fstream file;
	std::string error("");

	 pos.clear();
	name.clear();


	// If the file exists, we read data from it. Otherwise, we create it.
	file.open(fileName, std::ios::in | std::fstream::out);


	if( file.is_open() )
	{
		std::string line, str;

		int X, Y, W, H, cnt = NAME, lineNo = 0;

		while( std::getline(file, line) && error.empty() )
		{
			lineNo++;

			// Skip commentary and empty lines
			if( line.length() && line[0] != '#' )
			{
				switch( cnt )
				{
					case NAME: {

							name.push_back(line);
							cnt++;

						}
						break;

					case COORDINATES: {

							std::istringstream iss(line);

							if( !(iss >> X >> Y >> W >> H) )
							{
								error = "Error reading .ini file: line " + std::to_string(lineNo);
							}
							else
							{
								pos.push_back(X);
								pos.push_back(Y);
								pos.push_back(W);
								pos.push_back(H);
							}

							cnt = NAME;

						}
						break;

					default:
						;
				}
			}
		}

		res = name.size();

		file.close();
	}
	else
	{
		error = "Could not open .ini file";
	}

	if( !error.empty() )
	{
		pos.clear();
		name.clear();

		std::cout << "\nERROR: "<< error << std::endl;

		res = -1;
	}

	return res;
}
// -----------------------------------------------------------------------------------------------------------------------

// List child processes for the given process
void GetChildProcessList(DWORD dwProcessID)
{
	std::string		strProcessName;
	HANDLE			hProcessSnapshot;
	PROCESSENTRY32	processEntry32;
	
	hProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if( hProcessSnapshot != INVALID_HANDLE_VALUE )
	{
		processEntry32.dwSize = sizeof(PROCESSENTRY32);

		if( Process32First(hProcessSnapshot, &processEntry32) )
		{
			std::cout << "\tChild Processes\t:" << std::endl;

			do
			{
				if( dwProcessID == processEntry32.th32ParentProcessID )
				{
					CHAR  exe_name[1024];
					DWORD buffSize = 1024;

					HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, processEntry32.th32ProcessID);

					QueryFullProcessImageNameA(handle, 0, exe_name, &buffSize);

					CloseHandle(handle);

					std::cout << "\t\t\t  --> PID: " << processEntry32.th32ParentProcessID << "; EXE: " << exe_name << std::endl;
				}
			}
			while( strProcessName.empty() && Process32Next(hProcessSnapshot, &processEntry32) );
			
			CloseHandle(hProcessSnapshot) ;
		}
	}

	return;
}
// -----------------------------------------------------------------------------------------------------------------------

void showWndInfo(HWND hw, String title, int num, std::vector<int> &wndPos)
{
	RECT rect;

	if( GetWindowRect(hw, &rect) )
	{
		num *= 4;

		TCHAR wndClass[80];
		CHAR  exe_name[1024];
		DWORD processId, buffSize = 1024;

		// Get window class name
		GetClassName(hw, wndClass, 80);

		// Get full .exe name
		{
			GetWindowThreadProcessId(hw, &processId);

			HANDLE handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, processId);

			QueryFullProcessImageNameA(handle, 0, exe_name, &buffSize);

			CloseHandle(handle);
		}

		// Get original window position and coordinates
		int X = rect.left;
		int Y = rect.top;
		int Width  = rect.right  - X;
		int Height = rect.bottom - Y;

		std::wcout << "\tHandle\t\t: " << hw << "\n\tTitle\t\t: " << title << "\n\twndClass\t: " << wndClass << "\n\tExecutable\t: " << exe_name << std::endl;
		std::wcout.clear();

		std::cout << "\tWnd Position\t: [X:" << X << "; Y:" << Y << "; W:" << Width  << "; H:" << Height << "]";

		if( !onlyFindWindows )
		{
			// Set new window position and coordinates from our corresponding preset
			X	   = wndPos[num + 0];
			Y	   = wndPos[num + 1];
			Width  = wndPos[num + 2];
			Height = wndPos[num + 3];

			std::cout << " ===> [X:" << X << "; Y:" << Y << "; W:" << Width  << "; H:" << Height << "]" << std::endl;
		}
		else
		{
			std::cout << std::endl;
		}

		if( title.find(L"cmd.exe") != std::string::npos )
			GetChildProcessList(processId);

		if( !onlyFindWindows )
		{
			bool res;

			ShowWindow(hw, SW_RESTORE);

			// This window should be maximized on the primary (Left) Desktop
			if( X == 0 && Y == 0 && Width == 0 && Height == 0 )
			{
				res = SetWindowPos(hw, HWND_TOP, 100, 100, 1024, 800, SWP_NOREDRAW);
				ShowWindow(hw, SW_MAXIMIZE);
			}
			else
			{
				// This window should be maximized on the secondary (Right) Desktop
				if( X == -1 && Y == -1 && Width == -1 && Height == -1 )
				{
					res = SetWindowPos(hw, HWND_TOP, -1124, 100, 1024, 800, SWP_NOREDRAW);
					ShowWindow(hw, SW_MAXIMIZE);
				}
				else
				{
					// cmd.exe processes will be shown on top of each other starting from the bottom of the screen
					if( cmdFlag )
					{
						Y -= cmdCounter * 176;
					}

					// This window should be positioned using the preset coordinates
					res = SetWindowPos(hw, HWND_TOP, X, Y, Width, Height, 0);
				}
			}

			if( !res )
			{
				std::cout << "ERROR: could not move the window. Error code is '" << GetLastError() << "'" << std::endl;
			}
		}

		std::cout << std::endl;
	}
};
// -----------------------------------------------------------------------------------------------------------------------

BOOL CALLBACK enumWindowsProc(HWND hWnd, LPARAM lParam) 
{
	if( IsWindowVisible(hWnd) )
	{
		int length = ::GetWindowTextLength(hWnd);

		if( length )
		{
			length += 1;

			TCHAR* buffer = new TCHAR[length];
			memset(buffer, 0, length * sizeof(TCHAR));

			GetWindowText(hWnd, buffer, length);

			String windowTitle = String(buffer), name;

			delete[] buffer;

			bool found = false;

			// Check if we only want to display info about currently existing windows or if we really want to rearrange them
			for(int i = 0; i < wndName.size(); i++)
			{
				cmdFlag = false;

				name.resize(wndName[i].length());
				for(int j = 0; j < name.length(); j++)
					name[j] = wndName[i][j];

				if( onlyFindWindows )
				{
					foundCounter++;

					std::wcout << foundCounter << ": Found '" << windowTitle << "':" << std::endl;

					showWndInfo(hWnd, windowTitle, i, wndPos);

					std::wcout.clear();

					break;
				}
				else
				{
					if( windowTitle.find( name.c_str() ) != String::npos )
					{
						foundCounter++;

						std::wcout << foundCounter << ": Found '" << name << "':" << std::endl;

						if( windowTitle.find(L"\\cmd.exe") != String::npos )
						{
							cmdCounter++;
							cmdFlag = true;
						}

						showWndInfo(hWnd, windowTitle, i, wndPos);

						found = true;

						break;
					}
				}
			}
		}
	}

	return true;
}
// -----------------------------------------------------------------------------------------------------------------------

int _tmain(int argc, _TCHAR* argv[])
{
	bool argOk = true;
	bool doRun = true;

	// Parse arguments
	for(int i = 1; i < argc; ++i)
	{
		String arg = argv[i];

		argOk = false;

		if( arg[0] == '-' || arg[0] == '/' )
		{
			arg = arg.substr(1, arg.length()-1);

			argOk = true;

			do {

				if( arg[0] == '?' )
				{
					std::cout << " Supported arguments list:\n  /?\t: View help\n  /find\t: View list of windows that can be affected by this utility\n\n Run the program without arguments to reorder your windows" << std::endl;
					doRun = false;
					break;
				}

				if( arg == L"find" )
				{
					onlyFindWindows = true;
					break;
				}

				argOk = false;
			
			} while(false);
		}
	}


	if( !argOk )
	{
		std::cout << "ERROR:\n\tWrong command line arguments" << std::endl;
		std::cout << "\tTo view all supported arguments use '/?'" << std::endl;
		std::cout << "\tMake sure passed arguments start with '-' or '/'" << std::endl;
		std::cout << "\tOR: Start the program without any arguments" << std::endl;

		doRun = false;
	}


	if( doRun )
	{
		int num = readFromFile("wndRestore.ini", wndPos, wndName);

		if( num >= 0 )
		{
			std::cout << (num > 0 ? "Rearranging your windows...\n" : ".ini file is empty, nothing to rearrange...") << std::endl;

			// Move windows to their preset positions
			{
				foundCounter = 0;

				EnumWindows(enumWindowsProc, NULL);
			}
		}

		std::cout << "\nHit 'Enter' to Exit: ";

		getchar();
	}

	return 0;
}
// -----------------------------------------------------------------------------------------------------------------------
