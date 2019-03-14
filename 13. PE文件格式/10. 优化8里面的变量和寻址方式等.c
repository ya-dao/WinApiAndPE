#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>

/*
/// 注释的内容为优化版,为了将文件尽量缩减,减少使用变量,直接使用偏移量计算一些地址
(lpBase + 0x3c): DOS头中的e_lfanew
*(PDWORD)((DWORD)lpBase + *(PDWORD)((DWORD)lpBase + 0x3c): NTHeader
*(PDWORD)((DWORD)lpBase + *(PDWORD)((DWORD)lpBase + 0x3c) + 0x78: DataDirectory
((DWORD)lpBase + *(PWORD)((DWORD)lpBase + *(PDWORD)((DWORD)lpBase + 0x3c) + 0x18)): ((DWORD)&(pNtHeader->OptionalHeader)
*/

// 映像视图指针
LPVOID lpBase = NULL;
// 节表头
PIMAGE_SECTION_HEADER pSectionHeaders = NULL;
// 导出表
PIMAGE_EXPORT_DIRECTORY pExportDirectory = NULL;
// LoadLibraryA的ASCII码,转换成整数后,需要注意端序问题,这里是daoL rbiL Ayra
// DWORD strLoadLibrary[] = { 0x64616f4c, 0x7262694c, 0x41797261 , 0};
char *strLoadLibrary = "LoadLibraryA";
char *strGetProcAddress = "GetProcAddress";
char *user32 = "user32.dll";
char *messageBox = "MessageBoxA";
HMODULE userModule;


int myStrcmp(char *source, char *dest);
BOOL isPeFileAndGetPointer();
/*
__stdcall是Standard Call的缩写，是C++的标准调用方式,当然这是微软定义的标准，
__stdcall通常用于Win32 API中(可查看WINAPI的定义)。
由被调用者把参数弹出栈。切记：函数自己在退出时清空堆栈，返回值在EAX中。
*/
HMODULE(__stdcall *myLoadLibraryA)(char* moduleName);
int(_stdcall *myGetProcAddress)(HMODULE hModule, LPCSTR lpProcName);
int(_stdcall *myMessageBox)(HWND hWnd, char* lpText, char* lpCaption, UINT uType);

void main() {
	// 通过PEB获取Kernel32.dll的基地址
	//kernel32.dll模块基址 等价于 HMODULE h = LoadLibraryA("kernel32.dll");
	DWORD dwPEB;
	__asm
	{
		mov eax, FS:[0x30]//fs:[30h]指向PEB
		mov dwPEB, eax
	}
	lpBase = *(PDWORD)(*(PDWORD)(*(PDWORD)(*(PDWORD)(*(PDWORD)(dwPEB + 0xc) + 0x1c))) + 8);

	BOOL bResult = isPeFileAndGetPointer();
	if (bResult == TRUE)
	{
		if (pExportDirectory != NULL)
		{
			/// DWORD *functionsAddress = (DWORD)lpBase + RvaToFoa(pExportDirectory->AddressOfFunctions);
			DWORD *functionsAddress = (DWORD)lpBase + pExportDirectory->AddressOfFunctions;
			/// DWORD *functionNamesAddress = (DWORD)lpBase + RvaToFoa(pExportDirectory->AddressOfNames);
			DWORD *functionNamesAddress = (DWORD)lpBase + pExportDirectory->AddressOfNames;
			// 这里强转类型是DWORD因为该字段实际长度是双字,但实际类型要用WORD类型,因为后面要用他作为下标,如果实际类型也是DWORD,数字会大到越界
			/// WORD *nameOrdinalsAddress = (DWORD)lpBase + RvaToFoa(pExportDirectory->AddressOfNameOrdinals);
			WORD *nameOrdinalsAddress = (DWORD)lpBase + pExportDirectory->AddressOfNameOrdinals;
			for (int i = 0; i < pExportDirectory->NumberOfNames; i++)
			{
				if (myStrcmp(strGetProcAddress, (char*)((DWORD)lpBase + functionNamesAddress[i])) == 0)
				{
					// 根据对应函数名的索引值获取AddressOfFunctions对应的函数地址
					DWORD virtualAddress = functionsAddress[nameOrdinalsAddress[i]];
					// 将找到的LoadLibraryA函数地址赋值给函数指针并进行调用
					myGetProcAddress = virtualAddress + (DWORD)lpBase;
				}
				if (myStrcmp(strLoadLibrary, (char*)((DWORD)lpBase + functionNamesAddress[i])) == 0)
				{
					// 根据对应函数名的索引值获取AddressOfFunctions对应的函数地址
					DWORD virtualAddress = functionsAddress[nameOrdinalsAddress[i]];
					// 将找到的LoadLibraryA函数地址赋值给函数指针并进行调用
					myLoadLibraryA = virtualAddress + (DWORD)lpBase;
					userModule = myLoadLibraryA(user32);
				}
			}
			// 在成功获取GetProcAddress和User32.dll地址后,执行对应的代码
			if (userModule != NULL && myGetProcAddress != NULL)
			{
				myMessageBox = myGetProcAddress(userModule, messageBox);
				myMessageBox(NULL, "通过kernel32.dll导出表手动加载对应库函数.", "PE感染", 0);
			}
		}
	}

	__asm
	{
		//mov eax, 0x010031C9//原入口点
		//jmp eax
	}
}

