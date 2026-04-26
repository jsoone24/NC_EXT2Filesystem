# EXT2 Filesystem — Educational Implementation

A small ext2-style filesystem built on top of an in-memory disk simulator,
intended as a learning vehicle for filesystem internals (super block, group
descriptors, inode tables, block & inode bitmaps, direct/indirect blocks,
directory entries, etc.).

> **Note**: This is an educational toy — it is **not** binary-compatible with
> the real Linux ext2 driver. In particular, `inode.blocks` counts data blocks
> (not 512-byte sectors as real ext2 does), and the dump/debug commands assume
> the simulator's flat memory layout.

---

## Volume Specification

| Field                | Value                |
| -------------------- | -------------------- |
| Total volume size    | 2,147,487,744 B (2 GB) |
| Sector size          | 1024 B               |
| Block size           | 2048 B (= 2 sectors) |
| Total sectors        | 2,097,152 + 4        |
| Total blocks         | 1,048,578            |
| Block groups         | 64                   |
| Blocks per group     | 16,384               |
| Total inodes         | 1,024                |
| Inodes per group     | 16                   |
| Reserved inodes      | 11 (in group 0)      |
| Inode size           | 128 B                |
| Volume label         | `EXT2 BY NC`         |

Per-group on-disk layout (each group keeps its own copy for redundancy):

```
+----+----+----+----+-------------------+--------------------------+
| SB | GD | BBM| IBM|   Inode table     |       Data blocks        |
+----+----+----+----+-------------------+--------------------------+
  1    1    1    1         13                  16367 blocks
```

Boot block (1 block) sits at the very start of the volume, before group 0.

---

## Build and Run

```sh
make            # builds ./shell
./shell         # launches the interactive filesystem shell
make clean      # removes object files and the binary
```

Builds cleanly with modern `clang`/`gcc` (tested on macOS Darwin 25 with
clang 16+). The Makefile relaxes a few legacy warnings to errors so the
codebase compiles on stricter toolchains; see [`Makefile`](Makefile) for
the exact flag set.

---

## Shell Commands

After launching `./shell`, you must `format` then `mount` before any other
operation. The prompt shows `학번 : [/<currentDir>]#`.

### Lifecycle
| Command            | Effect                                           |
| ------------------ | ------------------------------------------------ |
| `format [label]`   | Initialize the disk as ext2 with optional label  |
| `mount`            | Read the super block / GDT and prepare for I/O   |
| `umount`           | Release in-memory FS state                       |
| `exit` / `quit`    | Tear down disk and exit                          |

### File operations
| Command                       | Effect                                  |
| ----------------------------- | --------------------------------------- |
| `ls`                          | List entries in current directory       |
| `touch <name>`                | Create an empty file                    |
| `fill <name> <size> -c\|-a`   | Create (`-c`) or append (`-a`) bytes    |
| `cat <name>`                  | Print file contents to stdout           |
| `rm <name>`                   | Remove a file                           |

### Directory operations
| Command           | Effect                                   |
| ----------------- | ---------------------------------------- |
| `cd [name]`       | Change directory (`.`, `..`, or a child) |
| `mkdir <name>`    | Create a directory                       |
| `rmdir <name>`    | Remove an empty directory                |
| `mkdirst <count>` | Create directories named `0` through `count-1` (testing) |

### Diagnostics
| Command                     | Effect                                       |
| --------------------------- | -------------------------------------------- |
| `df`                        | Show free / used block counts                |
| `dumpsuperblock`            | Hex-dump the super block (block 1)           |
| `dumpgd`                    | Hex-dump the group descriptor table          |
| `dumpblockbitmap`           | Hex-dump the block bitmap                    |
| `dumpinodebitmap`           | Hex-dump the inode bitmap                    |
| `dumpinodetable`            | Hex-dump the first two inode-table blocks    |
| `dumpdatablockbyname <n>`   | Hex-dump the data block of a named entry     |
| `dumpdatablockbynum <i>`    | Hex-dump data block at index `i`             |
| `dumpfileinode <n>`         | Print inode number + 15 block pointers       |

### Filename rules

Files use the legacy 8.3 short-name format: up to 8 characters before the
optional `.`, up to 3 after. Letters are uppercased. Allowed characters are
`A–Z`, `a–z`, `0–9`, and `.`. Names like `.bashrc`, `valid_2.txt`,
`nine99999.txt`, or `BIGNAME.LONGEXT` are rejected as invalid.

