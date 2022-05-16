#pragma once
#include "MftRecord.h"

class MFT
{

	DWORD bytesPerSector;
	DWORD bytesPerCluster;
	DWORD bytesPerMftRecord;
	LARGE_INTEGER mftOffset;

	std::list<MFT_RECORD_DATA_NODE> mftTable;

public:
	MFT()
	{
		this->bytesPerSector = 0;
		this->bytesPerMftRecord = 0;
		this->bytesPerCluster = 0;
		this->mftOffset = { 0 };
	}
	MFT(const MFT& mft) { *this = mft; }
	~MFT() = default;

	bool initMftTable(HANDLE hDrive);
	
	LARGE_INTEGER recordsInMft() { return LARGE_INTEGER(mftTable.back().highest_vcn.QuadPart / LARGE_INTEGER(bytesPerMftRecord).QuadPart); }
	MftRecord readMftRec(HANDLE hDrive, LARGE_INTEGER offset);
	LARGE_INTEGER getOffsetMftRec(LARGE_INTEGER ind);

	void setBytesPerSector(DWORD b) { bytesPerSector = b; }
	void setBytesPerCluster(DWORD b) { bytesPerCluster = b; }
	void setBytesPerMftRecord(DWORD b) { bytesPerMftRecord = b; }
	void setMftOffset(LARGE_INTEGER b) { mftOffset = b; }

	DWORD getBytesPerSector() const { return bytesPerSector; }
	DWORD getBytesPerCluster()const { return bytesPerCluster; }
	DWORD getBytesPerMftRecord()const { return bytesPerMftRecord; }
	LARGE_INTEGER getMftOffset()const { return mftOffset; }
	

	MFT& operator=(const MFT& mft)
	{
		if (this == &mft) return *this;
		this->bytesPerSector = mft.bytesPerSector;
		this->bytesPerMftRecord = mft.bytesPerMftRecord;
		this->bytesPerCluster = mft.bytesPerCluster;
		this->mftOffset = mft.mftOffset;
		this->mftTable = mft.mftTable;
	}
};


