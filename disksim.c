#include <stdlib.h>
#include <memory.h>
#include "ext2.h"
#include "disk.h"
#include "disksim.h"

typedef struct
{
	char*	address;
} DISK_MEMORY;

int disksim_read(DISK_OPERATIONS* this, SECTOR sector, void* data);
int disksim_write(DISK_OPERATIONS* this, SECTOR sector, const void* data);

int disksim_init( SECTOR numberOfSectors, unsigned int bytesPerSector, DISK_OPERATIONS* disk ) 
{	// pdata에 main에서 요청한 disk 크기 만큼 할당해서 연결

	if( disk == NULL ) //가르키는 DISK_OPERATIONS 구조체가 없다면 에러 처리
		return -1;

	disk->pdata = malloc( sizeof( DISK_MEMORY ) ); //disk.pdata에 메모리 할당
	if( disk->pdata == NULL )// 할당 실패시
	{
		disksim_uninit( disk );		// 해당 DISK_OPERATIONS의 pdata 영역 메모리 할당 해제(54 참조)
		return -1;
	}

	( ( DISK_MEMORY* )disk->pdata )->address = ( char* )malloc( bytesPerSector * numberOfSectors ); // 해당 주소에 (섹터수*섹터당 바이트)만큼 메모리 할당
	if( disk->pdata == NULL )	// 할당 실패시
	{
		disksim_uninit( disk );	// // 해당 DISK_OPERATIONS의 pdata 영역 메모리 할당 해제(54 참조)
		return -1;
	}

	disk->read_sector	= disksim_read;
	disk->write_sector	= disksim_write;
	disk->numberOfSectors	= numberOfSectors;
	disk->bytesPerSector	= bytesPerSector;
	// 메인의 g_disk, DISK_OPERATIONS 구조체에 함수, 섹터수, 섹터 당 바이트 설정
	return 0;
}

void disksim_uninit( DISK_OPERATIONS* this ) // pdata메모리 할당 해제
{
	if( this )
	{
		if( this->pdata )
			free( this->pdata ); // 인자로 받은 this의 pdata가 존재할 경우 할당 해제
	}
}

int disksim_read( DISK_OPERATIONS* this, SECTOR sector, void* data )	
{
	char* disk = ( ( DISK_MEMORY* )this->pdata )->address;  // 포인터 변수 disk에 DISK_OPERATIONS 구조체의 pdata 주소 대입

	if( sector < 0 || sector >= this->numberOfSectors )		// 인자로 받은 sector가 0보다 작거나 디스크 구조체의 섹터 수보다 많으면 에러처리
		return -1;

	memcpy( data, &disk[sector * this->bytesPerSector], this->bytesPerSector ); 
	// (입력받은 섹터번호*섹터당 바이트 수)한 주소로부터 한 섹터만큼의 내용 data에 복사
	return 0;
}

int disksim_write( DISK_OPERATIONS* this, SECTOR sector, const void* data )
{
	char* disk = ( ( DISK_MEMORY* )this->pdata )->address;	// 포인터 변수 disk에 DISK_OPERATIONS 구조체의 pdata 주소 대입

	if( sector < 0 || sector >= this->numberOfSectors )	// 인자로 받은 sector가 0보다 작거나 디스크 구조체의 섹터 수보다 많으면 에러처리
		return -1;

	memcpy( &disk[sector * this->bytesPerSector], data, this->bytesPerSector ); 
	// (입력받은 섹터번호*섹터당 바이트 수)한 주소로부터 한 섹터만큼에 data 대입
	return 0;
}

