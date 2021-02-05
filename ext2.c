typedef struct
{
	char*	address;
} DISK_MEMORY;

#include "ext2.h"
#define MIN( a, b )					( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX( a, b )					( ( a ) > ( b ) ? ( a ) : ( b ) )


int ext2_write(EXT2_NODE* file, unsigned long offset, unsigned long length, const char* buffer)	//
{
	BYTE	sector[MAX_SECTOR_SIZE];						//섹터크기만큼 배열 크기 설정
	DWORD	currentOffset, currentBlock, blockSeq = 0;		//
	DWORD	blockNumber, sectorNumber, sectorOffset;		//
	DWORD	readEnd;
	DWORD	blockSize;
	INODE node;
	int i;

	get_inode(file->fs, file->entry.inode, &node);
	currentBlock = node.block[0];
	readEnd = offset + length;

	currentOffset = offset;

	blockSize = MAX_BLOCK_SIZE;

	
	i = 1;
	while (offset > blockSize)	//기록할 위치 찾는 루프
	{

		currentBlock = get_data_block_at_inode(file->fs, node, ++i); //TODO
		blockSize += blockSize;		//암만봐도 NEXT_BLOCK_SIZE 넣는것 같다. TODO
		blockSeq++;
	}

	while (currentOffset < readEnd)
	{
		DWORD	copyLength;


		blockNumber = currentOffset / MAX_BLOCK_SIZE;

		if (currentBlock == 0)	//아이노드에 데이터가 아무것도 할당 안되어있는 경우
		{
			if (expand_block(file->fs, file->entry.inode) == EXT2_ERROR)	//file->entry.inode 에 해당하는 아이노드를 찾아서 해당 아이노드의 비어있는 블록에 새로운 데이터 블록 할당.
				return EXT2_ERROR;
			process_meta_data_for_block_used(file->fs, file->entry.inode);	//프로세스가 처리하다의 프로세스 같다. expand_block 에서 데이터 블록을 할당이나 해제 했을때, free_block_count나 block_bitmap같은 메타 데이터 수정
			get_inode(file->fs, file->entry.inode, &node);	
			currentBlock = node.block[0];					
		}

		if (blockSeq != blockNumber)	//다음 데이터 블록으로 넘어감
		{
			DWORD nextBlock;
			blockSeq++;
			++i;
			nextBlock = get_data_block_at_inode(file->fs, node, i);
			if (nextBlock == 0)
			{
				expand_block(file->fs, file->entry.inode);
				get_inode(file->fs, file->entry.inode, &node);
				process_meta_data_for_block_used(file->fs, file->entry.inode);

				nextBlock = get_data_block_at_inode(file->fs, node, i);
	
				if (nextBlock == 0)
				{
					return EXT2_ERROR;
				}
			}
			currentBlock = nextBlock;
		}
		sectorNumber = (currentOffset / (MAX_SECTOR_SIZE)) % (MAX_BLOCK_SIZE / MAX_SECTOR_SIZE);
		sectorOffset = currentOffset % MAX_SECTOR_SIZE;

		copyLength = MIN(MAX_SECTOR_SIZE - sectorOffset, readEnd - currentOffset);

		if (copyLength != MAX_SECTOR_SIZE)	//TODO
		{
			if (data_read(file->fs, file->fs->sb.block_group_number, currentBlock, sector)) //두번째 인자가 0일때는 그룹이 하나 일때, 여러개면 지금 있는게 맞다.
				break;
		}

		memcpy(&sector[sectorOffset],
			buffer,
			copyLength);

		if (data_write(file->fs, 0, currentBlock, sector))
			break;

		buffer += copyLength;
		currentOffset += copyLength;
	}

	node.size = MAX(currentOffset, node.size);
	set_inode_onto_inode_table(file->fs, file->entry.inode, &node);

	return currentOffset - offset;
}

UINT32 get_free_inode_number(EXT2_FILESYSTEM* fs);

void process_meta_data_for_inode_used(EXT2_NODE * retEntry, UINT32 inode_num, int fileType)
{
	/*
	아이노드를 할당, 해제했을 때 free_inode_count, inode_bitmap 등의 메타데이터를 알맞게 수정하는 함수
 	마찬가지로 아이노드를 수정한 후 이 함수를 호출할 경우 이 함수에서 수정된 아이노드의 번호를 가지고 수정내역을 다시 탐색 후 
	메타데이터를 수정해야 할 것 같은데 그럴 바에는 애초에 아이노드 할당이나 해제하는 함수에서 이 작업도 같이하는 편이 나을 듯
	세 번째 인자일 fileType은 파일이 디렉토리인지 아닌지 등을 판단해 메타데이터 수정 시 사용할 듯
	*/
}

int insert_entry(UINT32 inode_num, EXT2_NODE * retEntry, int fileType)//inode_num의 데이터 블록에 새로운 엔트리(retEntry) 추가. 성공여부 return
{
	/*
	인자로 받은 parent의 inode_num을 가지고 해당 아이노드를 읽어온 후 새로운 엔트리를 삽입해주는 함수
 	fat에서와 같이 루트 디렉토리, overwrite 등을 생각해 주어야 할 듯(빈 엔트리 찾지 못할 경우,
	 DIR_ENTRY_NO_MORE을 나타내는 디렉토리 엔트리 추가, 위치정보 업데이트 등)
	seungmin */
  // retEntry의 fileType 지정 필요
	// retEntry의 inode number가 없으면 새로운 inode number(retEntry->entry.inode) 할당
}

