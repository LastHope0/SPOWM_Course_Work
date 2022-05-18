#pragma once
#define FLAGS_COUNT 1
#include "Drive.h"

class Controller
{
	int argc;
	char** argv;

	bool mkp;
	bool show;
	
	std::filesystem::path deletedFilePath;
	std::filesystem::path recoveredFilePath;

	bool checkAndRecoverFile(Drive drive, std::wstring name, MftRecord rec);
	bool readMftTable(Drive& drive);

public:
	Controller(int argc, char* argv[])
	{
		this->argc = argc;
		this->argv = argv;
		mkp = false;
		show = false;
	}
	~Controller() = default;
	bool isHelp();
	bool parseCommandLine();
	bool start();
	
	std::list<LARGE_INTEGER> findDeletedFilesInDirectory(Drive drive, RootPath curPathInd);
};

