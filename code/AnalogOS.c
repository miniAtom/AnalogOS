/**
 * @Author: GWL
 * @Date:   2022-07-03 09:29:12
 * @Last Modified by:   GWL
 * @Last Modified time: 2022-07-03 21:04:56
 */

#include "myLib.h"

int main()
{
	// 申请100MB内存
	void *space = calloc(1, SPACE_SIZE);
	if (!space)
	{
		printf("内存申请失败！\n");
		system("pause");
		exit(1);
	}
	printf("\n%s 内存申请成功！地址为：%p\n", bytesToStr(SPACE_SIZE), space);

	// 采用伙伴算法，空闲块链表
	int maxBlock = 1;
	while ((1 << maxBlock) <= SPACE_SIZE)
	{
		maxBlock++;
	}
	// printf("最大块为 %s\n", bytesToStr(1 << maxBlock - 1));
	SPACE_NODE **freeSpaceArray = (SPACE_NODE **)calloc(maxBlock, sizeof(SPACE_NODE *));
	for (int i = 0; i < maxBlock; i++)
	{
		freeSpaceArray[i] = (SPACE_NODE *)calloc(1, sizeof(SPACE_NODE));
	}
	// 已使用空间和剩余空间
	long long usedSpace = 0, freeSpace = SPACE_SIZE;
	// checkSpace(usedSpace, freeSpace);

	// 记录空闲内存块地址和大小
	FREE_SPACE_NODE *freeSpaceListHead = (FREE_SPACE_NODE *)calloc(1, sizeof(FREE_SPACE_NODE));

	// 将空间装进空闲链表(尾插法)
	if (1)
	{
		FREE_SPACE_NODE *tail = freeSpaceListHead;
		int i = maxBlock - 1;
		while (freeSpace)
		{
			for (; i >= 0; i--)
			{
				if (freeSpace >= (1 << i))
				{
					// printf("%s\n", bytesToStr(1 << i));
					SPACE_NODE *t = (SPACE_NODE *)calloc(1, sizeof(SPACE_NODE));
					t->pointer = space + usedSpace;
					t->next = freeSpaceArray[i]->next;
					freeSpaceArray[i]->next = t;
					FREE_SPACE_NODE *p = (FREE_SPACE_NODE *)calloc(1, sizeof(FREE_SPACE_NODE));
					p->pointer = space + usedSpace;
					p->size = (1 << i);
					p->next = freeSpaceListHead;
					p->pre = tail;
					p->next->pre = p;
					p->pre->next = p;
					tail = p;
					freeSpace -= (1 << i);
					usedSpace += (1 << i);
					break;
				}
			}
		}
	}
	// checkFreeSpaceList(freeSpaceListHead);
	usedSpace = 0, freeSpace = SPACE_SIZE;
	// 空间使用情况
	char *blockStatus = (char *)calloc(SPACE_SIZE, sizeof(char));
	memset(blockStatus, '0', SPACE_SIZE * sizeof(char));

	// 记录已使用的指针指向的字节数
	long long *usedHash = (long long *)calloc(SPACE_SIZE, sizeof(long long));

	char *s = (char *)calloc(STR_SIZE, sizeof(char));
	while (1)
	{
		printf("\n请输入命令：");
		gets(s);
		if (strcmp(s, "exit()") == 0)
		{
			break;
		}
		eval(s, freeSpaceArray, maxBlock, &usedSpace, &freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	}

	//释放之前申请的空间
	for (int i = 0; i < maxBlock; i++)
	{
		while (freeSpaceArray[i]->next)
		{
			SPACE_NODE *t = freeSpaceArray[i]->next;
			freeSpaceArray[i]->next = t->next;
			free(t);
		}
		free(freeSpaceArray[i]);
	}
	free(freeSpaceArray);
	free(blockStatus);
	free(usedHash);
	while (freeSpaceListHead->next != freeSpaceListHead)
	{
		FREE_SPACE_NODE *node = freeSpaceListHead->next;
		freeSpaceListHead->next = node->next;
		free(node);
	}
	free(freeSpaceListHead);
	free(space);
	space = NULL;
	system("pause");
}