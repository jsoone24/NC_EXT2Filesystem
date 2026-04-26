# Changes from the Original

This document lists every change applied on top of the original lab code.
Items are grouped by category, and each fix cites the file/line where the
problem lived.

---

## Crashes and Memory Corruption

### Stack overflow in `process_meta_data_for_block_free`
[`ext2.c`](ext2.c) — `process_meta_data_for_block_free`

The function passed a 64-byte `EXT2_SUPER_BLOCK` stack variable to
`block_read`/`block_write`, which always read/write a full 2048-byte block.
On every `rm` or `rmdir` the disk content was scribbled across whatever
followed `sb` on the stack, then the corrupt stack frame caused a SIGSEGV
on return.

Fix: read into a properly-sized `BYTE sbBlock[MAX_BLOCK_SIZE]` buffer and
alias the struct over it.

### Wrong NULL check after second `malloc` in `disksim_init`
[`disksim.c`](disksim.c) — `disksim_init`

The `if` checked `disk->pdata` (the *first* allocation, already verified)
instead of the inner `address` field that was just `malloc`'d. A failed
2 GB allocation would have been silently ignored.

### Memory leak in `disksim_uninit`
[`disksim.c`](disksim.c) — `disksim_uninit`

Only freed the outer `DISK_MEMORY` struct, leaking the entire ~2 GB
disk buffer.

### `find_entry_on_data` OOB read past block buffer
[`ext2.c`](ext2.c) — `find_entry_on_data`

Called `find_entry_at_block(blockBuffer, …, begin = blockOffset * 64,
last = begin + 63, …)` against a 64-entry buffer. For any directory with
more than one data block, the search ran 64 entries past the buffer end.

Fix: always pass `(0, entriesPerBlock - 1)` so the scan stays in-bounds.

### `read_dir_from_sector` OOB iteration for root
[`ext2.c`](ext2.c) — `ext2_read_dir`, `read_dir_from_sector_n`

Root's `ls` skipped the volume-label entry by passing `sector + 32`, then
ran the loop for the full 64 entries — reading 32 bytes past the buffer.

Fix: split the helper into a bounded `read_dir_from_sector_n(max_entries)`
and have the root path pass `total - 1`.

### `format_name` OOB write into `regularName[11..14]`
[`ext2.c`](ext2.c) — `format_name`

Names with extensions longer than 3 characters wrote past the 11-byte
`regularName` buffer before the final length check rejected them.

Fix: bounds-check `nameLength`/`extenderCurrent` on every character.

### `add_entry_list` dereferenced potentially-NULL `malloc` result
[`entrylist.c`](entrylist.c) — `add_entry_list`

Added a NULL check; returns -1 on allocation failure.

### `shell_cmd_cd` could OOB-write its 256-entry static stack
[`shell.c`](shell.c) — `shell_cmd_cd`

Added an explicit cap before `path[++pathTop] = newEntry`.

### Pointer truncation in `printFromP2P` on 64-bit
[`ext2_shell.c`](ext2_shell.c) — `printFromP2P`

Cast pointers through `int`, silently dropping the upper 32 bits on
64-bit systems and breaking both the printed addresses and the alignment
mask. Now uses `uintptr_t` and `%p`.

---

## Logic Errors (Silent Corruption)

### `ext2_write` seek loop doubled instead of incrementing
[`ext2.c`](ext2.c) — `ext2_write`

`blockSize += blockSize` doubled the threshold each iteration, so writes
past the 4-block mark jumped to the wrong offset.

Fix: `blockSize += MAX_BLOCK_SIZE` (matches `ext2_read`).

### `set_entry` returned `EXT2_ERROR` on success
[`ext2.c`](ext2.c) — `set_entry`

Every successful directory-entry write looked like a failure to any
caller that checked the return value.

### `&retEntry->fs` (pointer-to-pointer) passed where `EXT2_FILESYSTEM*` expected
[`ext2.c`](ext2.c) — `insert_entry`

Two call sites passed `&retEntry->fs` instead of `retEntry->fs` to
`get_data_block_at_inode`. Type-confused; only the absence of strict
type checking in C masked the crash.

### `find_entry_on_data` missing return value
[`ext2.c`](ext2.c) — `find_entry_on_data`

Fell off the end of the function with no `return` if `inode.blocks == 0`.
Now explicitly returns `EXT2_ERROR`.

