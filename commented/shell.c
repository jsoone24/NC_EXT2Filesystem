#pragma warning(disable : 4995)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "shell.h"
#include "disksim.h"

#define		SECTOR					DWORD
#define		BLOCK_SIZE				1024
#define		SECTOR_SIZE				1024
#define		NUMBER_OF_SECTORS		( 4096 + 1 )

#define COND_MOUNT				0x01
#define COND_UMOUNT				0x02

// 명령어(name)에 따라 실행 조건(conditions)을 확인한 후 핸들러 호출(handler)
typedef struct
{
	char*	name;
	int		(*handler)(int, char**);
	char	conditions;
} COMMAND;

extern void shell_register_filesystem(SHELL_FILESYSTEM*);
extern void printf_by_sel(DISK_OPERATIONS* disk, SHELL_FS_OPERATIONS* fsOprs, const SHELL_ENTRY* parent, SHELL_ENTRY* entry, const char* name, int sel, int num);

void do_shell(void);
void unknown_command(void);
int seperate_string(char* buf, char* ptrs[]);

// 명령어처리
int shell_cmd_format(int argc, char* argv[]);
int shell_cmd_mount(int argc, char* argv[]);
int shell_cmd_touch(int argc, char* argv[]);
int shell_cmd_cd(int argc, char* argv[]);
int shell_cmd_ls(int argc, char* argv[]);
int shell_cmd_mkdir(int argc, char* argv[]);
int shell_cmd_dumpsuperblock(int argc, char * argv[]);
int shell_cmd_dumpgd(int argc, char * argv[]);
int shell_cmd_dumpblockbitmap(int argc, char * argv[]);
int shell_cmd_dumpinodebitmap(int argc, char * argv[]);
int shell_cmd_dumpinodetable(int argc, char * argv[]);
int shell_cmd_dumpdatablockbyname(int argc, char * agrv[]);
int shell_cmd_dumpfileinode(int argc, char * argv[]);
int shell_cmd_fill(int argc, char* argv[]);
int shell_cmd_dumpdatablockbynum(int argc, char * argv[]);

static COMMAND g_commands[] =
{
	{ "cd",		shell_cmd_cd,		COND_MOUNT	},
	{ "mount",	shell_cmd_mount,	COND_UMOUNT	},
	{ "touch",	shell_cmd_touch,	COND_MOUNT	},
	{ "fill",	shell_cmd_fill,		COND_MOUNT	},
	{ "ls",		shell_cmd_ls,		COND_MOUNT	},
	{ "format",	shell_cmd_format,	COND_UMOUNT	},
	{ "mkdir",	shell_cmd_mkdir,	COND_MOUNT	},
	{ "dumpdatablockbynum",	shell_cmd_dumpdatablockbynum, COND_MOUNT },
	{ "dumpsuperblock" , shell_cmd_dumpsuperblock, COND_MOUNT  },
	{ "dumpgd" , shell_cmd_dumpgd , COND_MOUNT },
	{ "dumpblockbitmap" , shell_cmd_dumpblockbitmap, COND_MOUNT  },
	{ "dumpinodebitmap" , shell_cmd_dumpinodebitmap, COND_MOUNT  },
	{ "dumpinodetable" , shell_cmd_dumpinodetable, COND_MOUNT  },
	{ "dumpdatablockbyname", shell_cmd_dumpdatablockbyname, COND_MOUNT  },
	{ "dumpfileinode", shell_cmd_dumpfileinode, COND_MOUNT  }
};

static SHELL_FILESYSTEM		g_fs;
static SHELL_FS_OPERATIONS	g_fsOprs;
static SHELL_ENTRY			g_rootDir;
static SHELL_ENTRY			g_currentDir;
static DISK_OPERATIONS		g_disk;


int g_commandsCount = sizeof(g_commands) / sizeof(COMMAND); // 명령어 개수
int g_isMounted; // 시작할 때는 마운트되지 않은 상태

int main(int argc, char* argv[])
{
	if (disksim_init(NUMBER_OF_SECTORS, SECTOR_SIZE, &g_disk) < 0) // 디스크 초기화 (disksim.c)
	{
		printf("disk simulator initialization has been failed\n");
		return -1;
	}

	shell_register_filesystem(&g_fs); // mount, umount, format 함수에 연결 (ext2_shell.c)

	do_shell(); // 명령어를 입력받는 함수

	return 0;
}

int check_conditions(int conditions) // 명령어 실행조건 검사
{									 // 조건이 맞으면 0, 다르면 -1 리턴
	if (conditions & COND_MOUNT && !g_isMounted)
	{
		printf("file system is not mounted\n");
		return -1;
	}

	if (conditions & COND_UMOUNT && g_isMounted)
	{
		printf("file system is already mounted\n");
		return -1;
	}

	return 0;
}

