#include<Windows.h>
#include<stdio.h>
#include<stdlib.h>
// 参考链接:
// https://bbs.pediy.com/thread-144908.htm
void Demining(int Index);

void main() {
	// 0是把所有雷通过修改内存的方式标记出来
	//Demining(0);
	// 1是模拟鼠标点击去把所有雷标识出来
	Demining(1);
}
void Demining(int Index)
{
	DWORD addr = 0x1005340;
	DWORD x_addr = 0x10056A8;
	DWORD y_addr = 0x10056AC;
	DWORD lei_addr = 0x1005194;
	char X, Y, num;
	unsigned char  old_byte, new_byte;
	DWORD index_x, index_y;

	HWND hwnd = FindWindow(NULL, "扫雷");
	DWORD hProcessId;

	GetWindowThreadProcessId(hwnd, &hProcessId);
	HANDLE Process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, hProcessId);
	if (Process == NULL)
	{
		MessageBox(NULL, "扫雷没有运行!", "错误", MB_OK);
		return;
	}

	ReadProcessMemory(Process, (LPCVOID)x_addr, &X, 1, NULL);    //获取横向方格长度
	ReadProcessMemory(Process, (LPCVOID)y_addr, &Y, 1, NULL);    //获取纵向方格长度
	ReadProcessMemory(Process, (LPCVOID)lei_addr, &num, 1, NULL);

	for (index_x = 1; index_x <= X; index_x++)
	{
		for (index_y = 1; index_y <= Y; index_y++)
		{
			if (Index == 0)
			{
				ReadProcessMemory(Process, (LPCVOID)(addr + (index_x << 5) + index_y), &old_byte, 1, NULL);
				if (old_byte == 0x0e || old_byte == 0x0d)
				{
					new_byte = 0x0f;
					if (old_byte == 0x0e)
					{
						num++;
						WriteProcessMemory(Process, (LPVOID)lei_addr, &num, 1, NULL);
					}
				}
				else if (old_byte == 0x8f || old_byte == 0x8d)
				{
					new_byte = 0x8e;
					num--;
					WriteProcessMemory(Process, (LPVOID)lei_addr, &num, 1, NULL);
				}
				else
				{
					new_byte = old_byte;
				}
				WriteProcessMemory(Process, (LPVOID)(addr + (index_x << 5) + index_y), &new_byte, 1, NULL);
			}
			if (Index == 1)
			{
				/* 
					addr + (index_x << 5):越过上面32个字节的墙
					(addr + (index_x << 5) + index_y):遍历每一列
				*/
				ReadProcessMemory(Process, (LPCVOID)(addr + (index_x << 5) + index_y), &old_byte, 1, NULL);
				// 判断非8E(旗)和非8F(雷),然后模拟鼠标点开他们
				if (!(old_byte & 0x80))
				{
					/* 
					模拟鼠标事件点击的坐标,通过反汇编得出
					;  lParam,低16位X坐标，高16位Y坐标
					0100209C  |.  C1E8 10       SHR EAX,10                               ;  右移16位，取X坐标
					0100209F  |.  83E8 27       SUB EAX,27                               ;  X坐标减去0x27
					010020A2  |.  C1F8 04       SAR EAX,4                                ;  算术右移4位
					010020A5  |.  50            PUSH EAX                                 ; /Arg2
					010020A6  |.  0FB745 14     MOVZX EAX,WORD PTR SS:[EBP+14]           ; |
					010020AA  |.  83C0 04       ADD EAX,4                                ; |Y坐标加0x04
					010020AD  |.  C1F8 04       SAR EAX,4                                ; |算术右移4位
					[ebp+14]这里是lParam的值，在WM_LBUTTONDOWN中，lParam代表了按下左键时的坐标位置。
					这里有一些运算是将用户点击的坐标转换成雷区格子的坐标:
					雷区格子坐标X = (用户点击图形坐标X - 0x27) >> 4
					雷区格子坐标Y = (用户点击图形坐标Y + 0x04) >> 4
					从这里我们可以得出如下结论:
					雷区格子的顶部的X坐标里主程序界面X坐标的距离为0x27;
					雷区格子的顶部的Y坐标里主程序界面X坐标的距离为0x04;
					雷区格子的图形界面为0x04 * 0x04。
					*/
					LPARAM lParam = (((index_x << 4) + 0x27) << 0x10) + (index_y << 4) - 4;
					SendMessage(hwnd, (UINT)WM_LBUTTONDOWN, 0, lParam);
					Sleep(1000);
					SendMessage(hwnd, (UINT)WM_LBUTTONUP, 0, lParam);
				}
			}
		}
	}
	InvalidateRect(hwnd, NULL, TRUE);
	CloseHandle(Process);
}