UINT32 get_available_data_block(EXT2_FILESYSTEM * fs, UINT32 inode_num)	//사용가능한 데이터 블록을 가져오는 함수?
{
	/*
	사실 선언만 되어 있고 사용되는 곳이 없어서 함수의 정확한 기능을 유추하기는 불가능. 따라서 기능은 정의하기 나름일 것 같음
	함수가 사용되고 있지 않음 -> 정의되지 않은 함수 내에서 사용될 것으로 생각됨 – expand_block 함수에서 사용될 것으로 예상
 	함수명과 인자를 바탕으로 생각해봤을 때 첫 번째 인자로 받은 EXT2_FILESYSTEM 구조체에서 free_block_count를 확인해 할당할 블록이 있는지 확인하고, 
	있다면 block_bitmap에서 빈 블록 번호를 return
 	expand_block 함수에서는 return 받은 블록 번호를 아이노드의 block 필드의 비어 있는 인덱스에 연결하고, process_meta_data_for_block_used를 통해 메타데이터 수정
	*/
}

unsigned char toupper(unsigned char ch);	//to upper 즉 대문자로 바꾸는 함수 같은데 c 라이브러리에 있는 함수인듯
int isalpha(unsigned char ch);				//알파벳 확인 함수
int isdigit(unsigned char ch);				//숫자 확인 함수

void upper_string(char* str, int length)	//상위 몇비트를 대문자로 바꾸는 함수
{
	while (*str && length-- > 0)
	{
		*str = toupper(*str);
		str++;
	}
}

int format_name(EXT2_FILESYSTEM* fs, char* name)	//파일 이름의 형식이 올바른지 체크하는 함수
{
	UINT32	i, length;
	UINT32	extender = 0, nameLength = 0;
	UINT32	extenderCurrent = 8;
	BYTE	regularName[MAX_ENTRY_NAME_LENGTH];

	memset(regularName, 0x20, sizeof(regularName));
	length = strlen(name);

	if (strncmp(name, "..", 2) == 0)
	{
		memcpy(name, "..         ", 11);
		return EXT2_SUCCESS;
	}
	else if (strncmp(name, ".", 1) == 0)
	{
		memcpy(name, ".          ", 11);
		return EXT2_SUCCESS;
	}
	else
	{
		upper_string(name, MAX_ENTRY_NAME_LENGTH);

		for (i = 0; i < length; i++)
		{
			if (name[i] != '.' && !isdigit(name[i]) && !isalpha(name[i]))
				return EXT2_ERROR;

			if (name[i] == '.')
			{
				if (extender)
					return EXT2_ERROR;
				extender = 1;
			}
			else if (isdigit(name[i]) || isalpha(name[i]))
			{
				if (extender)
					regularName[extenderCurrent++] = name[i];
				else
					regularName[nameLength++] = name[i];
			}
			else
				return EXT2_ERROR;
		}

		if (nameLength > 8 || nameLength == 0 || extenderCurrent > 11)
			return EXT2_ERROR;
	}

	memcpy(name, regularName, sizeof(regularName));
	return EXT2_SUCCESS;
}

/*
	같은 이름의 디렉터리 엔트리가 있는지 찾는 함수 (NULL인 경우는 아무 유효한 엔트리를 찾음)
	name이 존재하는 경우 : EXT2_SUCCESS 반환, 찾은 ENTRY로 retEntry가 가리키는 부분 초기화
	name이 존재하지 않는 경우 : EXT2_ERROR 반환
*/
int lookup_entry(EXT2_FILESYSTEM* fs, const int inode, const char* name, EXT2_NODE* retEntry)
{
	if (inode == 2) // 루트 디렉터리
		return find_entry_on_root(fs, inode, name, retEntry);
	else
		return find_entry_on_data(fs, inode, name, retEntry);
}

// 섹터(데이터 블록)에서 formattedName을 가진 엔트리를 찾아 그 위치를 number에 저장
int find_entry_at_sector(const BYTE* sector, const BYTE* formattedName, UINT32 begin, UINT32 last, UINT32* number)
{
	// 섹터 내부의 엔트리를 루프로 돌면서 formattedName과 이름이 같은 엔트리 검색
	// 있으면 number변수에 섹터 내에서의 위치를 저장하고, EXT2_SUCCESS 리턴
}

// 루트 디렉터리 영역에서 formattedName의 엔트리 검색해서 EXT2_NODE* ret에 저장
int find_entry_on_root(EXT2_FILESYSTEM* fs, INODE inode, char* formattedName, EXT2_NODE* ret)
{
	BYTE	sector[MAX_SECTOR_SIZE];
	UINT32	i, number;
	UINT32	entriesPerSector, lastEntry;
	INT32	result;
	SECTOR	rootBlock;
	EXT2_DIR_ENTRY*	entry;

	entriesPerSector	= fs->disk->bytesPerSector / sizeof( EXT2_DIR_ENTRY ); // 섹터 당 엔트리 수
	lastEntry			= entriesPerSector - 1; // 마지막 엔트리
	
	rootBlock = get_data_block_at_inode(fs, inode, 1); // 루트 디렉터리의 첫번째 데이터블록 번호
	read_root_sector(fs, sector); // 루트 디렉터리의 데이터블록 내용을 sector 버퍼에 write
	entry = (EXT2_DIR_ENTRY*)sector; // 섹터의 시작주소
	
	result = find_entry_at_sector(sector, formattedName, 0, lastEntry, &number); // 섹터에서 formattedName을 가진 엔트리를 찾아 그 위치를 number에 저장

	if( result == -1 ) // formattedName을 가진 엔트리가 없는 경우
		return EXT2_ERROR;
	else // 현재 섹터에서 찾았거나 마지막 엔트리까지 검색한 경우
	{
		if( result == -2 ) // 더 이상 엔트리가 없다면 에러
			return EXT2_ERROR;
		else // 해당 엔트리를 찾았다면 ret에서 가리키는 EXT2_NODE를 entry 정보로 초기화
		{
			memcpy( &ret->entry, &entry[number], sizeof( EXT2_DIR_ENTRY ) );

			ret->location.group	= 0; // ?
			ret->location.block	= rootBlock;
			ret->location.offset	= number;

			ret->fs = fs;
		}

		return EXT2_SUCCESS;
	}

	return EXT2_ERROR; // 어떤 섹터에도 해당 엔트리가 없다면 에러
}

