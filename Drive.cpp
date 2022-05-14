#include "Drive.h"


bool Drive::parseVBR() {
	NTFS_BOOT_SECTOR vbr = {};
	DWORD readedBytes;
	if (!ReadFile(hDrive, &vbr, sizeof(NTFS_BOOT_SECTOR), &readedBytes, NULL) || readedBytes != sizeof(NTFS_BOOT_SECTOR)) {
		std::cout << "[!] Couldn't read the VBR" << std::endl;
		return false;
	}
	//check is drive NTFS 
	if (
		vbr.oem_id.QuadPart != 0x202020205346544E
		|| vbr.bpb.bytes_per_sector < 0x100
		|| vbr.bpb.bytes_per_sector > 0x1000
		)
	{
		std::cout << "[!] This drive isn't NTFS" << std::endl;
		return false;
	}
	//check sectors per cluster
	switch (vbr.bpb.sectors_per_cluster) {
	case 1: case 2: case 4: case 8: case 16: case 32: case 64: case 128:
		break;
	default:
		std::cout << "[!] Wrong number of sectors per cluster" << std::endl;
		return false;
	}
	mft.setBytesPerSector(vbr.bpb.bytes_per_sector);
	mft.setBytesPerCluster(mft.getBytesPerSector() * vbr.bpb.sectors_per_cluster);
	mft.setMftOffset(LARGE_INTEGER(vbr.mft_lcn.QuadPart * mft.getBytesPerCluster()));

	//calculate the size in bytes of each MFT entry
	if (vbr.clusters_per_mft_record > 0)
		mft.setBytesPerMftRecord(mft.getBytesPerSector() * mft.getBytesPerCluster() * vbr.clusters_per_mft_record);
	else
		mft.setBytesPerMftRecord(2 << ~vbr.clusters_per_mft_record);
	return true;
}

bool Drive::open()
{
	hDrive = CreateFileW(drivePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, NULL);
	if (hDrive == INVALID_HANDLE_VALUE) return false;
	return true;
}

bool Drive::setFilePointer(LARGE_INTEGER distanceToMove, DWORD moveMethod, LARGE_INTEGER* newFilePointer)
{
	LARGE_INTEGER lNewFilePointer;
	if (!SetFilePointerEx(hDrive, distanceToMove, &lNewFilePointer, moveMethod) || lNewFilePointer.QuadPart != distanceToMove.QuadPart) return false;
	if (newFilePointer)
		*newFilePointer = lNewFilePointer;
	return true;
}
MftRecord Drive::readMftRec(LARGE_INTEGER offset)
{
	if (!setFilePointer(offset, FILE_BEGIN, NULL))
	{
		std::cout << "[!] Can't move to " << offset.QuadPart << " MFT record" << std::endl;
		return MftRecord();
	}
	return mft.readMftRec(hDrive, offset);
}
bool Drive::recoverFile(std::list<MFT_RECORD_DATA_NODE> data, std::wstring name, std::filesystem::path p)
{
	std::ofstream f;
	std::filesystem::path tempPath = p;
	tempPath /= name;

	int i = 0;
	while (std::filesystem::exists(tempPath))
	{
		i++;
		tempPath = p;
		tempPath /= std::wstring(L"(") + std::to_wstring(i) + std::wstring(L")");
		tempPath += name;
	}
	
	f.open(tempPath.c_str(), std::ios_base::out | std::ios_base::binary);
	if (!f.is_open() || !f.good())
	{
		std::cout << "[!] Can't recover \"";
		std::wcout << name;
		std::cout<< "\" file" << std::endl;
		return false;
	}
	if (data.empty())
	{
		std::cout << "[!] Can't get data from \"";
		std::wcout << name;
		std::cout << "\" record" << std::endl;
		return false;
	}
	LARGE_INTEGER readedBytes = {0};
	DWORD bytesInBuffer = 0, bytesToWrite = 0;
	DWORD bufferSize = this->mft.getBytesPerSector() * 1000;
	char* buffer = new char[bufferSize];
	for (auto it = data.begin(), end = data.end(); it != end; it++)
	{
		readedBytes.QuadPart = 0;
		if (!setFilePointer(it->offset, FILE_BEGIN, NULL))
		{
			std::cout << "[!] Can't write \"";
			std::wcout << name;
			std::cout << "\" data" << std::endl;
			return false;
		}
		while (readedBytes.QuadPart < it->len.QuadPart)
		{
			if (it->len.QuadPart - readedBytes.QuadPart < bufferSize)
				bytesToWrite = it->len.QuadPart - readedBytes.QuadPart;
			else
				bytesToWrite = bufferSize;

			if (!ReadFile(hDrive, buffer, bufferSize, &bytesInBuffer, NULL) || bytesInBuffer != bufferSize) {
				std::cout << "[!] Couldn't read the \"";
				std::wcout << name;
				std::cout << "\" record data" << std::endl;
				return false;
			}
			readedBytes.QuadPart += bytesToWrite;
			f.write(buffer + it->startByte, bytesToWrite);
		}
	}
	f.close();
	return true;
}

