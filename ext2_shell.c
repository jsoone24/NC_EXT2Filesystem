#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "ext2_shell.h"
typedef struct {
	char * address;
}DISK_MEMORY;
void printFromP2P(char * start, char * end);

int fs_dumpDataSector(DISK_OPERATIONS* disk, int usedSector) // 섹터 단위로 데이터 출력
{
	char * start;
	char * end;

	start = ((DISK_MEMORY *)disk->pdata)->address + usedSector * disk->bytesPerSector; // 디스크에서 사용된 섹터의 시작주소부터
	end = start + disk->bytesPerSector; // 마지막주소까지
	printFromP2P(start, end); // 출력
	printf("\n\n");

	return EXT2_SUCCESS;
}

void printFromP2P(char * start, char * end)
{
	int start_int, end_int;
	start_int = (int)start;
	end_int = (int)end;

	printf("start address : %#x , end address : %#x\n\n", start, end - 1);
	start = (char *)(start_int &= ~(0xf));
	end = (char *)(end_int |= 0xf);

	// start부터 end까지 데이터 출력
	while (start <= end)
	{
		if ((start_int & 0xf) == 0)
			fprintf(stdout, "\n%#08x   ", start);

		fprintf(stdout, "%02X  ", *(unsigned char *)start);
		start++;
		start_int++;
	}
	printf("\n\n");

}
void fs_dumpDataPart(DISK_OPERATIONS * disk, SHELL_FS_OPERATIONS * fsOprs, const SHELL_ENTRY * parent, SHELL_ENTRY * entry, const char * name)
{
	EXT2_NODE EXT2Parent;
	EXT2_NODE EXT2Entry;
	INODE node;
	int result;
	char * start, *end;

	shell_entry_to_ext2_entry(parent, &EXT2Parent); // EXT2_ENTRY로 변환
	if (result = ext2_lookup(&EXT2Parent, name, &EXT2Entry)) return result; // (ext2.c) 해당 이름을 가지는 엔트리의 위치를 찾아서 EXT2Entry에 저장

	get_inode(EXT2Entry.fs, EXT2Entry.entry.inode, &node); // (ext2.c 구현X) inode number를 가진 파일의 INODE의 주소를 node에 저장

	start = ((DISK_MEMORY *)disk->pdata)->address + (1 + node.block[0]) * disk->bytesPerSector; // ???
	end = start + disk->bytesPerSector; // 섹터 단위
	printFromP2P(start, end);
}
void fs_dumpfileinode(DISK_OPERATIONS * disk, SHELL_FS_OPERATIONS * fsOprs, const SHELL_ENTRY * parent, SHELL_ENTRY * entry, const char * name)
{
	EXT2_NODE EXT2Parent;
	EXT2_NODE EXT2Entry;
	INODE node;
	int result, inode, i;


	shell_entry_to_ext2_entry(parent, &EXT2Parent); // EXT2_ENTRY로 변환
	if (result = ext2_lookup(&EXT2Parent, name, &EXT2Entry)) return result; // (ext2.c) 해당 이름을 가지는 엔트리의 위치를 찾아서 EXT2Entry에 저장

	get_inode(EXT2Entry.fs, EXT2Entry.entry.inode, &node); // (ext2.c 구현X) inode number를 가진 파일의 INODE의 주소를 node에 저장
	inode = EXT2Entry.entry.inode;

	printf("inode number : %d\n", inode); // 검색한 inode number 출력
	for (i = 0; i < EXT2_N_BLOCKS; i++) {
		printf("iblock[%.2d] : %u\n", i, node.block[i]); // 해당 inode의 데이터 출력
	}

}
void fs_dumpDataBlockByNum(DISK_OPERATIONS * disk, SHELL_FS_OPERATIONS * fsOprs, const SHELL_ENTRY * parent, SHELL_ENTRY * entry, int num)
{
	char * start, *end;

	start = ((DISK_MEMORY *)disk->pdata)->address + (1 + num) * disk->bytesPerSector; // ???
	end = start + disk->bytesPerSector; // 섹터 단위
	printFromP2P(start, end);

}
void printf_by_sel(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, const char* name, int sel, int num)
{
	switch (sel) {
	case 1: // super block 섹터단위 출력
		fs_dumpDataSector(disk, 1);
		break;
	case 2: // group descriptor 섹터단위 출력
		fs_dumpDataSector(disk, 2);
		break;
	case 3: // block bitmap 섹터단위 출력
		fs_dumpDataSector(disk, 3);
		break;
	case 4: // inode bitmap 섹터단위 출력
		fs_dumpDataSector(disk, 4);
		break;
	case 5: // inode table 섹터단위 출력
		fs_dumpDataSector(disk, 5);
		fs_dumpDataSector(disk, 6);
		break;
	case 6: // 해당 이름을 가진 data block을 섹터단위로 출력
		fs_dumpDataPart(disk, fsOprs, parent, entry, name);
		break;
	case 7: // 해당 이름을 가진 파일의 inode 출력
		fs_dumpfileinode(disk, fsOprs, parent, entry, name);
		break;
	case 8: // 해당 num을 가진 data block을 섹터단위로 출력
		fs_dumpDataBlockByNum(disk, fsOprs, parent, entry, num);
		break;
	}
}