// 데이터 영역에서 formattedName의 엔트리 검색
int find_entry_on_data(EXT2_FILESYSTEM* fs, INODE first, const BYTE* formattedName, EXT2_NODE* ret)
{
	BYTE	sector[MAX_SECTOR_SIZE];
	UINT32	i, number;
	UINT32	entriesPerSector, lastEntry;
	INT32	blockNum;
	INT32	result;
	EXT2_DIR_ENTRY*	entry;

	entriesPerSector	= fs->disk->bytesPerSector / sizeof( EXT2_DIR_ENTRY ); // 섹터 당 엔트리 수
	lastEntry			= entriesPerSector - 1; // 마지막 엔트리

	for (i = 0; i < first.blocks; i++) // 데이터 블록 단위로 검색 
	{
		blockNum = get_data_block_at_inode(fs, first, i); // 데이터 블록 번호
		data_read(fs, 0, blockNum, sector); // 데이터 블록의 데이터를 sector 버퍼에 저장
		entry = (EXT2_DIR_ENTRY*)sector; // 섹터의 시작주소

		result = find_entry_at_sector(sector, formattedName, 0, lastEntry, &number); // 섹터에서 formattedName을 가진 엔트리를 찾아 그 위치를 number에 저장

		if( result == -1 ) // 해당 섹터에 formattedName을 가진 엔트리가 없다면 다음 섹터에서 검색
			continue; 
		else // 현재 섹터에서 찾았거나 마지막 엔트리까지 검색한 경우
		{
			if( result == -2 ) // 더 이상 엔트리가 없다면 에러
				return EXT2_ERROR;
			else // 해당 엔트리를 찾았다면 ret에서 가리키는 FAT_NODE를 entry 정보로 초기화
			{
				memcpy( &ret->entry, &entry[number], sizeof( EXT2_DIR_ENTRY ) ); // 엔트리의 내용을 복사

				ret->location.group	= 0; // ?
				ret->location.block	= blockNum;
				ret->location.offset	= number;

				ret->fs = fs;
			}

			return EXT2_SUCCESS;
		}

	}
}

// inode table에서 inode number에 대한 메타데이터를 inodeBuffer에 저장
int get_inode(EXT2_FILESYSTEM* fs, const UINT32 inode, INODE *inodeBuffer)
{
}

// 루트 디렉터리의 섹터단위 데이터블록을 sector 버퍼에 write
int read_root_sector(EXT2_FILESYSTEM* fs, BYTE* sector)	//루트 디렉터리에 관한 정보를 읽어옴. fs로 넘겨주면, sector에 담아줌
{
	UINT32 inode = 2; // 루트 디렉터리 inode number
	INODE inodeBuffer; // 아이노드 메타데이터
	SECTOR rootBlock; // 루트 디렉터리의 첫번째 데이터블록 번호
	get_inode(fs, inode, &inodeBuffer); // 루트 디렉터리의 메타데이터를 inodeBuffer에 저장
	rootBlock = get_data_block_at_inode(fs, inodeBuffer, 1); // 루트 디렉터리의 첫번째 데이터 블록 번호를 return
  
	return data_read(fs, 0, rootBlock, sector);	// 루트 디렉터리의 데이터 블록의 데이터를 sector 버퍼에 저장
}

// inode의 number번째 데이터 블록 번호를 return
int get_data_block_at_inode(EXT2_FILESYSTEM *fs, INODE inode, UINT32 number)	//inode : 어떤 파일의 아이노드, number : inode 구조체의 block필드에서 몇번째 데이터 블록을 불러올지 결정하는 변수 인듯
{
  /*
	두 번째 인자로 받은 아이노드의 block 필드에서 세 번째 인자로 받은 number인덱스에 해당하는 데이터 블록의 번호를 return
	이 때 block 필드의 13, 14, 15번 째 인덱스는 이중 혹은 삼중 포인터임으로 이 부분에 대한 해결이 필요할 듯
	??? - ext2_write에서 i=1 후 ++i를 하는 이유는? -> i++이 맞을 것 같음
	seungmin */
  
	//만약 number이 0~11이 들어오면 직접 데이터 블록 받아서 리턴
	//12면 간접 블록 들어가서 안에 어떤 데이터 블록을 가리키는지 가져올 필요가 있음
	//만약 넘버가 13이라면, 이중 간접 블록이 아니라 아이노드 12번째 구조체가 가리키는 간접 블록을 먼저 들어가서 거기서 12번재 다음 블록을 찾을 것으로 예상
	//블록 그룹 계산하지 않은 그냥 블록 그룹 내에서 블록 번호 리턴하는 것으로 생각
}

