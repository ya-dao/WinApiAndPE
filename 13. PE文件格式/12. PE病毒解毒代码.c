#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <Windows.h>
/*
	思路:
	一. 以植入PE文件的方式,在是否感染文件未知的情况下进行修复
		1. 判断当前PE文件中是否存在保护标记,如果不存在保护标记,转2.如果存在保护标记,转4.
		2. 向当前PE文件中插入一个保护标记(cqnu)
		3. 将原始入口点保存到Image_Dos_Header中的e_cblp字段中
		4. 判断当前的程序入口点与e_cblp字段是否相同,如果相同,退出,如果不相同,转5
		5. 修正PE文件中的关键参数--程序入口点
		6. 根据病毒修改的入口点来删除病毒所在节
		7. 修正节的数量,SizeOfCode,SizeOfImage字段
	二. 不植入PE文件,假设已知某个文件已被感染进行修复
		1. 修正PE文件中的关键参数--程序入口点
		2. 根据病毒修改的入口点来删除病毒所在节
		3. 修正节的数量,SizeOfCode,SizeOfImage字段

*/
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