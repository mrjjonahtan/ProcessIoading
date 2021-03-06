// ProcessIoading.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#pragma comment(linker,"/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#include "Include\PeToolsClass.h"



HANDLE hProcess = NULL;
unsigned long imageBase = 0;
unsigned char *pebuffer = NULL;
unsigned long messageBufferSize = 0;
unsigned long addressOfEntryPoint = 0;
unsigned long directoryLocat = 0;
unsigned long importLocat = 0;

PeToolsClass petc;

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
	//DuplicateHandle(GetCurrentProcess(),
	//	GetCurrentProcess(),
	//	GetCurrentProcess(),
	//	&hProcess,
	//	0,
	//	FALSE,
	//	DUPLICATE_SAME_ACCESS);//伪进程句柄转为真实进程句柄  
	hProcess = ::GetCurrentProcess();
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

//loading iat
void repairIAT(BYTE *pointer)
{
	
	TCHAR *temporaryBUffer = NULL;
	temporaryBUffer = (TCHAR*)malloc(sizeof(TCHAR) * 0x100);
	if (temporaryBUffer == NULL)
	{
		return;
	}
	memset(temporaryBUffer, 0, sizeof(TCHAR) * 0x50);

	DWORD importRVA = petc.getDWValue((pointer + directoryLocat + 8), 4);
	if (importRVA == 0)
	{
		return;
	}
	//DWORD importFOA = petc.rvaTofoa(pointer, importRVA);

	for (int i = 0; ; i++)
	{
		DWORD originalFirstThunkRVA = petc.getDWValue((pointer + importRVA + (i * 20)), 4);
		DWORD firstThunkRVA = petc.getDWValue((pointer + importRVA + 16 + (i * 20)), 4);
		if (originalFirstThunkRVA == 0 || firstThunkRVA == 0)
		{
			break;
		}
		DWORD timeDateStamp = petc.getDWValue((pointer + importRVA + 4 + (i * 20)), 4);
		if (timeDateStamp == -1)
		{
			//descriptor


			continue;
		}
		//DWORD nameRVA = petc.getDWValue((pei->pointer + importFOA + 12 + (i * 20)), 4);
		//DWORD nameFOA = petc.rvaTofoa(pointer, petc.getDWValue((pointer + importFOA + 12 + (i * 20)), 4));
		DWORD nameRVA = petc.getDWValue((pointer + importRVA + 12 + (i * 20)), 4);

		memset(temporaryBUffer, 0, sizeof(TCHAR) * 0x50);
		//wsprintf(temporaryBUffer, L"%08X", firstThunkRVA);
		//wsprintf(temporaryBUffer + 9, L"%08X", timeDateStamp);
		//wsprintf(temporaryBUffer + 18, L"%08X", originalFirstThunkRVA);
		petc.getCharPointer((pointer + nameRVA), temporaryBUffer, 0);

		//IAT
		//OutputDebugString((temporaryBUffer+27));

		for (int i = 0; ; i++)
		{
			DWORD thunkdataINT = petc.getDWValue(pointer + firstThunkRVA + (i * 4), 4);
			if (thunkdataINT == 0)
			{
				break;
			}
			//int sd = thunkdataINT & 0x80000000;
			if ((thunkdataINT & 0x80000000))
			{
				
				HMODULE hLoadDll = LoadLibrary(temporaryBUffer);

				if (!hLoadDll)
				{
					wchar_t hint[0x50] = {0};
					swprintf(hint, 0x50,L"DLL(%s)加载失败\n请检查环境DLL是否丢失，\n程序将尝试继续加载其他DLL并启动程序。", temporaryBUffer);
					MessageBox(NULL, hint, L"加载DLL提示", MB_OK);
				}

				DWORD iat = (DWORD)::GetProcAddress(hLoadDll, (LPCSTR)(thunkdataINT & 0x7fffffff));
				
				petc.putData(pointer + firstThunkRVA + (i * 4), iat, 4);
				
			}
			else
			{

				//DWORD hint = petc.getDWValue(pointer + thunkdataINT, 2);

				wchar_t wfunName[0x30] = { 0 };
				char funName[0x40] = { 0 };

				petc.getCharPointer(pointer + thunkdataINT + 2, wfunName, 0);

				sprintf_s(funName, "%ws", wfunName);
				
				DWORD iat = (DWORD)::GetProcAddress(LoadLibrary(temporaryBUffer), funName);

				petc.putData(pointer + firstThunkRVA + (i * 4), iat, 4);

			}

		}
		
	}
	if (temporaryBUffer != NULL)
	{
		free(temporaryBUffer);
		temporaryBUffer = NULL;
	}
}

