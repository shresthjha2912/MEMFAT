# MEMFAT

**MEMFAT** is an in-memory FAT-style virtual file system implemented in C++. It creates a fixed-size virtual disk inside System V shared memory and provides a small interactive shell, `foosh`, for managing directories and files inside the virtual disk.

The project simulates core file-system concepts such as block allocation, bitmap-based free-space tracking, FAT chaining, directory metadata, path resolution, and file copying between the host system and the virtual disk.

## Features

- 64 MB virtual disk stored in shared memory
- 65,536 fixed-size blocks
- 1 KB block size
- FAT-based block chaining
- Bitmap-based free block management
- Root directory initialization
- Directory creation
- Directory traversal with absolute and relative paths
- File import from host system to virtual disk
- File export from virtual disk to host system
- File-to-file copy inside the virtual disk
- File printing inside the shell
- Interactive shell interface

## Repository Structure

```text
.
├── Makefile
├── diskmanager
├── diskmanager.cpp
├── diskutils.cpp
├── diskutils.h
├── foosh
├── foosh.cpp
└── new.txt
```

| File | Description |
|---|---|
| `diskmanager.cpp` | Creates and removes the shared-memory virtual disk |
| `diskutils.cpp` | Implements FAT, bitmap, directory, path, copy, and print utilities |
| `diskutils.h` | Header file containing metadata structures and function declarations |
| `foosh.cpp` | Interactive shell for using the virtual disk |
| `Makefile` | Build script for compiling the project |
| `new.txt` | Sample host file for testing copy operations |

## System Design

MEMFAT uses a shared-memory segment as a virtual disk.

```text
Virtual Disk
│
├── Superblock / Disk Metadata
├── Free Block Bitmap
├── FAT Table
└── Data Blocks
    ├── Root Directory Block
    ├── Directory Blocks
    └── File Data Blocks
```

## Disk Layout

The virtual disk is configured with the following constants:

```text
Virtual disk size : 64 MB
Total blocks      : 65,536
Block size        : 1 KB
Root block        : 265
```

The first part of the disk stores metadata such as:

- total number of blocks
- number of free blocks
- root directory block number
- free block bitmap
- FAT table

The remaining blocks are used for directory entries and file contents.

## Metadata Structure

Each file or directory entry is represented using a packed metadata structure.

```cpp
struct metadata {
    char type;
    char name[23];
    int size;
    int firstblock;
};
```

| Field | Description |
|---|---|
| `type` | Entry type: `f` for file, `d` for directory |
| `name` | File or directory name |
| `size` | File size in bytes or directory entry count |
| `firstblock` | First block of the file or directory |

Each metadata entry occupies 32 bytes. Since each block is 1024 bytes, a directory block can store 32 metadata entries.

## Main Components

### 1. Disk Manager

The `diskmanager` program creates or removes the virtual disk.

It supports two modes:

```bash
./diskmanager create
./diskmanager remove
```

When creating the disk, it:

- creates a System V shared-memory segment
- initializes disk metadata
- initializes the free block bitmap
- initializes the FAT table
- creates the root directory
- adds `.` and `..` entries to the root directory

When removing the disk, it deletes the shared-memory segment.

### 2. Disk Utilities

The disk utility layer provides helper functions for:

- attaching to the virtual disk
- detaching from the virtual disk
- allocating free blocks
- freeing block chains
- reading and writing FAT entries
- reading and writing bitmap entries
- resolving absolute and relative paths
- adding directory entries
- removing directory entries
- creating directories
- listing directories
- copying files
- printing files

### 3. Foosh Shell

The `foosh` shell is an interactive command-line interface for operating on the virtual disk.

When started, it attaches to the virtual disk and shows:

```text
Number of blocks
Number of free blocks
First block of the root directory
```

It then opens a prompt:

```text
[foosh] VD:/ >
```

## Build Instructions

Compile the project using:

```bash
make
```

This builds:

```text
diskmanager
foosh
```

If the Makefile does not work in your environment, compile manually:

```bash
g++ -Wall -O2 diskmanager.cpp diskutils.cpp -o diskmanager
g++ -Wall -O2 foosh.cpp diskutils.cpp -o foosh
```

## Running the Project

### Step 1: Create the Virtual Disk

```bash
./diskmanager create
```

Expected output:

```text
+++ Virtual disk created with 65536 blocks
```

### Step 2: Start the Shell

```bash
./foosh
```

Example startup output:

```text
+++ Number of blocks = 65536
+++ Number of free blocks = ...
+++ First block of the root directory = 265
[foosh] VD:/ >
```

### Step 3: Use File-System Commands

Inside the shell, run commands such as:

