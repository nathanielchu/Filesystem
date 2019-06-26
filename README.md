# OSPFS

Implements a simple in-memory file system driver for Linux.

## Overview

OSPFS, although simpler than most filesystems, is powerful enough to read and write files organized in a hierarchical directory structure. ospfs currently does not support timestamps, permissions, or special device files. Additionally, this module is only capable of using an in-memory file system, meaning that "on disk" is actually stored entirely in main memory. Thus, changes made to the file system are not saved across reboots.

## Features

- Implements directory operations for creation, reading, and deletion
- Implements file operations for reading, writing, truncation, appending, and deletion
- Supports symbolic and hard links
- Designed with block device abstraction

## Installation

Extract into ubuntu distribution and run in QEMU emulator with `./run-qemu`. 

## Usage

The file system module is loaded into the kernel and mounted, allowing access with usual Unix commands such as cat, dd, emacs, and even firefox. 

## Background

### Overview

Operating systems generally support many different file systems, from old-school FAT (common in DOS machines) to new formats such as Linux's ext3. Each file system type is written to fit a common interface, called the VFS layer. The file system driver stores its data in a built-in RAM disk.

### File system structure

OSPFS, like most UNIX file systems, divides available disk space into two main types of regions: indoe regions and data regions. UNIX file systems assign one inode to each file int he file system; a file's indoe holds critical metadata about the file such as its attributes and pointers to its data blocks. The data regions are divided into large data blocks (typically 4KB or more), within which the file system stores file data and directory metadata. Directory entries contain file names and pointers to inodes; a file is said to be hard-linked if multiple directory entries in the file system refer to that file's inode.

Both files and directories logically consist of a series of data blocks, which may be scattered throughout the disk much like the pages of a process's virtual address space can be scattered throughout physical memory.

### Sectors and blocks

Modern disks perform reads and writes in units of sectors, which today are almost univerally 512 bytes each. File systems actually allocate and use disk storage in units of blocks. Be wary of the distinction between the two terms: sector size is a property of the disk hardware, whereas block size is an aspect of the operating system using the disk. A file system's block size must be at least the sector size of the underlying disk, but it could be greater (often it is a power of two multiple of the sector size).

The original UNIX file system used a block size of 512 bytes, the same as the sector size of the underlying disk. Most modern file systems use a larger block size; however, because storage space has gotten much cheaper and it is more efficient to manage storage at larger granularities. OSPFS uses a block size of 1024 bytes.

### Superblocks

File systems typically reserve certain disk blocks, at "easy-to-find" locations on the disk (such as the very start or the very end), to hold meta-data describing properties of the file system as a whole: the block size, disk size, any meta-data required to find the root directory, the time the file system was last mounted, the time the file system was last checked for errors, and so on. These special blocks are called superblocks.

Our file system will have exactly one superblock, which will always be at block 1 on the disk.  Block 0 is typically reserved to hold boot loaders and partition tables, so file systems generally never use the very first disk block. Most "real" file systems maintain multiple superblocks, replicated throughout several widely-spaced regions of the disk, so that if one of them is corrupted or the disk develops a media error in that region, the other superblocks can still be found and used to access the file system. OSPFS omits this feature for simplicity.

The block bitmap: managing free disk blocks

Just as a kernel must manage the system's physical memory to ensure that each physical page is used for only one purpose at a time, a file system must manage the blocks of storage on a disk to ensure that each block is used for only one purpose at a time. In file systems it is common to keep track of free disk blocks using a bitmap rather than a linked list, because a bitmap is more storage-efficient than a linked list and easier to keep consistent on the disk. ("FAT" file systems use a linked list.) Searching for a free block in a bitmap can take more CPU time than simply removing the first element of a linked list, but for file systems this isn't a problem because the I/O cost of actually accessing the free block after we find it dominates performance.

To set up a free block bitmap, we reserve a contiguous region of space on the disk large enough to hold one bit for each disk block. For example, since our file system uses 1024-byte blocks, each bitmap block contains 1024 * 8 = 8192 bits, or enough bits to describe 8192 disk blocks. In other words, for every 8192 disk blocks the file system uses, we must reserve one disk block for the block bitmap. A given bit in the bitmap is set (value 1) if the corresponding block is free, and clear (value 0) if the corresponding block is in use. The block bitmap in our file system always starts at disk block 2, immediately after the superblock. We'll reserve enough bitmap blocks to hold one bit for each block in the entire disk, including the blocks containing the boot sector, the superblock, and the bitmap itself. We will simply make sure that the bitmap bits corresponding to these special, "reserved" areas of the disk are always clear (marked in-use).

### Inodes

Each file and directory on the system corresponds to exactly one inode. The inode stores metadata information about the file, such as its size, its type (file or directory), and the locations of its data blocks. Directory information, such as the file's name and directory location, is not stored in the inode; instead, directory entries refer to inodes by number. This allows the file system to safely move files from one directory to another, and also allows for "hard links".

Indoes are relatively small structures; OSPFS's take up 64 bytes each. They are generally stored in a contiguous table on disk. The size of this table is specified when the file system is created, and in most file systems it cannot later be changed. Just like blocks, an inode can be either in use or free, and just like blocks, a file system can run out of inodes. It is actually possible to run out of inodes before running out of blocks, and in this case no more files can be created even though there is space available to store them. For this reason, and because inodes themselves do not take up much space, the inode table is often created with far more available inodes than the file system is expected to need.