int ext2_read_superblock(EXT2_FILESYSTEM* fs, EXT2_NODE* root)	//슈퍼블록을 읽는 함수 인것 같다. root는 읽은 슈퍼블록을 담을 곳을 인자로 넘겨 받음
{
	INT result;						//결과를 리턴을 위한 변수
	BYTE sector[MAX_SECTOR_SIZE];	//섹터크기 만큼 바이트 설정. 연속적으로 고정된 공간 할당 위해 정적배열 사용 (1024바이트)

	if (fs == NULL || fs->disk == NULL)	//fs가 지정되지 않으면 에러
	{
		WARNING("DISK OPERATIONS : %p\nEXT2_FILESYSTEM : %p\n", fs, fs->disk);
		return EXT2_ERROR;
	}


	meta_read(fs, 0, SUPER_BLOCK, sector);	//
	memcpy(&fs->sb, sector, sizeof(EXT2_SUPER_BLOCK));	//첫번째 인자가 목적지, 두번째 인자가 어떤 것을 복사할지, 세번째는 크기
	meta_read(fs, 0, GROUP_DES, sector);	//그룹 디스크립터 읽는듯.
	memcpy(&fs->gd, sector, sizeof(EXT2_GROUP_DESCRIPTOR));
	//디스크에서 슈퍼블록 정보 입력을 받아와서 인자로 들어온 fs의 슈퍼블록을 업데이트하는 과정으로 생각됨.

	if (fs->sb.magic_signature != 0xEF53)	//슈퍼블럭인지 판단하는 필드. 고유값 확인으로.
		return EXT2_ERROR;

	ZeroMemory(sector, sizeof(MAX_SECTOR_SIZE));	//메모리 초기화.
	if (read_root_sector(fs, sector))	//슈퍼블록이 루트 디렉터리를 제대로 가리키는지 체크하기 위한 부분이 아닐까 생각.
		return EXT2_ERROR;

	ZeroMemory(root, sizeof(EXT2_NODE));
	memcpy(&root->entry, sector, sizeof(EXT2_DIR_ENTRY));	//앞의 테스트를 거치면 제대로 된 슈퍼블록이므로 root에 담아서 리턴해준다.
	root->fs = fs;

	return EXT2_SUCCESS;
}

UINT32 get_free_inode_number(EXT2_FILESYSTEM* fs)	//비어있는 아이노드 번호를 출력하는 것 같다.
{
	//EXT2_FILESYSTEM 구조체를 보면 슈퍼블록 그룹디스크립터 있음.그룹 디스크립터는 비어있는 아이노드 수, 아이노드 테이블 시작 주소, 비트맵 시작주소 가지고 있음
	//일단 아이노드 수 체크해서 없으면 에러 있으면 비트맵 비교를 통해 가장 앞에 비어있는 아이노드 번호를 리턴 해주어야 한다고 생각.
	//리턴 형이 unsigned int 32비트 형식이니까 그대로 아이노드 번호를 리턴해주어야할것으로 생각.
}

int set_inode_onto_inode_table(EXT2_FILESYSTEM *fs, const UINT32 which_inode_num_to_write, INODE * inode_to_write)	//아이노드를 아이노드 테이블에 저장하는 과정으로 생각됨
{
	//호출하는 쪽에서 get_free_inode_number을 통해서 비어있는 아이노드 번호를 알아내서 which_inode_num_to_write로 넘겨줄것으로 예상
	//호출하는 쪽에서 새로 생성되는 파일에 대한 아이노드 구조체를 새로 만듬. 그리고 그 구조체를 인자로 넘겨줄것으로 예상됨
	//이 함수에서는 그러면 아이노드 테이블에 아이노드 정보를 기록하고 성공여부를 리턴할 것으로 예상됨.
}

// 디렉터리의 엔트리들을 리스트에 담음
int ext2_read_dir(EXT2_NODE* dir, EXT2_NODE_ADD adder, void* list)
{
	BYTE   sector[MAX_SECTOR_SIZE];
	INODE* inodeBuffer;
	UINT32 inode;
	int i, result, num;

	inodeBuffer = (INODE*)malloc(sizeof(INODE));

	ZeroMemory(sector, MAX_SECTOR_SIZE);
	ZeroMemory(inodeBuffer, sizeof(INODE));

	result = get_inode(dir->fs, dir->entry.inode, inodeBuffer); // inode number에 대한 메타데이터를 inodeBuffer에 저장
	if (result == EXT2_ERROR)
		return EXT2_ERROR;

	// 데이터 블록 단위로 루프
	for (i = 0; i < inodeBuffer->blocks; ++i)
	{
		num = get_data_block_at_inode(dir->fs, *inodeBuffer, i + 1); // inodeBuffer의 number(i+1)번째 데이터 블록 번호를 return
		data_read(dir->fs, 0, num, sector); // 디스크 영역에서 현재 블록그룹의 num번째 데이터 블록의 데이터를 sector 버퍼에 읽어옴

		if (dir->entry.inode == 2) // 루트 디렉터리
			read_dir_from_sector(dir->fs, sector + 32, adder, list); // 디렉터리 정보를 담은 sector 버퍼를 읽어 엔트리를 list에 추가 (+32?)
		else
			read_dir_from_sector(dir->fs, sector, adder, list); // 디렉터리 정보를 담은 sector 버퍼를 읽어 엔트리를 list에 추가
	}

	return EXT2_SUCCESS;
}

