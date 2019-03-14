#include<Windows.h>
void main3() {
	//__asm
	//{
	//	/*
	//		这个调用的原理是将需要传入的字符串参数先压入栈中,然后向函数传入对应字符串在栈中的地址作为参数.
	//	*/
	//	push word ptr 0
	//	push '4321'
	//	push word ptr 0
	//	push 'dcba'
	//	mov eax, esp
	//	// 开始传入参数,对话框类型
	//	push 0
	//	// Title
	//	push eax
	//	// 因为上面的字符串在栈中占了6个字节(字符串4 + 结束符'0'2)
	//	add eax, 6
	//	push eax
	//	// 第一个参数
	//	push 0
	//	call MessageBoxA
	//	add esp, 8
	//}


	// 5045cec4 bcfeb8d0 c8be0000
	//char test1[] = "PE文件感染";
	//// b8c3cec4 bcfed2d1 b1bbb8d0 c8be0000
	//char test2[] = "该文件已被感染";
	//__asm
	//{
	//	push    0
	//	lea     EAX, test1
	//	push    EAX
	//	lea     EAX, test2        // 局部变量用LEA 
	//	push    EAX
	//	push    0
	//	call    DWORD PTR[MessageBoxA]
	//}
}
/*
	1. 获取GetProcAddress地址
	2. 获取LoadLibrary地址
	3. 加载User32.dll模块
	4. 获取MessageBoxA地址
	5. 执行MessageBoxA函数
*/
void main4() {
	/*
	HMODULE a = LoadLibraryA("user32");
	FARPROC b = GetProcAddress(a,"MessageBoxA");
	MessageBoxA(NULL, "tes1", "tes2", 0);
	*/

	/*
		总结调试出来的_asm规则:
			1. 栈必须以16位对齐
			2. 调用库函数传参的原理是将需要传入的字符串参数先压入栈中,然后向函数传入对应字符串在栈中的地址作为参数.
				这就是为什么每次都是先向栈中压入字符串,再压入ESP的原因
			3. Windows函数传参的顺序是,从右到左压栈
	*/
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
				PUSH  DWORD PTR 0XD0B8FEBC;
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
}