#ifndef _DISK_H_
#define _DISK_H_

#include "common.h"

typedef struct DISK_OPERATIONS
{
	int		( *read_sector	)( struct DISK_OPERATIONS*, SECTOR, void* );		//첫번째 인자가 디스크 동작 연결, 두번째 인자가 어디서 읽을건지, 세번째 인자가 읽어서 저장할 곳으로 예상 읽기 전에 공간 초기화 필요
	int		( *write_sector	)( struct DISK_OPERATIONS*, SECTOR, const void* );	//첫번째 인자가 디스크 동작 연결, 두번째 인자가 어디에?, 세번째 인자가 어떤걸 쓸지 넘겨주는 듯
	SECTOR	numberOfSectors;	//섹터의 수
	int		bytesPerSector;		//섹터당 바이트 수 = bytes per block = 1024Byte
	void*	pdata;				//디스크 메모리 
} DISK_OPERATIONS;				//디스크 동작에 관한 구조체

#endif