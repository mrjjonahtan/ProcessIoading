#include "../stdafx.h"
#include "../Include/petoolsclass.h"

PeToolsClass::PeToolsClass()
{
}


PeToolsClass::~PeToolsClass()
{
}

void PeToolsClass::getValue(unsigned char *pointerValue, int number, wchar_t *tvlue)
{
	char *vaby = NULL;
	vaby = (char*)malloc(0x200);
	if (vaby == NULL)
	{
		return;
	}
	for (int i = number - 1, j = 0; i >= 0; i--, j++)
	{
		memset(vaby, 0, 0x200);
		DWORD valueTem = *(pointerValue + i);
		sprintf_s(vaby, 0x200, "%02X", valueTem);
		for (int k = 0; *(vaby + k) != 0; k++)
		{
			*(tvlue + j) = *(vaby + k);
			j++;
		}
		j--;
	}
	if (vaby != NULL) {
		free(vaby);
	}
}

unsigned long PeToolsClass::getDWValue(unsigned char *pointerValue, int number)
{
	DWORD revalue = 0;
	for (int i = (number - 1); i >= 0; i--)
	{
		revalue = (revalue << 8) + *(pointerValue + i);
	}
	return revalue;
}

unsigned long PeToolsClass::getOptionSizeValue(unsigned char *pointerValue)
{
	unsigned long revalue = 0;
	revalue = getDWValue((pointerValue + getPELocation(pointerValue) + 4 + 16), 2);
	return revalue;
}

unsigned long PeToolsClass::getPELocation(unsigned char *pointerValue)
{
	unsigned long revalue = 0;
	revalue = getDWValue((pointerValue + 60), 4);
	return revalue;
}

unsigned long PeToolsClass::getSectionNumber(unsigned char *pointerValue)
{
	unsigned long revalue = 0;
	revalue = getDWValue((pointerValue + getPELocation(pointerValue) + 4 + 2), 2);
	return revalue;
}

void PeToolsClass::getCharPointer(unsigned char *pointerValue, wchar_t *tvlue, int max)
{
	if (max == 0)
	{
		for (int i = 0; pointerValue[i] != 0; i++)
		{
			tvlue[i] = pointerValue[i];
		}
	}
	else
	{
		for (int i = 0; pointerValue[i] != 0 && i < max; i++)
		{
			tvlue[i] = pointerValue[i];
		}
	}

}

unsigned long PeToolsClass::rvaTofoa(unsigned char *pointerValue, unsigned long RVA)
{
	struct mSection {
		union
		{
			unsigned long PhysicalAddress;
			unsigned long virtualSize;
		} Misc;
		unsigned long virtualAddress;
		unsigned long sizeOfRawData;
		unsigned long pointertorawdata;
	};
	unsigned long rtf = 0;
	unsigned long pelocat = getPELocation(pointerValue);
	unsigned long sizeOfHeaders = getDWValue((pointerValue + pelocat + 24 + 60), 4);
	if (RVA > sizeOfHeaders)
	{
		unsigned long optionSize = getOptionSizeValue(pointerValue);
		int snumber = getSectionNumber(pointerValue);
		mSection section[10] = { 0 };

		for (int i = 0; i < snumber; i++)
		{
			unsigned long locat = i * 40;
			section[i].Misc.virtualSize = getDWValue((pointerValue + pelocat + optionSize + 24 + 8 + locat), 4);
			section[i].virtualAddress = getDWValue((pointerValue + pelocat + optionSize + 24 + 12 + locat), 4);
			section[i].sizeOfRawData = getDWValue((pointerValue + pelocat + optionSize + 24 + 16 + locat), 4);
			section[i].pointertorawdata = getDWValue((pointerValue + pelocat + optionSize + 24 + 20 + locat), 4);
		}

		for (int i = 0; i < snumber; i++)
		{
			if (RVA >= section[i].virtualAddress && RVA < (getAlignData(section[i].Misc.virtualSize, 0x1000) + section[i].virtualAddress))
			{
				rtf = (RVA - section[i].virtualAddress) + section[i].pointertorawdata;
			}
		}
	}
	else
	{
		rtf = RVA;
	}
	return rtf;
}

unsigned long PeToolsClass::foaTorva(unsigned char *pointerValue, unsigned long FOA)
{
	struct mSection {
		union
		{
			DWORD PhysicalAddress;
			DWORD virtualSize;
		} Misc;
		DWORD virtualAddress;
		DWORD sizeOfRawData;
		DWORD pointertorawdata;
	};
	DWORD ftr = 0;
	DWORD pelocat = getPELocation(pointerValue);
	DWORD optionSize = getOptionSizeValue(pointerValue);
	int snumber = getSectionNumber(pointerValue);

	mSection section[10] = { 0 };

	for (int i = 0; i < snumber; i++)
	{
		DWORD locat = i * 40;
		section[i].Misc.virtualSize = getDWValue((pointerValue + pelocat + optionSize + 24 + 8 + locat), 4);
		section[i].virtualAddress = getDWValue((pointerValue + pelocat + optionSize + 24 + 12 + locat), 4);
		section[i].sizeOfRawData = getDWValue((pointerValue + pelocat + optionSize + 24 + 16 + locat), 4);
		section[i].pointertorawdata = getDWValue((pointerValue + pelocat + optionSize + 24 + 20 + locat), 4);
	}
	for (int i = 0; i < snumber; i++)
	{
		if (FOA >= section[i].pointertorawdata && FOA < section[i].sizeOfRawData + section[i].pointertorawdata)
		{
			ftr = (FOA - section[i].pointertorawdata) + section[i].virtualAddress;
		}
	}
	return ftr;
}

unsigned long PeToolsClass::getAlignData(unsigned long data, unsigned long alig)
{
	DWORD revalue = 0;
	revalue = data % alig;
	if (revalue != 0)
	{
		revalue = (data - revalue) + alig;
	}
	else
	{
		revalue = data;
	}
	return revalue;
}

unsigned long PeToolsClass::getApplicationSize(unsigned char *pointerValue)
{
	unsigned long pelocat = getPELocation(pointerValue);
	unsigned long reValue = getDWValue((pointerValue + pelocat + 4), 2);
	return reValue;
}
