#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

int main() {
	int fd;
	char buf[2048];
	int i;

	fd = open("test/designproblemtest", O_APPEND | O_CREAT | O_WRONLY);
	// Should crash after truncate() deallocates/unassigns the block, but before the file size is changed.
	ioctl(fd, 42, 13);

	memset(buf, 0xFF, 2048);
	// Each add_block() contains 4 writes (allocate block, set to 0, set block number in inode, update file size).
	// change_size calls add_block() twice, and has one more write to set file size.
	// Two blocks of data are written.
	// So a total of 2(4) + 1 + 2 = 11 writes are done by this call.
	write(fd, buf, 2048);
	close(fd);
	// 2 writes will complete: free_block and removing the block number from the inode.
	// The file size, however, remains at 2048 when the filesystem starts crashing.
	truncate("test/designproblemtest", 1024);

	// So now, reading this file will attempt to access a nonexistent block.
	// e.g. cat test/designproblemtest will fail
	fd = open("test/designproblemtest", O_RDONLY);
	ioctl(fd, 42, -1);
	if (read(fd, buf, 2048) == -1) {
		fprintf(stderr, "Reading test/designproblemtest failed with errno: %d\n", errno);
	}
	return 0;
}
