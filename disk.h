#ifndef _DISK_H_
#define _DISK_H_

#include "common.h"

typedef struct DISK_OPERATIONS
{
	int		( *read_sector	)( struct DISK_OPERATIONS*, SECTOR, void* );		//디스크에서 읽는 동작 연결 필요
	int		( *write_sector	)( struct DISK_OPERATIONS*, SECTOR, const void* );	//디스크에 쓰는 동작 연결 필요
	SECTOR	numberOfSectors;	//섹터의 수
	int		bytesPerSector;		//섹터당 바이트 수
	void*	pdata;				//디스크 메모리 
} DISK_OPERATIONS;				//디스크 동작에 관한 구조체

#endif