// 파티션을 해당 파일시스템으로 포맷
int fs_format(DISK_OPERATIONS* disk, void* param)
{
	printf("formatting as a %s\n", (char *)param);
	ext2_format(disk); // (ext2.c)

	return  1;
}

static SHELL_FILE_OPERATIONS g_file =
{
	fs_create,
	NULL,
	NULL,
	fs_write
};

static SHELL_FS_OPERATIONS   g_fsOprs =
{
	fs_read_dir,
	fs_stat,
	fs_mkdir,
	fs_rmdir,
	fs_lookup,
	&g_file,
	NULL
};

int fs_mount(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, SHELL_ENTRY* root)
{
	EXT2_FILESYSTEM* fs;
	EXT2_NODE ext2_entry;
	int result;

	*fsOprs = g_fsOprs; // g_fsOprs구조체에 EXT2오퍼레이션함수 등록

	// EXT2_FILESYSTEM 구조체를 쉘 레벨 구조체에 연결 후 초기화
	fsOprs->pdata = malloc(sizeof(EXT2_FILESYSTEM));
	fs = FSOPRS_TO_EXT2FS(fsOprs); // fsOprs->pdata에 EXT2_FILESYSTEM 구조체 연결
	ZeroMemory(fs, sizeof(EXT2_FILESYSTEM)); // 0으로 초기화
	fs->disk = disk; // 디스크 등록

	result = ext2_read_superblock(fs, &ext2_entry); // (ext2.c)

	if (result == EXT2_SUCCESS) // ext2_read_superblock 수행 성공 시 설정된 값들 출력
	{
		printf("number of groups         : %d\n", NUMBER_OF_GROUPS);
		printf("blocks per group         : %d\n", fs->sb.block_per_group);
		printf("bytes per block          : %d\n", disk->bytesPerSector);
		printf("free block count	 : %d\n", fs->sb.free_block_count);
		printf("free inode count	 : %d\n", fs->sb.free_inode_count);
		printf("first non reserved inode : %d\n", fs->sb.first_non_reserved_inode);
		printf("inode structure size	 : %d\n", fs->sb.inode_structure_size);
		printf("first data block number  : %d\n", fs->sb.first_data_block_each_group);
		printf("\n----------------------------------------------\n");
	}

	printf("%s", ext2_entry.entry.name);
	ext2_entry_to_shell_entry(fs, &ext2_entry, root); // SHELL_ENTRY로 변환

	return result;
}

// (구현X) mount 해제
void fs_umount(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs)
{
	return;
}

static SHELL_FILESYSTEM g_fat =
{
	"EXT2",
	fs_mount,
	fs_umount,
	fs_format
};

// 엔트리 리스트에 새로운 엔트리 추가
int adder(EXT2_FILESYSTEM* fs, void* list, EXT2_NODE* entry)
{
	SHELL_ENTRY_LIST*   entryList = (SHELL_ENTRY_LIST*)list;
	SHELL_ENTRY         newEntry;

	ext2_entry_to_shell_entry(fs, entry, &newEntry); // SHELL_ENTRY로 변환

	add_entry_list(entryList, &newEntry); // ENTRY_LIST에 추가 (entrylist.c)

	return EXT2_SUCCESS;
}

// 파일 쓰기
int fs_write(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, unsigned long offset, unsigned long length, const char* buffer)
{
	EXT2_NODE EXT2Entry;

	shell_entry_to_ext2_entry(entry, &EXT2Entry); // EXT2_ENTRY로 변환

	return ext2_write(&EXT2Entry, offset, length, buffer); // (ext2.c) offset 위치부터 length만큼 buffer의 내용을 씀
}

