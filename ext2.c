typedef struct
{
	char*	address;
} DISK_MEMORY;

#include "ext2.h"
#define MIN( a, b )					( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX( a, b )					( ( a ) > ( b ) ? ( a ) : ( b ) )

int ext2_write(EXT2_NODE* file, unsigned long offset, unsigned long length, const char* buffer)
{
	BYTE	sector[MAX_SECTOR_SIZE];
	DWORD	currentOffset, currentBlock, blockSeq = 0;
	DWORD	blockNumber, sectorNumber, sectorOffset;
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
	while (offset > blockSize)
	{

		currentBlock = get_data_block_at_inode(file->fs, node, ++i);
		blockSize += blockSize;
		blockSeq++;
	}

	while (currentOffset < readEnd)
	{
		DWORD	copyLength;


		blockNumber = currentOffset / MAX_BLOCK_SIZE;
		if (currentBlock == 0)
		{
			if (expand_block(file->fs, file->entry.inode) == EXT2_ERROR)
				return EXT2_ERROR;
			process_meta_data_for_block_used(file->fs, file->entry.inode);
			get_inode(file->fs, file->entry.inode, &node);
			currentBlock = node.block[0];
		}

		if (blockSeq != blockNumber)
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

		if (copyLength != MAX_SECTOR_SIZE)
		{
			if (data_read(file->fs, 0, currentBlock, sector))
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
}

int insert_entry(UINT32 inode_num, EXT2_NODE * retEntry, int fileType)
{
}

UINT32 get_available_data_block(EXT2_FILESYSTEM * fs, UINT32 inode_num)	//사용가능한 데이터 블록을 가져오는 함수?
{
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

int lookup_entry(EXT2_FILESYSTEM* fs, const int inode, const char* name, EXT2_NODE* retEntry)	//같은 이름의 디렉터리 엔트리가 있는지 찾는 함수
{
}

int find_entry_at_sector(const BYTE* sector, const BYTE* formattedName, UINT32 begin, UINT32 last, UINT32* number)	//
{
}

int find_entry_on_root(EXT2_FILESYSTEM* fs, INODE inode, char* formattedName, EXT2_NODE* ret)	//
{
}

int find_entry_on_data(EXT2_FILESYSTEM* fs, INODE first, const BYTE* formattedName, EXT2_NODE* ret)	//
{
}

int get_inode(EXT2_FILESYSTEM* fs, const UINT32 inode, INODE *inodeBuffer)	//
{
}

int read_root_sector(EXT2_FILESYSTEM* fs, BYTE* sector)	//루트 디렉터리에 관한 정보를 읽어옴. fs로 넘겨주면, sector에 담아줌
{
	UINT32 inode = 2;	//루트 디렉터리는 아이노드 2번
	INODE inodeBuffer;
	SECTOR rootBlock;
	get_inode(fs, inode, &inodeBuffer);
	rootBlock = get_data_block_at_inode(fs, inodeBuffer, 1);	//아이노드 정보에서 가장 처음 직접 데이터 블록을 가져옴

	return data_read(fs, 0, rootBlock, sector);	//fs 로부터 데이터를 읽어서 sector에 저장.
}

int get_data_block_at_inode(EXT2_FILESYSTEM *fs, INODE inode, UINT32 number)	//inode : 어떤 파일의 아이노드, number : inode 구조체의 block필드에서 몇번째 데이터 블록을 불러올지 결정하는 변수 인듯
{
	//만약 n이 0~11이 들어오면 간접 블록 데이터 블록 받아서 리턴
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

	meta_read(fs, 0, SUPER_BLOCK, sector);	//곰곰이 생각해봤는데 슈퍼블록을 간단하게 계산하기 위해서 그냥 가장 첫 그룹의 블럭을 참고하는 것 같음
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

int set_inode_onto_inode_table(EXT2_FILESYSTEM *fs, const UINT32 which_inode_num_to_write, INODE * inode_to_write)	//아이노느들 아이노드 테이블에 저장하는 과정으로 생각됨
{
	//호출하는 쪽에서 get_free_inode_number을 통해서 비어있는 아이노드 번호를 알아내서 which_inode_num_to_write로 넘겨줄것으로 예상
	//호출하는 쪽에서 새로 생성되는 파일에 대한 아이노드 구조체를 새로 만듬. 그리고 그 구조체를 인자로 넘겨줄것으로 예상됨
	//이 함수에서는 그러면 아이노드 테이블에 아이노드 정보를 기록하고 성공여부를 리턴할 것으로 예상됨.
}

int ext2_read_dir(EXT2_NODE* dir, EXT2_NODE_ADD adder, void* list)
{
	BYTE   sector[MAX_SECTOR_SIZE];
	INODE* inodeBuffer;
	UINT32 inode;
	int i, result, num;

	inodeBuffer = (INODE*)malloc(sizeof(INODE));

	ZeroMemory(sector, MAX_SECTOR_SIZE);
	ZeroMemory(inodeBuffer, sizeof(INODE));

	result = get_inode(dir->fs, dir->entry.inode, inodeBuffer);
	if (result == EXT2_ERROR)
		return EXT2_ERROR;

	for (i = 0; i < inodeBuffer->blocks; ++i)
	{
		num = get_data_block_at_inode(dir->fs, *inodeBuffer, i + 1);
		data_read(dir->fs, 0, num, sector);

		if (dir->entry.inode == 2)
			read_dir_from_sector(dir->fs, sector + 32, adder, list);
		else
			read_dir_from_sector(dir->fs, sector, adder, list);
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

int ext2_mkdir(const EXT2_NODE* parent, const char* entryName, EXT2_NODE* retEntry)
{
	EXT2_NODE      dotNode, dotdotNode;
	DWORD         firstCluster;
	BYTE         name[MAX_NAME_LENGTH];
	int            result;
	int            i;

	strcpy((char*)name, entryName);

	if (format_name(parent->fs, (char*)name))
		return EXT2_ERROR;

	ZeroMemory(retEntry, sizeof(EXT2_NODE));
	memcpy(retEntry->entry.name, name, MAX_ENTRY_NAME_LENGTH);
	retEntry->entry.name_len = strlen((char*)retEntry->entry.name);
	retEntry->fs = parent->fs;

	result = insert_entry(parent->entry.inode, retEntry, FILE_TYPE_DIR);
	if (result == EXT2_ERROR)
		return EXT2_ERROR;

	expand_block(parent->fs, retEntry->entry.inode);

	ZeroMemory(&dotNode, sizeof(EXT2_NODE));
	memset(dotNode.entry.name, 0x20, 11);
	dotNode.entry.name[0] = '.';
	dotNode.fs = retEntry->fs;
	dotNode.entry.inode = retEntry->entry.inode;
	insert_entry(retEntry->entry.inode, &dotNode, FILE_TYPE_DIR);

	ZeroMemory(&dotdotNode, sizeof(EXT2_NODE));
	memset(dotdotNode.entry.name, 0x20, 11);
	dotdotNode.entry.name[0] = '.';
	dotdotNode.entry.name[1] = '.';
	dotdotNode.entry.inode = parent->entry.inode;
	dotdotNode.fs = retEntry->fs;
	insert_entry(retEntry->entry.inode, &dotdotNode, FILE_TYPE_DIR);

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

	return fs->disk->read_sector(fs->disk, real_index, sector);
}
int data_write(EXT2_FILESYSTEM * fs, SECTOR group, SECTOR block, BYTE* sector)
{
	const SECTOR BOOT_BLOCK = 1;
	SECTOR real_index = BOOT_BLOCK + group * fs->sb.block_per_group + block;

	return fs->disk->write_sector(fs->disk, real_index, sector);
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

UINT32 expand_block(EXT2_FILESYSTEM * fs, UINT32 inode_num)	//???????
{
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
void process_meta_data_for_block_used(EXT2_FILESYSTEM * fs, UINT32 inode_num)	//????????????
{
}



int ext2_read(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, unsigned long offset, unsigned long length, char* buffer)
{
	//함수 선언부, 인자 받는 부분 수정 필요시 수정해야 될 수도. 일단 fs_read와 맞춰놓음
}

void ext2_umount(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs)
{
	//함수 선언부, 인자 받는 부분 수정 필요시 수정해야 될 수도. 일단 fs_umount와 맞춰놓음
}