#ifndef _FAT_H_
#define _FAT_H_

#include "common.h"
#include "disk.h"
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#define GNU_SOURCE

#include <string.h>

#define EXT2_N_BLOCKS         	15				//직,간접 블록 개수
#define	NUMBER_OF_SECTORS		( 4096 + 1 )	//총 섹터 수
#define	NUMBER_OF_GROUPS		2				//블록 그룹 수
#define	NUMBER_OF_INODES		200				//볼륨 전체의 아이노드 수
#define	VOLUME_LABLE			"EXT2 BY NC"	//볼륨 이름

#define MAX_SECTOR_SIZE			1024			//최대 섹터 크기 (Byte)
#define MAX_BLOCK_SIZE			1024			//최대 블록 크기 (Byte)
#define MAX_NAME_LENGTH			256				//최대 이름 제한
#define MAX_ENTRY_NAME_LENGTH	11				//최대 엔트리 이름 제한

#define ATTR_READ_ONLY			0x01			//아이노드 필드 설정 시 사용되는 가ㅄ으로 예상됨
#define ATTR_HIDDEN				0x02			//아이노드 필드 설정 시 사용되는 가ㅄ으로 예상됨
#define ATTR_SYSTEM				0x04			//아이노드 필드 설정 시 사용되는 가ㅄ으로 예상됨
#define ATTR_VOLUME_ID			0x08			//아이노드 필드 설정 시 사용되는 가ㅄ으로 예상됨
#define ATTR_DIRECTORY			0x10			//아이노드 필드 설정 시 사용되는 가ㅄ으로 예상됨
#define ATTR_ARCHIVE			0x20			//아이노드 필드 설정 시 사용되는 가ㅄ으로 예상됨
#define ATTR_LONG_NAME			ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID

#define DIR_ENTRY_FREE			0xE5			//디렉터리 엔트리들이 연속적으로 저장되다가 중간에 디렉터리 엔트리가 삭제되면, fragmentaion 발생. 그때 그 빈 공간의 이름 부분을 이 변수로 채워넣는다.
#define DIR_ENTRY_NO_MORE		0x00			//마지막 디렉터리엔트리임을 뜻하는 변수일것 같다. 
#define DIR_ENTRY_OVERWRITE		1				//뭘까?
//디렉터리 엔트리 관리시 필요한 필드. 이름쪽에 들어간다.

#define GET_INODE_GROUP(x)		((x) - 1)/( NUMBER_OF_INODES / NUMBER_OF_GROUPS )	// 아이노드가 속한 그룹 리턴
#define SQRT(x)  				( (x) * (x)  )
#define TRI_SQRT(x)				( (x) * (x) * (x) )
#define WHICH_GROUP_BLONG(x)	(((x) - 1)/( NUMBER_OF_INODES / NUMBER_OF_GROUPS ))	// ?

#define TSQRT(x) 				((x)*(x)*(x))
#define GET_INODE_FROM_NODE(x) 	((x)->entry.inode)	// EXT2_NODE의 inode number return

#ifdef _WIN32
#pragma pack(push,fatstructures)
#endif
#pragma pack(1)


#define cal_inode_per_block(x) (x == 0 ? 8 : (x==1 ? 16 : 32))



