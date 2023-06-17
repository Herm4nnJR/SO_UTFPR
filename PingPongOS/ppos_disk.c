#include <stdio.h>
#include <stdlib.h>

#include <signal.h>
#include "ppos.h"
#include "ppos_data.h"
#include "ppos-core-globals.h"
#include "ppos_disk.h"
#include "disk.h"

task_t **suspendQueue;
disk_t *diskControl;
struct sigaction diskAction;

void diskManager(void *args){
	printf("%s\n", (char*)args);
	while(1){
		sem_down(diskControl->diskAcessSem);
		if(diskControl->awakened){
			diskControl->awakened = 0;
			task_resume(diskControl->diskAcessQueue->task);
			DiskRequest *aux = diskControl->diskAcessQueue;
			if(diskControl->diskAcessQueue->next != diskControl->diskAcessQueue){
				diskControl->diskAcessQueue = diskControl->diskAcessQueue->next;
				diskControl->diskAcessQueue->prev = aux->prev;
				if(aux->prev == diskControl->diskAcessQueue)
					diskControl->diskAcessQueue->next = aux->prev;
			}
			else{
				diskControl->diskAcessQueue = NULL;
			}
			free(aux);
		}
		if(disk_cmd(DISK_CMD_STATUS, 0, 0) == DISK_STATUS_IDLE && 
				diskControl->diskAcessQueue != NULL)
			disk_cmd(diskControl->diskAcessQueue->type, 
					diskControl->diskAcessQueue->block, 
					diskControl->diskAcessQueue->buffer);
		sem_up(diskControl->diskAcessSem);
		task_yield();
	}
}

int inSleepQueue(task_t *task){
	if(task != NULL && sleepQueue != NULL){
		task_t *walk = sleepQueue;
		do{
			if(walk == task)
				return 1;
			walk = walk->next;
		}while(walk != sleepQueue);
	}
	return 0;
}

void diskAcessHandler(){
	if(inSleepQueue(diskControl->diskManagerTask))
		task_resume(diskControl->diskManagerTask);
	diskControl->awakened = 1;
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

	diskControl = (disk_t*)malloc(sizeof(disk_t));
	diskControl->diskManagerTask = (task_t*)malloc(sizeof(task_t));

	sem_create(diskControl->diskAcessSem, 1);
	task_create(diskControl->diskManagerTask, diskManager, "Gerenciador de disco inicializado");
	diskControl->diskManagerTask->userTask = diskControl->awakened = 0;
	diskControl->diskAcessQueue = NULL;

	task_join(diskControl->diskManagerTask);
	task_yield();

	diskAction.sa_handler = diskAcessHandler;
	sigemptyset(&diskAction.sa_mask);
	diskAction.sa_flags = 0;
	if(sigaction(SIGUSR1, &diskAction, 0) < 0){
		perror("Erro ao inicializar o tratador do gerenciador de disco:\n");
		return -1;
	}
	return 0;
}	

void createNewRequest(char type, int block, void *buffer){
	DiskRequest *new = (DiskRequest*)malloc(sizeof(DiskRequest));
	new->task = taskExec;
	new->type = type;
	new->block = block;
	new->buffer = buffer;
	if(diskControl->diskAcessQueue != NULL){
		new->prev = diskControl->diskAcessQueue->prev;
		new->next = diskControl->diskAcessQueue;
		diskControl->diskAcessQueue->prev->next = new;
		diskControl->diskAcessQueue->prev = new;
	}
	else{
		diskControl->diskAcessQueue = new;
		new->next = new->prev = new;
	}
}

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer){
	if(block < 0 || buffer == NULL)
		return -1;

	sem_down(diskControl->diskAcessSem);
	createNewRequest(DISK_CMD_READ, block, buffer);

	if(inSleepQueue(diskControl->diskManagerTask))
		task_resume(diskControl->diskManagerTask);

	sem_up(diskControl->diskAcessSem);
	task_suspend(taskExec, suspendQueue);

	return 0;
}

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer){
	if(block < 0 || buffer == NULL)
		return -1;

	sem_down(diskControl->diskAcessSem);
	createNewRequest(DISK_CMD_WRITE, block, buffer);

	if(inSleepQueue(diskControl->diskManagerTask))
		task_resume(diskControl->diskManagerTask);

	sem_up(diskControl->diskAcessSem);
	task_suspend(taskExec, suspendQueue);

	return 0;
}
