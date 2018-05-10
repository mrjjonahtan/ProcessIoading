// ProcessIoading.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Include\PeToolsClass.h"

HANDLE hProcess = NULL;
unsigned long imageBase = 0;
unsigned char *pebuffer = NULL;
unsigned long messageBufferSize = 0;
unsigned long addressOfEntryPoint = 0;
unsigned long directoryLocat = 0;
unsigned long importLocat = 0;

struct sectionHeader
{
	union
	{
		DWORD PhysicalAddress;
		DWORD VirtualSize;
	} Misc;
	DWORD VirtualAddress;
	DWORD SizeOfRawData;
	DWORD PointerToRawData;
	DWORD Characteristics;
};


void init()
{
	DuplicateHandle(GetCurrentProcess(),
		GetCurrentProcess(),
		GetCurrentProcess(),
		&hProcess,
		0,
		FALSE,
		DUPLICATE_SAME_ACCESS);//伪进程句柄转为真实进程句柄  
}

void freeM()
{
	//不用的时候必须关闭 不然会有资源泄露  
	if (hProcess != NULL)
	{
		CloseHandle(hProcess);
		hProcess = NULL;
	}
	if (pebuffer != NULL) {
		free(pebuffer);
		pebuffer = NULL;
	}
}

void getMessage(BYTE *srcPointer)
{
	DWORD ifanew = 0;
	DWORD sizeOfHeaders = 0;
	DWORD numberOfSections = 0;
	DWORD sizeOfOptionalHeader = 0;
	DWORD numberOfSectionsOffSetLocation = 0;
	//DWORD addressOfEntryPoint = 0;
	DWORD sizeOfImage = 0;
	PeToolsClass petc;

	//
	ifanew = petc.getPELocation(srcPointer);
	numberOfSections = petc.getDWValue((srcPointer + ifanew + 6), 2);
	sizeOfOptionalHeader = petc.getDWValue((srcPointer + ifanew + 20), 2);

	imageBase = petc.getDWValue((srcPointer + ifanew + 24 + 28), 4);
	sizeOfImage = petc.getDWValue((srcPointer + ifanew + 24 + 56), 4);
	sizeOfHeaders = petc.getDWValue((srcPointer + ifanew + 24 + 60), 4);
	addressOfEntryPoint = petc.getDWValue((srcPointer + ifanew + 24 + 16), 4);

	//
	DWORD platform = petc.getApplicationSize(srcPointer);
	if (platform == 0x014C)
	{
		directoryLocat = petc.getDWValue((srcPointer + 60), 4) + 24 + 96;
	}
	else if (platform == 0x8664)
	{
		directoryLocat = petc.getDWValue((srcPointer + 60), 4) + 24 + 112;
	}

	importLocat = petc.getDWValue((srcPointer + directoryLocat + 8), 4);

	//
	sectionHeader ah[10] = { 0 };

	numberOfSectionsOffSetLocation = ifanew + 24 + sizeOfOptionalHeader;
	for (WORD i = 0; i < numberOfSections; i++)
	{
		ah[i].Misc.VirtualSize = petc.getDWValue((srcPointer + numberOfSectionsOffSetLocation + i * 40 + 8), 4);
		ah[i].VirtualAddress = petc.getDWValue((srcPointer + numberOfSectionsOffSetLocation + i * 40 + 12), 4);
		ah[i].SizeOfRawData = petc.getDWValue((srcPointer + numberOfSectionsOffSetLocation + i * 40 + 16), 4);
		ah[i].PointerToRawData = petc.getDWValue((srcPointer + numberOfSectionsOffSetLocation + i * 40 + 20), 4);
		ah[i].Characteristics = petc.getDWValue((srcPointer + numberOfSectionsOffSetLocation + i * 40 + 36), 4);
	}

	//
	messageBufferSize = petc.getAlignData(sizeOfImage, 0x1000);
	pebuffer = (BYTE *)malloc(messageBufferSize);
	if (pebuffer == NULL) {
		return;
	}
	memset(pebuffer, 0, messageBufferSize);

	memcpy_s(pebuffer, messageBufferSize, srcPointer, sizeOfHeaders);

	for (WORD i = 0; i < numberOfSections; i++)
	{
		memcpy_s((pebuffer + ah[i].VirtualAddress), messageBufferSize, (srcPointer + ah[i].PointerToRawData), ah[i].SizeOfRawData);
	}

	
}


void getDate()
{
	FILE *openFile = NULL;
	unsigned char *pointer = NULL;
	unsigned long size = 0;


	fopen_s(&openFile, "D:\\calc.exe", "rb");
	if (openFile == NULL) {
		return;
	}
	if (fseek(openFile, 0L, SEEK_END)) {
		return;
	}

	size = ftell(openFile);
	if (size == 0) {
		return;
	}

	pointer = (BYTE*)malloc(sizeof(BYTE)*size);
	if (pointer == NULL) {
		return;
	}

	memset(pointer, 0L, sizeof(BYTE)*size);

	fseek(openFile, 0L, 0L);
	if (fread(pointer, sizeof(BYTE)*size, 1, openFile) <= 0) {
		return;
	}

	if (openFile != NULL) {
		fclose(openFile);
		openFile = NULL;
	}

	//
	getMessage(pointer);


	if (pointer != NULL)
	{
		free(pointer);
		pointer = NULL;
	}
}

int main()
{
	init();

	getDate();


	if (hProcess == NULL)
	{
		return 0;
	}

	if (!imageBase)
	{
		return 0;
	}

	PVOID shellImageBase = VirtualAllocEx(hProcess, (LPVOID)imageBase, messageBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	DWORD retByte = 0;
	//写数据到内存
	WriteProcessMemory(hProcess, shellImageBase, pebuffer, messageBufferSize, &retByte);

	//
	unsigned char *memBuffer[0x250] = {0};
	ReadProcessMemory(hProcess, (LPVOID)(importLocat + imageBase), memBuffer, 8, NULL);



	//free
	freeM();

	//
	HANDLE ThrdHnd = ::GetCurrentThread();
	CONTEXT Ctx;
	Ctx.ContextFlags = CONTEXT_FULL;
	GetThreadContext(ThrdHnd, &Ctx);
	void *p = (void*)shellImageBase;
	memcpy((PVOID)(Ctx.Ebx + 8), &p, 4);

	DWORD dwJmpAddr = (DWORD)shellImageBase + addressOfEntryPoint;
	_asm {
		jmp dwJmpAddr
	}

    return 0;
}

