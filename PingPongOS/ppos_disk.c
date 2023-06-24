#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "ppos.h"
#include "ppos-core-globals.h"
#include "ppos_disk.h"
#include "disk.h"

#define SUCCESS 0
#define FAILURE -1

struct sigaction diskAction;
disk_t *ctrl;

static int insertRequest(DiskRequest *req){
	if(req == NULL){
		perror("Novo pedido é NULL\n");
		return FAILURE;
	}
	if(ctrl->accessQueue->prev == NULL || ctrl->accessQueue->prev->next == NULL){
		perror("Fila de pedido de acesso ao disco não é circular\n");
		return FAILURE;
	}
	req->next = ctrl->accessQueue;
	req->prev = ctrl->accessQueue->prev;
	ctrl->accessQueue->prev->next = req;
	ctrl->accessQueue->prev = req;
	return SUCCESS;
}

static int removeRequest(task_t *task){
	if(task == NULL){
		perror("Tarefa do pedido é NULL\n");
		return FAILURE;
	}
	DiskRequest *walk = ctrl->accessQueue;
	do{
		if(walk->task == task){
			if(walk->next != walk){
				walk->next->prev = walk->prev;
				walk->prev->next = walk->next;
				ctrl->accessQueue = walk->next;
			}
			else{
				ctrl->accessQueue = NULL;
			}
			free(walk);
			return SUCCESS;
		}
		walk = walk->next;
	}while(walk != ctrl->accessQueue);
	perror("Tarefa não encontrada\nPossível erro durante a inserção ou tarefa não fez pedido de acesso ao disco\n");
	return FAILURE;
}

static int createRequest(char type, int block, void *buffer){
	DiskRequest *new = (DiskRequest*)malloc(sizeof(DiskRequest));
	new->task = taskExec;
	new->type = type;
	new->block = block;
	new->buffer = buffer;
	if(ctrl == NULL){
		perror("Estrutura de controle do disco é NULL\n");
		return FAILURE;
	}
	if(ctrl->accessQueue != NULL){
		if(insertRequest(new) < 0){
			perror("Não foi possível inserir novo pedido de acesso ao dico na fila\n");
			return FAILURE;
		}
	}
	else{
		ctrl->accessQueue = new;
		new->prev = new->next = new;
	}
	return SUCCESS;
}


void diskManagerBody(void *args){
	printf("%s\n", (char*)args);
	sem_down(ctrl->diskSemaphore);

}

void diskHandler(){
	printf("Operação concluída");
}

int disk_mgr_init (int *numBlocks, int *blockSize){
	//Inicializa o disco
	if(disk_cmd(DISK_CMD_INIT, 0, 0) < 0){
		perror("Erro ao inicializar o disco\n");
		return FAILURE;
	}

	//Obtém o número de blocos e tamanho de cada
	*numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
	*blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
	if(*numBlocks <= 0 || *blockSize <= 0){
		perror("Erro ao obter o número/tamanho de blocos\n");
		return FAILURE;
	}

	//Inicializa a estrutura de controle do disco
	ctrl = (disk_t*)malloc(sizeof(disk_t));
	ctrl->accessQueue = NULL;
	ctrl->suspendQueue = NULL;
	ctrl->awakened = ctrl->active = 0;
	ctrl->diskSemaphore = (semaphore_t*)malloc(sizeof(semaphore_t));
	if(sem_create(ctrl->diskSemaphore, 1) < 0){
		perror("Erro ao criar o semáforo\n");
		return FAILURE;
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
		return FAILURE;
	}

	//Finaliza a função com sucesso
	return SUCCESS;
}

int disk_block_read (int block, void *buffer){
	return 0;
}

int disk_block_write (int block, void *buffer){
	return 0;
}
