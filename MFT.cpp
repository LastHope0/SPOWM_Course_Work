#include "MFT.h"

bool MFT::initMftTable(HANDLE hDrive)
{
	MFT_RECORD_DATA_NODE node = {};
	MftRecord mftTableRec = readMftRec(hDrive, mftOffset);
	if (mftTableRec.isClear() || !mftTableRec.isValidMftEntry() || !mftTableRec.checkAndRecoverMarkers())
	{
		std::cout << "[!] Record $MFT is corrypted" << std::endl;
		return false;
	}
	this->mftTable = mftTableRec.getData();
	if (this->mftTable.empty())
	{
		std::cout << "[!] Can't read $MFT data" << std::endl;
		return false;
	}
	return true;
}

MftRecord MFT::readMftRec(HANDLE hDrive, LARGE_INTEGER offset)
{
	MftRecord mftRec;
	byte* buffer = new byte[bytesPerMftRecord];
	DWORD readedBytes = 0;
	if (!ReadFile(hDrive, buffer, bytesPerMftRecord, &readedBytes, NULL) || readedBytes != bytesPerMftRecord) {
		std::cout << "[!] Couldn't read "<<offset.QuadPart <<" MFT record" << std::endl;
		return mftRec;
	}
	
	mftRec = MftRecord(buffer, bytesPerSector, bytesPerCluster, bytesPerMftRecord, offset);
	delete []buffer;
	return mftRec;
}

LARGE_INTEGER MFT::getOffsetMftRec(LARGE_INTEGER ind)
{
	LARGE_INTEGER recOff = {0};
	ind.QuadPart *= bytesPerMftRecord;
	auto i = mftTable.begin(), end = mftTable.end();
	for (; i != end && !(ind.QuadPart >= i->lowest_vcn.QuadPart && ind.QuadPart <= i->highest_vcn.QuadPart); i++);

	if(ind.QuadPart >= i->lowest_vcn.QuadPart && ind.QuadPart <= i->highest_vcn.QuadPart)
		recOff.QuadPart = i->offset.QuadPart+ind.QuadPart - i->lowest_vcn.QuadPart;
	
	return recOff;
}