/*
判断是否为有效的PE文件
如果无效直接返回
如果有效则把解析PE格式相关的结构体指针也得到
*/
BOOL isPeFileAndGetPointer() {
	BOOL bResult = FALSE;
	// 判断是否为MZ头
	/// pDosHeader = (PIMAGE_DOS_HEADER)lpBase;
	/// if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	if (*(PWORD)lpBase != IMAGE_DOS_SIGNATURE)
	{
		return bResult;
	}
	// 根据IMAGE_DOS_HEADER中的e_lfanew的值得到PE头的位置
	/// pNtHeader = (PIMAGE_NT_HEADERS)((DWORD)lpBase + pDosHeader->e_lfanew);
	// 判断是否为PE\0\0
	/// if (pNtHeader->Signature != IMAGE_NT_SIGNATURE)
	if (*(PDWORD)((DWORD)lpBase + *(PDWORD)((DWORD)lpBase + 0x3c)) != IMAGE_NT_SIGNATURE)
	{
		return bResult;
	}
	// 获取节表的位置,从NT_HEADER中的&(pNtHeader->OptionalHeader)处加上SizeOfOptionalHeader就是节表的位置了
	// &(pNtHeader->OptionalHeader):取出OptionalHeader在该文件中的偏移地址
	/// pSectionHeaders = (PIMAGE_SECTION_HEADER)((DWORD)&(pNtHeader->OptionalHeader)
	/// 	+ pNtHeader->FileHeader.SizeOfOptionalHeader);
	pSectionHeaders = (PIMAGE_SECTION_HEADER)(((DWORD)lpBase + *(PWORD)((DWORD)lpBase + *(PDWORD)((DWORD)lpBase + 0x3c) + 0x18))
		+ *(PWORD)((DWORD)lpBase + *(PDWORD)((DWORD)lpBase + 0x3c) + 0x14));

	// 获取数据目录表中的导出表的地址: 计算出文件内偏移,再加上该文件映射到内存中的基址就OK
	/// DWORD pExportAddress = (DWORD)lpBase + RvaToFoa(pNtHeader->OptionalHeader.DataDirectory[0].VirtualAddress);
	DWORD pExportAddress = ((DWORD)lpBase + *(PDWORD)((DWORD)lpBase + *(PDWORD)((DWORD)lpBase + 0x3c) + 0x78));

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
手动实现strcmp避免调用库函数
*/
int myStrcmp(char *source, char *dest) {
	while ((*source) && (*source == *dest))
	{
		source++;
		dest++;
	}
	if (*(unsigned char*)source != *(unsigned char*)dest)
		return 1;
	else
		return 0;
}