typedef struct
{
	UINT32 max_inode_count;				//파일 시스템에서 샤용가능한 최대 아이노드 수 = 200
	UINT32 block_count;					//파일 시스템 내의 전체 블록 수 = 4097
	UINT32 reserved_block_count;		//아무 대책없이 파일시스템이 꽉 차는 경우를 막기 위해 예약된 블록 영역
	UINT32 free_block_count;			//비어있는 블록 수 = 4061
	UINT32 free_inode_count;			//비어있는 아이노드 수 = 190

	UINT32 first_data_block;			//첫번째 블록, 즉 블록 그룹 0이 시작되는 블록 = 1
	UINT32 log_block_size;				//블록 크기 지정하는 가ㅄ 0:1KB, 1:2KB, 2:4KB = 0(1KB)
	UINT32 log_fragmentation_size;		//단편화 발생 시 기록되는 크기 0 1 2로 기록 위에와 크기 같음
	UINT32 block_per_group;				//블록 그룹마다 블록이 몇개 있는지 = 2048
	UINT32 fragmentation_per_group;		//각 블록 그룹에 속한 단편화된 개수
	UINT32 inode_per_group;				//각 블록그룹에 속한 아이노드 수 = 100
	UINT16 magic_signature;				//슈퍼블록인지 확인하는 고유가ㅄ
	UINT16 errors;						//에러 처리를 위한 플래그. 자세한 에러코드는 책 320쪽 참고 
	UINT32 first_non_reserved_inode;	//예약되지 않은 아이노드의 첫번째 인덱스, 일반적으로 10개의 아이노드가 예약되어있음 = 11
	UINT16 inode_structure_size;		//아이노드 구조체 크기 = 128Byte

	UINT16 block_group_number;			//현재 슈퍼블록을 포함하고 있는 블록 그룹 번호. 각 블록그룹마다 존재하는 슈퍼블록 원본인지 복사본인지 파악
	UINT32 first_data_block_each_group;	//첫번째 데이터 블록 번호 = 17
} EXT2_SUPER_BLOCK;						//super block 에 대한 구조체. 각 블록 그룹마다 있다. 총 1KB

typedef struct
{
	UINT32 start_block_of_block_bitmap;	//블록 비트맵의 블록 번호. 그룹 디스크립터 테이블 크기가 고정되어 있지 않기 대문에 바로 다음에 위치하는 블록 비트맵 위치 나타냄 = 2
	UINT32 start_block_of_inode_bitmap;	//아이노드 비트맵의 블록 번호. 블록 비트맵 위치 나타냄 = 3
	UINT32 start_block_of_inode_table;	//아이노드 태이블의 블록 번호. 실제 아이노드 데이터가 담겨있는 블록이 시작되는 블록의 번호이다. = 4
	UINT32 free_blocks_count;			//블록 그룹내의 비어있는 블록 수. 데이터 저장 시 비어있는 블록이 많은 블록 그룹이 우선적으로 선택됨.
	UINT32 free_inodes_count;			//블록 그룹내의 비어있는 아이노드 수
	UINT16 directories_count;			//블록 그룹내에 생성된 디렉토리 수. 디렉토리에 데이터 저장시 되도록 부모 디렉터리와 같은 블록 그룹에 저장하려함. 그래서 이 디렉토리 수가 많을 수록
										//디렉터리를 넣을 공간이 적다는 의미이기 때문에 저장시 이 개수가 적은 곳에 저장하게됨
	BYTE padding[2];					//4바이트 단위로 정렬을 위한 패딩이다. directories_count는 2바이트로 4바이트 기준 저장 시 2바이트가 남는데 이 빈공간을 채움
	BYTE reserved[12];					//예약된 영역. 32바이트를 맞추기 위해 12바이트를 패딩하기 위한 영역
} EXT2_GROUP_DESCRIPTOR;				//모든 블록 그룹에 대한 디스크립터. 각 블록 그룹마다 저장되어있다. 하나의 디스크립터는 총 32바이트

