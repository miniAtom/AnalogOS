/**
 * @Author: GWL
 * @Date:   2022-07-03 09:29:12
 * @Last Modified by:   GWL
 * @Last Modified time: 2022-07-03 22:16:30
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPACE_SIZE 100 * 1024 * 1024
#define STR_SIZE 100
typedef _Bool bool;
typedef long long ELEMENT_TYPE;

// 伙伴链表节点
typedef struct spaceNode
{
	void *pointer;
	struct spaceNode *next;
} SPACE_NODE;

// 空闲链表节点
typedef struct usedSpaceNode
{
	void *pointer;
	long long size;
	struct usedSpaceNode *pre;
	struct usedSpaceNode *next;
} FREE_SPACE_NODE;

// 链表节点
typedef struct listNode
{
	ELEMENT_TYPE val;
	struct listNode *next;
} LIST_NODE;

// 大根堆
typedef struct
{
	ELEMENT_TYPE *nums;
	long long size, maxSize;
} HEAP;

// 栈
typedef struct
{
	LIST_NODE *head;
	long long size;
} STACK;

// 队列
typedef struct
{
	LIST_NODE *head, *tail;
	long long size;
} QUEUE;

// 树节点
typedef struct treeNode
{
	ELEMENT_TYPE val;
	struct treeNode *left, *right, *parent;
} TREE_NODE;

// 树
typedef struct
{
	TREE_NODE *root;
	long long size;
} TREE;

// 边
typedef struct edgeNode
{
	ELEMENT_TYPE val;
	LIST_NODE *listHead;
	struct edgeNode *next;
} EDGE_NODE;

// 图
typedef struct
{
	// 存储顶点
	LIST_NODE *nodeHead;
	// 存储边
	EDGE_NODE *edgeHead;
	long long edgeSize, nodeSize;
} MAP;

// 判断地址是否合法
bool addressIsValid(void *address, void *space)
{
	if (address < space || address >= space + SPACE_SIZE)
	{
		printf("%p 超出地址范围！\n", address);
		printf("正确地址范围为：[%p, %p]\n", space, space + SPACE_SIZE - 1);
		return 0;
	}
	return 1;
}

// 字节数转为带单位的字符串
char *bytesToStr(long long size)
{
	char *s = (char *)calloc(100, sizeof(char));
	if (size == 0)
	{
		sprintf(s, "%7.2f MB", size);
		return s;
	}
	char unit[][3] = {" B", "KB", "MB", "GB", "TB", "PB", "EB"};
	int i = 0;
	while ((long long)pow(1024, i) <= size)
	{
		i++;
	}
	i--;
	float num = size / pow(1024, i);
	sprintf(s, "%7.2f %s", num, unit[i]);
	return s;
}

// 查看空间总体使用情况
void checkSpace(int usedSpace, int freeSpace)
{
	// printf("\n");
	printf("已使用空间：%s\n", bytesToStr(usedSpace));
	printf("  剩余空间：%s\n", bytesToStr(freeSpace));
	printf("    总空间：%s\n", bytesToStr(usedSpace + freeSpace));
}

// 查看内存块使用情况
void checkBlock(void *space, char *blockStatus, void *a)
{
	// printf("\n");
	if (blockStatus[a - space] == '0')
	{
		printf("%p 未使用！\n", a);
	}
	else
	{
		printf("%p 已使用！\n", a);
	}
}

// 查看空闲块链表
void checkFreeSpaceList(FREE_SPACE_NODE *freeSpaceListHead)
{
	printf("空闲块链表：\n");
	FREE_SPACE_NODE *t = freeSpaceListHead->next;
	while (t != freeSpaceListHead)
	{
		printf("%s：%p\n", bytesToStr(t->size), t->pointer);
		t = t->next;
	}
}

// 查看伙伴数组
void checkFreeSpaceArray(SPACE_NODE **freeSpaceArray, int maxBlock)
{
	printf("\n伙伴数组：\n");
	for (int i = maxBlock - 1; i >= 0; i--)
	{
		SPACE_NODE *t = freeSpaceArray[i]->next;
		while (t)
		{
			printf("%s：%p\n", bytesToStr(1 << i), t->pointer);
			t = t->next;
		}
	}
}

int countBit(long long size)
{
	int bit = 0;
	while ((1 << bit) < size)
	{
		bit++;
	}
	return bit;
}

// 寻找目标节点的前一个
SPACE_NODE *findPreNodeInArray(void *pointer, SPACE_NODE **freeSpaceArray, long long size)
{
	int bit = countBit(size);
	SPACE_NODE *t = freeSpaceArray[bit];
	while (t->next && t->next->pointer != pointer)
	{
		t = t->next;
	}
	return t;
}

void myFree(void *pointer, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	long long size = usedHash[pointer - space];
	if (!size)
	{
		printf("地址未使用，无需释放！\n");
		return;
	}
	usedHash[pointer - space] = 0;
	*usedSpace -= size;
	*freeSpace += size;
	memset(&blockStatus[pointer - space], '0', sizeof(char) * size);
	memset(pointer, 0, size);
	// 在空闲链表插入新节点
	FREE_SPACE_NODE *nowNode = (FREE_SPACE_NODE *)calloc(1, sizeof(FREE_SPACE_NODE));
	nowNode->pointer = pointer;
	nowNode->size = size;
	FREE_SPACE_NODE *t = freeSpaceListHead;
	while (t->next->pointer < nowNode->pointer + size && t->next != freeSpaceListHead)
	{
		t = t->next;
	}
	nowNode->next = t->next;
	nowNode->pre = t;
	nowNode->next->pre = nowNode->pre->next = nowNode;
	// 把能合并的全合并，再重新划分
	// 合并要求：下一个不是头，地址相连
	// 右合并
	while (nowNode->next != freeSpaceListHead && nowNode->pointer + nowNode->size == nowNode->next->pointer)
	{
		FREE_SPACE_NODE *temNowNode = nowNode->next;
		// 合并
		nowNode->size += temNowNode->size;
		// 在伙伴数组中删除
		int bit = countBit(temNowNode->size);
		SPACE_NODE *preNodeInArray = findPreNodeInArray(temNowNode->pointer, freeSpaceArray, temNowNode->size);
		SPACE_NODE *nowNodeInArray = preNodeInArray->next;
		preNodeInArray->next = preNodeInArray->next->next;
		free(nowNodeInArray);
		// 在空闲链表中删除
		temNowNode->next->pre = temNowNode->pre;
		temNowNode->pre->next = temNowNode->next;
		free(temNowNode);
	}
	// 左合并
	while (nowNode->pre != freeSpaceListHead && nowNode->pointer == nowNode->pre->pointer + nowNode->pre->size)
	{
		FREE_SPACE_NODE *temNowNode = nowNode->pre;
		// 合并
		nowNode->size += temNowNode->size;
		nowNode->pointer -= temNowNode->size;
		// 在伙伴数组中删除
		int bit = countBit(temNowNode->size);
		SPACE_NODE *preNodeInArray = findPreNodeInArray(temNowNode->pointer, freeSpaceArray, temNowNode->size);
		SPACE_NODE *nowNodeInArray = preNodeInArray->next;
		preNodeInArray->next = preNodeInArray->next->next;
		free(nowNodeInArray);
		// 在空闲链表中删除
		temNowNode->next->pre = temNowNode->pre;
		temNowNode->pre->next = temNowNode->next;
		free(temNowNode);
	}
	// 拆分
	int i = maxBlock - 1;
	// resSize = temNowNode->size;
	FREE_SPACE_NODE *tail = nowNode->pre;
	// 把nowNode从空闲链表里取下来
	nowNode->pre->next = nowNode->next;
	nowNode->next->pre = nowNode->pre;
	while (nowNode->size)
	{
		for (; i >= 0; i--)
		{
			if (nowNode->size >= (1 << i))
			{
				FREE_SPACE_NODE *listNode = (FREE_SPACE_NODE *)calloc(1, sizeof(FREE_SPACE_NODE));
				listNode->pointer = nowNode->pointer;
				listNode->size = (1 << i);
				listNode->next = tail->next;
				listNode->pre = tail;
				listNode->next->pre = listNode;
				listNode->pre->next = listNode;
				tail = listNode;
				SPACE_NODE *arrNode = (SPACE_NODE *)calloc(1, sizeof(SPACE_NODE));
				arrNode->pointer = nowNode->pointer;
				arrNode->next = freeSpaceArray[i]->next;
				freeSpaceArray[i]->next = arrNode;
				nowNode->pointer += (1 << i);
				nowNode->size -= (1 << i);
				break;
			}
		}
	}
	free(nowNode);
	printf("%p 释放成功！\n", pointer);
}

void *myMalloc(long long size, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (size <= 0)
	{
		printf("内存大小不合法！\n");
		return NULL;
	}
	// printf("\n");
	if (size > *freeSpace)
	{
		printf("剩余空间不足！\n");
		printf("您申请的空间大小为：%s，但空间仅剩余%s\n", bytesToStr(size), bytesToStr(*freeSpace));
		return NULL;
	}

	// 计算符合条件的最小块
	int blockBit = countBit(size);
	// printf("符合要求的最小块为：%s\n", bytesToStr(1 << blockBit));
	bool flag = 0;
	SPACE_NODE *t = NULL;
	void *ans = NULL;
	for (int i = blockBit; i < maxBlock; i++)
	{
		if (freeSpaceArray[i]->next)
		{
			t = freeSpaceArray[i]->next;
			// 把t从伙伴数组取下
			freeSpaceArray[i]->next = t->next;
			flag = 1;
			*usedSpace += size, *freeSpace -= size;
			memset(&blockStatus[t->pointer - space], '1', size);
			ans = t->pointer;
			usedHash[ans - space] = size;

			FREE_SPACE_NODE *nowNode = freeSpaceListHead->next;
			while (nowNode->pointer != t->pointer)
			{
				nowNode = nowNode->next;
			}

			// 处理剩下的那部分
			if (size < (1 << i))
			{
				long long resSize = (1 << i) - size;
				nowNode->pointer = ans + size;
				nowNode->size = resSize;
				// 把能合并的全合并，再重新划分
				// 合并要求：下一个不是头，地址相连
				while (nowNode->next != freeSpaceListHead && nowNode->pointer + nowNode->size == nowNode->next->pointer)
				{
					FREE_SPACE_NODE *temNowNode = nowNode->next;
					// 合并
					nowNode->size += temNowNode->size;
					// 在伙伴数组中删除
					int bit = countBit(temNowNode->size);
					SPACE_NODE *preNodeInArray = findPreNodeInArray(temNowNode->pointer, freeSpaceArray, temNowNode->size);
					SPACE_NODE *nowNodeInArray = preNodeInArray->next;
					preNodeInArray->next = preNodeInArray->next->next;
					free(nowNodeInArray);
					// 在空闲链表中删除
					temNowNode->next->pre = temNowNode->pre;
					temNowNode->pre->next = temNowNode->next;
					free(temNowNode);
				}
				// 拆分
				int i = maxBlock - 1;
				// resSize = temNowNode->size;
				FREE_SPACE_NODE *tail = nowNode->pre;
				// 把nowNode从空闲链表里取下来
				nowNode->pre->next = nowNode->next;
				nowNode->next->pre = nowNode->pre;
				while (nowNode->size)
				{
					for (; i >= 0; i--)
					{
						if (nowNode->size >= (1 << i))
						{
							FREE_SPACE_NODE *listNode = (FREE_SPACE_NODE *)calloc(1, sizeof(FREE_SPACE_NODE));
							listNode->pointer = nowNode->pointer;
							listNode->size = (1 << i);
							listNode->next = tail->next;
							listNode->pre = tail;
							listNode->next->pre = listNode;
							listNode->pre->next = listNode;
							tail = listNode;
							SPACE_NODE *arrNode = (SPACE_NODE *)calloc(1, sizeof(SPACE_NODE));
							arrNode->pointer = nowNode->pointer;
							arrNode->next = freeSpaceArray[i]->next;
							freeSpaceArray[i]->next = arrNode;
							nowNode->pointer += (1 << i);
							nowNode->size -= (1 << i);
							break;
						}
					}
				}
				free(nowNode);
			}
			else
			{
				nowNode->next->pre = nowNode->pre;
				nowNode->pre->next = nowNode->next;
				free(nowNode);
				free(t);
			}
			break;
		}
	}
	if (flag)
	{
		printf("%s 内存申请成功！地址为：%p\n", bytesToStr(size), t->pointer);
		return ans;
	}
	else
	{
		printf("没有满足要求的内存块，空间分配失败！\n");
		return NULL;
	}
}

void *myCalloc(long long num, long long size, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	void *ans = myMalloc(num * size, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	if (ans)
	{
		memset(ans, 0, num * size);
	}
	return ans;
}

// --------------------链表开始--------------------
// 创建空的链表
LIST_NODE *initList(SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	LIST_NODE *head = (LIST_NODE *)myCalloc(1, sizeof(LIST_NODE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	if (head)
	{
		printf("链表创建成功！地址为：%p\n", head);
	}
	else
	{
		printf("链表创建失败！\n");
	}
	return head;
}

// 回收链表
void delList(LIST_NODE *head, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)head - space])
	{
		printf("错误！链表不存在！\n");
		return;
	}
	while (head->next)
	{
		LIST_NODE *t = head->next;
		// printf("%lld", usedHash[(void *)t - space]);
		head->next = t->next;
		myFree(t, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	}
	myFree(head, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	printf("链表删除成功！\n");
}

// 判断链表是否为空
bool listEmpty(LIST_NODE *head, void *space, long long *usedHash)
{
	if (!usedHash[(void *)head - space])
	{
		printf("链表不存在！\n");
		return 0;
	}
	if (head->next)
	{
		printf("链表非空！\n");
	}
	else
	{
		printf("链表为空！\n");
	}
	return head->next != NULL;
}

// 求链表长度
long long listLength(LIST_NODE *head, void *space, long long *usedHash)
{
	if (!usedHash[(void *)head - space])
	{
		printf("错误！链表不存在！\n");
		return -1;
	}
	long long ans = 0;
	LIST_NODE *t = head->next;
	while (t)
	{
		ans++;
		t = t->next;
	}
	printf("链表长度为%lld\n", ans);
	return ans;
}

// 读取链表节点元素
int getListNodeVal(LIST_NODE *node, void *space, long long *usedHash)
{
	if (!usedHash[(void *)node - space])
	{
		printf("获取失败，该节点为空！\n");
		return 0;
	}
	printf("节点元素为：%lld\n", node->val);
	return node->val;
}

int compare(ELEMENT_TYPE a, ELEMENT_TYPE b)
{
	return a - b;
}

// 查找链表某元素的位序，不存在返回0
long long findListNode(LIST_NODE *head, ELEMENT_TYPE val, void *space, long long *usedHash)
{
	if (!usedHash[(void *)head - space])
	{
		printf("错误！链表不存在！\n");
		return -1;
	}
	LIST_NODE *t = head->next;
	for (long long i = 1; t; i++, t = t->next)
	{
		if (compare(val, t->val) == 0)
		{
			printf("元素位于链表的第%lld位\n", i);
			return i;
		}
	}
	printf("元素不存在于链表中！\n");
	return 0;
}

// 查找链表前驱
void listPreNode(LIST_NODE *head, ELEMENT_TYPE val, void *space, long long *usedHash)
{
	if (!usedHash[(void *)head - space])
	{
		printf("错误！链表不存在！\n");
		return;
	}
	LIST_NODE *t = head;
	for (long long i = 1; t->next; i++, t = t->next)
	{
		if (compare(val, t->next->val) == 0)
		{
			if (t == head)
			{
				printf("元素位于链表第1位，无前驱\n");
				return;
			}
			printf("元素的前驱地址为：%p，值为：%lld\n", t, t->val);
			return;
		}
	}
	printf("元素不存在于链表中！\n");
	return;
}

// 查找链表元素后继
void listNextNode(LIST_NODE *head, ELEMENT_TYPE val, void *space, long long *usedHash)
{
	if (!usedHash[(void *)head - space])
	{
		printf("错误！链表不存在！\n");
		return;
	}
	LIST_NODE *t = head->next;
	for (long long i = 1; t; i++, t = t->next)
	{
		if (compare(val, t->val) == 0)
		{
			if (t->next == NULL)
			{
				printf("元素位于链表最后一位，无后继\n");
				return;
			}
			printf("元素的后继地址为：%p，值为：%lld\n", t->next, t->next->val);
			return;
		}
	}
	printf("元素不存在于链表中！\n");
	return;
}

// 遍历链表
void visitList(LIST_NODE *head, void *space, long long *usedHash)
{
	if (!usedHash[(void *)head - space])
	{
		printf("错误！链表不存在！\n");
		return;
	}
	LIST_NODE *t = head->next;
	if (!t)
	{
		printf("链表为空！\n");
		return;
	}
	printf("遍历链表：\n");
	for (long long i = 1; t; i++, t = t->next)
	{
		printf("第%lld个元素值为：%lld，地址为%p\n", i, t->val, t);
	}
	printf("链表遍历结束！\n");
}

// 将链表置空
void clearList(LIST_NODE *head, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)head - space])
	{
		printf("错误！链表不存在！\n");
		return;
	}
	while (head->next)
	{
		LIST_NODE *t = head->next;
		// printf("%lld", usedHash[(void *)t - space]);
		head->next = t->next;
		myFree(t, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	}
	printf("成功将链表置空！\n");
}

// 修改链表元素的值
void changeListVal(LIST_NODE *head, long long pos, ELEMENT_TYPE val, void *space, long long *usedHash)
{
	if (!usedHash[(void *)head - space])
	{
		printf("错误！链表不存在！\n");
		return;
	}
	LIST_NODE *t = head->next;
	for (long long i = 1; t; i++, t = t->next)
	{
		if (i == pos)
		{
			t->val = val;
			printf("成功将链表第%lld个元素更改为%lld\n", pos, val);
			return;
		}
	}
	printf("更改失败！链表不存在第%lld个元素！\n", pos);
}

// 往链表第i个元素后插入新元素
void addListNode(LIST_NODE *head, long long pos, ELEMENT_TYPE val, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)head - space])
	{
		printf("错误！链表不存在！\n");
		return;
	}
	if (pos < 0)
	{
		printf("插入失败！链表不存在第%lld个元素！\n", pos);
		return;
	}
	LIST_NODE *t = head;
	for (long long i = 0; t; i++, t = t->next)
	{
		if (i == pos)
		{
			LIST_NODE *newNode = (LIST_NODE *)myCalloc(1, sizeof(LIST_NODE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
			newNode->next = t->next;
			t->next = newNode;
			newNode->val = val;
			printf("成功在链表第%lld个元素后插入新元素%lld\n", pos, val);
			return;
		}
	}
	printf("插入失败！链表不存在第%lld个元素！\n", pos);
}

// 删除链表第i个元素
void delListNode(LIST_NODE *head, long long pos, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)head - space])
	{
		printf("错误！链表不存在！\n");
		return;
	}
	if (pos <= 0)
	{
		printf("删除失败！链表不存在第%lld个元素！\n", pos);
		return;
	}
	LIST_NODE *t = head;
	for (long long i = 1; t; i++, t = t->next)
	{
		if (i == pos)
		{
			LIST_NODE *p = t->next;
			t->next = t->next->next;
			myFree(p, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
			printf("成功删除链表第%lld个元素\n", pos);
			return;
		}
	}
	printf("删除失败！链表不存在第%lld个元素！\n", pos);
}

// --------------------链表结束--------------------

// --------------------数组开始--------------------

// 创建数组
ELEMENT_TYPE *initArray(long long n, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	ELEMENT_TYPE *arr = (ELEMENT_TYPE *)myCalloc(n, sizeof(ELEMENT_TYPE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	if (arr)
	{
		printf("数组创建成功！地址为：%p\n", arr);
	}
	else
	{
		printf("输出创建失败！\n");
	}
	return arr;
}

// 删除数组
void delArray(ELEMENT_TYPE *arr, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)arr - space])
	{
		printf("删除失败，数组不存在！\n");
	}
	myFree(arr, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	printf("数组删除成功！\n");
}

// 获取数组指定位置的值
ELEMENT_TYPE getArrayVal(ELEMENT_TYPE *arr, long long pos, void *space, long long *usedHash)
{
	if (!usedHash[(void *)arr - space])
	{
		printf("数组不存在！\n");
		return -1;
	}
	if (pos < 0 || pos >= usedHash[(void *)arr - space] / sizeof(ELEMENT_TYPE))
	{
		printf("数组越界！\n");
		return -1;
	}
	printf("数组第%lld位元素为：%lld\n", pos, arr[pos]);
	return arr[pos];
}

// 修改数组指定位置的值
void changeArrayVal(ELEMENT_TYPE *arr, long long pos, ELEMENT_TYPE val, void *space, long long *usedHash)
{
	if (!usedHash[(void *)arr - space])
	{
		printf("数组不存在！\n");
		return;
	}
	if (pos < 0 || pos >= usedHash[(void *)arr - space] / sizeof(ELEMENT_TYPE))
	{
		printf("数组越界！\n");
		return;
	}
	arr[pos] = val;
	printf("成功将数组第%lld位元素修改为%lld\n", pos, arr[pos]);
}

// 遍历数组
void visitArray(ELEMENT_TYPE *arr, void *space, long long *usedHash)
{
	if (!usedHash[(void *)arr - space])
	{
		printf("数组不存在！\n");
		return;
	}
	long long size = usedHash[(void *)arr - space] / sizeof(ELEMENT_TYPE);
	printf("数组元素为：\n");
	printf("%lld", arr[0]);
	for (long long i = 1; i < size; i++)
	{
		printf(", %lld", arr[i]);
	}
	printf("\n遍历结束！\n");
}

// --------------------数组结束--------------------

// --------------------堆开始--------------------

// 创建空堆(大根堆)
HEAP *initHeap(long long maxSize, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	HEAP *heap = (HEAP *)myCalloc(1, sizeof(HEAP), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	if (!heap)
	{
		printf("堆创建失败！\n");
		return NULL;
	}
	heap->maxSize = maxSize;
	heap->nums = (ELEMENT_TYPE *)myCalloc(1, sizeof(ELEMENT_TYPE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	if (!heap->nums)
	{
		myFree(heap, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		printf("堆创建失败！\n");
		return NULL;
	}
	printf("堆创建成功！地址为：%p\n", heap);
	return heap;
}

// 删除堆
void delHeap(HEAP *heap, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)heap - space])
	{
		printf("堆不存在！\n");
		return;
	}
	myFree(heap->nums, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	myFree(heap, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	printf("堆删除成功！\n");
}

// 清空堆
void clearHeap(HEAP *heap, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)heap - space])
	{
		printf("堆不存在！\n");
		return;
	}
	heap->size = 0;
	printf("堆清空成功！\n");
}

void swap(ELEMENT_TYPE *a, ELEMENT_TYPE *b)
{
	ELEMENT_TYPE t = *a;
	*a = *b;
	*b = t;
}

// 大根堆
int adjust(ELEMENT_TYPE *nums, int x, int n)
{
	int pos = x + 1;
	int left = 2 * pos - 1, right = 2 * pos + 1 - 1;
	// 左右子树都存在
	if (right < n)
	{
		if (nums[left] >= nums[x] && nums[left] >= nums[right])
		{
			swap(&nums[left], &nums[x]);
			return -1;
		}
		else if (nums[right] >= nums[x] && nums[right] >= nums[left])
		{
			swap(&nums[right], &nums[x]);
			return 1;
		}
	}
	else if (left < n && right >= n)
	{
		// 只有左子树
		if (nums[left] > nums[x])
		{
			swap(&nums[left], &nums[x]);
			return -1;
		}
	}
	return 0;
}

void adjustHeap(ELEMENT_TYPE *nums, int x, int n)
{
	int pos = x + 1;
	int flag = adjust(nums, pos - 1, n);
	while (flag)
	{
		pos *= 2;
		if (flag == -1)
		{
			flag = adjust(nums, pos - 1, n);
		}
		else
		{
			pos++;
			flag = adjust(nums, pos - 1, n);
		}
	}
}

void adjustAllHeap(ELEMENT_TYPE *nums, int n)
{
	for (int i = n / 2; i >= 0; i--)
	{
		adjustHeap(nums, i, n);
	}
}

// 插入堆
void heapPush(HEAP *heap, ELEMENT_TYPE val, void *space, long long *usedHash)
{
	if (!usedHash[(void *)heap - space])
	{
		printf("堆不存在！\n");
		return;
	}
	if (heap->size >= heap->maxSize)
	{
		printf("堆已满，无法添加！\n");
		return;
	}
	heap->nums[heap->size++] = val;
	adjustAllHeap(heap->nums, heap->size);
	printf("入堆成功！\n");
}

// 输出堆顶元素
void heapPop(HEAP *heap, void *space, long long *usedHash)
{
	if (!usedHash[(void *)heap - space])
	{
		printf("堆不存在！\n");
		return;
	}
	if (!heap->size)
	{
		printf("堆为空！\n");
		return;
	}
	printf("堆顶元素为：%lld\n", heap->nums[0]);
	heap->nums[0] = heap->nums[--heap->size];
	adjustAllHeap(heap->nums, heap->size);
}

// 获取堆顶元素
void getHeapTop(HEAP *heap, void *space, long long *usedHash)
{
	if (!usedHash[(void *)heap - space])
	{
		printf("堆不存在！\n");
		return;
	}
	if (!heap->size)
	{
		printf("堆为空！\n");
		return;
	}
	printf("堆顶元素为：%lld\n", heap->nums[0]);
}

// 获取堆的大小
void getHeapSize(HEAP *heap, void *space, long long *usedHash)
{
	if (!usedHash[(void *)heap - space])
	{
		printf("堆不存在！\n");
		return;
	}
	printf("堆中有%lld个元素！\n", heap->size);
}

// 输出堆
void outputHeap(HEAP *heap, void *space, long long *usedHash)
{
	if (!usedHash[(void *)heap - space])
	{
		printf("堆不存在！\n");
		return;
	}
	if (heap->size == 0)
	{
		printf("堆为空！\n");
		return;
	}
	printf("堆中元素依次为：\n");
	printf("%lld", heap->nums[0]);
	for (long long i = 1; i < heap->size; i++)
	{
		printf(", %lld", heap->nums[i]);
	}
	printf("\n");
}

// --------------------堆结束--------------------

// --------------------栈开始--------------------

// 创建栈
STACK *initStack(SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	STACK *stack = (STACK *)myCalloc(1, sizeof(STACK), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	if (!stack)
	{
		printf("栈创建失败！\n");
		return NULL;
	}
	stack->head = (LIST_NODE *)myCalloc(1, sizeof(LIST_NODE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	if (!stack->head)
	{
		myFree(stack, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		printf("栈创建失败！\n");
		return NULL;
	}
	stack->size = 0;
	printf("栈创建成功！地址为：%p\n", stack);
	return stack;
}

// 删除栈
void *delStack(STACK *stack, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)stack - space])
	{
		printf("栈不存在！\n");
	}
	while (stack->size)
	{
		LIST_NODE *t = stack->head->next;
		stack->head->next = t->next;
		myFree(t, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		stack->size--;
	}
	myFree(stack->head, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	myFree(stack, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);

	printf("栈删除成功！\n");
}

// 置空栈
void *clearStack(STACK *stack, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)stack - space])
	{
		printf("栈不存在！\n");
	}
	while (stack->size)
	{
		LIST_NODE *t = stack->head->next;
		stack->head->next = t->next;
		myFree(t, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		stack->size--;
	}

	printf("栈置空成功！\n");
}

// 获取栈顶元素
ELEMENT_TYPE getStackTopVal(STACK *stack, void *space, long long *usedHash)
{
	if (!usedHash[(void *)stack - space])
	{
		printf("栈不存在！\n");
		return 0;
	}
	if (stack->size == 0)
	{
		printf("栈为空！\n");
		return 0;
	}
	printf("栈顶元素为：%lld\n", stack->head->next->val);
	return stack->head->next->val;
}

// 获取栈的大小
long long getStackSize(STACK *stack, void *space, long long *usedHash)
{
	if (!usedHash[(void *)stack - space])
	{
		printf("栈不存在！\n");
		return -1;
	}
	printf("栈中有%lld个元素\n", stack->size);
	return stack->size;
}

// 进栈
void stackPush(STACK *stack, ELEMENT_TYPE val, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)stack - space])
	{
		printf("栈不存在！\n");
		return;
	}
	LIST_NODE *node = (LIST_NODE *)myCalloc(1, sizeof(LIST_NODE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	if (!node)
	{
		printf("空间申请失败！\n");
		return;
	}
	node->val = val;
	node->next = stack->head->next;
	stack->head->next = node;
	stack->size++;
	printf("进栈成功！\n");
}

// 出栈
void stackPop(STACK *stack, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)stack - space])
	{
		printf("栈不存在！\n");
		return;
	}
	if (stack->size == 0)
	{
		printf("栈为空！\n");
		return;
	}
	LIST_NODE *node = stack->head->next;
	stack->head->next = node->next;
	stack->size--;
	printf("出栈成功！出栈元素为%lld\n", node->val);
	myFree(node, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
}

// --------------------栈结束--------------------

// --------------------队列开始--------------------

// 创建队列
QUEUE *initQueue(SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	QUEUE *queue = (QUEUE *)myCalloc(1, sizeof(QUEUE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	if (!queue)
	{
		printf("队列创建失败！\n");
		return NULL;
	}
	queue->head = myCalloc(1, sizeof(LIST_NODE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	if (!queue->head)
	{
		myFree(queue, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		printf("队列创建失败！\n");
		return NULL;
	}
	queue->tail = queue->head;
	queue->size = 0;
	printf("队列创建成功！地址为：%p\n", queue);
	return queue;
}

// 删除队列
void delQueue(QUEUE *queue, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)queue - space])
	{
		printf("队列不存在！\n");
		return;
	}
	while (queue->size)
	{
		LIST_NODE *node = queue->head->next;
		queue->head->next = node->next;
		myFree(node, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		queue->size--;
	}
	myFree(queue->head, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	myFree(queue, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	printf("队列删除成功\n");
}

// 置空队列
void clearQueue(QUEUE *queue, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)queue - space])
	{
		printf("队列不存在！\n");
		return;
	}
	while (queue->size)
	{
		LIST_NODE *node = queue->head->next;
		queue->head->next = node->next;
		myFree(node, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		queue->size--;
	}
	queue->tail = queue->head;
	printf("队列置空成功\n");
}

// 获取队首元素
ELEMENT_TYPE getQueueHeadVal(QUEUE *queue, void *space, long long *usedHash)
{
	if (!usedHash[(void *)queue - space])
	{
		printf("队列不存在！\n");
		return -1;
	}
	if (queue->size == 0)
	{
		printf("队列为空！\n");
		return -1;
	}
	printf("队首元素为：%lld\n", queue->head->next->val);
	return queue->head->next->val;
}

// 获取队列的大小
long long getQueueSize(QUEUE *queue, void *space, long long *usedHash)
{
	if (!usedHash[(void *)queue - space])
	{
		printf("队列不存在！\n");
		return -1;
	}
	printf("队列中有%lld个元素\n", queue->size);
	return queue->size;
}

// 入队列
void queuePush(QUEUE *queue, ELEMENT_TYPE val, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)queue - space])
	{
		printf("队列不存在！\n");
		return;
	}
	LIST_NODE *node = (LIST_NODE *)myCalloc(1, sizeof(LIST_NODE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	if (!node)
	{
		printf("入队失败！\n");
		return;
	}
	node->val = val;
	node->next = queue->tail->next;
	queue->tail->next = node;
	queue->tail = node;
	queue->size++;
	printf("入队成功！\n");
}

// 出队列
void queuePop(QUEUE *queue, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)queue - space])
	{
		printf("队列不存在！\n");
		return;
	}
	if (queue->size == 0)
	{
		printf("队列为空！\n");
		return;
	}
	LIST_NODE *node = queue->head->next;
	queue->head->next = node->next;
	queue->size--;
	printf("出队成功！元素值为%lld\n", node->val);
	myFree(node, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
}

// --------------------队列结束--------------------

// --------------------树开始--------------------

// 创建空树
TREE *initTree(SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	TREE *tree = (TREE *)myCalloc(1, sizeof(TREE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	if (tree)
	{
		printf("树创建成功！地址为：%p\n", tree);
	}
	else
	{
		printf("树创建失败！\n");
	}
	return tree;
}

// 递归删除树节点
void recursiveDelTreeNode(TREE *tree, TREE_NODE *root, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (root)
	{
		TREE_NODE *left = root->left;
		TREE_NODE *right = root->right;
		myFree(root, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		tree->size--;
		recursiveDelTreeNode(tree, left, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		recursiveDelTreeNode(tree, right, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	}
}

// 删除树
void delTree(TREE *tree, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return;
	}
	recursiveDelTreeNode(tree, tree->root, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	myFree(tree, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	printf("树删除成功！\n");
}

// 清空树
void clearTree(TREE *tree, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return;
	}
	recursiveDelTreeNode(tree, tree->root, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	tree->size = 0;
	printf("树清空成功！\n");
}

// 求树的节点数量
long long countTreeNode(TREE *tree, void *space, long long *usedHash)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return -1;
	}
	printf("树中有%lld个节点\n", tree->size);
	return tree->size;
}

// 递归求树的深度
long long recursiveCountTreeDeep(TREE_NODE *root, long long *maxDeep)
{
	long long deep = 0;
	if (root)
	{
		long long left = recursiveCountTreeDeep(root->left, maxDeep);
		long long right = recursiveCountTreeDeep(root->right, maxDeep);
		deep = fmax(left, right) + 1;
		*maxDeep = fmax(*maxDeep, deep);
	}
	return deep;
}

// 求树的深度
long long countTreeDeep(TREE *tree, void *space, long long *usedHash)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return -1;
	}
	long long maxDeep = 0;
	recursiveCountTreeDeep(tree->root, &maxDeep);
	printf("树的深度为：%lld\n", maxDeep);
}

// 返回树的根节点地址
TREE_NODE *treeRoot(TREE *tree, void *space, long long *usedHash)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return NULL;
	}
	printf("树的根节点地址为：%p\n", tree->root);
	return tree->root;
}

// 获取树中节点的值
ELEMENT_TYPE getTreeNodeVal(TREE *tree, TREE_NODE *root, void *space, long long *usedHash)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return -1;
	}
	if (!usedHash[(void *)root - space])
	{
		printf("树节点不存在！\n");
		return -1;
	}
	printf("该树节点的值为：%lld\n", root->val);
	return root->val;
}

// 修改树节点的值
void changeTreeNodeVal(TREE *tree, TREE_NODE *root, ELEMENT_TYPE val, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return;
	}
	if (!usedHash[(void *)root - space])
	{
		printf("树节点不存在！\n");
		return;
	}
	root->val = val;
	printf("成功修改该树节点的值！\n");
}

// 给树的根节点赋值
void addTreeRootVal(TREE *tree, ELEMENT_TYPE val, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return;
	}
	if (!tree->root)
	{
		tree->root = myCalloc(1, sizeof(TREE_NODE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		if (!tree->root)
		{
			printf("根节点创建失败！\n");
			return;
		}
		else
		{
			printf("根节点创建成功！地址为：%p\n", tree->root);
			tree->size++;
		}
	}
	tree->root->val = val;
	printf("成功给根节点赋值！\n");
}

// 返回树节点的祖先
TREE_NODE *getTreeNodeParent(TREE *tree, TREE_NODE *root, void *space, long long *usedHash)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return NULL;
	}
	if (root == tree->root)
	{
		printf("该节点是树的根节点，无祖先！\n");
		return NULL;
	}
	if (!usedHash[(void *)root - space])
	{
		printf("该节点不存在！\n");
		return NULL;
	}
	printf("该节点的祖先地址为：%p，值为：%lld\n", root->parent, root->parent->val);
	return root->parent;
}

// 返回树节点的孩子
void getTreeNodeChildren(TREE *tree, TREE_NODE *root, void *space, long long *usedHash)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return;
	}

	if (root->left)
	{
		printf("节点的左孩子地址为：%p，值为：%lld\n", root->left, root->left->val);
	}
	else
	{
		printf("节点的左孩子为空！\n");
	}
	if (root->right)
	{
		printf("节点的右孩子地址为：%p，值为：%lld\n", root->right, root->right->val);
	}
	else
	{
		printf("节点的右孩子为空！\n");
	}
}

// 返回树节点的兄弟
TREE_NODE *getTreeNodeBrother(TREE *tree, TREE_NODE *root, void *space, long long *usedHash)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return NULL;
	}
	TREE_NODE *parent = root->parent;
	if (root == parent->left)
	{
		if (parent->right)
		{
			printf("该节点的右兄弟地址为：%p，值为：%lld\n", parent->right, parent->right->val);
		}
		else
		{
			printf("该节点右兄弟为空！\n");
		}
		return parent->right;
	}
	else
	{
		if (parent->left)
		{
			printf("该节点的左兄弟地址为：%p，值为：%lld\n", parent->left, parent->left->val);
		}
		else
		{
			printf("该节点左兄弟为空！\n");
		}
		return parent->left;
	}
}

// 插入左子树
void addTreeNodeInLeft(TREE *tree, TREE_NODE *root, ELEMENT_TYPE val, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return;
	}
	if (!usedHash[(void *)root - space])
	{
		printf("树节点不存在！\n");
		return;
	}
	if (!root->left)
	{
		root->left = (TREE_NODE *)myCalloc(1, sizeof(TREE_NODE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		if (!root->left)
		{
			printf("新节点创建失败！\n");
			return;
		}
		root->left->parent = root;
		tree->size++;
		printf("成功插入左子树！\n");
	}
	else
	{
		printf("左子树已存在，值已更新！\n");
	}
	root->left->val = val;
}

// 插入右子树
void addTreeNodeInRight(TREE *tree, TREE_NODE *root, ELEMENT_TYPE val, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return;
	}
	if (!usedHash[(void *)root - space])
	{
		printf("树节点不存在！\n");
		return;
	}
	if (!root->right)
	{
		root->right = (TREE_NODE *)myCalloc(1, sizeof(TREE_NODE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		if (!root->right)
		{
			printf("新节点创建失败！\n");
			return;
		}
		root->right->parent = root;
		tree->size++;
		printf("成功插入右子树！\n");
	}
	else
	{
		printf("右子树已存在，值已更新！\n");
	}
	root->right->val = val;
}

// 删除该节点及其子树
void delTreeNodeAndChildren(TREE *tree, TREE_NODE *root, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return;
	}
	if (!usedHash[(void *)root - space])
	{
		printf("树节点不存在！\n");
		return;
	}
	recursiveDelTreeNode(tree, root, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	printf("成功删除该节点及其子树！\n");
}

void preOrderTree(TREE_NODE *root)
{
	if (root)
	{
		printf(" %lld", root->val);
		preOrderTree(root->left);
		preOrderTree(root->right);
	}
}

// 前序遍历
void preOrderVisitTree(TREE *tree, void *space, long long *usedHash)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return;
	}
	if (!tree->root)
	{
		printf("树为空！\n");
		return;
	}
	printf("前序遍历：\n");
	preOrderTree(tree->root);
	printf("\n");
}

void inOrderTree(TREE_NODE *root)
{
	if (root)
	{
		inOrderTree(root->left);
		printf(" %lld", root->val);
		inOrderTree(root->right);
	}
}

// 中序遍历
void inOrderVisitTree(TREE *tree, void *space, long long *usedHash)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return;
	}
	if (!tree->root)
	{
		printf("树为空！\n");
		return;
	}
	printf("中序遍历：\n");
	inOrderTree(tree->root);
	printf("\n");
}

void postOrderTree(TREE_NODE *root)
{
	if (root)
	{
		postOrderTree(root->left);
		postOrderTree(root->right);
		printf(" %lld", root->val);
	}
}

// 后序遍历
void postOrderVisitTree(TREE *tree, void *space, long long *usedHash)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return;
	}
	if (!tree->root)
	{
		printf("树为空！\n");
		return;
	}
	printf("后序遍历：\n");
	postOrderTree(tree->root);
	printf("\n");
}

// 层序遍历
void levelOrderVisitTree(TREE *tree, void *space, long long *usedHash)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return;
	}
	if (!tree->root)
	{
		printf("树为空！\n");
		return;
	}
	QUEUE *queue = (QUEUE *)calloc(1, sizeof(QUEUE));
	if (!queue)
	{
		printf("空间不足，层序遍历失败！\n");
		return;
	}
	queue->head = (LIST_NODE *)calloc(1, sizeof(LIST_NODE));
	if (!queue->head)
	{
		printf("空间不足，层序遍历失败！\n");
		return;
	}
	queue->tail = queue->head;
	LIST_NODE *node = (LIST_NODE *)calloc(1, sizeof(LIST_NODE));
	node->val = (long long)(tree->root);
	node->next = NULL;
	queue->tail->next = node;
	queue->tail = node;
	queue->size++;
	printf("层序遍历：\n");
	while (queue->size)
	{
		long long size = queue->size;
		for (long long i = 0; i < size; i++)
		{
			LIST_NODE *t = queue->head->next;
			if (t->val)
			{
				TREE_NODE *temTreeNode = (TREE_NODE *)(t->val);
				printf("%lld\t", temTreeNode->val);
				LIST_NODE *left = (LIST_NODE *)calloc(1, sizeof(LIST_NODE));
				left->next = NULL;
				queue->tail->next = left;
				queue->tail = left;
				left->val = (ELEMENT_TYPE)(temTreeNode->left);
				queue->size++;
				LIST_NODE *right = (LIST_NODE *)calloc(1, sizeof(LIST_NODE));
				right->next = NULL;
				queue->tail->next = right;
				queue->tail = right;
				right->val = (ELEMENT_TYPE)(temTreeNode->right);
				queue->size++;
				// printf("left=%p, right=%p\n", left->val, right->val);
			}
			else
			{
				// 空树
				printf("NULL\t");
			}
			queue->head->next = t->next;
			free(t);
			queue->size--;
		}
		printf("\n");
	}
	printf("遍历结束！\n");
}

void recursiveVisualTree(TREE_NODE *root, long long deep)
{
	if (root)
	{
		recursiveVisualTree(root->right, deep + 1);
		for (long long i = 0; i < deep; i++)
		{
			printf("    ");
		}
		printf("%lld\n", root->val);
		recursiveVisualTree(root->left, deep + 1);
	}
}

// 可视化树的结构
void visualTree(TREE *tree, void *space, long long *usedHash)
{
	if (!usedHash[(void *)tree - space])
	{
		printf("树不存在！\n");
		return;
	}
	if (!tree->root)
	{
		printf("树为空！\n");
		return;
	}
	recursiveVisualTree(tree->root, 0);
}

// --------------------树结束--------------------

// --------------------图开始--------------------

// 创建图
MAP *initMap(SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	MAP *map = (MAP *)myCalloc(1, sizeof(MAP), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	if (!map)
	{
		printf("图创建失败！\n");
		return NULL;
	}
	map->nodeHead = (LIST_NODE *)myCalloc(1, sizeof(LIST_NODE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	map->edgeHead = (EDGE_NODE *)myCalloc(1, sizeof(EDGE_NODE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	if (!map->nodeHead || !map->edgeHead)
	{
		printf("图创建失败！\n");
		myFree(map, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		return NULL;
	}
	printf("图创建成功！地址为：%p\n", map);
	return map;
}

// 删除图
void delAllMap(MAP *map, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)map - space])
	{
		printf("图不存在！\n");
		return;
	}
	while (map->edgeHead->next)
	{
		EDGE_NODE *edge = map->edgeHead->next;
		map->edgeHead->next = edge->next;
		LIST_NODE *head = edge->listHead;
		while (head->next)
		{
			LIST_NODE *node = head->next;
			head->next = node->next;
			myFree(node, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
		myFree(head, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		myFree(edge, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	}
	myFree(map->edgeHead, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	while (map->nodeHead->next)
	{
		LIST_NODE *node = map->nodeHead->next;
		map->nodeHead->next = node->next;
		myFree(node, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	}
	myFree(map->nodeHead, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	myFree(map, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	printf("图删除成功！\n");
}

// 清空图
void clearMap(MAP *map, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)map - space])
	{
		printf("图不存在！\n");
		return;
	}
	while (map->edgeHead->next)
	{
		EDGE_NODE *edge = map->edgeHead->next;
		map->edgeHead->next = edge->next;
		LIST_NODE *head = edge->listHead;
		while (head->next)
		{
			LIST_NODE *node = head->next;
			head->next = node->next;
			myFree(node, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
		myFree(head, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		myFree(edge, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	}
	while (map->nodeHead->next)
	{
		LIST_NODE *node = map->nodeHead->next;
		map->nodeHead->next = node->next;
		myFree(node, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	}
	map->edgeSize = map->nodeSize = 0;
	printf("图清空成功！\n");
}

// 插入顶点
void addMapNode(MAP *map, ELEMENT_TYPE val, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)map - space])
	{
		printf("图不存在！\n");
		return;
	}
	LIST_NODE *node = map->nodeHead;
	while (node->next)
	{
		if (node->next->val == val)
		{
			printf("节点已存在！\n");
			return;
		}
		node = node->next;
	}
	LIST_NODE *newNode = (LIST_NODE *)myCalloc(1, sizeof(LIST_NODE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	if (!newNode)
	{
		printf("节点添加失败！\n");
		return;
	}
	newNode->val = val;
	node->next = newNode;
	map->nodeSize++;
	printf("节点添加成功！\n");
}

// 删除顶点
void delMapNode(MAP *map, ELEMENT_TYPE val, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)map - space])
	{
		printf("图不存在！\n");
		return;
	}
	LIST_NODE *node = map->nodeHead;
	bool flag = 0;
	while (node->next)
	{
		if (node->next->val == val)
		{
			LIST_NODE *t = node->next;
			node->next = node->next->next;
			myFree(t, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
			flag = 1;
			map->nodeSize--;
			break;
		}
		node = node->next;
	}
	if (!flag)
	{
		printf("删除失败，节点不存在！\n");
		return;
	}
	EDGE_NODE *edge = map->edgeHead;
	while (edge->next)
	{
		if (edge->next->val == val)
		{
			// 整条删除
			EDGE_NODE *temEdge = edge->next;
			edge->next = edge->next->next;
			LIST_NODE *head = temEdge->listHead;
			while (head->next)
			{
				LIST_NODE *temNode = head->next;
				head->next = head->next->next;
				myFree(temNode, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
				map->edgeSize--;
			}
			myFree(head, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
			myFree(temEdge, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
		else
		{
			// 清除与节点相连的
			LIST_NODE *head = edge->next->listHead;
			while (head->next)
			{
				if (head->next->val == val)
				{
					LIST_NODE *temNode = head->next;
					head->next = head->next->next;
					map->edgeSize--;
					myFree(temNode, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
					if (!edge->next->listHead->next)
					{
						myFree(edge->next->listHead, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
						EDGE_NODE *temEdgeNode = edge->next;
						edge->next = temEdgeNode->next;
						myFree(temEdgeNode, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
					}
					break;
				}
				else
				{
					head = head->next;
				}
			}
			edge = edge->next;
		}
	}
	printf("节点删除成功！\n");
}

// 插入边
void addMapEdge(MAP *map, ELEMENT_TYPE start, ELEMENT_TYPE end, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)map - space])
	{
		printf("图不存在！\n");
		return;
	}
	int flag = 0;
	LIST_NODE *node = map->nodeHead->next;
	while (node)
	{
		if (node->val == start)
		{
			flag++;
		}
		if (node->val == end)
		{
			flag += 2;
		}
		node = node->next;
	}
	if (flag == 0)
	{
		printf("起点和终点都不存在！\n");
	}
	else if (flag == 1)
	{
		printf("终点不存在！\n");
	}
	else if (flag == 2)
	{
		printf("起点不存在！\n");
	}
	else
	{
		EDGE_NODE *edge = map->edgeHead;
		while (edge->next)
		{
			if (edge->next->val == start)
			{
				LIST_NODE *node = edge->next->listHead;
				while (node->next)
				{
					if (node->next->val == end)
					{
						printf("边已存在！\n");
						return;
					}
					node = node->next;
				}
				LIST_NODE *newNode = (LIST_NODE *)myCalloc(1, sizeof(LIST_NODE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
				if (!newNode)
				{
					printf("添加失败！\n");
					return;
				}
				newNode->val = end;
				node->next = newNode;
				map->edgeSize++;
				printf("边添加成功！\n");
				return;
			}
			edge = edge->next;
		}
		// 该顶点没有边
		EDGE_NODE *newEdge = (EDGE_NODE *)myCalloc(1, sizeof(EDGE_NODE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		if (!newEdge)
		{
			printf("边添加失败！\n");
			return;
		}
		newEdge->val = start;
		newEdge->listHead = (LIST_NODE *)myCalloc(1, sizeof(LIST_NODE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		if (!newEdge->listHead)
		{
			printf("边添加失败！\n");
			myFree(newEdge, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
			return;
		}
		LIST_NODE *temNode = (LIST_NODE *)myCalloc(1, sizeof(LIST_NODE), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		if (!temNode)
		{
			printf("边添加失败！\n");
			myFree(newEdge->listHead, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
			myFree(newEdge, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
			return;
		}
		temNode->val = end;
		newEdge->listHead->next = temNode;
		edge->next = newEdge;
		map->edgeSize++;
		printf("边添加成功！\n");
	}
}

// 删除边
void delMapEdge(MAP *map, ELEMENT_TYPE start, ELEMENT_TYPE end, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (!usedHash[(void *)map - space])
	{
		printf("图不存在！\n");
		return;
	}
	EDGE_NODE *edge = map->edgeHead;
	while (edge->next)
	{
		if (edge->next->val == start)
		{
			LIST_NODE *node = edge->next->listHead;
			while (node->next)
			{
				if (node->next->val == end)
				{
					LIST_NODE *t = node->next;
					node->next = node->next->next;
					myFree(t, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
					map->edgeSize--;
					// 如果该点没有边了，就把边链表删除
					if (!edge->next->listHead->next)
					{
						myFree(edge->next->listHead, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
						EDGE_NODE *temEdgeNode = edge->next;
						edge->next = temEdgeNode->next;
						myFree(temEdgeNode, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
					}
					printf("删除成功！\n");
					return;
				}
				node = node->next;
			}
			printf("边不存在！\n");
			return;
		}
		edge = edge->next;
	}
	printf("边不存在！\n");
}

// 输出进入某顶点的所有边
void inMapNode(MAP *map, ELEMENT_TYPE val, void *space, long long *usedHash)
{
	if (!usedHash[(void *)map - space])
	{
		printf("图不存在！\n");
		return;
	}
	// 检查节点是否存在
	bool flag = 0;
	LIST_NODE *node = map->nodeHead->next;
	while (node)
	{
		if (node->val == val)
		{
			flag = 1;
			break;
		}
		node = node->next;
	}
	if (!flag)
	{
		printf("该节点不存在！\n");
		return;
	}
	flag = 0;
	EDGE_NODE *edge = map->edgeHead->next;
	while (edge)
	{
		LIST_NODE *head = edge->listHead->next;
		while (head)
		{
			if (head->val == val)
			{
				if (flag == 0)
				{
					printf("进入该节点的点有：");
				}
				flag = 1;
				printf("%lld  ", edge->val);
				break;
			}
			head = head->next;
		}
		edge = edge->next;
	}
	if (!flag)
	{
		printf("不存在进入该节点的点！\n");
	}
	else
	{
		printf("\n");
	}
}

// 输出离开某顶点的所有边
void outMapNode(MAP *map, ELEMENT_TYPE val, void *space, long long *usedHash)
{
	if (!usedHash[(void *)map - space])
	{
		printf("图不存在！\n");
		return;
	}
	// 检查节点是否存在
	bool flag = 0;
	LIST_NODE *node = map->nodeHead->next;
	while (node)
	{
		if (node->val == val)
		{
			flag = 1;
			break;
		}
		node = node->next;
	}
	if (!flag)
	{
		printf("该节点不存在！\n");
		return;
	}
	flag = 0;
	EDGE_NODE *edge = map->edgeHead->next;
	while (edge)
	{
		if (edge->val == val)
		{
			LIST_NODE *head = edge->listHead->next;
			if (head)
			{
				printf("离开该节点的点有：");
				flag = 1;
			}
			while (head)
			{
				flag = 1;
				printf("%lld  ", head->val);
				head = head->next;
			}
			break;
		}
		edge = edge->next;
	}
	if (!flag)
	{
		printf("不存在离开该节点的点！\n");
	}
	else
	{
		printf("\n");
	}
}

// 输出点的个数
long long mapNodeNum(MAP *map, void *space, long long *usedHash)
{
	if (!usedHash[(void *)map - space])
	{
		printf("图不存在！\n");
		return -1;
	}
	printf("图的顶点共有%lld个\n", map->nodeSize);
	return map->nodeSize;
}

// 输出边的个数
long long mapEdgeNum(MAP *map, void *space, long long *usedHash)
{
	if (!usedHash[(void *)map - space])
	{
		printf("图不存在！\n");
		return -1;
	}
	printf("图的边共有%lld条\n", map->edgeSize);
	return map->edgeSize;
}

// 遍历所有顶点
void visitAllMapNode(MAP *map, void *space, long long *usedHash)
{
	if (!usedHash[(void *)map - space])
	{
		printf("图不存在！\n");
		return;
	}
	if (map->nodeSize == 0)
	{
		printf("该图为空！\n");
		return;
	}
	printf("图的顶点有：\n");
	LIST_NODE *node = map->nodeHead->next;
	while (node)
	{
		printf(" %lld", node->val);
		node = node->next;
	}
	printf("\n");
}

// 遍历所有边
void visitAllMapEdge(MAP *map, void *space, long long *usedHash)
{
	if (!usedHash[(void *)map - space])
	{
		printf("图不存在！\n");
		return;
	}
	if (map->edgeSize == 0)
	{
		printf("该图没有边！\n");
		return;
	}
	printf("图的边有：\n");
	EDGE_NODE *edge = map->edgeHead->next;
	while (edge)
	{
		LIST_NODE *head = edge->listHead->next;
		while (head)
		{
			printf("%lld->%lld  ", edge->val, head->val);
			head = head->next;
		}
		printf("\n");
		edge = edge->next;
	}
}

// --------------------图结束--------------------

// 获取参数
char **getArguments(char *s, int n)
{
	char **arr = (char **)calloc(n, sizeof(char *));
	int flag1 = strstr(s, "(") - s;
	int flag2 = flag1;
	for (int i = 0; i < n - 1; i++)
	{
		arr[i] = (char *)calloc(STR_SIZE, sizeof(char));
		flag1 = flag2 + 1;
		flag2 = strstr(&s[flag1], ",") - s;
		strncpy(arr[i], &s[flag1], flag2 - flag1);
	}
	arr[n - 1] = (char *)calloc(STR_SIZE, sizeof(char));
	flag1 = flag2 + 1;
	flag2 = strstr(&s[flag1], ")") - s;
	strncpy(arr[n - 1], &s[flag1], flag2 - flag1);
	return arr;
}

// 字符串转地址
void *strToPath(char *s)
{
	return (void *)strtoll(s, NULL, 16);
}

void eval(char *s, SPACE_NODE **freeSpaceArray, int maxBlock, long long *usedSpace, long long *freeSpace, void *space, char *blockStatus, long long *usedHash, FREE_SPACE_NODE *freeSpaceListHead)
{
	if (strstr(s, "checkSpace"))
	{
		// 查看空间总体使用情况
		checkSpace(*usedSpace, *freeSpace);
	}
	else if (strstr(s, "checkBlock"))
	{
		// 查看内存块使用情况
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			checkBlock(space, blockStatus, address);
		}
	}
	else if (strstr(s, "checkFreeSpaceList"))
	{
		// 查看空闲块链表
		checkFreeSpaceList(freeSpaceListHead);
	}
	else if (strstr(s, "myFree"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			myFree(address, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "myMalloc"))
	{
		char **arr = getArguments(s, 1);
		void *p = myMalloc(atoll(arr[0]), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	}
	else if (strstr(s, "myCalloc"))
	{
		char **arr = getArguments(s, 2);
		void *p = myCalloc(atoll(arr[0]), atoll(arr[1]), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	}
	else if (strstr(s, "initList"))
	{
		LIST_NODE *head = initList(freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	}
	else if (strstr(s, "delAllList"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			delList(address, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "listEmpty"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			listEmpty(address, space, usedHash);
		}
	}
	else if (strstr(s, "listLength"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			listLength(address, space, usedHash);
		}
	}
	else if (strstr(s, "getListNodeVal"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			getListNodeVal(address, space, usedHash);
		}
	}
	else if (strstr(s, "findListNode"))
	{
		char **arr = getArguments(s, 2);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			findListNode(address, atoll(arr[1]), space, usedHash);
		}
	}
	else if (strstr(s, "listPreNode"))
	{
		char **arr = getArguments(s, 2);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			listPreNode(address, atoll(arr[1]), space, usedHash);
		}
	}
	else if (strstr(s, "listNextNode"))
	{
		char **arr = getArguments(s, 2);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			listNextNode(address, atoll(arr[1]), space, usedHash);
		}
	}
	else if (strstr(s, "visitList"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			visitList(address, space, usedHash);
		}
	}
	else if (strstr(s, "clearList"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			clearList(address, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "changeListVal"))
	{
		char **arr = getArguments(s, 3);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			changeListVal(address, atoll(arr[1]), atoll(arr[2]), space, usedHash);
		}
	}
	else if (strstr(s, "addListNode"))
	{
		char **arr = getArguments(s, 3);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			addListNode(address, atoll(arr[1]), atoll(arr[2]), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "delListNode"))
	{
		char **arr = getArguments(s, 2);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			delListNode(address, atoll(arr[1]), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "initArray"))
	{
		char **arr = getArguments(s, 1);
		initArray(atoll(arr[0]), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	}
	else if (strstr(s, "delAllArray"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			delArray(address, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "getArrayVal"))
	{
		char **arr = getArguments(s, 2);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			getArrayVal(address, atoll(arr[1]), space, usedHash);
		}
	}
	else if (strstr(s, "changeArrayVal"))
	{
		char **arr = getArguments(s, 3);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			changeArrayVal(address, atoll(arr[1]), atoll(arr[2]), space, usedHash);
		}
	}
	else if (strstr(s, "visitArray"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			visitArray(address, space, usedHash);
		}
	}
	else if (strstr(s, "initHeap"))
	{
		char **arr = getArguments(s, 1);
		initHeap(atoll(arr[0]), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	}
	else if (strstr(s, "delAllHeap"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			delHeap(address, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "clearHeap"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			clearHeap(address, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "heapPush"))
	{
		char **arr = getArguments(s, 2);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			heapPush(address, atoll(arr[1]), space, usedHash);
		}
	}
	else if (strstr(s, "heapPop"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			heapPop(address, space, usedHash);
		}
	}
	else if (strstr(s, "getHeapTop"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			getHeapTop(address, space, usedHash);
		}
	}
	else if (strstr(s, "getHeapSize"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			getHeapSize(address, space, usedHash);
		}
	}
	else if (strstr(s, "outputHeap"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			outputHeap(address, space, usedHash);
		}
	}
	else if (strstr(s, "initStack"))
	{
		initStack(freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	}
	else if (strstr(s, "delAllStack"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			delStack(address, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "clearStack"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			clearStack(address, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "getStackTopVal"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			getStackTopVal(address, space, usedHash);
		}
	}
	else if (strstr(s, "getStackSize"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			getStackSize(address, space, usedHash);
		}
	}
	else if (strstr(s, "stackPush"))
	{
		char **arr = getArguments(s, 2);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			stackPush(address, atoll(arr[1]), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "stackPop"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			stackPop(address, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "initQueue"))
	{
		initQueue(freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	}
	else if (strstr(s, "delAllQueue"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			delQueue(address, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "clearQueue"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			clearQueue(address, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "getQueueHeadVal"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			getQueueHeadVal(address, space, usedHash);
		}
	}
	else if (strstr(s, "getQueueSize"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			getQueueSize(address, space, usedHash);
		}
	}
	else if (strstr(s, "queuePush"))
	{
		char **arr = getArguments(s, 2);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			queuePush(address, atoll(arr[1]), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "queuePop"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			queuePop(address, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "initTree"))
	{
		initTree(freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	}
	else if (strstr(s, "delAllTree"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			delTree(address, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "clearTree"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			clearTree(address, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "countTreeNode"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			countTreeNode(address, space, usedHash);
		}
	}
	else if (strstr(s, "countTreeDeep"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			countTreeDeep(address, space, usedHash);
		}
	}
	else if (strstr(s, "treeRoot"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			treeRoot(address, space, usedHash);
		}
	}
	else if (strstr(s, "getTreeNodeVal"))
	{
		char **arr = getArguments(s, 2);
		void *address1 = strToPath(arr[0]);
		void *address2 = strToPath(arr[1]);
		bool flag1 = addressIsValid(address1, space);
		bool flag2 = addressIsValid(address2, space);
		if (flag1 && flag2)
		{
			getTreeNodeVal(address1, address2, space, usedHash);
		}
	}
	else if (strstr(s, "changeTreeNodeVal"))
	{
		char **arr = getArguments(s, 3);
		void *address1 = strToPath(arr[0]);
		void *address2 = strToPath(arr[1]);
		bool flag1 = addressIsValid(address1, space);
		bool flag2 = addressIsValid(address2, space);
		if (flag1 && flag2)
		{
			changeTreeNodeVal(address1, address2, atoll(arr[2]), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "addTreeRootVal"))
	{
		char **arr = getArguments(s, 2);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			addTreeRootVal(address, atoll(arr[1]), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "getTreeNodeParent"))
	{
		char **arr = getArguments(s, 2);
		void *address1 = strToPath(arr[0]);
		void *address2 = strToPath(arr[1]);
		bool flag1 = addressIsValid(address1, space);
		bool flag2 = addressIsValid(address2, space);
		if (flag1 && flag2)
		{
			getTreeNodeParent(address1, address2, space, usedHash);
		}
	}
	else if (strstr(s, "getTreeNodeChildren"))
	{
		char **arr = getArguments(s, 2);
		void *address1 = strToPath(arr[0]);
		void *address2 = strToPath(arr[1]);
		bool flag1 = addressIsValid(address1, space);
		bool flag2 = addressIsValid(address2, space);
		if (flag1 && flag2)
		{
			getTreeNodeChildren(address1, address2, space, usedHash);
		}
	}
	else if (strstr(s, "getTreeNodeBrother"))
	{
		char **arr = getArguments(s, 2);
		void *address1 = strToPath(arr[0]);
		void *address2 = strToPath(arr[1]);
		bool flag1 = addressIsValid(address1, space);
		bool flag2 = addressIsValid(address2, space);
		if (flag1 && flag2)
		{
			getTreeNodeBrother(address1, address2, space, usedHash);
		}
	}
	else if (strstr(s, "addTreeNodeInLeft"))
	{
		char **arr = getArguments(s, 3);
		void *address1 = strToPath(arr[0]);
		void *address2 = strToPath(arr[1]);
		bool flag1 = addressIsValid(address1, space);
		bool flag2 = addressIsValid(address2, space);
		if (flag1 && flag2)
		{
			addTreeNodeInLeft(address1, address2, atoll(arr[2]), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "addTreeNodeInRight"))
	{
		char **arr = getArguments(s, 3);
		void *address1 = strToPath(arr[0]);
		void *address2 = strToPath(arr[1]);
		bool flag1 = addressIsValid(address1, space);
		bool flag2 = addressIsValid(address2, space);
		if (flag1 && flag2)
		{
			addTreeNodeInRight(address1, address2, atoll(arr[2]), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "delTreeNodeAndChildren"))
	{
		char **arr = getArguments(s, 2);
		void *address1 = strToPath(arr[0]);
		void *address2 = strToPath(arr[1]);
		bool flag1 = addressIsValid(address1, space);
		bool flag2 = addressIsValid(address2, space);
		if (flag1 && flag2)
		{
			delTreeNodeAndChildren(address1, address2, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "preOrderVisitTree"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			preOrderVisitTree(address, space, usedHash);
		}
	}
	else if (strstr(s, "inOrderVisitTree"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			inOrderVisitTree(address, space, usedHash);
		}
	}
	else if (strstr(s, "postOrderVisitTree"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			postOrderVisitTree(address, space, usedHash);
		}
	}
	else if (strstr(s, "levelOrderVisitTree"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			levelOrderVisitTree(address, space, usedHash);
		}
	}
	else if (strstr(s, "visualTree"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			visualTree(address, space, usedHash);
		}
	}
	else if (strstr(s, "initMap"))
	{
		initMap(freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
	}
	else if (strstr(s, "delAllMap"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			delAllMap(address, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "clearMap"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			clearMap(address, freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "addMapNode"))
	{
		char **arr = getArguments(s, 2);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			addMapNode(address, atoll(arr[1]), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "delMapNode"))
	{
		char **arr = getArguments(s, 2);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			delMapNode(address, atoll(arr[1]), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "addMapEdge"))
	{
		char **arr = getArguments(s, 3);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			addMapEdge(address, atoll(arr[1]), atoll(arr[2]), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "delMapEdge"))
	{
		char **arr = getArguments(s, 3);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			delMapEdge(address, atoll(arr[1]), atoll(arr[2]), freeSpaceArray, maxBlock, usedSpace, freeSpace, space, blockStatus, usedHash, freeSpaceListHead);
		}
	}
	else if (strstr(s, "inMapNode"))
	{
		char **arr = getArguments(s, 2);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			inMapNode(address, atoll(arr[1]), space, usedHash);
		}
	}
	else if (strstr(s, "outMapNode"))
	{
		char **arr = getArguments(s, 2);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			outMapNode(address, atoll(arr[1]), space, usedHash);
		}
	}
	else if (strstr(s, "mapNodeNum"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			mapNodeNum(address, space, usedHash);
		}
	}
	else if (strstr(s, "mapEdgeNum"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			mapEdgeNum(address, space, usedHash);
		}
	}
	else if (strstr(s, "visitAllMapNode"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			visitAllMapNode(address, space, usedHash);
		}
	}
	else if (strstr(s, "visitAllMapEdge"))
	{
		char **arr = getArguments(s, 1);
		void *address = strToPath(arr[0]);
		bool flag = addressIsValid(address, space);
		if (flag)
		{
			visitAllMapEdge(address, space, usedHash);
		}
	}
	else
	{
		printf("未定义的指令序列！\n");
	}
}
