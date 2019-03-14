#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
// 文件句柄
HANDLE hFile = NULL;
// 映像句柄
HANDLE hMap = NULL;
// 映像视图指针
LPVOID lpBase = NULL;
// DOS头
PIMAGE_DOS_HEADER pDosHeader = NULL;
// PE头
PIMAGE_NT_HEADERS pNtHeader = NULL;
// 节表头
PIMAGE_SECTION_HEADER pSectionHeader = NULL;

BOOL fileOpen(char *filePath);
BOOL isPeFileAndGetPointer();
void parseBasePe();
void enumSections();
void release();
/*
	地址计算相关函数
*/
int getSectionNumber(int option, DWORD address);
void calcAddress(int sectionIndex, int option, DWORD address);
/*
	根据内存相对偏移地址RVA计算出文件偏移FOA
*/
void main() {

	char filePath[MAX_PATH] = { 0 };
	printf("输入文件路径:\n");
	scanf("%s", filePath);

	BOOL bResult = fileOpen(filePath);
	if (bResult == TRUE)
	{
		bResult = isPeFileAndGetPointer();
	}
	if (bResult == TRUE)
	{
		parseBasePe();
		enumSections();
	}

	printf("地址转换:\n");
	int option = 0;
	int knownAddress = 0;

	while (TRUE)
	{
		printf("选择已知的地址类型:1. 虚拟地址 2. 相对虚拟地址  3. 文件偏移  4.退出\n");
		scanf("%d", &option);
		if (option == 4)
		{
			break;
		}
		printf("输入已知的地址值:\n");
		scanf("%X", &knownAddress);
		int sectionIndex = getSectionNumber(option, knownAddress);
		calcAddress(sectionIndex, option, knownAddress);
	}

	if (bResult == TRUE)
	{
		release();
	}

	system("pause");
}
/*
	打开文件并创建映像视图
	该函数的主要功能是打开文件并创建内存文件映像.通常对文件进行连续读写时直接使用ReadFile()和WriteFile()两个函数.
	当不连续操作文件时,每次在ReadFile()或者WriteFile()后就要使用SetFilePointer()来调整文件指针的位置,这样的操作较为繁琐.
	内存文件映像的作用是把整个文件映射入进程的虚拟空间中,这样操作文件就像操作内存变量或内存数据一样方便.
	创建内存文件映像所使用到的函数有两个:分别是CreateFileMapping()和MapViewOfFile()
	https://blog.csdn.net/ktpd_pro/article/details/56481519
*/
BOOL fileOpen(char *filePath) {
	BOOL bResult = FALSE;
	hFile = CreateFile(filePath, GENERIC_WRITE | GENERIC_READ, 
		FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile  == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		printf("打开文件失败!%d\n", error);
		return bResult;
	}
	hMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE | SEC_IMAGE, 0, 0, 0);
	if (hMap == NULL)
	{
		printf("创建文件映射失败!\n");
		CloseHandle(hFile);
		return bResult;
	}
	lpBase = MapViewOfFile(hMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	if (lpBase == NULL)
	{
		printf("映射文件失败!\n");
		CloseHandle(hFile);
		CloseHandle(hMap);
		return bResult;
	}
	bResult = TRUE;
	return bResult;
}
/*
	判断是否为有效的PE文件
	如果无效直接返回
	如果有效则把解析PE格式相关的结构体指针也得到
*/
BOOL isPeFileAndGetPointer() {
	BOOL bResult = FALSE;
	// 判断是否为MZ头
	pDosHeader = (PIMAGE_DOS_HEADER)lpBase;
	if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		printf("不符合DOS头\n");
		release();
		return bResult;
	}
	// 根据IMAGE_DOS_HEADER中的e_lfanew的值得到PE头的位置
	pNtHeader = (PIMAGE_NT_HEADERS)((DWORD)lpBase + pDosHeader->e_lfanew);
	// 判断是否为PE\0\0
	if (pNtHeader->Signature != IMAGE_NT_SIGNATURE)
	{
		printf("不符合PE头\n");
		release();
		return bResult;
	}
	// 获取节表的位置,从NT_HEADER中的&(pNtHeader->OptionalHeader)处加上SizeOfOptionalHeader就是节表的位置了
	// &(pNtHeader->OptionalHeader):取出OptionalHeader在该文件中的偏移地址
	printf("%08X,%08X\n", &(pNtHeader->OptionalHeader), (pNtHeader->OptionalHeader));
	pSectionHeader = (PIMAGE_SECTION_HEADER)((DWORD)&(pNtHeader->OptionalHeader)
		+ pNtHeader->FileHeader.SizeOfOptionalHeader);
	bResult = TRUE;
	return bResult;
}
/*
	解析PE文件
*/
void parseBasePe() {
	// 入口地址
	printf("入口地址:%08X\n", pNtHeader->OptionalHeader.AddressOfEntryPoint);
	// 映射基地址
	printf("映射基地址:%08X\n", pNtHeader->OptionalHeader.ImageBase);
	// 节表数量
	printf("节表数量:%08X\n", pNtHeader->FileHeader.NumberOfSections);
	// 内存对齐
	printf("内存对齐:%08X\n", pNtHeader->OptionalHeader.SectionAlignment);
	// 文件对齐
	printf("文件对齐:%08X\n", pNtHeader->OptionalHeader.FileAlignment);
}

