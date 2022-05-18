#include "Controller.h"

bool Controller::parseCommandLine()
{
	if (argc < 3)
	{
		std::cout << "[!] Wrong parameters." << std::endl << "Add key \"--help\" for help" << std::endl;
		return false;
	}
	const char invalidSym[] = ":*?«<>|.";
	int i = 1;
	deletedFilePath = argv[i++];
	for (; argv[i] && !strpbrk(argv[i], invalidSym)  && strcmp("-show", argv[i]) && strcmp("-mkp", argv[i]); i++)
	{
		deletedFilePath += " ";
		deletedFilePath += argv[i];
	}
	if(argv[i] && strcmp("-show", argv[i]) && strcmp("-mkp", argv[i]))
		recoveredFilePath = argv[i++];
	for (; argv[i] && !strpbrk(argv[i], invalidSym) &&  strcmp("-show", argv[i]) && strcmp("-mkp", argv[i]); i++)
	{
		recoveredFilePath += " ";
		recoveredFilePath += argv[i];
	}
		
	if (argv[i] && !strcmp(argv[i], "-show"))
	{
		show = true;
		return true;
	}
	std::string keys[] = { "-mkp" };
	for (; i < argc; i++)
	{
		bool goodKey = false;
		for (int j = 0; j < FLAGS_COUNT; j++)
			if (!strcmp(argv[i], keys[j].c_str()))
			{
				switch (j)
				{
				case 0: mkp = true; break;
				}
				goodKey = true;
			}
		if (!goodKey)
		{
			if (!strcmp(argv[i], "-show") || !strcmp(argv[i], "--help"))
				std::cout << "Key \"" << argv[i] << "\" should be instead [OPTION2]" << std::endl;
			else
				std::cout << "[!] Wrong parameters: Unknown key \"" << argv[i] << "\"" << std::endl;
			std::cout << "Add key \"--help\" for help" << std::endl;
			return false;
		}
	}
	if (!deletedFilePath.has_root_path() || deletedFilePath.string().size() < 3 || deletedFilePath.root_path().string().size() < 3)
	{
		std::cout << "[!] Wrong parameters: Write absolute path for deleted files in first parameter." << std::endl << "Add key \"--help\" for help" << std::endl;
		return false;
	}
	if (!mkp && !show && (!std::filesystem::exists(recoveredFilePath) || !std::filesystem::is_directory(std::filesystem::status(recoveredFilePath))))
	{
		std::cout << "[!] Path " << recoveredFilePath << " doesn't exists or isn't directory" << std::endl << "Add key \"--help\" for help" << std::endl;
		return false;
	}
	if (mkp && !show && !std::filesystem::exists(recoveredFilePath))
		if (!std::filesystem::create_directories(recoveredFilePath))
		{
			std::cout << "[!] Couldn't create path for recovered files" << std::endl;
			return false;
		}
}
bool Controller::start()
{
	std::wstring drivePath = L"\\\\.\\";
	drivePath += deletedFilePath.root_name().c_str();

	Drive drive(drivePath);

	std::cout << "[*] Opening " << deletedFilePath.root_name() << ": drive ..." << std::endl;
	if (!drive.open()) {
		std::cout << "[!] Can't open " << deletedFilePath.root_name() << ": drive ..." << std::endl;
		return false;
	}

	std::cout << "[*] Reading the VBR ..." << std::endl;
	if (!drive.parseVBR()) return false;

	std::cout << "[*] Jumping to the location of the starting sector of the MFT" << std::endl;
	if (!drive.setFilePointer(drive.getMftOffset(), FILE_BEGIN, NULL))
	{
		std::cout << "[!] Couldn't jump to the location of the starting sector of the MFT" << std::endl;
		return false;
	}
	std::cout << "[*] Parsing MFT data" << std::endl;
	if (!drive.parseMftTable()) return false;

	if (show) std::cout << "[*] Deleted files : " << std::endl;
	else
		std::cout << "[*] Recovering deleted files ..." << std::endl;
	readMftTable(drive);
	std::cout << "[*] Closing D: drive handle..." << std::endl;
	drive.close();
	return true;
}

