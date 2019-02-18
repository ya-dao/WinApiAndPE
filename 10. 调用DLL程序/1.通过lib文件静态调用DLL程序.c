#include<Windows.h>
/*
	静态调用:通过链接器将DLL的导出函数写进可执行文件.
	默认情况下,需要将.lib文件放置于项目目录下,否则在编译时会出现无法打开.lib文件的错误.
	.dll程序需要放在可执行文件目录下,否则在运行时会出现丢失dll文件的错误.
*/
/*
	告诉链接器需要在9.DLL编程.lib文件中找到DLL中导出函数的信息
*/
#pragma comment (lib, "9.DLL编程.lib")
void MsgBox(char *szMsg);

void main() {
	MsgBox("try to invoke MsgBox method in dll");
	system("pause");
}