/*
	枚举节表
*/
void enumSections() {
	int sectionNumber = pNtHeader->FileHeader.NumberOfSections;
	printf("节表名称 | V.偏移 | V.大小 | R.偏移 | R.大小 | 标志\n");
	for (int i = 0; i < sectionNumber; i++)
	{
		printf("%6s | %08X | %08X | %08X | %08X | %08X\n",
			pSectionHeader[i].Name, 
			pSectionHeader[i].VirtualAddress, pSectionHeader[i].Misc.VirtualSize, 
			pSectionHeader[i].PointerToRawData, pSectionHeader[i].SizeOfRawData,
			pSectionHeader[i].Characteristics);
	}
}
/*
	通过给定的地址类型和地址,获取其所属节表编号
*/
int getSectionNumber(int option, DWORD address) {
	int sectionNumber = pNtHeader->FileHeader.NumberOfSections;
	switch (option)
	{
	case 1:
	// 虚拟地址
		{
			int imageBase = pNtHeader->OptionalHeader.ImageBase;
			for (int i = 0; i < sectionNumber; i++)
			{
				if (address >= imageBase + pSectionHeader[i].VirtualAddress 
					&& address <= imageBase + pSectionHeader[i].VirtualAddress 
					+ pSectionHeader[i].Misc.VirtualSize)
				{
					return i;
				}
			}
			break;
		}
	case 2:
		// 相对虚拟地址
		{
			for (int i = 0; i < sectionNumber; i++)
			{
				if (address >= pSectionHeader[i].VirtualAddress
					&& address <= pSectionHeader[i].VirtualAddress
					+ pSectionHeader[i].Misc.VirtualSize)
				{
					return i;
				}
			}
			break;
		}
	case 3:
		// 文件偏移地址
		{
			for (int i = 0; i < sectionNumber; i++)
			{
				if (address >= pSectionHeader[i].PointerToRawData
					&& address <= pSectionHeader[i].PointerToRawData + pSectionHeader[i].SizeOfRawData)
				{
					return i;
				}
			}
			break;
		}
	}
	return -1;
}
/*
	根据传入的其所在节表号, 地址类型, 具体地址来计算其他类型
	根据内存相对偏移地址RVA计算出文件偏移FOA
*/
void calcAddress(int sectionIndex, int option, DWORD address) {
	int dwRva = 0, dwVa = 0, dwFoa = 0;
	switch (option)
	{
	case 1:
		dwVa = address;
		dwRva = dwVa - pNtHeader->OptionalHeader.ImageBase;
		dwFoa = dwRva - pSectionHeader[sectionIndex].VirtualAddress + pSectionHeader[sectionIndex].PointerToRawData;
		break;
	case 2:
		dwRva = address;
		dwVa = dwRva + pNtHeader->OptionalHeader.ImageBase;
		dwFoa = dwRva - pSectionHeader[sectionIndex].VirtualAddress + pSectionHeader[sectionIndex].PointerToRawData;
		break;
	case 3:
		dwFoa = address;
		dwRva = dwFoa - pSectionHeader[sectionIndex].PointerToRawData + pSectionHeader[sectionIndex].VirtualAddress;
		dwVa = dwRva + pNtHeader->OptionalHeader.ImageBase;
		break;
	}
	printf("RVA:%08X\n", dwRva);
	printf("VA:%08X\n", dwVa);
	printf("FOA:%08X\n", dwFoa);
}
void release() {
	CloseHandle(hFile);
	CloseHandle(hMap);
	UnmapViewOfFile(lpBase);
}