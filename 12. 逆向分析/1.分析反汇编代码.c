#define _CRT_SECURE_NO_WARNINGS
#include<Windows.h>
#include<stdio.h>
#include<stdlib.h>
void showFor() {
	int nSum = 0;
	for (int i = 0; i < 100; i++)
	{
		nSum += i;
	}

	printf("nSum = %d\n", nSum);
}

void showDoWhile() {
	int nNum = 1, nSum = 0;

	do {
		nSum += nNum;
		nNum++;
	} while (nNum <= 100);

	printf("nSum = %d\n", nSum);
}

void showWhile() {
	int nNum = 1, nSum = 0;

	while (nNum <= 100) {
		nSum += nNum;
		nNum++;
	}

	printf("nSum = %d\n", nSum);
}

void showSwitch() {
	int nNum = 0;
	scanf("%d", &nNum);
	switch (nNum)
	{
	case 1:
		printf("1\n");
		break;
	case 2:
		printf("2\n");
		break;
	case 3:
		printf("3\n");
		break;
	case 4:
		printf("4\n");
		break;
	default:
		break;
	}
}

void main() {
	showSwitch();
	showFor();
	showDoWhile();
	showWhile();
	system("pause");
}