int read_dir_from_sector(EXT2_FILESYSTEM* fs, BYTE* sector, EXT2_NODE_ADD adder, void* list)	//블록단위가 아닌 섹터 단위에서 디렉토리 엔트리 리스트들을 가져오는 함수 인듯. 
{
	UINT i, max_entries_Per_Sector;
	EXT2_DIR_ENTRY*   dir;
	EXT2_NODE   node;

	max_entries_Per_Sector = MAX_SECTOR_SIZE / sizeof(EXT2_DIR_ENTRY);	//최대 섹터 크기를 디렉터리 엔트리 크기로 나누어서 섹터에 들어갈 수 있는 디렉터리 엔트리 개수를 구한다.
	dir = (EXT2_DIR_ENTRY*)sector;	//디렉토리 엔트리 주소를 sector로 받아서 dir에 저장하고 dir로 이용

	for (i = 0; i < max_entries_Per_Sector; i++)
	{
		if (dir->name[0] == DIR_ENTRY_FREE)	//탐색하다가 중간에 비어 있는 공간이 있으면 그냥 통과. fragmentation일 수도 있으니.
			;
		else if (dir->name[0] == DIR_ENTRY_NO_MORE)	//더 이상 디렉터리 엔트리가 없으면. 루프 나옴.
			break;
		else	//비어있는 공간도 아니고 실제로 디렉터리 엔트리가 있으면 list에 저장하고 다음 엔트리가 있는지 탐색
		{
			node.fs = fs;
			node.entry = *dir;
			adder(fs, list, &node);	//adder 함수가 구현되어 있지 않은것 같은데 구현 필요한듯. adder를 하면 list에 node.entry의 정보가 달려오지 않을까.
		}
		dir++;
	}

	return (i == max_entries_Per_Sector ? 0 : -1);	//끝가지 돌면 0리턴, 아니면 -1리턴.
}

char* my_strncpy(char* dest, const char* src, int length)
{
	while (*src && *src != 0x20 && length-- > 0)
		*dest++ = *src++;

	return dest;
}


// parent에 새로운 디렉터리 생성
int ext2_mkdir(const EXT2_NODE* parent, const char* entryName, EXT2_NODE* retEntry)
{
	EXT2_NODE      dotNode, dotdotNode;
	DWORD         firstCluster;
	BYTE         name[MAX_NAME_LENGTH];
	int            result;
	int            i;

	strcpy((char*)name, entryName); // 입력한 이름 복사

	if (format_name(parent->fs, (char*)name)) // EXT2 버전의 형식에 맞게 이름 수정
		return EXT2_ERROR;

	/* newEntry */
	ZeroMemory(retEntry, sizeof(EXT2_NODE));
	memcpy(retEntry->entry.name, name, MAX_ENTRY_NAME_LENGTH); // name을 복사
	retEntry->entry.name_len = strlen((char*)retEntry->entry.name); // name의 길이 저장
	retEntry->fs = parent->fs; // EXT2_FILESYSTEM 복사

	result = insert_entry(parent->entry.inode, retEntry, FILE_TYPE_DIR); // 부모 디렉터리에 새로운 엔트리(retEntry) 추가
	if (result == EXT2_ERROR) // 에러 발생시 종료
		return EXT2_ERROR;

	expand_block(parent->fs, retEntry->entry.inode); // 새로운 엔트리(retEntry)의 데이터블록 할당

	/* dotEntry */
	ZeroMemory(&dotNode, sizeof(EXT2_NODE));
	memset(dotNode.entry.name, 0x20, 11); // 이름을 space로 초기화
	dotNode.entry.name[0] = '.'; // 엔트리 이름 설정 '.'
	dotNode.fs = retEntry->fs; // 파일시스템 복사
	dotNode.entry.inode = retEntry->entry.inode; // retEntry의 아이노드 복사
	insert_entry(retEntry->entry.inode, &dotNode, FILE_TYPE_DIR); // 새로운 디렉터리(retEntry)에 dotEntry 추가

	/* dotdotEntry */
	ZeroMemory(&dotdotNode, sizeof(EXT2_NODE));
	memset(dotdotNode.entry.name, 0x20, 11); // 이름을 space로 초기화
	dotdotNode.entry.name[0] = '.'; // 엔트리 이름 설정 '..'
	dotdotNode.entry.name[1] = '.';
	dotdotNode.entry.inode = parent->entry.inode; // 부모 디렉터리의 아이노드 복사
	dotdotNode.fs = retEntry->fs; // 파일시스템 복사
	insert_entry(retEntry->entry.inode, &dotdotNode, FILE_TYPE_DIR); // 새로운 디렉터리(retEntry)에 dotdotEntry 추가

	return EXT2_SUCCESS;
}

int meta_read(EXT2_FILESYSTEM * fs, SECTOR group, SECTOR block, BYTE* sector)	//블록 그룹과 블럭을 입력하면 디스크에서 실제 몇번 블럭인지 앞에서 부터 다 계산해서 디스크에서 가져온다.
{
	const SECTOR BOOT_BLOCK = 1;
	SECTOR real_index = BOOT_BLOCK + group * fs->sb.block_per_group + block;

	return fs->disk->read_sector(fs->disk, real_index, sector);
}
int meta_write(EXT2_FILESYSTEM * fs, SECTOR group, SECTOR block, BYTE* sector)	//블록 그룹과 블럭을 입력하면 디스크에서 실제 몇번 블럭인지 앞에서 부터 다 계산해서 디스크에 쓴다.
{
	const SECTOR BOOT_BLOCK = 1;
	SECTOR real_index = BOOT_BLOCK + group * fs->sb.block_per_group + block;

	return fs->disk->write_sector(fs->disk, real_index, sector);
}
int data_read(EXT2_FILESYSTEM * fs, SECTOR group, SECTOR block, BYTE* sector)
{
	const SECTOR BOOT_BLOCK = 1;
	SECTOR real_index = BOOT_BLOCK + group * fs->sb.block_per_group + block;


	return fs->disk->read_sector(fs->disk, real_index, sector); // 성공 여부 리턴 (disksim.c -> disksim_read)
}
int data_write(EXT2_FILESYSTEM * fs, SECTOR group, SECTOR block, BYTE* sector)
{
	const SECTOR BOOT_BLOCK = 1;
	SECTOR real_index = BOOT_BLOCK + group * fs->sb.block_per_group + block;


	return fs->disk->write_sector(fs->disk, real_index, sector); // 성공 여부 리턴 (disksim.c -> disksim_write)
}

