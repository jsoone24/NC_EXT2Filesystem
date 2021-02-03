#include "common.h"
#include "shell.h"

#ifndef NULL
#define NULL	( ( void* )0 )
#endif

//파일 시스템의 디렉터리 엔트리를 쉘 상의 환경에 맞게 쓸 수 있어야 함
//그 것에 대한 것.

int init_entry_list(SHELL_ENTRY_LIST* list)	//맨 처음 쉘을 키면 아무런 것이 없다. 엔트리를 서로 연결 시켜줄 수 있게 쉘 엔트리 크기만큼 공간 할당.
{
	memset(list, 0, sizeof(SHELL_ENTRY_LIST));
	return 0;
}

int add_entry_list(SHELL_ENTRY_LIST* list, SHELL_ENTRY* entry)	//쉘 상에서 관리하는 엔트리에 새로운 엔트리를 추가하는 과정.
{
	SHELL_ENTRY_LIST_ITEM*	newItem;	//엔트리 리스트 아이템 포인터 생성

	newItem = (SHELL_ENTRY_LIST_ITEM*)malloc(sizeof(SHELL_ENTRY_LIST_ITEM));// 새 공간 할당
	newItem->entry = *entry;	//가장 최근의 엔트리를 새로만든 엔트리로 설정
	newItem->next = NULL;		//다음 포인터 없다고 설정

	if (list->count == 0)	//개수가 0개라고 되어있으면, 리스트 상에서 처음이자 마지막 엔트리를 새로만든 엔트리 아이템으로 설정
		list->first = list->last = newItem;
	else	//개수가 1개이상 있었으면 마지막 원소의 다음으로 새 아이템 연결, 마지막 원소를 새로 넣은 엔트리 아이템으로 지정
	{
		list->last->next = newItem;
		list->last = newItem;
	}

	list->count++;	//개수 증가.

	return 0;
}

void release_entry_list(SHELL_ENTRY_LIST* list)	//리스트 전체 할당 해제 하는 과정.
{
	SHELL_ENTRY_LIST_ITEM*	currentItem;
	SHELL_ENTRY_LIST_ITEM*	nextItem;

	if (list->count == 0)
		return;

	nextItem = list->first;

	do
	{
		currentItem = nextItem;
		nextItem = currentItem->next;
		free(currentItem);
	} while (nextItem);

	list->count = 0;
	list->first = NULL;
	list->last = NULL;
}