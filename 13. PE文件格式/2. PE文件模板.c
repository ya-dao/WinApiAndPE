#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
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
PIMAGE_SECTION_HEADER pSectionHeaders = NULL;
// 节表头结构体
IMAGE_SECTION_HEADER imgSection = { 0 };

BOOL isPeFileAndGetPointer();
void release();

void main() {
	char filePath[MAX_PATH] = { 0 };
	printf("输入文件路径:\n");
	scanf("%s", filePath);

	hFile = CreateFile(filePath,
		GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	hMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, 0);
	lpBase = MapViewOfFile(hMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

	BOOL bResult = isPeFileAndGetPointer();

	system("pause");
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
	pSectionHeaders = (PIMAGE_SECTION_HEADER)((DWORD)&(pNtHeader->OptionalHeader)
		+ pNtHeader->FileHeader.SizeOfOptionalHeader);
	bResult = TRUE;
	return bResult;
}

void release() {
	UnmapViewOfFile(lpBase);
	CloseHandle(hMap);
	CloseHandle(hFile);
}