int ext2_format(DISK_OPERATIONS* disk)	//디스크를 ext2파일 시스템으로 초기화 하는 함수
{
	EXT2_SUPER_BLOCK sb;
	EXT2_GROUP_DESCRIPTOR gd;
	EXT2_GROUP_DESCRIPTOR  gd_another_group;

	QWORD sector_num_per_group = (disk->numberOfSectors - 1) / NUMBER_OF_GROUPS;	//디스크로부터 디스크 섹터 개수에 대한 정보를 읽어온다. 미리 설정된 그룹 개수만큼 나누어 그룹당 섹터 개수를 계산한다.
	int i, gi, j;
	const int BOOT_SECTOR_BASE = 1;	//부트 섹터를 제외한 파일시스템의 기본번지 설정번지에 위치하도록

	char sector[MAX_SECTOR_SIZE];	//슈퍼블록을 넣을 메모리 공간을 할당받는다. 1KB만큼 하랑 받음

	// super block
	if (fill_super_block(&sb, disk->numberOfSectors, disk->bytesPerSector) != EXT2_SUCCESS)	//메모리의 어떤 공간에 슈퍼블록 구조체에 관한 내용을 채워넣는다.
		return EXT2_ERROR;	//실패시 에러

	ZeroMemory(sector, sizeof(sector));	//슈퍼블록이 들어갈 메모리 공간을 0으로 초기화 한다.
	memcpy(sector, &sb, sizeof(sb));	//아까 슈퍼블록 구조체에 넣어놨던 데이터를 미리 정해둔 크기의 메모리에 올린다.
	disk->write_sector(disk, BOOT_SECTOR_BASE + 0, sector);	//disk의 동작을 통해 부트섹터를 제외한 디스크의 가장 처음 블록에 슈퍼블록을 저장한다.

	//Group Descriptor Table
	if (fill_descriptor_block(&gd, &sb, disk->numberOfSectors, disk->bytesPerSector) != EXT2_SUCCESS)	//슈퍼블록을 디스크에 저장하는 과정과 마찬가지로 디스크에 블록 디스크립터를 저장한다.
		return EXT2_ERROR;

	gd_another_group = gd;
	gd_another_group.free_inodes_count = NUMBER_OF_INODES / NUMBER_OF_GROUPS;
	gd_another_group.free_blocks_count = sb.free_block_count / NUMBER_OF_GROUPS;

	ZeroMemory(sector, sizeof(sector));
	for (j = 0; j < NUMBER_OF_GROUPS; j++)
	{
		if (j == 0)memcpy(sector + j * sizeof(gd), &gd, sizeof(gd));
		else memcpy(sector + j * sizeof(gd_another_group), &gd_another_group, sizeof(gd_another_group));
	}

	disk->write_sector(disk, BOOT_SECTOR_BASE + 1, sector);	//내부 원리 이해잘 못했음

	// block bitmap
	ZeroMemory(sector, sizeof(sector));

	sector[0] = 0xff;
	sector[1] = 0xff;
	sector[2] = 0x01;
	disk->write_sector(disk, BOOT_SECTOR_BASE + 2, sector);

	// inode bitmap
	ZeroMemory(sector, sizeof(sector));

	sector[0] = 0xff;
	sector[1] = 0x03;
	disk->write_sector(disk, BOOT_SECTOR_BASE + 3, sector);

	// inode table
	ZeroMemory(sector, sizeof(sector));

	for (i = 4; i < sb.first_data_block_each_group; i++)
		disk->write_sector(disk, BOOT_SECTOR_BASE + i, sector);

	for (gi = 1; gi < NUMBER_OF_GROUPS; gi++)
	{
		sb.block_group_number = gi;

		ZeroMemory(sector, sizeof(sector));
		memcpy(sector, &sb, sizeof(sb));

		disk->write_sector(disk, sector_num_per_group * gi + BOOT_SECTOR_BASE, sector);

		ZeroMemory(sector, sizeof(sector));
		for (j = 0; j < NUMBER_OF_GROUPS; j++)
		{
			memcpy(sector + j * sizeof(gd), &gd, sizeof(gd));
		}
		disk->write_sector(disk, sector_num_per_group * gi + BOOT_SECTOR_BASE + 1, sector);

		// block bitmap
		ZeroMemory(sector, sizeof(sector));
		sector[0] = 0xff;
		sector[1] = 0xff;
		sector[2] = 0x01;
		disk->write_sector(disk, sector_num_per_group * gi + BOOT_SECTOR_BASE + 2, sector);

		//inode bitmap
		ZeroMemory(sector, sizeof(sector));

		disk->write_sector(disk, sector_num_per_group * gi + BOOT_SECTOR_BASE + 3, sector);

		// inode table
		ZeroMemory(sector, sizeof(sector));
		for (i = 4; i < sb.first_data_block_each_group; i++)
			disk->write_sector(disk, sector_num_per_group * gi + BOOT_SECTOR_BASE + i, sector);
	}

	PRINTF("max inode count                : %u\n", sb.max_inode_count);
	PRINTF("total block count              : %u\n", sb.block_count);
	PRINTF("byte size of inode structure   : %u\n", sb.inode_structure_size);
	PRINTF("block byte size                : %u\n", MAX_BLOCK_SIZE);
	PRINTF("total sectors count            : %u\n", NUMBER_OF_SECTORS);
	PRINTF("sector byte size               : %u\n", MAX_SECTOR_SIZE);
	PRINTF("\n");

	create_root(disk, &sb);	//루트디렉터리를 만들어 슈퍼블럭과 연결시킨다.

	return EXT2_SUCCESS;
}
int ext2_create(EXT2_NODE* parent, char* entryName, EXT2_NODE* retEntry)	//파일시스템에서 파일을 새로 생성할때 호출되는 함수.
{
	if ((parent->fs->gd.free_inodes_count) == 0) return EXT2_ERROR;	//상성가능한 아이노드 공간이 없으면 에러
	UINT32 inode;
	BYTE name[MAX_NAME_LENGTH] = { 0, };
	BYTE sector[MAX_BLOCK_SIZE];
	int result;

	strcpy(name, entryName);
	if (format_name(parent->fs, name) == EXT2_ERROR) return EXT2_ERROR;	//이름이 형식에 맞지 않으면 에러

	ZeroMemory(retEntry, sizeof(EXT2_NODE));
	memcpy(retEntry->entry.name, name, MAX_ENTRY_NAME_LENGTH);
	retEntry->fs = parent->fs;
	inode = parent->entry.inode;
	if ((result = lookup_entry(parent->fs, inode, name, retEntry)) == EXT2_SUCCESS) return EXT2_ERROR;	//이미 디렉터리에 해당 이름의 파일이 있으면 에러
	else if (result == -2) return EXT2_ERROR;

	if (insert_entry(inode, retEntry, 0) == EXT2_ERROR)return EXT2_ERROR;
	return EXT2_SUCCESS;
}
int ext2_lookup(EXT2_NODE* parent, const char* entryName, EXT2_NODE* retEntry)	//entryName을 갖는 엔트리가 있는지 검색해 그 위치를 리턴
{
	EXT2_DIR_ENTRY_LOCATION	begin;
	BYTE	formattedName[MAX_NAME_LENGTH] = { 0, };

	strcpy(formattedName, entryName);

	if (format_name(parent->fs, formattedName)) //파일이름이 올바른지 체크
		return EXT2_ERROR;

	return lookup_entry(parent->fs, parent->entry.inode, formattedName, retEntry);
}

