// shellExec.cpp : Defines the entry point for the console application.

#include "stdafx.h"
#include "myApp.h"

// -----------------------------------------------------------------------------------------------

int _tmain(int argc, _TCHAR* argv[])
{
	myApp app;

	app.process(argc, argv);

	return 0;
}
// -----------------------------------------------------------------------------------------------
