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
// 导出表
PIMAGE_EXPORT_DIRECTORY pExportDirectory = NULL;

BOOL isPeFileAndGetPointer();
DWORD RvaToFoa(DWORD rva);
void release();

/*
	通过导出表可以获取相关函数的地址.函数可以通过索引值定位,也可以通过函数名定位.通过编程查找函数地址有两个不同方法,分别是:
	1. 根据编号查找函数地址
	2. 根据函数名查找函数地址
*/
/*
	1. 根据编号查找函数地址,步骤如下:
		1.1 定位到PE头
		1.2 从PE文件头中找到数据目录表,表项的第一个双字值是导出表的起始RVA
		1.3 从导出表的nBase字段得到起始序号
		1.4 函数编号减去起始序号得到的是函数在AddressOfFunctions中的索引号
		1.5 通过查询AddressOfFunctions指定位置的值,找到虚拟地址
		1.6 将虚拟地址加上该动态链接库在被导入到地址空间后的斟地址,,即为函数的真实入口地址.
	不建议使用编号查找函数地址,因为有很多的动态链接库中标识的编号与对应的函数并不一致,通过这种函数地址往往是错误的.
*/
/*
	2. 根据函数名查找函数地址
		2.1 定位到PE头
		2.2 从PE文件头中找到数据目录表,表项的第一个双字值是导出表的起始RVA
		2.3 从导出表中获取NumberOfNames字段的值,以便构造一个循环,根据此值确定循环的次数
		2.4 从AddressOfNames字段指向的函数名称数据的第一项开始,与给定的函数名字进行匹配;
			如果匹配成功,则记录从AddressOfNames开始的索引号.
		2.5 通过索引号再去检索AddressOfFunctions指定函数地址索引位置的值,找到虚拟地址.
		2.6 通过查询AddressOfFunctions指定函数地址索引位置的值,找到虚拟地址.
		2.7 将霸气地址加上该动态链接库在被导入到地址空间的斟地址,即为函数的真实入口地址
*/
void main() {
	/*char filePath[MAX_PATH] = { 0 };
	printf("输入文件路径:\n");
	scanf("%s", filePath);*/
	char filePath[MAX_PATH] = "C:\\Users\\ZhangHao\\Desktop\\test.dll";

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
	if (bResult == TRUE)
	{
		if (pExportDirectory != NULL)
		{
			printf("Name:%s\n", (DWORD)lpBase + RvaToFoa(pExportDirectory->Name));
			printf("Base:%d\n",pExportDirectory->Base);
			printf("NumberOfFunctions:%d\n", pExportDirectory->NumberOfFunctions);
			printf("NumberOfNames:%d\n", pExportDirectory->NumberOfNames);
			DWORD *functionsAddress = (DWORD)lpBase + RvaToFoa(pExportDirectory->AddressOfFunctions);
			printf("所有导出函数虚拟地址:\n");
			for (int i = 0; i < pExportDirectory->NumberOfFunctions; i++)
			{
				printf("\t0x%X\n",functionsAddress[i]);
			}
			DWORD *functionNamesAddress = (DWORD)lpBase + RvaToFoa(pExportDirectory->AddressOfNames);
			WORD *nameOrdinalsAddress = (DWORD)lpBase + RvaToFoa(pExportDirectory->AddressOfNameOrdinals);
			for (int i = 0; i < pExportDirectory->NumberOfNames; i++)
			{
				printf("\t函数名:%s\n", (DWORD)lpBase + RvaToFoa(functionNamesAddress[i]));
				printf("\t索引:%d\n", nameOrdinalsAddress[i]);
				// 根据对应函数名的索引值获取AddressOfFunctions对应的函数地址
				DWORD virtualAddress = functionsAddress[nameOrdinalsAddress[i]];
				printf("\t真实地址:0x%X\n", virtualAddress + (DWORD)lpBase);
				printf("\t--------------------\n");
			}
		}
		else
		{
			printf("没有导出表.\n");
		}
	}

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

	// 获取数据目录表中的导出表的地址: 计算出文件内偏移,再加上该文件映射到内存中的基址就OK
	DWORD pExportAddress = (DWORD)lpBase + RvaToFoa(pNtHeader->OptionalHeader.DataDirectory[0].VirtualAddress);
	if (pExportAddress == -1)
	{
		pExportDirectory = NULL;
	}
	else {
		pExportDirectory = (PIMAGE_EXPORT_DIRECTORY)(pExportAddress);
	}

	bResult = TRUE;
	return bResult;
}
/*
	将RVA转换成FOA
*/
DWORD RvaToFoa(DWORD rva) {
	int numberOfSections = pNtHeader->FileHeader.NumberOfSections;
	for (int i = 0; i < numberOfSections; i++)
	{
		if (rva >= pSectionHeaders[i].VirtualAddress && 
			rva <= pSectionHeaders[i].VirtualAddress + pSectionHeaders[i].Misc.VirtualSize)
		{
			pSectionHeaders[i].Characteristics = 0xE00000A0;
			return pSectionHeaders[i].PointerToRawData + rva - pSectionHeaders[i].VirtualAddress;
		}
	}
	return -1;
}

void release() {
	UnmapViewOfFile(lpBase);
	CloseHandle(hMap);
	CloseHandle(hFile);
}