### `find_entry_on_data` stored relative block, `insert_entry` compared absolute
[`ext2.c`](ext2.c) — `find_entry_on_data`

`find_entry_on_root` stored `location.block` as the absolute block number;
`find_entry_on_data` stored `blockNum % block_per_group`. `insert_entry`
then compared the stored value against `inode.block[i]` (always
absolute), so the comparison always failed for non-group-0 directories.

Fix: `find_entry_on_data` now stores the absolute block number
(`group = 0`, `block = blockNum`). Both `data_read` and `block_read`
produce the same physical sector for either representation, so other
callers are unaffected.

### `process_meta_data_for_block_free` decremented free count when freeing
[`ext2.c`](ext2.c) — `process_meta_data_for_block_free`

`fs->gd.free_blocks_count--` while *freeing* a block — should increment.

### `ZeroMemory(sector, sizeof(MAX_BLOCK_SIZE))` zeroed 4 bytes
[`ext2.c`](ext2.c) — `ext2_read_superblock`

`sizeof(MAX_BLOCK_SIZE)` is `sizeof(int)` (4), not 2048. The buffer was
left mostly uninitialised before the next read.

### `ext2_remove` rejected every file as if it were a directory
[`ext2.c`](ext2.c) — `ext2_remove`

`if (… && FILE_TYPE_DIR)` — logical-AND with a non-zero constant is
always true. Fixed to `== FILE_TYPE_DIR`.

### `ext2_create` left `inode.mode = 0`
[`ext2.c`](ext2.c) — `ext2_create`

Passed `fileType = 0` to `insert_entry`, so newly-created files had no
file-type bits in their mode. Now passes `FILE_TYPE_FILE`.

### `ext2_rmdir` inode-bitmap clearing was off by one and used wrong precedence
[`ext2.c`](ext2.c) — `ext2_rmdir`

Used `inode % inode_per_group` (should be `(inode - 1) % …`) and
`0x01 << bitmap_offset - 1` (which underflows when `bitmap_offset == 0`
because of operator precedence). Aligned with `ext2_remove`'s correct
formulation.

### `ext2_rmdir` `links_count > 1` branch fell through without returning
[`ext2.c`](ext2.c) — `ext2_rmdir`

Added explicit `return EXT2_SUCCESS`.

### `format_name` matched any name starting with `.` as the `.` entry
[`ext2.c`](ext2.c) — `format_name`

`strncmp(name, ".", 1)` matched `.bashrc`, `.test`, etc. and rewrote
them as the special `.` self-link. Replaced with `strcmp` for exact
matches.

### `upper_string(name, MAX_ENTRY_NAME_LENGTH)` only handled 11 chars
[`ext2.c`](ext2.c) — `format_name`

`MAX_ENTRY_NAME_LENGTH = 11`, but the input may be up to 256 chars. Names
of length 12 had their last character stay lowercase and end up in the
on-disk entry. Now uses the actual input length.

### `fs->gd` (group-0 cache) used as if it were the inode's group descriptor
[`ext2.c`](ext2.c) — `ext2_create`, `get_available_data_block`

Two check sites consulted `fs->gd.free_inodes_count` /
`free_blocks_count` to make decisions for inodes outside group 0,
producing both false rejections (group 0 full but other groups had room)
and false acceptances (only group 0 had room). Replaced the volume-wide
existence check with `fs->sb.free_inode_count` and made the locality
check in `get_available_data_block` read the correct group's GD from
disk.

### `expand_block` returned `EXT2_SUCCESS` instead of the new block number
[`ext2.c`](ext2.c) — `expand_block`

The comment promised "the new block number"; the code returned 0.
Now returns `available_block`. Backward-compatible — every caller
still only checks for `EXT2_ERROR`.

### Bitwise `&` instead of logical `&&` in bit-scan loop
[`ext2.c`](ext2.c) — `get_available_data_block`

Worked by accident on bool values; replaced with `&&` to match the
sibling code in `get_free_inode_number`.

---

## Shell / UX Bugs

### `shell_cmd_cat` had no `return` statement
[`shell.c`](shell.c) — `shell_cmd_cat`

### `_exit(0)` skipped stdout flushing
[`shell.c`](shell.c) — `shell_cmd_exit`