// mount, umount, format 함수 포인터 지정
void shell_register_filesystem(SHELL_FILESYSTEM* fs)
{
	*fs = g_fat;
}

// 파일 생성
int	fs_create(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name, SHELL_ENTRY* retEntry)
{
	EXT2_NODE	EXT2Parent;
	EXT2_NODE	EXT2Entry;
	int				result;

	shell_entry_to_ext2_entry(parent, &EXT2Parent); // EXT2_ENTRY로 변환

	result = ext2_create(&EXT2Parent, name, &EXT2Entry); // (ext2.c) 파일명을 EXT2_ENTRY 형식에 맞게 수정한 뒤 해당 이름을 가지는 엔트리가 존재하지 않으면 부모 디렉터리에 추가해줌

	ext2_entry_to_shell_entry(EXT2Parent.fs, &EXT2Entry, retEntry); // SHELL_ENTRY로 변환

	return result;
}


// 파일 삭제
int fs_remove(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name)
{
	EXT2_NODE	EXT2Parent;
	EXT2_NODE	file;
	int			result;

	shell_entry_to_ext2_entry(parent, &EXT2Parent); // EXT2_ENTRY로 변환 후
	if (result = ext2_lookup(&EXT2Parent, name, &file)) // 현재 디렉터리에서 해당 파일을 찾음
		return result;

	return ext2_remove(&file); // 찾은 파일을 삭제
}

/*
	precondition : shell_entry는 적절한 값으로 초기화 되어있는 상태이다.
	postcondition : ext2_entry가 가리키는 공간에는 shell_entry에 대응하는 ext2_entry가 저장되어 있다.
*/
int shell_entry_to_ext2_entry(const SHELL_ENTRY* shell_entry, EXT2_NODE* ext2_entry) // SHELL_ENTRY를 EXT2_ENTRY로 변환
{
	EXT2_NODE* entry = (EXT2_NODE*)shell_entry->pdata; // 엔트리 보관 버퍼

	*ext2_entry = *entry; // 데이터 저장

	return EXT2_SUCCESS;
}

/*
	postcondition : shell_entry는 shell level directory entry를 갖고 동시에 ext2 directory entry를
	pdata 버퍼에 보관한다. 즉, shell level에서는 언제든 양쪽 entry에 접근이 가능해진다.
*/
int ext2_entry_to_shell_entry(EXT2_FILESYSTEM* fs, const EXT2_NODE* ext2_entry, SHELL_ENTRY* shell_entry) // EXT2_ENTRY를 SHELL_ENTRY로 변환
{
	EXT2_NODE* entry = (EXT2_NODE*)shell_entry->pdata; // 엔트리 보관 버퍼
	INODE inodeBuffer;
	BYTE* str = "/";

	ZeroMemory(shell_entry, sizeof(SHELL_ENTRY)); // 데이터를 저장할 영역을 0으로 초기화

	int inode = ext2_entry->entry.inode; // inode number
	int result = get_inode(fs, inode, &inodeBuffer); // (ext2.c 구현X) inode number를 가진 파일의 INODE주소를 inodeBuffer에 저장 

	if (ext2_entry->entry.name[0] != '.' && inode == 2); // 현재 디렉터리가 아닌 경우
	else {
		str = shell_entry->name;
		str = my_strncpy(str, ext2_entry->entry.name, 8); // 이름 설정
		if (ext2_entry->entry.name[8] != 0x20) // 확장자가 있는 경우
		{
			str = my_strncpy(str, ".", 1);
			str = my_strncpy(str, &ext2_entry->entry.name[8], 3); // 확장자 설정
		}
	}
	if (FILE_TYPE_DIR & inodeBuffer.mode) // 디렉터리인 경우
		shell_entry->isDirectory = 1; // 디렉터리라고 설정
	else // 파일인 경우
		shell_entry->isDirectory = 0;

	shell_entry->permition = 0x01FF & inodeBuffer.mode; // 파일의 접근 제어 정보 저장

	shell_entry->size = inodeBuffer.size; // 파일의 사이즈 저장

	*entry = *ext2_entry; // 데이터 저장

	return EXT2_SUCCESS;
}

// 해당 이름을 가지는 엔트리의 위치를 찾음
int fs_lookup(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, const char* name)
{
	EXT2_NODE	EXT2Parent;
	EXT2_NODE	EXT2Entry;
	int				result;

	shell_entry_to_ext2_entry(parent, &EXT2Parent); // EXT2_ENTRY로 변환

	if (result = ext2_lookup(&EXT2Parent, name, &EXT2Entry))return result; // (ext2.c) 해당 이름을 가지는 엔트리의 위치를 찾아서 EXT2Entry에 저장

	ext2_entry_to_shell_entry(EXT2Parent.fs, &EXT2Entry, entry); // SHELL_ENTRY로 변환

	return result;
}