---

## Source Layout

| File                | Role                                                  |
| ------------------- | ----------------------------------------------------- |
| [`shell.c`](shell.c)             | Interactive REPL, command dispatch, argument parsing |
| [`shell.h`](shell.h)             | `SHELL_ENTRY`, `SHELL_FS_OPERATIONS`, list helpers   |
| [`ext2.c`](ext2.c)               | Core filesystem logic — format, lookup, allocation, read/write, indirect blocks |
| [`ext2.h`](ext2.h)               | On-disk structs (super block, group descriptor, inode, directory entry) and forward declarations |
| [`ext2_shell.c`](ext2_shell.c)   | Glue between the generic `SHELL_*` interface and `ext2_*` operations |
| [`ext2_shell.h`](ext2_shell.h)   | Glue header                                          |
| [`disksim.c`](disksim.c)         | In-memory disk simulator (`read_sector`/`write_sector`) |
| [`disksim.h`](disksim.h)         | Disk simulator header                                |
| [`disk.h`](disk.h)               | `DISK_OPERATIONS` interface                          |
| [`entrylist.c`](entrylist.c)     | Linked list used by `ls` to accumulate directory entries |
| [`common.h`](common.h)           | Logging macros, `EXT2_SUCCESS`/`EXT2_ERROR`          |
| [`types.h`](types.h)             | `BYTE`, `WORD`, `DWORD`, `QWORD` aliases             |

---

## Architecture Notes

- **`fs->gd` is a cache for group 0's descriptor only.** Volume-wide free
  counts are kept in `fs->sb` (and synchronised across all on-disk SB copies
  on every allocation/free). Locality decisions inside
  `get_available_data_block` read the GD table fresh from disk so they see
  the right group's state.

- **`location.block` always stores an absolute block number.** Both
  `find_entry_on_root` and `find_entry_on_data` use this convention so that
  callers like `insert_entry` can compare directly against `inode.block[i]`.
  The data_read/block_read math is invariant to the (group, block-relative)
  vs (0, absolute) representation, so this normalization is safe.

- **`block_read`/`block_write` operate on whole blocks** (2 sectors at a
  time) regardless of the underlying disk sector size. The `meta_*` and
  `data_*` helpers translate (group, block) into absolute sector indices,
  always reserving one block for the boot sector at the start of the volume.

- **Directory entries are fixed 32 bytes** (see `EXT2_DIR_ENTRY` in
  [`ext2.h`](ext2.h)), so each block holds exactly 64 entries. Growing a
  directory past 64 entries triggers `expand_block`, which threads the new
  block through the inode's direct, indirect, double-indirect, or
  triple-indirect pointers as appropriate.

---

## Changes from the Original Toy Implementation

This repository started as a college lab project. A polish/bug-fix pass
re-shaped it into a build-clean, runnable, modern-toolchain version. See
[`CHANGES.md`](CHANGES.md) for the full list — highlights:

- Fixed a stack overflow in `process_meta_data_for_block_free` that
  segfaulted every `rm` and `rmdir`.
- Fixed two `&retEntry->fs` typos (passing pointer-to-pointer where a
  pointer was expected).
- Fixed a `set_entry` that returned `EXT2_ERROR` on success.
- Fixed the `&& FILE_TYPE_DIR` always-true branch that made `ext2_remove`
  reject every file as if it were a directory.
- Fixed `ext2_write`'s seek loop, which doubled `blockSize` instead of
  adding `MAX_BLOCK_SIZE` — large writes landed in the wrong block.
- Fixed `find_entry_on_data` reading 32 bytes past its buffer when a
  directory grew past one block.
- Fixed `format_name` writing past `regularName[10]` and matching
  `.bashrc` as if it were the `.` self-link.
- Fixed `disksim_init` checking the wrong pointer for NULL after the
  second `malloc` and leaking a 2 GB buffer in `disksim_uninit`.
- Made the build clean on modern `clang` (forward declarations, ctype.h,
  64-bit-safe pointer formatting in dump output).
- Added input validation to `fill`, `dumpfileinode`,
  `dumpdatablockbyname`, `dumpdatablockbynum`, and `cd` so malformed
  invocations no longer segfault.
- Restored proper UTF-8 Korean for the shell prompt (was mojibaked from
  CP949 → Latin-1 → UTF-8).
