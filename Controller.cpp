#include "Controller.h"

bool Controller::parseCommandLine()
{
	if (argc < 3)
	{
		std::cout << "[!] Wrong parameters." << std::endl << "Add key \"--help\" for help" << std::endl;
		return false;
	}
	deletedFilePath = argv[1],
		recoveredFilePath = argv[2];

	std::string keys[] = { "-mkp", "-all" };
	for (int i = 3; i < argc; i++)
	{
		bool goodKey = false;
		for (int j = 0; j < FLAGS_COUNT; j++)
			if (!strcmp(argv[i], keys[j].c_str()))
			{
				switch (j)
				{
				case 0: mkp = true; break;
				case 1: all = true; break;
				}
				goodKey = true;
			}
		if (!goodKey)
		{
			std::cout << "[!] Wrong parameters: Unknown key \"" << argv[i] << "\"" << std::endl << "Add key \"--help\" for help" << std::endl;
			return false;
		}
		if (!deletedFilePath.has_root_path())
		{
			std::cout << "[!] Wrong parameters: Write absolute path for deleted files in first parameter." << std::endl << "Add key \"--help\" for help" << std::endl;
			return false;
		}
		if (!mkp && (!std::filesystem::exists(recoveredFilePath) || !std::filesystem::is_directory(std::filesystem::status(recoveredFilePath))))
		{
			std::cout << "[!] Path " << recoveredFilePath << " doesn't exists or isn't directory" << std::endl << "Add key \"--help\" for help" << std::endl;
			return false;
		}
		if (mkp && !std::filesystem::exists(recoveredFilePath))
			if (!std::filesystem::create_directories(recoveredFilePath))
			{
				std::cout << "[!] Couldn't create path for recovered files" << std::endl;
				return false;
			}
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
		std::cout << "Couldn't jump to the location of the starting sector of the MFT" << std::endl;
		return false;
	}
	std::cout << "[*] Parsing MFT data" << std::endl;
	if (!drive.parseMftTable()) return false;

	std::cout << "[*] Recovering deleted files ..." << std::endl;
	bool found = false;

	for (LARGE_INTEGER i = { 11 }, end = drive.mft.recordsInMft(); i.QuadPart < end.QuadPart; i.QuadPart++)
	{
		MftRecord rec = drive.readMftRec(drive.mft.getOffsetMftRec(i));
		if (rec.isClear()) return false;
		if (!rec.isValidMftEntry() || !rec.checkAndRecoverMarkers()) continue;

		std::wstring name = rec.getName();
		if (name.empty()) continue;
		std::list<MFT_RECORD_DATA_NODE> data;
		if (rec.isDeleted() && !all)
		{
			if (name == deletedFilePath.wstring())
			{
				if (rec.isDirectory())
				{
					std::cout << "[!] This is directory." << std::endl << "if you want recover files in this directory add key \" -all \"" << std::endl << "For addition information run program with 1 key \" --help \"" << std::endl;
					return false;
				}
				data = rec.getData();
				if (data.empty())
				{
					std::cout << "[!] Can't find file data" << std::endl;
					return false;
				}

				drive.recoverFile(data, recoveredFilePath / name, recoveredFilePath);
				found = true;
				break;
			}
		}
		if (deletedFilePath.filename().empty())
		{
			if (!rec.isDeleted() || rec.isDirectory()) continue;
			data = rec.getData();
			if (data.empty())
			{
				std::cout << "[!] Can't find file data" << std::endl;
				return false;
			}
			drive.recoverFile(data, recoveredFilePath / name, recoveredFilePath);
			found = true;
		}
		else
		{
			if (name != deletedFilePath.filename()) continue;
			RootPath rp;
			rp.seq_num = rec.getSeqNum();
			rp.rootPath_id = i;
			std::list<LARGE_INTEGER> deletedFilesInd = findFilesInDirectory(drive, rp);
			for (auto it = deletedFilesInd.begin(), end = deletedFilesInd.end(); it != end; it++)
			{
				MftRecord rec = drive.readMftRec(drive.mft.getOffsetMftRec(i));
				if (rec.isClear()) return false;
				if (!rec.isValidMftEntry() || !rec.checkAndRecoverMarkers()) continue;
				name = rec.getName();
				if (name.empty()) continue;
				data = rec.getData();
				if (data.empty())
				{
					std::cout << "[!] Can't find file data" << std::endl;
					return false;
				}
				drive.recoverFile(data, recoveredFilePath / name, recoveredFilePath);
				found = true;
			}
			break;
		}
	}

	if (found) std::cout << "[*] The files has been recovered successfully!" << std::endl;
	else if (!all) std::cout << "[!] Trere isn't deleted file" << std::endl;
	else std::cout << "[!] There aren't deleted files" << std::endl;

	std::cout << "[*] Closing D: drive handle..." << std::endl;
	return true;
}
bool Controller::isHelp()
{
	if (2 == argc && !strcmp(argv[1], "--help"))
	{
		printf("info");
		// дописать хелп
		return true;
	}
	return false;
}

std::list<LARGE_INTEGER> Controller::findFilesInDirectory(Drive drive, RootPath curPathInd)
{
	std::list<LARGE_INTEGER> ind_list;
	for (LARGE_INTEGER i = { 11 }, end = drive.mft.recordsInMft(); i.QuadPart < end.QuadPart; i.QuadPart++)
	{
		MftRecord rec = drive.readMftRec(drive.mft.getOffsetMftRec(i));
		if (rec.isClear()) return ind_list;
		if (!rec.isValidMftEntry() || !rec.checkAndRecoverMarkers()) continue;
		if (rec.isDirectory())
		{
			RootPath rootPath = { 0 };
			rootPath.seq_num = rec.getSeqNum();
			rootPath.rootPath_id = i;
			std::list<LARGE_INTEGER> tmp = findFilesInDirectory(drive, rootPath);
			for (auto it = tmp.begin(), end = tmp.end(); it != end; it++)
				ind_list.push_back(*it);
			continue;
		}
		if (rec.getRootPathInd() == curPathInd) ind_list.push_back(i);
	}
	return ind_list;
}