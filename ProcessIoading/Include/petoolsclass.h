#pragma once

class PeToolsClass
{
public:
	PeToolsClass();
	~PeToolsClass();

	void getValue(unsigned char *pointerValue, int number, wchar_t *tvlue);
	unsigned long getDWValue(unsigned char *pointerValue, int number);
	unsigned long getPELocation(unsigned char *pointerValue);
	unsigned long getOptionSizeValue(unsigned char *pointerValue);
	unsigned long getSectionNumber(unsigned char *pointerValue);
	void getCharPointer(unsigned char *pointerValue, wchar_t *tvlue, int max);
	unsigned long getAlignData(unsigned long data, unsigned long alig);
	unsigned long rvaTofoa(unsigned char *pointerValue, unsigned long RVA);
	unsigned long foaTorva(unsigned char *pointerValue, unsigned long FOA);
	unsigned long getApplicationSize(unsigned char *pointerValue);
};