// 디렉터리의 엔트리들을 리스트에 담음
int fs_read_dir(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY_LIST* list)
{
	EXT2_NODE   entry;

	if (list->count)
		release_entry_list(list); // 리스트 삭제 (entrylist.c)

	shell_entry_to_ext2_entry(parent, &entry); // EXT2_ENTRY로 변환
	ext2_read_dir(&entry, adder, list); // 디렉터리의 엔트리들을 리스트에 담음

	return EXT2_SUCCESS;
}

// 문자열 비교
int my_strnicmp(const char* str1, const char* str2, int length)
{
	char   c1, c2;

	// length만큼 둘 다 공백이 아닌동안 대문자로 바꿔서 비교 후 같으면 0을 리턴
	while (((*str1 && *str1 != 0x20) || (*str2 && *str2 != 0x20)) && length-- > 0)
	{
		c1 = toupper(*str1);
		c2 = toupper(*str2);

		if (c1 > c2)
			return -1;
		else if (c1 < c2)
			return 1;

		str1++;
		str2++;
	}

	return 0;
}

// 현재 디렉터리에 해당 이름의 엔트리가 존재하는지 검사
int is_exist(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name)
{
	SHELL_ENTRY_LIST      list;
	SHELL_ENTRY_LIST_ITEM*   current;

	init_entry_list(&list); // ENTRY_LIST를 초기화 (entrylist.c)

	fs_read_dir(disk, fsOprs, parent, &list); // 디렉터리의 엔트리들을 리스트에 담음
	current = list.first; // 리스트의 첫 엔트리부터

	while (current) // 마지막 엔트리까지 같은 이름이 있는지 검사
	{
		if (my_strnicmp((char*)current->entry.name, name, 12) == 0) // 있으면
		{
			release_entry_list(&list); // ENTRY_LIST 삭제 후 에러 리턴 (entrylist.c)
			return EXT2_ERROR;
		}

		current = current->next; // 다음 엔트리
	}

	release_entry_list(&list); // ENTRY_LIST 삭제 후 성공 리턴
	return EXT2_SUCCESS;
}

// 디렉터리 생성
int fs_mkdir(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, const char* name, SHELL_ENTRY* retEntry)
{
	EXT2_FILESYSTEM* ext2;
	EXT2_NODE      EXT2_Parent;
	EXT2_NODE      EXT2_Entry;
	int               result;

	ext2 = (EXT2_FILESYSTEM*)fsOprs->pdata; // EXT2_FILESYSTEM 데이터

	if (is_exist(disk, fsOprs, parent, name)) // 현재 디렉터리에 해당 이름의 엔트리가 있는지 검사
		return EXT2_ERROR;

	shell_entry_to_ext2_entry(parent, &EXT2_Parent); // EXT2_ENTRY로 변환

	result = ext2_mkdir(&EXT2_Parent, name, &EXT2_Entry); // (ext2.c) 디렉터리를 생성하고 '.'와 '..'을 추가

	ext2_entry_to_shell_entry(ext2, &EXT2_Entry, retEntry); // SHELL_ENTRY로 변환

	return result;
}



int fs_stat(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, unsigned int total, unsigned int used)
{
	/*
	shell.c의 shell_cmd_df에서는 free sectors, used sectors 출력
 	shell.c에서는 위의 인자들을 넣어 호출하고 있는데 첫 번째 인자인 DISK_OPERATIONS는 빼도 될 듯 -> 쓸 데가 없음
 	두 번째 인자로 받은 SHELL_FS_OPERATIONS의 pdata는 EXT2_FILESYSTEM을 가르키고 있음    
	-> ext2_df를 호출하면서 인자로는 이 EXT2_FILESYSTEM과 total, used를 넘겨줌
	*/
}

int fs_rmdir(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, SHELL_ENTRY* parent, const char* name)
{
	/*
	이 함수도 DISK_OPERATIONS를 왜 넘겨주는지 모르겠음(사용 안하는 것 같음)
 	인자로 받은 parent를 shell_entry_to_ext2_entry 함수로 ext_entry로 변환, 
	ext2_lookup으로 해당 엔트리 찾은 후 그 엔트리의 주소를 ext2_rmdir에 인자로 전달
	*/
}
