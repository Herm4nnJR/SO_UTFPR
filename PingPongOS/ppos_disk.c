#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <limits.h>
#include "ppos.h"
#include "ppos-core-globals.h"
#include "ppos_disk.h"
#include "disk.h"

#define SUCCESS 0
#define FAILURE -1

#define FCFS 0
#define SSTF 1
#define CSCAN 2

#ifndef DISKSCHEDULER
	#define DISKSCHEDULER FCFS
#endif

struct sigaction diskAction;
disk_t *ctrl;

static DiskRequest* diskFCFS(){
	if(!ctrl){
		perror("Controlador do disco não inicializado\n");
		return NULL;
	}
	return ctrl->accessQueue;
}

static DiskRequest* diskSSTF(){
	if(!ctrl){
		perror("Controlador do disco não inicializado\n");
		return NULL;
	}
	DiskRequest *next = ctrl->accessQueue;
	if(ctrl->accessQueue != NULL){
		DiskRequest *walk = ctrl->accessQueue->next;
		int proximity = abs(ctrl->headPosition - ctrl->accessQueue->block);
		while(walk != ctrl->accessQueue){
			int aux = abs(ctrl->headPosition - walk->block);
			if(aux < proximity){
				next = walk;
				proximity = aux;
			}
			walk = walk->next;
		}
	}
	return next;
}

static DiskRequest* diskCSCAN(){
	if(!ctrl){
		perror("Controlador do disco não inicializado\n");
		return NULL;
	}
	if(ctrl->accessQueue == NULL)
		return NULL;
	DiskRequest *next = NULL, *lower = NULL;
	DiskRequest *walk = ctrl->accessQueue;
	int proximity = INT_MAX;
	do{
		if(walk->block < ctrl->headPosition){
			if(lower == NULL || walk->block < lower->block)
				lower = walk;
		}
		else{
			int aux = abs(ctrl->headPosition - walk->block);
			if(aux < proximity){
				next = walk;
				proximity = aux;
			}
		}
		walk = walk->next;
	}while(walk != ctrl->accessQueue);
	return (next == NULL) ? lower : next;
}

//Considerar implementar o scheduler durante a inserção das tarefas na fila de pedidos
//diskManagerBody só pega o primeiro elemento da fila
//Possível menor tempo médio?
static DiskRequest* diskScheduler(){
	DiskRequest *next = NULL;
	switch(DISKSCHEDULER){
		case FCFS: next = diskFCFS(); break;
		case SSTF: next = diskSSTF(); break;
		case CSCAN: next = diskCSCAN(); break;
		default: perror("Opção de escalonador inválida\n"); break;
	}
	return next;
}

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
	}while(ctrl->accessQueue != NULL && walk != ctrl->accessQueue);
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

static void destroyRequestQueue(DiskRequest *req){
	if(ctrl->accessQueue != NULL){
		if(req->next == ctrl->accessQueue)
			free(req);
		else
			destroyRequestQueue(req->next);
	}
}

static void destroyDiskControl(){
	if(ctrl != NULL){
		destroyRequestQueue(ctrl->accessQueue);
		if(ctrl->diskSemaphore != NULL)
			sem_destroy(ctrl->diskSemaphore);
		free(ctrl);
	}
}

void diskManagerBody(){
	while(ctrl->active >= 0){
		sem_down(ctrl->diskSemaphore);
		if(ctrl->awakened){
			ctrl->awakened = 0;
			task_resume(ctrl->accessQueue->task);
			removeRequest(ctrl->accessQueue->task);
		}
		if(disk_cmd(DISK_CMD_STATUS, 0, 0) == DISK_STATUS_IDLE && ctrl->accessQueue != NULL){
			DiskRequest *next = diskScheduler();
			if(DISKSCHEDULER == CSCAN){
				int lastBlock = disk_cmd(DISK_CMD_DISKSIZE, 0, 0) - 1;
				ctrl->walked += (lastBlock - ctrl->headPosition) + (lastBlock + 1) + next->block;
			}
			else
				ctrl->walked += abs(next->block - ctrl->headPosition);
			ctrl->headPosition = next->block;
			if(disk_cmd(next->type, next->block, next->buffer) < 0){
				perror("Erro ao ler/escrever o disco\n");
				exit(1);
			}
		}
		sem_up(ctrl->diskSemaphore);
		ctrl->active = 0;
//		task_suspend(ctrl->diskManager, ctrl->suspendQueue);
		task_yield();
	}
	destroyDiskControl();
	task_exit(0);
}