//loading import Descriptor
void importDescriptor(BYTE *pointer)
{
	DWORD boundImportRVA = petc.getDWValue((pointer + directoryLocat + 88), 4);

	if (!boundImportRVA)
	{
		return;
	}

	DWORD fistBoundImportRVA = boundImportRVA;
	wchar_t temporaryBUffer[0x20] = { 0 };

	for (int i = 0, j = 0; ; i++)
	{
		memset(temporaryBUffer, 0, sizeof(TCHAR) * 0x20);
		DWORD timeDateStamp = petc.getDWValue(pointer + boundImportRVA, 4);
		WORD offsetModulNmae = petc.getDWValue(pointer + boundImportRVA + 4, 2);
		WORD numberOfModuleForwarderRefs = petc.getDWValue(pointer + boundImportRVA + 6, 2);
		if (offsetModulNmae == 0 && timeDateStamp == 0)
		{
			break;
		}
		DWORD nameRVA = offsetModulNmae + fistBoundImportRVA;

		//loading other dll
		if (numberOfModuleForwarderRefs)
		{
			DWORD boundImportRVAtem = boundImportRVA + 8;
			wchar_t temporaryBuffer[0x20] = { 0 };
			for (int i = 0, j = 0; ; i++)
			{
				memset(temporaryBuffer, 0, sizeof(TCHAR) * 0x20);
				DWORD timeDateStamp = petc.getDWValue(pointer + boundImportRVAtem, 4);
				WORD offsetModulNmae = petc.getDWValue(pointer + boundImportRVAtem + 4, 2);
				WORD numberOfModuleForwarderRefs = petc.getDWValue(pointer + boundImportRVAtem + 6, 2);
				if (offsetModulNmae == 0 && timeDateStamp == 0)
				{
					break;
				}
				DWORD nameRVA = offsetModulNmae + boundImportRVAtem;

				petc.getCharPointer(pointer + nameRVA, temporaryBuffer, 0);

				LoadLibrary(temporaryBuffer);

			}
		}

		//
		petc.getCharPointer(pointer + nameRVA, temporaryBUffer, 0);
		LoadLibrary(temporaryBUffer);

		if (numberOfModuleForwarderRefs == 0)
		{
			numberOfModuleForwarderRefs = 1;
		}
		boundImportRVA = boundImportRVA + (numberOfModuleForwarderRefs * 8);
		j++;
	}

}

//
void getMessage(BYTE *srcPointer)
{
	DWORD ifanew = 0;
	DWORD sizeOfHeaders = 0;
	DWORD numberOfSections = 0;
	DWORD sizeOfOptionalHeader = 0;
	DWORD numberOfSectionsOffSetLocation = 0;
	//DWORD addressOfEntryPoint = 0;
	DWORD sizeOfImage = 0;

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
		//
		MessageBox(NULL, L"64程序进程加载，还在编码中，请稍后...", L"提示", MB_OK);
		return;
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

	//iat
	repairIAT(pebuffer);
	//descriptor
	//importDescriptor(pebuffer);

}


void getDate(char *path)
{
	FILE *openFile = NULL;
	unsigned char *pointer = NULL;
	unsigned long size = 0;

	if (path == NULL)
	{
		return;
	}

	fopen_s(&openFile, path, "rb");//Everything calc Dbgview
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

//需要路径参数
int main(int argc, char* argv[])
{
	char path[0x256] = { 0 };

	//sprintf_s(path, "%s", "D:\\Dbgview.exe");
	sprintf_s(path, "%s", argv[1]);

	if (!path)
	{
		return 0;
	}


	init();

	getDate(path);


	if (hProcess == NULL || !imageBase || !messageBufferSize)
	{
		return 0;
	}


	PVOID shellImageBase = VirtualAllocEx(hProcess, (LPVOID)imageBase, messageBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!shellImageBase)
	{
		MessageBox(NULL, L"VirtualAllocEx申请空间失败！", L"提示", MB_OK);
		return 1;
	}
	DWORD retByte = 0;
	//写数据到内存
	WriteProcessMemory(hProcess, shellImageBase, pebuffer, messageBufferSize, &retByte);

	//
	//unsigned char memBuffer[0x250] = {0};
	//ReadProcessMemory(hProcess, (LPVOID)(addressOfEntryPoint + imageBase), memBuffer, 8, NULL);


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

