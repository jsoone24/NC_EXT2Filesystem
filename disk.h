#ifndef _DISK_H_
#define _DISK_H_

#include "common.h"

typedef struct DISK_OPERATIONS
{
	int		( *read_sector	)( struct DISK_OPERATIONS*, SECTOR, void* );		//디스크에서 읽는 동작 연결 필요. 첫번째 인자가 읽을 곳, 두번 째 인자가 읽을 크기, 세번째 인자가 읽어서 저장할 곳으로 예상 읽기 전에 공간 초기화 필요
	int		( *write_sector	)( struct DISK_OPERATIONS*, SECTOR, const void* );	//디스크에 쓰는 동작 연결 필요. 첫번째 인자가 어디에 쓸지, 두번째 인자가 얼마나 쓸지, 세번째 인자가 어떤걸 쓸지 넘겨주는 듯
	SECTOR	numberOfSectors;	//섹터의 수
	int		bytesPerSector;		//섹터당 바이트 수
	void*	pdata;				//디스크 메모리 
} DISK_OPERATIONS;				//디스크 동작에 관한 구조체

#endif