UINT32 expand_block(EXT2_FILESYSTEM * fs, UINT32 inode_num)	// inode에 새로운 데이터블록 할당
{
	/*
	두 번째 인자로 받은 inode_num에 해당하는 아이노드를 찾아서(get_inode 이용하면 될 듯) 
	해당 아이노드의 block 필드의 비어있는 인덱스에 새로운 데이터 블록 할당
	seungmin */
  // 데이터블록 비트맵 업데이트 필요

}
int fill_super_block(EXT2_SUPER_BLOCK * sb, SECTOR numberOfSectors, UINT32 bytesPerSector)	//슈퍼블록 포인터와 섹터 개수 섹터당 바이트 수를 받으면 슈퍼블록 구조체를 채워넣는다.
{
	ZeroMemory(sb, sizeof(EXT2_SUPER_BLOCK));

	sb->max_inode_count = NUMBER_OF_INODES;
	sb->block_count = numberOfSectors;
	sb->reserved_block_count = 0;
	sb->free_block_count = numberOfSectors - (17 * NUMBER_OF_GROUPS) - 1;
	sb->free_inode_count = NUMBER_OF_INODES - 10;
	sb->first_data_block = 1;
	sb->log_block_size = 0;
	sb->log_fragmentation_size = 0;
	sb->block_per_group = (numberOfSectors - 1) / NUMBER_OF_GROUPS;
	sb->fragmentation_per_group = 0;
	sb->inode_per_group = NUMBER_OF_INODES / NUMBER_OF_GROUPS;
	sb->magic_signature = 0xEF53;
	sb->errors = 0;
	sb->first_non_reserved_inode = 11;
	sb->inode_structure_size = 128;
	sb->block_group_number = 0;
	sb->first_data_block_each_group = 1 + 1 + 1 + 1 + 13;

	return EXT2_SUCCESS;
}
int fill_descriptor_block(EXT2_GROUP_DESCRIPTOR * gd, EXT2_SUPER_BLOCK * sb, SECTOR numberOfSectors, UINT32 bytesPerSector) //그룹 디스크립터 포인터와 슈퍼블록 포인터, 섹터 개수, 섹터당 바이트 수를 받으면 그룹디스크립터 구조체를 채워넣는다.
{
	ZeroMemory(gd, sizeof(EXT2_GROUP_DESCRIPTOR));

	gd->start_block_of_block_bitmap = 2;
	gd->start_block_of_inode_bitmap = 3;
	gd->start_block_of_inode_table = 4;
	gd->free_blocks_count = (UINT16)(sb->free_block_count / NUMBER_OF_GROUPS + sb->free_block_count % NUMBER_OF_GROUPS);
	gd->free_inodes_count = (UINT16)(((sb->free_inode_count) + 10) / NUMBER_OF_GROUPS - 10);
	gd->directories_count = 0;

	return EXT2_SUCCESS;
}
int create_root(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK * sb)	//루트 디렉터리를 생성하는 함수
{
	BYTE   sector[MAX_SECTOR_SIZE];	//
	SECTOR   rootSector = 0;		//
	EXT2_DIR_ENTRY * entry;			//
	EXT2_GROUP_DESCRIPTOR * gd;		//
	EXT2_SUPER_BLOCK * sb_read;		//
	QWORD sector_num_per_group = (disk->numberOfSectors - 1) / NUMBER_OF_GROUPS;
	INODE * ip;
	const int BOOT_SECTOR_BASE = 1;
	int gi;

	ZeroMemory(sector, MAX_SECTOR_SIZE);
	entry = (EXT2_DIR_ENTRY*)sector;

	memcpy(entry->name, VOLUME_LABLE, 11);
	entry->name_len = strlen(VOLUME_LABLE);
	entry->inode = 2;	//루트 아이노드의 아이노드 번호는 2번
	entry++;
	entry->name[0] = DIR_ENTRY_NO_MORE;
	rootSector = 1 + sb->first_data_block_each_group;
	disk->write_sector(disk, rootSector, sector);

	sb_read = (EXT2_SUPER_BLOCK *)sector;
	for (gi = 0; gi < NUMBER_OF_GROUPS; gi++)
	{
		disk->read_sector(disk, sector_num_per_group * gi + BOOT_SECTOR_BASE, sector);
		sb_read->free_block_count--;

		disk->write_sector(disk, sector_num_per_group * gi + BOOT_SECTOR_BASE, sector);
	}
	sb->free_block_count--;

	gd = (EXT2_GROUP_DESCRIPTOR *)sector;
	disk->read_sector(disk, BOOT_SECTOR_BASE + 1, sector);


	gd->free_blocks_count--;
	gd->directories_count = 1;

	for (gi = 0; gi < NUMBER_OF_GROUPS; gi++)
		disk->write_sector(disk, sector_num_per_group * gi + BOOT_SECTOR_BASE + 1, sector);

	disk->read_sector(disk, BOOT_SECTOR_BASE + 2, sector);
	sector[2] |= 0x02;
	disk->write_sector(disk, BOOT_SECTOR_BASE + 2, sector);

	ZeroMemory(sector, MAX_SECTOR_SIZE);
	ip = (INODE *)sector;
	ip++;
	ip->mode = 0x1FF | 0x4000;
	ip->size = 0;
	ip->blocks = 1;
	ip->block[0] = sb->first_data_block_each_group;
	disk->write_sector(disk, BOOT_SECTOR_BASE + 4, sector);

	return EXT2_SUCCESS;
}

