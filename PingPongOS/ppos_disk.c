#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "ppos_disk.h"
#include "disk.h"

int disk_mgr_init (int *numBlocks, int *blockSize){
	if(disk_cmd(DISK_CMD_INIT, 0, 0) < 0){
		perror("Não foi possível inicializar o disco\n");
		return -1;
	}
	*numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
	*blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
	if(*numBlocks <= 0 || *blockSize <= 0)
		return -1;
	return 0;
}

int disk_block_read (int block, void *buffer){
	return 0;
}

int disk_block_write (int block, void *buffer){
	return 0;
}