static int x = 0; // mount, format 명령어 실행할 때마다 +1

void do_shell(void) // 명령어 입력받음
{
	char buf[1000];
	char command[100];
	char* argv[100];
	int argc;
	int i, j = 0;

	printf("%s File system shell\n", g_fs.name);

	while (-1)
	{
		printf("ÇÐ¹ø : [/%s]# ", g_currentDir.name);

		fgets(buf, 1000, stdin); // 표준 입력을 받아 최대 1000 byte만큼 버퍼에 입력값 저장
		argc = seperate_string(buf, argv); // 인자의 개수 저장

		if (argc == 0) // 입력이 없을 때
			continue;

		for (i = 0; i < g_commandsCount; i++) // 해당 명령어를 배열에서 찾음
		{
			if (strcmp(g_commands[i].name, argv[0]) == 0) // 명령어가 일치하면
			{
				if (check_conditions(g_commands[i].conditions) == 0) // 조건을 검사한 후
					g_commands[i].handler(argc, argv); // 해당 명령어 실행 함수 호출

				break;
			}
		}

		if (argc != 0 && i == g_commandsCount) // 배열 내에 해당 명령어가 없다면
			unknown_command(); // 사용가능한 명령어 리스트 출력
	}
}

void unknown_command(void) // 사용가능한 명령어 리스트 출력
{
	int i;

	printf(" * ");
	for (i = 0; i < g_commandsCount; i++)
	{
		if (i < g_commandsCount - 1)
			printf("%s, ", g_commands[i].name);
		else
			printf("%s", g_commands[i].name);
	}
	printf("\n");
}

int seperate_string(char* buf, char* ptrs[]) // 입력값을 단어 단위로 나눠 배열에 저장
{
	char prev = 0;
	int count = 0;

	while (*buf) // 입력값이 NULL이 아닐 때까지
	{
		if (isspace(*buf)) // 공백이면 단어가 끝났다는 의미로 NULL 저장
			*buf = 0;
		else if (prev == 0) // 한 단어가 끝났다면 배열의 다음 인덱스에 값 저장
			ptrs[count++] = buf;

		prev = *buf++; // 단어의 시작주소 저장
	}

	return count; // 인자의 개수를 리턴
}

int shell_cmd_dumpsuperblock(int argc, char * argv[]) // super block 섹터단위 출력
{
	SHELL_ENTRY entry;
	printf_by_sel(&g_disk, &g_fsOprs, &g_currentDir, &entry, argv[1], 1, 0);
	return 0;
}
int shell_cmd_dumpgd(int argc, char * argv[]) // group descriptor 섹터단위 출력
{
	SHELL_ENTRY entry;
	printf_by_sel(&g_disk, &g_fsOprs, &g_currentDir, &entry, argv[1], 2, 0);
	return 0;
}
int shell_cmd_dumpblockbitmap(int argc, char * argv[]) // block bitmap 섹터단위 출력
{
	SHELL_ENTRY entry;
	printf_by_sel(&g_disk, &g_fsOprs, &g_currentDir, &entry, argv[1], 3, 0);
	return 0;
}
int shell_cmd_dumpinodebitmap(int argc, char * argv[]) // inode bitmap 섹터단위 출력
{
	SHELL_ENTRY entry;
	printf_by_sel(&g_disk, &g_fsOprs, &g_currentDir, &entry, argv[1], 4, 0);
	return 0;
}
int shell_cmd_dumpinodetable(int argc, char * argv[]) // inode table 섹터단위 출력
{
	SHELL_ENTRY entry;
	printf_by_sel(&g_disk, &g_fsOprs, &g_currentDir, &entry, argv[1], 5, 0);
	return 0;
}
int shell_cmd_dumpdatablockbyname(int argc, char * argv[]) // 해당 이름을 가진 data block을 섹터단위로 출력
{
	SHELL_ENTRY entry;
	printf_by_sel(&g_disk, &g_fsOprs, &g_currentDir, &entry, argv[1], 6, 0);
	return 0;
}
int shell_cmd_dumpfileinode(int argc, char * argv[]) // 해당 이름을 가진 파일의 inode 출력
{
	SHELL_ENTRY entry;
	printf_by_sel(&g_disk, &g_fsOprs, &g_currentDir, &entry, argv[1], 7, 0);
	return 0;
}
int shell_cmd_dumpdatablockbynum(int argc, char * argv[]) // 해당 num을 가진 data block을 섹터단위로 출력
{
	SHELL_ENTRY entry;
	printf_by_sel(&g_disk, &g_fsOprs, &g_currentDir, &entry, argv[1], 8, atoi(argv[1]));
	return 0;
}

