#include <stdio.h>
#include <stdlib.h>

#include <signal.h>
#include "ppos.h"
#include "ppos_data.h"
#include "ppos_disk.h"
#include "disk.h"

task_t **suspendQueue;
task_t diskManagerTask;
semaphore_t *diskAcess;

struct sigaction diskAction;
char awakened = 0;

void diskManager(void *args){
	printf("%s\n", (char*)args);
	while(1){
		sem_down(diskAcess);
		sem_up(diskAcess);
		task_suspend(&diskManagerTask, suspendQueue);
	}
}

void diskAcessHandler(){
	if(*suspendQueue == &diskManagerTask)
		task_resume(&diskManagerTask);
	awakened = 1;
}

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize){
	*numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
	*blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
	if(*numBlocks < 0 || *blockSize < 0)
		return -1;
	sem_create(diskAcess, 1);
	task_create(&diskManagerTask, diskManager, "Gerenciador de disco inicializado");
	diskManagerTask.userTask = 0;
//	task_suspend(&diskManagerTask, suspendQueue);

	diskAction.sa_handler = diskAcessHandler;
	sigemptyset(&diskAction.sa_mask);
	diskAction.sa_flags = 0;
	if(sigaction(SIGUSR1, &diskAction, 0) < 0){
		perror("Erro ao inicializar o tratador do gerenciador de disco:\n");
		return -1;
	}
	return 0;
}	

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer){
	return 0;
}

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer){
	return 0;
}