void process_meta_data_for_block_used(EXT2_FILESYSTEM * fs, UINT32 inode_num)
{
  /*
	처리하다의 process
 	expand_block 등에서 데이터 블록을 할당, 해제했을 때 free_block_count, block_bitmap 등의 메타데이터를 알맞게 수정하는 함수
 	expand_block은 할당의 경우였고 해제의 경우에도 사용될 듯
 	다만 데이터 블록을 수정한 후 이 함수를 호출할 경우 이 함수에서 수정된 아이노드의 번호를가지고 수정내역을 다시 탐색 후 
	메타데이터를 수정해야 할 것 같은데 그럴 바에는 애초에 블록 할당이나 해제하는 함수에서 이 작업도 같이하는 편이 나을 것 같음
	seungmin */
	//inode번호로 아이노드 테이블에서 아이노드를 가져옴. 아이노드 데이터블럭에서 가장 마지막 블럭을 비트맵에 사용중이라고 표시
}


/******************************************************************************/
/* Remove file                                                                */
/******************************************************************************/

// 파일 삭제
int ext2_remove(EXT2_NODE* file)
{
	INODE* inodeBuffer;
	int result;

	inodeBuffer = (INODE*)malloc(sizeof(INODE));
	ZeroMemory(inodeBuffer, sizeof(INODE));
	result = get_inode(file->fs, file->entry.inode, inodeBuffer); // inode number에 대한 메타데이터를 inodeBuffer에 저장
	if (result == EXT2_ERROR)
		return EXT2_ERROR;

	if(inodeBuffer->mode & FILE_TYPE_DIR )  // 해당 엔트리가 디렉터리이면 에러 - mode에서 file type 추출해야 함
		return EXT2_ERROR;

	file->entry.name[0] = DIR_ENTRY_FREE; // 해당 엔트리의 name에 삭제된 엔트리라고 저장
	set_inode_onto_inode_table(file->fs, file->entry.inode, inodeBuffer); // 디스크의 해당 엔트리의 위치에 변경된 정보 저장
	// 또 뭐해야하지..?

	return EXT2_SUCCESS;
}
  
  
  
int ext2_read(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, unsigned long offset, unsigned long length, char* buffer)
{
	//함수 선언부, 인자 받는 부분 수정 필요시 수정해야 될 수도. 일단 fs_read와 맞춰놓음
}

void ext2_umount(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs)
{
	//함수 선언부, 인자 받는 부분 수정 필요시 수정해야 될 수도. 일단 fs_umount와 맞춰놓음
}
  
  
int ext2_df(EXT2_FILESYSTEM* fs, unsigned int total, unsigned int used) 
{
	/*
	EXT2_FILESYSTEM의 EXT2_SUPER_BLOCK 필드에서 block_count와 block_count-free_block_count를 
	각각 total과 used에 저장(블록사이즈=섹터사이즈라 그냥 넣어도 될 듯)
	*/
}

int ext2_rmdir(EXT2_NODE* dir)
{
	/*
	하위 엔트리 존재 여부 확인하여 에러 처리, 디렉토리인지 확인하여 에러처리(아이노드 등 확인해야 될 듯)
 	엔트리의 이름 DIR_ENTRY_FREE로 빈 엔트리로 표시
 	메타 데이터 수정(free_block_count 등)
	*/
}