bool Controller::readMftTable(Drive& drive)
{
	bool found = false;
	for (LARGE_INTEGER i = { 16 }, end = drive.mft.recordsInMft(); i.QuadPart < end.QuadPart; i.QuadPart++)
	{
		MftRecord rec = drive.readMftRec(drive.mft.getOffsetMftRec(i));
		if (rec.isClear() || !rec.getSeqNum() || !rec.isValidMftEntry() || !rec.checkAndRecoverMarkers()) continue;
		std::wstring name = rec.getName();
		if (name.empty()) continue;
		if (deletedFilePath.string().size() == 3)
		{
			if (checkAndRecoverFile(drive, name, rec))
				found = true;
		}
		else
		{
			if (deletedFilePath.filename() != name) continue;
			if (rec.isDirectory())
			{
				RootPath rp;
				rp.seq_num = rec.getSeqNum();
				rp.rootPath_id = i;
				if (rec.isDeleted()) --rp.seq_num;
				std::list<LARGE_INTEGER> deletedFilesInd = findDeletedFilesInDirectory(drive, rp);
				for (auto it = deletedFilesInd.begin(), end = deletedFilesInd.end(); it != end; it++)
				{
					MftRecord rec = drive.readMftRec(drive.mft.getOffsetMftRec(*it));
					if (rec.isClear() || !rec.getSeqNum() || !rec.isValidMftEntry() || !rec.checkAndRecoverMarkers()) continue;

					std::wstring name = rec.getName();
					if (name.empty()) continue;
					if (checkAndRecoverFile(drive, name, rec))
						found = true;
				}
			}
			else if (checkAndRecoverFile(drive, name, rec))
				found = true;
		}
	}
	if (!show)
		if (found)
		{
			std::cout << "[*] The files have been recovered successfully!" << std::endl;
			return true;
		}
		else
		{
			std::cout << "[!] There aren't deleted files" << std::endl;
			return false;
		}
}	

bool Controller::checkAndRecoverFile(Drive drive, std::wstring name, MftRecord rec)
{
	std::list<MFT_RECORD_DATA_NODE> data;
	if (!rec.isDeleted() || rec.isDirectory()) return false;

	if (show)
	{
		std::wcout << name << std::endl;
		std::wcout.clear();
		return false;
	}
	data = rec.getData();
	if (data.empty())
	{
		std::cout << "[!] Can't find \"";
		std::wcout << name;
		std::cout << "\" file data" << std::endl;
		return false;
	}
	if (drive.recoverFile(data, name, recoveredFilePath))
		return true;

	return false;
}
bool Controller::isHelp()
{
	if (2 == argc && !strcmp(argv[1], "--help"))
	{
		system("cls");

		std::cout << std::endl << "NAME" << std::endl << std::endl;
		std::cout << " ntfsUndelete - Utility for recovering files from drive with filesistem NTFS" << std::endl << std::endl;
		std::cout << "SYNOPSIS" << std::endl << std::endl;
		std::cout << " ntfsUndelete [OPTION1] [OPTION2] [KEYS]" << std::endl << std::endl;
		std::cout << "DESCRIPTION" << std::endl << std::endl;
		std::cout << " [OPTION1] - string like \"driveLetter:\\fileName\"." << std::endl;
		std::cout << "              driveLetter - drive letter to search for deleted files" << std::endl;
		std::cout << "              fileName - the name of the remote file or directory. If the file name of the directory, all files belonging to it will be recursively restored" << std::endl << std::endl;
		std::cout << " [OPTION2] - path for recovered files" << std::endl << std::endl;
		std::cout << "KEYS" << std::endl << std::endl;
		std::cout << " --help - output user manual" << std::endl;
		std::cout << " -show - only show files names" << std::endl;
		std::cout << " -mkp - make path. Ñreates a recovery path if it does not exist" << std::endl << std::endl;
		return true;
	}
	return false;
}


std::list<LARGE_INTEGER> Controller::findDeletedFilesInDirectory(Drive drive, RootPath curPathInd)
{
	std::list<LARGE_INTEGER> ind_list;
	for (LARGE_INTEGER i = { 16 }, end = drive.mft.recordsInMft(); i.QuadPart < end.QuadPart; i.QuadPart++)
	{
		MftRecord rec = drive.readMftRec(drive.mft.getOffsetMftRec(i));
		if (rec.isClear() || !rec.isValidMftEntry() || !rec.checkAndRecoverMarkers()) continue;

		if (rec.getRootPathInd() != curPathInd) continue;
		if (rec.isDirectory())
		{
			RootPath rootPath = { 0 };
			rootPath.seq_num = rec.getSeqNum();
			rootPath.rootPath_id = i;
			if (rec.isDeleted()) --rootPath.seq_num;
			std::list<LARGE_INTEGER> tmp = findDeletedFilesInDirectory(drive, rootPath);
			for (auto it = tmp.begin(), end = tmp.end(); it != end; it++)
				ind_list.push_back(*it);
		}
		else if (rec.isDeleted()) ind_list.push_back(i);
	}
	return ind_list;
}