/******************************************************************************/
/* Shell commands...                                                          */
/******************************************************************************/
int shell_cmd_cd(int argc, char* argv[]) // 현재 디렉터리 이동
{
	SHELL_ENTRY	newEntry;
	int			result, i;
	static SHELL_ENTRY	path[256];
	static int			pathTop = 0;

	path[0] = g_rootDir; // 시작 디렉터리는 루트 디렉터리

	if (argc > 2) // 인자가 너무 많으면
	{
		printf("usage : %s [directory]\n", argv[0]);
		return 0;
	}

	if (argc == 1) // "cd"라고 입력한 경우
		pathTop = 0; // 루트 디렉터리 선택
	else // 이동할 디렉터리를 입력한 경우
	{
		if (strcmp(argv[1], ".") == 0) // "cd ."라고 입력한 경우
			return 0; // 현재 디렉터리이므로 종료
		else if (strcmp(argv[1], "..") == 0 && pathTop > 0) // "cd .."라고 입력한 경우
			pathTop--; // 상위 디렉터리 선택
		else // "cd [directory]"라고 입력한 경우
		{
			result = g_fsOprs.lookup(&g_disk, &g_fsOprs, &g_currentDir, &newEntry, argv[1]); // 해당 이름을 가진 엔트리가 있는지 검색 (ext2_shell.c -> fs_lookup)

			if (result) // 없으면 
			{
				printf("directory not found\n");
				return -1;
			}
			else if (!newEntry.isDirectory) // 있으면 해당 엔트리가 디렉터리인지 확인
			{
				printf("%s is not a directory\n", argv[1]); // 파일이면
				return -1;
			}
			path[++pathTop] = newEntry; // 디렉터리이면 해당 엔트리 주소 저장

		}
	}

	g_currentDir = path[pathTop]; // 선택된 디렉터리로 이동

	return 0;
}

int shell_cmd_exit(int argc, char* argv[]) // 종료
{
	disksim_uninit(&g_disk); // 디스크의 데이터를 삭제하고 (disksim.c)
	_exit(0); // 정상종료

	return 0;
}

int shell_cmd_mount(int argc, char* argv[])
{
	int result;
	x++;
	if (g_fs.mount == NULL) // mount 함수가 지정되지 않았을 때
	{
		printf("The mount functions is NULL\n");
		return 0;
	}

	result = g_fs.mount(&g_disk, &g_fsOprs, &g_rootDir); // (ext2_shell.c -> fs_mount)
	g_currentDir = g_rootDir; // mount 이후 현재 디렉터리를 루트 디렉터리로 설정

	if (result < 0) // mount 실패
	{
		printf("%s file system mounting has been failed\n", g_fs.name);
		return -1;
	}
	else // mount 성공
	{
		printf(" : %s file system has been mounted successfully\n", g_fs.name);
		g_isMounted = 1;
	}

	return 0;
}

int shell_cmd_umount(int argc, char* argv[]) // mount 해제
{
	g_isMounted = 0;

	if (g_fs.umount == NULL) // umount 함수가 지정되지 않았을 때
		return 0;

	g_fs.umount(&g_disk, &g_fsOprs); // (구현X) 클러스터 리스트 삭제 (ext2_shell.c -> fs_umount)
	return 0;
}

int shell_cmd_touch(int argc, char* argv[]) // 파일 생성
{
	SHELL_ENTRY	entry;
	int			result;

	if (argc < 2) // 인자가 너무 적으면
	{
		printf("usage : touch [files...]\n");
		return 0;
	}

	// 파일명을 EXT2_ENTRY 형식에 맞게 수정한 뒤 해당 이름을 가지는 엔트리가 존재하지 않으면 부모 디렉터리에 추가해줌
	result = g_fsOprs.fileOprs->create(&g_disk, &g_fsOprs, &g_currentDir, argv[1], &entry); // (ext2_shell.c -> fs_create)

	if (result) // 생성 실패 시
	{
		printf("create failed\n");
		return -1;
	}

	return 0;
}

