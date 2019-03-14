#include<Windows.h>
#include<stdio.h>
#include<stdlib.h>
void main1() {

	/*
		01003836: 设置时间为1


		0x01002005: 处理点击之后的操作


		0x0100383B地址处设置定时器
	*/
	// 通过寻找扫雷对应的窗口句柄和获取其进程ID
	HWND hWnd = FindWindowA(NULL, "扫雷");
	DWORD hPid = 0;
	GetWindowThreadProcessId(hWnd, &hPid);

	// 通过进程ID获取其进程句柄
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, hPid);
	/*
		0x01005330:扫雷游戏保存数据的地址
			从该地址开始依次保存了3个DWORD类型的数据:
			雷的数量: 0x63  列数: 0x1E  行数: 0x10
		0x01005340:扫雷游戏保存数据的地址
			此区域保存的是雷区的分布图
			雷区周围用一圈0x10表示墙
			经过测试可得出结论以下值分别表示的意思:
				0x10:墙
				0x0F:空白块
				0x8F:雷
				0x8E:旗
	*/
	DWORD dwDataAddress = 0x01005330;
	// 雷的数量
	DWORD dwNumberOfMine = 0;
	// 实际读取的字节数
	DWORD dwBytesOfRead = 0;
	// 宽高
	DWORD dwHeight = 0, dwWidth = 0;

	// 读取雷的数量
	ReadProcessMemory(hProcess, (LPCVOID)dwDataAddress, &dwNumberOfMine, sizeof(DWORD), &dwBytesOfRead);
	// 读取宽
	ReadProcessMemory(hProcess, (LPCVOID)(dwDataAddress + 4), &dwWidth, sizeof(DWORD), &dwBytesOfRead);
	// 读取宽
	ReadProcessMemory(hProcess, (LPCVOID)(dwDataAddress + 8), &dwHeight, sizeof(DWORD), &dwBytesOfRead);
	// 只针对游戏中的高级版本,需要先判断
	if (dwHeight != 16 || dwWidth != 30)
	{
		printf("游戏非高级难度,失败!\n");
		CloseHandle(hProcess);
		return;
	}
	// 雷区地址
	DWORD dwMineAddress = 0x01005340;
	/* 
		游戏格子数量 = dwHeight  * dwWidth;
		上下墙 = dwHeight  * 2;
		左右墙 = dwWidth * 2;
		4个角落
	*/
	DWORD dwByteOfMineArea = dwHeight  * dwWidth + dwHeight * 2 + dwHeight * 2 + 4;
	// 分配跟雷区同样字节大小的内存来装载该区域
	PBYTE pByte = (PBYTE)malloc(dwByteOfMineArea);

	// 读取整个雷区的数据
	ReadProcessMemory(hProcess, (LPCVOID)dwMineAddress, pByte, dwByteOfMineArea, &dwBytesOfRead);
	// 该值代表旗
	BYTE bClear = 0x8E;
	int n = dwNumberOfMine;
	for (int i = 0; i < dwByteOfMineArea; i++)
	{
		if (pByte[i] == 0x8F) {
			DWORD addressToWrite = dwMineAddress + i;
			WriteProcessMemory(hProcess
				, (LPVOID)addressToWrite, (LPVOID)&bClear, sizeof(BYTE)
				, &dwBytesOfRead);
			n--;
		}
	}

	// 刷新扫雷的客户区
	RECT rectangle;
	// 函数的作用总的来说就是把客户区的大小写进第二个参数所指的Rect结构当中。
	GetClientRect(hWnd, &rectangle);
	// 该函数向指定的窗体更新区域添加一个矩形，然后窗口客户区域的这一部分将被重新绘制。
	InvalidateRect(hWnd, &rectangle, TRUE);

	// 释放存放雷区数据的内存空间
	free(pByte);

	CloseHandle(hProcess);

	printf("雷的数量99,还剩%d个雷没搞定.\n", n);

	system("pause");
}