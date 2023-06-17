// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Julho de 2017

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.

// estrutura que representa um disco no sistema operacional

#include "ppos_data.h"

typedef struct request{
	task_t *task;			//Tarefa solicitante
	struct request *next;	//Próxima tarefa da fila
	struct request *prev;	//Tarefa anterior da fila
	char type;				//Tipo de operação
	int block;				//Bloco da operação
	void *buffer;			//Endereço do buffer de dados
}DiskRequest;

typedef struct{	//Controle geral do disco
	task_t *diskManagerTask;		//Tarefa gerenciadora de disco
	DiskRequest *diskAcessQueue;	//Fila de tarefas solicitantes de acesso ao disco
	semaphore_t *diskAcessSem;		//Semáforo de acesso ao disco
	char awakened;					//Indica se a tarefa foi acordada por um sinal
} disk_t ;

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) ;

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) ;

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) ;

#endif