```text
md docs
cd docs
ls
dir
cp `new.txt file.txt
prn file.txt
cd /
ls
```

### Step 4: Exit the Shell

```text
exit
```

or:

```text
quit
```

### Step 5: Remove the Virtual Disk

After exiting the shell, remove the shared-memory disk:

```bash
./diskmanager remove
```

## Shell Commands

| Command | Alias | Description |
|---|---|---|
| `md <path>` | `mkdir <path>` | Create a directory |
| `cd <path>` | `chdir <path>` | Change current directory |
| `dir` | - | List current directory in compact format |
| `ls [path]` | - | List directory in detailed format |
| `cp <src> <dest>` | `copy <src> <dest>` | Copy files |
| `prn <path>` | `type <path>` | Print file contents |
| `exit` | `quit` | Exit the shell |

## Path Support

MEMFAT supports both absolute and relative paths.

### Absolute Path

```text
/docs/file.txt
```

### Relative Path

```text
docs/file.txt
```

### Special Directory Entries

```text
.
..
```

Example:

```text
cd ..
cd /docs
cd .
```

## Copy Command Usage

The `cp` command supports copying files between the host file system and the virtual disk.

MEMFAT uses a leading backtick character to indicate a host-system file.

### Host to Virtual Disk

```text
cp `new.txt file.txt
```

This copies `new.txt` from the host system into the current virtual-disk directory as `file.txt`.

### Virtual Disk to Host

```text
cp file.txt `output.txt
```

This copies `file.txt` from the virtual disk to the host system as `output.txt`.

### Virtual Disk to Virtual Disk

```text
cp file.txt copy.txt
```

This creates a copy of `file.txt` inside the virtual disk.

### Host to Host

Host-to-host copy is not supported by this shell.

## Example Session

```text
$ ./diskmanager create
+++ Virtual disk created with 65536 blocks

$ ./foosh
+++ Number of blocks = 65536
+++ Number of free blocks = 65270
+++ First block of the root directory = 265
[foosh] VD:/> md docs
[foosh] VD:/> cd docs
[foosh] VD:/docs> cp `new.txt file.txt
[foosh] VD:/docs> ls
Total 3 entries
-----------------------------------------------------------------------
TYPE    NAME    SIZE    FIRST BLOCK
-----------------------------------------------------------------------
d       ./      3       ...
d       ../     0       ...
f       file.txt        ...     ...
-----------------------------------------------------------------------
[foosh] VD:/docs> prn file.txt
lisdjrgafo
[foosh] VD:/docs> exit
+++ Number of free blocks = ...

$ ./diskmanager remove
+++ Virtual disk removed
```

## Internal Functions

Some important functions implemented in `diskutils.cpp` are:

| Function | Purpose |
|---|---|
| `joindisk()` | Attaches the process to the shared-memory virtual disk |
| `leavedisk()` | Detaches the process from shared memory |
| `getfreeblock()` | Allocates a free block using the bitmap |
| `freeblock()` | Frees a chain of blocks |
| `get_fat()` | Reads the next block from the FAT table |
| `set_fat()` | Updates the FAT table |
| `get_bitmap()` | Reads the allocation status of a block |
| `set_bitmap()` | Updates the allocation status of a block |
| `split_path()` | Splits a path into components |
| `find_in_dir()` | Searches for an entry inside a directory |
| `resolve_path()` | Resolves absolute or relative paths |
| `add_to_dir()` | Adds a file or directory entry |
| `remove_from_dir()` | Removes a directory entry |
| `md_cmd()` | Implements directory creation |
| `ls_cmd()` | Implements directory listing |
| `cd_cmd()` | Implements directory navigation |
| `cp_cmd()` | Implements file copying |
| `prn_cmd()` | Prints file contents |

## Implementation Details

### Shared Memory

The virtual disk is stored in a System V shared-memory segment. Both `diskmanager` and `foosh` access the same memory segment using the same key generated through `ftok()`.

### Free-Space Management

Free blocks are tracked using a bitmap. A block is marked as allocated when it is assigned to a file or directory and marked free when the file block chain is released.

### FAT Table

The FAT table stores the next block for each allocated file or directory block.

A value of `-1` marks the end of a block chain.

### Directories

Directories are implemented as files containing fixed-size metadata entries. Each directory contains `.` and `..` entries.

If a directory block becomes full, MEMFAT allocates another block and links it using the FAT table.

### Files

File contents are split into 1 KB chunks. Each chunk is stored in a disk block, and the blocks are linked through the FAT table.

## Cleanup

To remove compiled binaries:

```bash
make clean
```

To remove the shared-memory virtual disk:

```bash
./diskmanager remove
```

If the program terminates unexpectedly, the shared-memory segment may remain active. In that case, run:

```bash
./diskmanager remove
```

## Notes

- The virtual disk exists only in shared memory.
- Data is not persisted after removing the virtual disk.
- `diskmanager create` must be run before starting `foosh`.
- The project is intended for educational use and file-system simulation.
- File and directory names are limited by the metadata structure.
- Host files are identified in `cp` commands using a leading backtick.

## Limitations

- No persistent on-disk storage
- No permissions or ownership model
- No journaling or crash recovery
- No concurrent shell synchronization
- No recursive directory deletion
- Limited filename length
- Host-to-host copy is not supported

## Summary

MEMFAT demonstrates the internal design of a FAT-style file system using shared memory. It implements block allocation, metadata management, directory traversal, file copying, and an interactive shell, making it useful for understanding how simple file systems manage storage internally.