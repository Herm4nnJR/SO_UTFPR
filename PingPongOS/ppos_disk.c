#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "ppos.h"
#include "ppos-core-globals.h"
#include "ppos_disk.h"
#include "disk.h"

struct sigaction diskAction;
disk_t *ctrl;

void diskManagerBody(void *args){
	printf("%s\n", (char*)args);
}

void diskHandler(){
	printf("Operação concluída");
}

int disk_mgr_init (int *numBlocks, int *blockSize){
	//Inicializa o disco
	if(disk_cmd(DISK_CMD_INIT, 0, 0) < 0){
		perror("Erro ao inicializar o disco\n");
		return -1;
	}

	//Obtém o número de blocos e tamanho de cada
	*numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
	*blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
	if(*numBlocks <= 0 || *blockSize <= 0){
		perror("Erro ao obter o número/tamanho de blocos\n");
		return -1;
	}

	//Inicializa a estrutura de controle do disco
	ctrl = (disk_t*)malloc(sizeof(disk_t));
	ctrl->accessQueue = NULL;
	ctrl->suspendQueue = NULL;
	ctrl->awakened = ctrl->active = 0;
	ctrl->diskSemaphore = (semaphore_t*)malloc(sizeof(semaphore_t));
	if(sem_create(ctrl->diskSemaphore, 1) < 0){
		perror("Erro ao criar o semáforo\n");
		return -1;
	}

	//Cria a task gerenciadora de disco
	ctrl->diskManager = (task_t*)malloc(sizeof(task_t));
	task_create(ctrl->diskManager, diskManagerBody, "Gerenciador de disco inicializado\n");
	ctrl->diskManager->userTask = 0;

	//Inicializa o tratador de sinal de operações de disco concluídas
	diskAction.sa_handler = diskHandler;
	sigemptyset(&diskAction.sa_mask);
	diskAction.sa_flags = 0;
	if(sigaction(SIGUSR1, &diskAction, 0) < 0){
		perror("Erro ao inicializar o tratador do gerenciador de disco\n");
		return -1;
	}

	//Finaliza a função com sucesso
	return 0;
}

int disk_block_read (int block, void *buffer){
	return 0;
}

int disk_block_write (int block, void *buffer){
	return 0;
}
