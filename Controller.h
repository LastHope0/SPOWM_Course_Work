#pragma once
#define FLAGS_COUNT 2
#include "Drive.h"

class Controller
{
	int argc;
	char** argv;

	bool mkp;
	bool all;
	
	std::filesystem::path deletedFilePath;
	std::filesystem::path recoveredFilePath;

public:
	Controller(int argc, char* argv[])
	{
		this->argc = argc;
		this->argv = argv;
		mkp = false;
		all = false;
	}
	~Controller() = default;
	bool isHelp();
	bool parseCommandLine();
	bool start();
	
	std::list<LARGE_INTEGER> findFilesInDirectory(Drive drive, RootPath curPathInd);
};