int shell_cmd_fill(int argc, char* argv[]) // 파일의 크기를 지정해 해당 크기만큼 내용을 채운 파일을 생성
{
	SHELL_ENTRY	entry;
	char*		buffer;
	char*		tmp;
	int			size;
	int			result;
	char opt[3] = { 0, };
	const char CREATE[3] = "-c";
	const char APPEND[3] = "-a";
	unsigned long offset;

	if (argc != 4) // 인자의 개수가 적절하지 않으면
	{
		printf("usage : fill [file] [size] [option]\n");
		return 0;
	}

	sscanf(argv[2], "%d", &size); // argv[2]로부터 데이터를 읽어 size에 저장
	sscanf(argv[3], "%s", opt); // argv[3]로부터 데이터를 읽어 opt에 저장

	if (strcmp(opt, CREATE) == 0) // CREATE 옵션은 새로운 파일 생성
	{
		// 파일명을 EXT2_ENTRY 형식에 맞게 수정한 뒤 해당 이름을 가지는 엔트리가 존재하지 않으면 부모 디렉터리에 추가해줌
		result = g_fsOprs.fileOprs->create(&g_disk, &g_fsOprs, &g_currentDir, argv[1], &entry); // (ext2_shell.c -> fs_create)
		if (result) // 생성 실패 시
		{
			printf("create failed\n");
			return -1;
		}
		offset = 0;
	}
	if (strcmp(opt, APPEND) == 0) { // APPEND 옵션은 기존 파일에 내용 추가
		g_fsOprs.lookup(&g_disk, &g_fsOprs, &g_currentDir, &entry, argv[1]); // 해당 이름을 가진 엔트리가 있는지 검색 (ext2_shell.c -> fs_lookup)
		offset = entry.size; // offset을 엔트리 크기로 지정
	}

	buffer = (char*)malloc(size + 13); // 파일에 내용을 쓰기 위한 버퍼
	tmp = buffer; // 버퍼의 시작주소
	while (tmp < buffer + size) // 버퍼의 크기만큼 해당 문자열을 채움
	{
		memcpy(tmp, "Can you see? ", 13);
		tmp += 13;
	}
	// 생성한 파일의 크기만큼 버퍼의 내용으로 채움
	g_fsOprs.fileOprs->write(&g_disk, &g_fsOprs, &g_currentDir, &entry, offset, size, buffer); // (ext2_shell.c -> fs_write)
	free(buffer); // 버퍼 해제

	return 0;
}

int shell_cmd_rm(int argc, char* argv[]) // 파일 삭제
{
	int i;

	if (argc < 2) // 인자가 너무 적으면
	{
		printf("usage : rm [files...]\n");
		return 0;
	}

	for (i = 1; i < argc; i++) // 현재 디렉터리에서 해당 파일을 찾아 삭제
	{
		if (g_fsOprs.fileOprs->remove(&g_disk, &g_fsOprs, &g_currentDir, argv[i])) // (구현X ext2_shell.c -> fs_remove)
			printf("cannot remove file\n");
	}

	return 0;
}

typedef struct
{
	char*	address;
} DISK_MEMORY; // ???

int shell_cmd_format(int argc, char* argv[]) // 파티션을 해당 파일시스템으로 포맷
{
	int		result;
	unsigned int k = 0;
	char*	param = NULL;
	x++;

	if (argc >= 2) // 추가적인 인자가 있으면 저장
		param = argv[1];

	result = g_fs.format(&g_disk, param); // 파티션을 해당 파일시스템으로 포맷 (ext2_shell.c -> fs_format)

	if (result < 0) // format 실패 시
	{
		printf("%s formatting is failed\n", g_fs.name);
		return -1;
	}

	printf("disk has been formatted successfully\n");
	return 0;
}

double get_percentage(unsigned int number, unsigned int total) // 비율값 리턴
{
	return ((double)number) / total * 100.;
}

int shell_cmd_df(int argc, char* argv[]) // 디스크 사용량 출력
{
	unsigned int used, total;
	int result;

	g_fsOprs.stat(&g_disk, &g_fsOprs, &total, &used); // 총 섹터 수와 사용중인 섹터 수 계산 (구현X ext2_shell.c -> fs_stat)

	printf("free sectors : %u(%.2lf%%)\tused sectors : %u(%.2lf%%)\ttotal : %u\n",
		total - used, get_percentage(total - used, g_disk.numberOfSectors),
		used, get_percentage(used, g_disk.numberOfSectors),
		total); // 디스크 사용량 출력

	return 0;
}

int shell_cmd_mkdir(int argc, char* argv[]) // 디렉터리 생성
{
	SHELL_ENTRY	entry;
	int result;

	if (argc != 2) // 인자의 개수가 적절하지 않으면
	{
		printf("usage : %s [name]\n", argv[0]);
		return 0;
	}

	// 해당 이름을 가지는 엔트리가 있는지 검사 후 없으면 디렉터리 생성 (ext2_shell.c -> fs_mkdir)
	result = g_fsOprs.mkdir(&g_disk, &g_fsOprs, &g_currentDir, argv[1], &entry);

	if (result) // 생성 실패 시
	{
		printf("cannot create directory\n");
		return -1;
	}

	return 0;
}