void diskHandler(){
	if(!ctrl->active == 0){
		task_resume(ctrl->diskManager);
		ctrl->active = 1;
	}
	ctrl->awakened = 1;
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
	ctrl->awakened = ctrl->active = ctrl->walked = ctrl->headPosition = 0;
	ctrl->diskSemaphore = (semaphore_t*)malloc(sizeof(semaphore_t));
	if(sem_create(ctrl->diskSemaphore, 1) < 0){
		perror("Erro ao criar o semáforo\n");
		return FAILURE;
	}

	//Cria a task gerenciadora de disco
	ctrl->diskManager = (task_t*)malloc(sizeof(task_t));
	task_create(ctrl->diskManager, diskManagerBody, NULL);
	ctrl->diskManager->userTask = 0;
//	task_join(ctrl->diskManager);

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
	//Verifica se pode fazer o pedido
	if(block < 0){
		perror("Bloco selecionado inválido\n");
		return FAILURE;
	}
	if(buffer == NULL){
		perror("buffer é NULL\n");
		return FAILURE;
	}
	if(ctrl == NULL){
		perror("Estrutura de controle do disco não inicializada\n");
		return FAILURE;
	}
	if(ctrl->diskSemaphore == NULL){
		perror("Semáforo do disco não foi inicializado\n");
		return FAILURE;
	}

	//Requisita o semáforo e cria um novo pedido de acesso ao disco
	sem_down(ctrl->diskSemaphore);
	createRequest(DISK_CMD_READ, block, buffer);

	//Coloca o disco na lista de tarefas prontas
	if(!ctrl->active){
		task_resume(ctrl->diskManager);
		ctrl->active = 1;
	}

	//Libera o semáforo e suspende a tarefa atual até o pedido ser concluído
	sem_up(ctrl->diskSemaphore);
	task_suspend(taskExec, ctrl->suspendQueue);
	task_yield();

	//Finaliza a função com sucesso
	return SUCCESS;
}

int disk_block_write (int block, void *buffer){
	//Verifica se pode fazer o pedido
	if(block < 0){
		perror("Bloco selecionado inválido\n");
		return FAILURE;
	}
	if(buffer == NULL){
		perror("buffer é NULL\n");
		return FAILURE;
	}
	if(ctrl == NULL){
		perror("Estrutura de controle do disco não inicializada\n");
		return FAILURE;
	}
	if(ctrl->diskSemaphore == NULL){
		perror("Semáforo do disco não foi inicializado\n");
		return FAILURE;
	}

	//Requisita o semáforo e cria um novo pedido de acesso ao disco
	sem_down(ctrl->diskSemaphore);
	createRequest(DISK_CMD_WRITE, block, buffer);

	//Coloca o disco na lista de tarefas prontas
	if(!ctrl->active){
		task_resume(ctrl->diskManager);
		ctrl->active = 1;
	}

	//Libera o semáforo e suspende a tarefa atual até o pedido ser concluído
	sem_up(ctrl->diskSemaphore);
	task_suspend(taskExec, ctrl->suspendQueue);
	task_yield();

	//Finaliza a função com sucesso
	return SUCCESS;
}

void disk_mgr_close(){
	if(ctrl){
		ctrl->active = -1;
		printf("Walked: %ld blocks\n", ctrl->walked);
	}
}
