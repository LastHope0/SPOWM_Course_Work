#pragma once
#include "MFT.h"
#include <filesystem>
#include <fstream>

class Drive
{
	HANDLE hDrive;
	std::wstring drivePath;

public:
	MFT mft;

	Drive() 
	{
		hDrive = INVALID_HANDLE_VALUE;
	}
	Drive(const std::wstring& drivePath) 
	{
		this->drivePath = drivePath;
	}
	Drive(const Drive& drive)
	{
		hDrive = drive.hDrive;
		drivePath = drive.drivePath;
		mft = drive.mft;
	}
	~Drive() { CloseHandle(hDrive); }

	bool open();
	bool parseVBR();
	bool parseMftTable() { return mft.initMftTable(hDrive); }
	bool setFilePointer(LARGE_INTEGER bytes, DWORD moveMethod, LARGE_INTEGER* newFilePointer);
	bool recoverFile(std::list<MFT_RECORD_DATA_NODE> data, std::wstring name, std::filesystem::path p);
	MftRecord readMftRec(LARGE_INTEGER offset) 
	{ 
		if (!setFilePointer(offset, FILE_BEGIN, NULL))
		{
			std::cout << "[!] Can't move to MFT record" << std::endl;
			return MftRecord();
		}
		return mft.readMftRec(hDrive, offset); 
	}
	LARGE_INTEGER getMftOffset() { return mft.getMftOffset(); }
};