int shell_cmd_rmdir(int argc, char* argv[]) // 디렉터리 삭제
{
	int result;

	if (argc != 2) // 인자의 개수가 적절하지 않으면
	{
		printf("usage : %s [name]\n", argv[0]);
		return 0;
	}

	// 해당 이름을 가지는 엔트리를 찾아 디렉터리인지 확인 후 삭제 (구현X ext2_shell.c -> fs_rmdir)
	result = g_fsOprs.rmdir(&g_disk, &g_fsOprs, &g_currentDir, argv[1]);

	if (result) // 삭제 실패 시
	{
		printf("cannot remove directory\n");
		return -1;
	}

	return 0;
}

int shell_cmd_mkdirst(int argc, char* argv[]) // 여러 개의 디렉터리 생성
{											  // 디렉터리 이름은 0부터 count-1까지임
	SHELL_ENTRY	entry;
	int		result, i, count;
	char	buf[10];

	if (argc != 2) // 인자의 개수가 적절하지 않으면
	{
		printf("usage : %s [count]\n", argv[0]);
		return 0;
	}

	sscanf(argv[1], "%d", &count); // argv[1]로부터 정수 형식의 데이터를 읽어 count에 저장
	for (i = 0; i < count; i++)
	{
		printf(buf, "%d", i); // i를 정수 형식으로 buf에 저장
		result = g_fsOprs.mkdir(&g_disk, &g_fsOprs, &g_currentDir, buf, &entry); // i라는 이름의 디렉터리 생성 (ext2_shell.c -> fs_mkdir)

		if (result) // 생성 실패 시 (만약 4라는 이름의 디렉터리가 존재하고 8개의 디렉터리를 만들고자 했다면 3까지만 생성됨)
		{
			printf("cannot create directory\n");
			return -1;
		}
	}

	return 0;
}

int shell_cmd_cat(int argc, char* argv[]) // 파일의 내용을 출력
{
	SHELL_ENTRY	entry;
	char		buf[1025] = { 0, };
	int			result;
	unsigned long	offset = 0;

	if (argc != 2) // 인자의 개수가 적절하지 않으면
	{
		printf("usage : %s [file name]\n", argv[0]);
		return 0;
	}

	// 해당 이름의 엔트리를 찾음 (ext2_shell.c -> fs_lookup)
	result = g_fsOprs.lookup(&g_disk, &g_fsOprs, &g_currentDir, &entry, argv[1]);
	if (result) // 파일이 없으면
	{
		printf("%s lookup failed\n", argv[1]);
		return -1;
	}

	// 파일의 크기만큼 버퍼 크기 단위로 데이터를 읽어 출력함 (구현X ext2_shell.c -> fs_read)
	while (g_fsOprs.fileOprs->read(&g_disk, &g_fsOprs, &g_currentDir, &entry, offset, 1024, buf) > 0)
	{
		printf("%s", buf);
		offset += 1024; // 읽은만큼 offset 증가
		memset(buf, 0, sizeof(buf)); // 새로운 데이터를 읽기 위해 0으로 초기화
	}
	printf("\n");
}

int shell_cmd_ls(int argc, char* argv[]) // 현재 디렉터리의 엔트리 목록 출력
{
	SHELL_ENTRY_LIST		list;
	SHELL_ENTRY_LIST_ITEM*	current;

	if (argc > 2) // 인자가 너무 많으면
	{
		printf("usage : %s [path]\n", argv[0]);
		return 0;
	}

	init_entry_list(&list); // 엔트리 목록을 담기위한 리스트 초기화 (entrylist.c)
	if (g_fsOprs.read_dir(&g_disk, &g_fsOprs, &g_currentDir, &list)) // 디렉터리의 엔트리를 읽어 리스트에 추가 (ext2_shell.c -> fs_read_dir)
	{
		printf("Failed to read_dir\n");
		return -1;
	}

	current = list.first; // 리스트의 첫번째 엔트리부터

	printf("[File names] [D] [File sizes]\n");
	while (current) // 마지막 엔트리까지 출력
	{
		printf("%-12s  %1d  %12d\n",
			current->entry.name, current->entry.isDirectory, current->entry.size);
		current = current->next;
	}
	printf("\n");

	release_entry_list(&list); // 리스트 삭제 (entrylist.c)
	return 0;
}
