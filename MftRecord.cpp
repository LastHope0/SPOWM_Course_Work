#include "MftRecord.h"


bool MftRecord::checkAndRecoverMarkers()
{
	if (isClear())
		throw std::bad_weak_ptr();
	if (!mftRecHead->usa_count)return true;
	USHORT* arrMarkers = (USHORT*)(body+mftRecHead->usa_offs);
	USHORT signature = *arrMarkers;
	++arrMarkers;
	USHORT arrMarkersLen = mftRecHead->usa_count;
	--arrMarkersLen;
	USHORT temp;
	for (DWORD i = 1; i <= arrMarkersLen; i++)
	{
		temp = *(USHORT*)(body + (i * bytesPerSector - 2));
		if (temp != signature)
		{
			std::cout << "[!] $MFT record was corrupted" << std::endl;
			return false;
		}
		*(USHORT*)(body + (i * bytesPerSector - 2)) = arrMarkers[i-1];
	}
	return true;
}

USHORT MftRecord::findAttributeHeaderOffset(const DWORD attributeTypeCode) const{
	//retrieve the first attribute header offset from the MFT entry header
	if (isClear())
		throw std::bad_weak_ptr();
	USHORT attributeHeaderOffset = 0;
	memcpy(&attributeHeaderOffset, body + 0x14, sizeof(USHORT));

	DWORD currentAttributeTypeCode = 0;
	memcpy(&currentAttributeTypeCode, body + attributeHeaderOffset, sizeof(DWORD));

	DWORD attributeSize;
	//while the current attribute type code isn't the one we are looking for and isn't the one that indicates the end of the attributes
	while (currentAttributeTypeCode != attributeTypeCode && currentAttributeTypeCode != ATTR_TYPES::AT_END) {
		attributeSize = 0;
		memcpy(&attributeSize, body + attributeHeaderOffset + 0x04, sizeof(DWORD));

		attributeHeaderOffset += attributeSize; //move to the next attribute
		currentAttributeTypeCode = 0;
		memcpy(&currentAttributeTypeCode, body + attributeHeaderOffset, sizeof(DWORD)); //retrieve the next attribute type code
	}

	if (currentAttributeTypeCode == ATTR_TYPES::AT_END) { //if there is no attribute of attributeType
		return 0;
	}
	return attributeHeaderOffset;
}

std::wstring MftRecord::getName()const{
	std::wstring namestr;
	USHORT fileNameAttributeHeaderOffset = findAttributeHeaderOffset(ATTR_TYPES::AT_FILE_NAME);
	if (fileNameAttributeHeaderOffset == 0) { //if there is no $FILE_NAME attribute in the MFT entry
		return namestr;
	}
	ATTR_RECORD_HEADER* attrHeader = (ATTR_RECORD_HEADER*)(body + fileNameAttributeHeaderOffset);
	size_t size = body[fileNameAttributeHeaderOffset + attrHeader->u.r.value_offset + 64];
	if (!size) return namestr;

	wchar_t* name = new wchar_t[size + 1];
	memcpy(name, body + fileNameAttributeHeaderOffset + attrHeader->u.r.value_offset + 66, size * 2);
	name[size] = L'\0';
	
	namestr = name;
	delete[]name;
	return namestr;
}

RootPath MftRecord::getRootPathInd() const
{
	RootPath rootPath = { 0 };
	LARGE_INTEGER root = { 0 };
	USHORT fileNameAttributeHeaderOffset = findAttributeHeaderOffset(ATTR_TYPES::AT_FILE_NAME);
	if (fileNameAttributeHeaderOffset == 0) { //if there is no $FILE_NAME attribute in the MFT entry
		return rootPath;
	}
	ATTR_RECORD_HEADER* attrHeader = (ATTR_RECORD_HEADER*)(body + fileNameAttributeHeaderOffset);
	
	memcpy(&root, body + fileNameAttributeHeaderOffset + attrHeader->u.r.value_offset + 0, 8);
	rootPath.seq_num = root.QuadPart >> 6;
	rootPath.rootPath_id.QuadPart = root.QuadPart & 0x0000ffffffffffff;
	return rootPath;
}

std::list<MFT_RECORD_DATA_NODE> MftRecord::getData()
{
	std::list<MFT_RECORD_DATA_NODE> data;
	MFT_RECORD_DATA_NODE node = {};
	USHORT fileDataAttributeHeaderOffset = findAttributeHeaderOffset(ATTR_TYPES::AT_DATA);
if (fileDataAttributeHeaderOffset == 0) {
		return data;
	}
	ATTR_RECORD_HEADER* attrHeader = (ATTR_RECORD_HEADER*)(body + fileDataAttributeHeaderOffset);
	if (isResident(fileDataAttributeHeaderOffset))
	{
		node.highest_vcn = { 0 };
		node.lowest_vcn = { 0 };
		node.offset.QuadPart = offset.QuadPart + fileDataAttributeHeaderOffset + attrHeader->u.r.value_offset;
		node.len.QuadPart = attrHeader->u.r.value_length;
		data.push_back(node);
	}
	else
	{
		LARGE_INTEGER tempOffsetToData = {0};
		USHORT offsetOfData = fileDataAttributeHeaderOffset + attrHeader->u.nr.mapping_pairs_offset;		// смещение списка серий
		byte numOfLcnBytes = (body[offsetOfData] & 0xF0) >> 4;							//retrieve the upper nibble of the value size tuple
		byte numOfLenBytes = body[offsetOfData] & 0x0F;	//retrieve the lower nibble of the value size tuple
		node.lowest_vcn.QuadPart = 0;
		while (numOfLcnBytes && numOfLenBytes)
		{
			++offsetOfData;
			memcpy(&node.len.QuadPart, body + offsetOfData, numOfLenBytes);
			node.len.QuadPart *= bytesPerCluster;

			offsetOfData += numOfLenBytes;
			memcpy(&tempOffsetToData.QuadPart, body + offsetOfData, numOfLcnBytes);
			tempOffsetToData.QuadPart *= bytesPerCluster;
			node.offset.QuadPart += tempOffsetToData.QuadPart;

			offsetOfData += numOfLcnBytes;
			numOfLcnBytes = (body[offsetOfData] & 0xF0) >> 4;
			numOfLenBytes = body[offsetOfData] & 0x0F;

			node.highest_vcn.QuadPart = node.lowest_vcn.QuadPart + node.len.QuadPart;
			data.push_back(node);
			node.lowest_vcn.QuadPart = node.highest_vcn.QuadPart+ 1;
		}
	}
	return data;
}