typedef struct
{
	UINT16  mode;						/* File mode 아이노드 타입 : 파일인지 디렉토리인지, 권한 등 책 331페이지 참고*/
	UINT16  uid;						/* Low 16 bits of Owner Uid 소유자 식별자*/
	UINT32  size; 						/* Size in bytes 파일크기를 바이트 단위로 나타냄*/
	UINT32  atime;						/* Access time 마지막 접근 시간*/
	UINT32  ctime;						/* Creation time - 생성시간*/
	UINT32  mtime;						/* Modification time 파일을 마지막으로 변경한 시간*/
	UINT32  dtime;						/* Deletion Time 파일이 삭제된 시간*/
	UINT16  gid;  						/* Low 16 bits of Group Id 그룹 식별자*/
	UINT16  links_count;				/* Links count 하드 링크 수*/
	UINT32  blocks;     				/* Blocks count 파일 데이터를 저장하는데 필요한 블록 개수*/
	UINT32  flags;      				/* File flags 파일 플래그 책 334페이지 참고, 아래 151번줄에 하위 13~16비트에 대한 갑ㅅ 정의 되어있음*/
	UINT32  i_reserved1;				// OS dependent 1 -????? 특정 운영체제 정보라는데 실질적으로 사용되지 않는다고 함
	UINT32  block[EXT2_N_BLOCKS];		/* Pointers to blocks 데이터 블록을 가리키는 포인터 배열 15개. 12개 직접블록 1개 간접 포인터, 1개 이중 간접 포인터 1개 3중 간접포인터*/
	UINT32  generation;					/* File version (for NFS) 네트워크 파일시스템을 위한 파일 버전*/
	UINT32  file_acl;  					/* File ACL 파일의 확장 속성*/
	UINT32  dir_acl;   					/* Directory ACL 디렉토리 접근 제어 리스트*/
	UINT32  faddr;     					/* Fragment address 단편 주소 */
	UINT32  i_reserved2[3];				// OS dependent 2 -????? 특정 운영체제 정보라는데 실질적으로 사용되지 않는다고 함
} INODE; 								//아이노드 구조체. 크기는 슈퍼블록에 기록된 크기로 계산되지만 일반적으로 128바이트임

//아이노드도 리스트로 되어있기 때문에 인덱스를 가짐. 이중에서 1 번은 사용불가능한 데이터 블록의 아이노드, 2 번은 루트 디렉터리의 아이노드 5번은 부트로더의 아이노드
//6번은 삭제 불가 디렉터리의 아이노드 11번은 예약되지 않은 첫 번째 아이노드 의미

typedef struct							
{
	UINT32 inode;						//이 디렉터리 엔트리가 가리키는 데이터의 아이노드 번호. 이 아이노드번호로 아이노드를 찾으면 파일인지 디렉터리인지 알 수 있음
	BYTE name[11];						//파일 명의 길이
	UINT32 name_len;					//파일 명
	BYTE pad[13];						//아마 책에서 Record Length에 해당하는 것 같음 생성시 엔트리 크기를 4의 배수로 맞추기 위한 영역인듯
} EXT2_DIR_ENTRY;						//디렉터리 엔트리로 디렉터리 내부에 어떤 데이터들이 저장되어 있는지 파악. 아마 연결리스트로 연결될거 같음

#ifdef _WIN32
#pragma pack(pop, fatstructures)
#else
#pragma pack()
#endif

typedef struct
{
	EXT2_SUPER_BLOCK		sb;			//슈퍼블럭을 가리킴
	EXT2_GROUP_DESCRIPTOR	gd;			//그룹 디스크립터를 가리킴 
	DISK_OPERATIONS*		disk;		//디스크 동작에 대한 것 disk.h 에 있다.
} EXT2_FILESYSTEM;						//이 구조체 하나가 블록 그룹 하나를 지칭 할 것으로 예상됨

typedef struct
{
	UINT32 group;						//디렉터리 엔트리가 저장되어있는 블록 그룹의 번호
	UINT32 block;						//고유한 번호로안하고 위에 블록 그룹에서의 데이터 블록 번호. 블록 그룹에서 상대적임
	UINT32 offset;						//위의 그룹, 블럭 번호를 통해서 알아낸 데이터 블록 상에서의 offset 번호
} EXT2_DIR_ENTRY_LOCATION;				//먼저 디렉터리 엔트리에 대한 정보를 찾고, 디렉터리 엔트리의 위치를 빠르게 찾기 위해서 사용하는 구조체

typedef struct
{
	EXT2_FILESYSTEM * fs;				//위의 구조체
	EXT2_DIR_ENTRY entry;				//디렉터리 엔트리의 최상단 아니면 최하단을 가리키는것으로 예상
	EXT2_DIR_ENTRY_LOCATION location;	//위의 구조체
} EXT2_NODE;							//블록 그룹 하나를 관리하기 위한 객체인 것 같다. 이게 블록 그룹 개수 만큼 있을 것 같다. 배열이든 연결리스트로든

