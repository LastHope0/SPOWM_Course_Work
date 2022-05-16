#pragma once
#include <iostream>
#include <windows.h>
#include <list>
#include <vector>
#include <exception>
#include "NTFS_DATA_STRUCTURES.h"

class MftRecord
{
	byte* body;
	MFT_RECORD_HEADER* mftRecHead;
	
	DWORD bytesPerSector;
	DWORD bytesPerCluster;
	DWORD bytesPerMftRecord;
	LARGE_INTEGER offset;
	USHORT findAttributeHeaderOffset(const DWORD attributeType) const;
public:
	MftRecord()
	{
		this->body = nullptr;
		this->mftRecHead = nullptr;
		this->bytesPerSector = 0;
		this->bytesPerCluster = 0;
		this->bytesPerMftRecord = 0;
		this->offset = { 0 };
	}
	MftRecord(byte* body, DWORD bytesPerSector, DWORD bytesPerCluster, DWORD bytesPerMftRecord, LARGE_INTEGER offset)
	{
		this->bytesPerSector = bytesPerSector;
		this->bytesPerCluster = bytesPerCluster;
		this->bytesPerMftRecord = bytesPerMftRecord;

		this->body = new byte[bytesPerMftRecord];
		memcpy(this->body, body, bytesPerMftRecord);
		mftRecHead = (MFT_RECORD_HEADER*)body;
		this->offset = offset;
	}
	MftRecord(const MftRecord& mftRec){*this = mftRec;}
	~MftRecord(){ delete[]body; }

	bool isClear() const { return body == nullptr; }
	bool isDeleted() const { return mftRecHead->flags == MFT_RECORD_FLAGS::MFT_RECORD_NOT_USED || mftRecHead->flags == (MFT_RECORD_FLAGS::MFT_RECORD_NOT_USED | MFT_RECORD_FLAGS::MFT_RECORD_IS_DIRECTORY); }
	bool isDirectory() const { return mftRecHead->flags == MFT_RECORD_FLAGS::MFT_RECORD_IS_DIRECTORY; }
	bool isValidMftEntry() const { return body[0x0] == 0x46 && body[0x1] == 0x49 && body[0x2] == 0x4c && body[0x3] == 0x45; }
	bool isResident(USHORT attrHeaderOffset) const { 	return !body[attrHeaderOffset + 0x08] ; }
	bool checkAndRecoverMarkers();
	
	std::wstring getName() const;
	RootPath getRootPathInd() const;
	USHORT getSeqNum() const { return mftRecHead->sequence_number; };

	std::list<MFT_RECORD_DATA_NODE> getData();
	MftRecord& operator=(const MftRecord& mftRec)
	{
		if (this == &mftRec) return *this;
		this->bytesPerSector = mftRec.bytesPerSector;
		this->bytesPerCluster = mftRec.bytesPerCluster;
		this->bytesPerMftRecord = mftRec.bytesPerMftRecord;
		this->body = new byte[bytesPerMftRecord];
		memcpy(this->body, mftRec.body, bytesPerMftRecord);
		mftRecHead = (MFT_RECORD_HEADER*)body;
		this->offset = mftRec.offset;
		return *this;
	}
};