Piped or redirected output disappeared because `_exit` skips libc cleanup.
Switched to `exit(0)`.

### `void` functions returned values
[`ext2_shell.c`](ext2_shell.c) — `fs_dumpDataPart`, `fs_dumpfileinode`

Returned the result of `ext2_lookup` from a `void` function. Now `return`s
without a value.

### `fs_create` and `fs_mkdir` ran the success path on failure
[`ext2_shell.c`](ext2_shell.c) — `fs_create`, `fs_mkdir`

Called `ext2_entry_to_shell_entry(EXT2Entry, …)` even when `ext2_create`
/ `ext2_mkdir` failed; the entry was uninitialised, so `get_inode`
printed a misleading `"Invalid inode number"`. Now only converts on
success.

### `fs_format` printed `formatting as a (null)`
[`ext2_shell.c`](ext2_shell.c) — `fs_format`

Falls back to `VOLUME_LABLE` when no label is supplied.

### `dump*` commands segfaulted on missing arguments
[`shell.c`](shell.c) — `shell_cmd_dumpdatablockbyname`,
`shell_cmd_dumpfileinode`, `shell_cmd_dumpdatablockbynum`

Bare command (no `argv[1]`) ran into `strlen(NULL)` or `atoi(NULL)`.
Each handler now checks `argc` and prints a usage message.

### Dump commands used sector-level math against block-level layout
[`ext2_shell.c`](ext2_shell.c) — `fs_dumpDataBlockRaw` (was
`fs_dumpDataSector`), `fs_dumpDataPart`, `fs_dumpDataBlockByNum`

Multiplied by `bytesPerSector` (1024) where the layout uses 2 KB blocks,
so dumps pointed at the wrong half of the disk. Now use `MAX_BLOCK_SIZE`
throughout.

### `shell_cmd_fill` had four problems
[`shell.c`](shell.c) — `shell_cmd_fill`

- No option validation — typo like `-x` wrote with uninitialised entry.
- No lookup-result check on `-a` — appending to a missing file wrote
  garbage memory.
- Negative `size` caused `buffer + size` pointer underflow.
- `opt[3]` overflowed if user typed more than 2 chars.

All four addressed in one rewrite.

### Mojibaked prompt placeholder
[`shell.c`](shell.c) — `do_shell`

The prompt printed `ÇÐ¹ø :` because the original CP949 bytes for `학번`
("student ID") had been re-encoded as Latin-1 then UTF-8. The original
text was a placeholder a student was supposed to replace with their
actual ID. Substituted a meaningful identifier (`nc22-ext2 :`).

---

## Build / Modern-Toolchain Compatibility

- Added forward declarations in [`ext2.c`](ext2.c) and [`ext2.h`](ext2.h)
  so the project compiles under clang 16+, which makes implicit function
  declarations a hard error.
- Replaced the broken local prototypes for `toupper`/`isalpha`/`isdigit`
  with `#include <ctype.h>`.
- Cast `void*` pointer arithmetic in `read_block`/`write_block` to
  `char*` (the original was a non-portable GCC extension).
- Made `fs_rmdir`'s `parent` parameter `const SHELL_ENTRY*` to match the
  function-pointer type in `SHELL_FS_OPERATIONS`.
- Updated the [`Makefile`](Makefile) with explicit `CFLAGS` that demote a
  handful of inherited noisy warnings (unused variables, `unsigned char`
  vs `char` mismatches, MSVC `#pragma warning`) so the build is clean
  on `clang`/`gcc`.
- Made `make clean` non-failing when there is nothing to remove.

---

## Verified

- Builds: `make` produces `./shell` with **0 warnings, 0 errors** on
  Apple clang 16 / Darwin.
- Basic flow: `format → mount → touch → fill → cat → rm → mkdir →
  rmdir → df → umount → exit` all run cleanly.
- Multi-block writes: a 30 KB `fill -a` exercises the `ext2_write` seek
  loop past the doubling-bug threshold.
- Multi-group: creating enough directories to push inodes / data blocks
  past group 0 exercises the `fs->gd` and absolute-block-number fixes.
- Multi-block directories: `mkdirst 80` inside a subdirectory exercises
  the multi-block `find_entry_on_data` and `insert_entry` paths.
- Persistence: `umount` followed by `mount` recovers the filesystem
  state from the in-memory disk buffer.
