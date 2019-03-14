//万能补丁码
//获取LoadLibraryA的函数地址并调用
//
//
//release版
//保留从未引用的函数和数据    否 (/OPT:NOREF)
//优化 已禁用 (/Od)
//禁用安全检查 (/GS-)
//
//
//
//因为notepad的.text剩余只有0x100左右的空间
//而直接复制之前的函数生成的指令太多，大概有0x300左右
//所以不直接复制之前的函数，而是把那些语句尽量合并，精简，最后生成指令大小为0xe1
//所以这个程序看起来有点不好理解，
//可以不看这个程序，直接用随书代码生成的指令，，
//
//
//如果是win7x64,用SysWOW64下的notepad
//win7的notepad有重定位，需要把重定位去掉(IMAGE_NT_HEADERS32.IMAGE_FILE_HEADER.Characteristics | 1)才能行，
//不然程序加载的时候会把写入的数据修改了，会运行失败
//把入口点(IMAGE_NT_HEADERS32.AddressOfEntryPoint.AddressOfEntryPoint)改为0xb700
//从文件偏移0xab00(0xb700的文件偏移) 处开始写入
//写入的数据来自13-07_getLoadLib.exe
//文件偏移0x400到0x4e1
//
//
//把13-08_winResult.dll名字改为pa.dll
//和打补丁后的notepad放在同一目录，然后运行notepad
//
//


#include <Windows.h>


int  main1()
{


	DWORD dwPEB;
	DWORD dwDllBase;//当前地址
	PIMAGE_EXPORT_DIRECTORY pImageExportDirectory;//指向导出表的指针

	TCHAR ** lpAddressOfNames;
	PWORD lpAddressOfNameOrdinals;
	DWORD szStr[] = { 0x64616f4c, 0x7262694c, 0x41797261, //"LoadLibraryA"
		0x00617000 };//"pa"



	__asm
	{
		mov eax, FS:[0x30]//fs:[30h]指向PEB
		mov dwPEB, eax
	}


	//kernel32.dll模块基址 等价于 HMODULE h = LoadLibraryA("kernel32.dll");
	dwDllBase = *(PDWORD)(*(PDWORD)(*(PDWORD)(*(PDWORD)(*(PDWORD)(dwPEB + 0xc) + 0x1c))) + 8);

	//kernel32.dll中指向导出表的指针
	/*
		(dwDllBase + 0x3c): DOS头中的e_lfanew
		*(PDWORD)(dwDllBase + *(PDWORD)(dwDllBase + 0x3c): NTHeader
		*(PDWORD)(dwDllBase + *(PDWORD)(dwDllBase + 0x3c) + 0x78: DataDirectory
	*/
	pImageExportDirectory = (PIMAGE_EXPORT_DIRECTORY)(dwDllBase + *(PDWORD)(dwDllBase + *(PDWORD)(dwDllBase + 0x3c) + 0x78));

	//按名字导出函数列表
	lpAddressOfNames = (TCHAR **)(dwDllBase + pImageExportDirectory->AddressOfNames);

	/*
		这里写两个循环的目的:就相当于手动实现了一个strcmp函数
		原因可能如下:
			因为这里是注入到其他PE文件中,所以不方便调用其他字符串处理函数
		i: 先判断两个函数的首字母是否相同
		j: 在首字母相同的情况,再逐个比对剩余字符.源字符串数据中在LoadLibraryA和pa中间隔了一个0,用作字符串结束标志位.
		如果源字符串指针移动到了0下标处,说明两个字符串完全相同.
	*/
	int i = 0;
	do
	{
		int j = 0;
		do
		{
			if (((PTCHAR)szStr)[j] != (dwDllBase + lpAddressOfNames[i])[j])
			{
				break;
			}

			if (((PTCHAR)szStr)[j] == 0)
			{
				lpAddressOfNameOrdinals = (PWORD)(dwDllBase +
					pImageExportDirectory->AddressOfNameOrdinals);//按名字导出函数索引列表

				((PROC)(dwDllBase + ((PDWORD)(dwDllBase + pImageExportDirectory->AddressOfFunctions))
					[lpAddressOfNameOrdinals[i]]))((PTCHAR)szStr + 0xd);//调用LoadLibraryA加载pa,0xd正好是p的偏移

				__asm
				{
					leave	//平衡堆栈
					mov eax, 0x010031C9//原入口点
					jmp eax
				}
			}
		} while (++j);
	} while (++i);

	return 0;
}