#define SUPER_BLOCK 0					//슈퍼블럭의 블럭 위치. 항상 0번째임
#define GROUP_DES  1					//그룹 디스크립터는 슈퍼블록 뒤에 항상 위치 그래서 1번
#define BLOCK_BITMAP 2					//블록 비트맵은 그룹 디스크립터 바로 뒤에 위치 그래서 2번
#define INODE_BITMAP 3					//아이노드 비트맵은 블록 비트맵 바로 뒤에 위치 그래서 3번
#define INODE_TABLE(x) (4 + x)			//아이노드 테이블은 여러 블록에 있을 수 있기 때문에 4~ 번

#define FILE_TYPE_FIFO               0x1000	//명명된 파이프?
#define FILE_TYPE_CHARACTERDEVICE    0x2000	//문자 장치
#define FILE_TYPE_DIR				 0x4000	//디렉터리
#define FILE_TYPE_BLOCKDEVICE        0x6000	//블록 장치
#define FILE_TYPE_FILE				 0x8000	//이건 뭘까? 332페이지에서는 정규파일이라고 함
//332페이지 참고 아이노드의 File mode의 하위 13~16비트에 저장됨.

int meta_read(EXT2_FILESYSTEM *, SECTOR group, SECTOR block, BYTE* sector);			//data_read랑 똑같이 생겼는데 뭘까? 이건 디스크에서 데이터를 읽어주는 함수이다.
int meta_write(EXT2_FILESYSTEM * fs, SECTOR group, SECTOR block, BYTE* sector);		//data_write랑 똑같이 생김. 디스크에 데이터를 쓰는 함수이다.
int data_read(EXT2_FILESYSTEM *, SECTOR group, SECTOR block, BYTE* sector);			//위랑 내용 같음
int data_write(EXT2_FILESYSTEM * fs, SECTOR group, SECTOR block, BYTE* sector);		//위랑 내용 같음

int ext2_format(DISK_OPERATIONS* disk);												//ext2파일 시스템으로 초기화 하는 함수
int ext2_create(EXT2_NODE* parent, char* entryName, EXT2_NODE* retEntry);			//파일시스템에서 파일을 새로 생성할때 호출되는 함수
int ext2_lookup(EXT2_NODE* parent, const char* entryName, EXT2_NODE* retEntry);		//entryName을 갖는 엔트리가 있는지 검색해 그 위치를 리턴

int ext2_df(EXT2_FILESYSTEM *fs, unsigned int * total, unsigned int * used);
int ext2_rmdir(EXT2_NODE* dir);

//int ext2_read(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* entry, unsigned long offset, unsigned long length, char* buffer);
//ext2_shell.c에서 사용 위해 헤더에 추가 필요 예상 fs_read에서 호출 예정. 함수 선언부, 인자 받는 부분 수정 필요시 수정해야 될 수도. 일단 fs_read와 맞춰놓음
//void ext2_umount(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs);
//ext2_shell.c에서 사용 위해 헤더에 추가 필요 예상 fs_umount에서 호출 예정. 함수 선언부, 인자 받는 부분 수정 필요시 수정해야 될 수도. 일단 fs_umount와 맞춰놓음

UINT32 expand_block(EXT2_FILESYSTEM * , UINT32 );									//????
int fill_super_block(EXT2_SUPER_BLOCK * sb, SECTOR numberOfSectors, UINT32 bytesPerSector);										//메모리 어떤 곳에 슈퍼블록 내용을 채워넣는 함수
int fill_descriptor_block(EXT2_GROUP_DESCRIPTOR * gd, EXT2_SUPER_BLOCK * sb, SECTOR numberOfSectors, UINT32 bytesPerSector);	//메모리 어떤 곳에 파일디스크립터 내용을 채워넣는 함수
int create_root(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK * sb);						//루트 디렉터리를 생성하는 함수
void process_meta_data_for_block_used(EXT2_FILESYSTEM * fs, UINT32 inode_num, UINT32 select);		//

typedef int(*EXT2_NODE_ADD)(EXT2_FILESYSTEM*, void*, EXT2_NODE*);
//디렉토리 엔트리정보를 읽어 리스트로 계속 뒤에 추가하는 함수. EXT2_NODE 내부의 fs, entry필드에 연결해야하는 디렉터리 엔트리에 대한 정보가 담겨서 들어온다.
//첫번째 인자는 왜 필요한 걸까.
#endif
