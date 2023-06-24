#include "ppos.h"
#include "ppos-core-globals.h"
#include <limits.h>
#include <signal.h>
#include <sys/time.h>

// ****************************************************************************
// Coloque aqui as suas modificações, p.ex. includes, defines variáveis, 
// estruturas e funções

#include "disk.h"

#define QUANTUM 20 //Define o quantum máximo das tarefas

#define RR 0	//Escolhe o escalonador Round Robin
#define FCFS 1	//Escolhe o escalonador First Come First Serverd
#define PRIOP 2	//Escolhe o escalonador por prioridade preemptivo
#define PRIOC 3	//Escolhe o escalonador por prioridade cooperativo 
#define PRIOD 4	//Escolhe o escalonador por prioridade dinâmico

//Define o scheduler como uma macro definida no pré-processamento
//SCHEDULER é passado no comando de compilação
#ifndef SCHEDULER			//Se SCHEDULER não for passado...
	#define SCHEDULER PRIOD	//... define como escalonador padrão o PRIOf
#endif

//Define se o sistema vai imprimir o tempo de execução das tarefas
#ifndef PRINT		//Se PRINT não for passado no pré-processamento/compilação
	#define PRINT 1	//Define que o sistema vai imprimir o tempo
#endif

//Structs do temporizador
struct sigaction action;
struct itimerval timer;

//Variáveis globais auxiliares
unsigned int saveActiveTime;	//Marca o ínicio/término do tempo de cada tarefa no processador

//Define a prioridade de uma tarefa
void task_setprio (task_t *task, int prio){

	//Limita as prioridades entre um valor de -20 a 20
	if(prio > 20)		//Se a nova prioridade for menor que -20, limita a prioridade para -20
		prio = 20;
	else if(prio < -20)	//Se a nova prioridade for maior que 20, limita a prioridade para 20
		prio = -20;

    if(!task)										//Se task for NULL, muda a prioridade da tarefa em execução
        taskExec->priod = taskExec->prioe = prio;
    else											//Do contrário, muda a prioridade da task
        task->priod = task->prioe = prio;
}

//Retorna a prioridade estática de uma tarefa
int task_getprio (task_t *task){
    if(!task)					//Se task for NULL, retorna a prioridade estática da tarefa em execução
        return taskExec->prioe;
    else						//Do contrário, retorna a prioridade estática da task
        return task->prioe;
}

//Retorna o ID da tarefa em execução
int task_getid(){
	return taskExec->id;
}

//Escalonador baseado em fila, RR ou FCFS
//Retorna a primeira tarefa da fila de tarefas prontas
task_t* schedulerFCFS(){
	if(PPOS_IS_PREEMPT_ACTIVE) //Se a preempção estiver ativada (RR), restaura o quantum da primeira tarefa da fila
		readyQueue->taskQuantum = QUANTUM;
	return readyQueue;
}

//Escalonador baseado em prioridade, PRIOc, PRIOp e PRIOd
task_t* schedulerPRIO(int alfa) {
    task_t *prox = readyQueue;	//Ponteiro para a próxima atividade

    if(readyQueue){					//Se a fila não estiver vazia
        char boolean = 0;			//Booleano auxiliar
        task_t *walk = readyQueue;	//Ponteiro auxiliar para percorrer a fila/lista

		//Percorre a fila/lista
        while(walk != readyQueue || !boolean){

			//Se o ponteiro estiver a apontando para o começo da lista circular e booleano for 0 (primeira vez passando por esse nó)
			//Seta o booleano como verdadeiro
			if(walk == readyQueue)
				boolean = !boolean;

			//Procura a tarefa com menor prioridade e aplica o envelhecimento
			else{
				if(walk->priod <= prox->priod){	//Se a prioridade dinâmica da tarefa atual for menor que a prioridade dinâmica da tarefa salva 
					if(alfa < 0){				//Se o envelhecimento está sendo aplicado
						prox->priod += alfa;	//Aplica o envelhecimento na outra tarefa
						if(prox->priod < -20)	//Se a prioridade ficar menor que -20
							prox->priod = -20;	//Limita a prioridade para -20
					}
					prox = walk;				//Salva o ponteiro para a nova tarefa
				}
				else
					walk->priod += alfa;		//Do contrário, aplica o envelhecimento na tarefa atual
			}
            walk = walk->next;	//Aponta para a próxima tarefa
        }

		if(alfa < 0)						//Se estiver aplicando envelhecimento
	        prox->priod = prox->prioe;		//Restaura a prioridade dinâmica
											//
		if(PPOS_IS_PREEMPT_ACTIVE)			//Se a preempção estiver ativada
			prox->taskQuantum = QUANTUM;	//Restaura o quantum da próxima tarefa
    }
    return prox;	//Retorna a próxima tarefa
}

//Escolhe o scheduler correto e ativa/desativa a preempção
task_t* scheduler(){
	task_t *nextTask;		//Ponteiro para a próxima tarefa
	PPOS_PREEMPT_DISABLE;	//Desativa a preempção por padrão

	switch(SCHEDULER){
		case 0: PPOS_PREEMPT_ENABLE;					//RR (ativa preempção)
		case 1: nextTask = schedulerFCFS(); break;		//FCFS
		case 2: PPOS_PREEMPT_ENABLE;					//PRIOp (ativa preempção)
		case 3: nextTask = schedulerPRIO(0); break;		//PRIOc
		case 4: PPOS_PREEMPT_ENABLE;					//PRIOd (ativa preempção)
				nextTask = schedulerPRIO(-1); break;
	}
	return nextTask;	//Retorna a próxima tarefa
}

//Tratador do temporizador, chamado a cada tick
void handler(){
	systemTime++;	//Incrementa o contador de tempo do sistema

	//Se a preempção estiver ativada e a tarefa for de usuário
	if(PPOS_IS_PREEMPT_ACTIVE && taskExec->userTask == 1){
		//Se o quantum maior que 20 -> decrementa o quantum da tarefa
		//Se o quantum for igual a 0 -> Passa o processador para o dispatcher
		(taskExec->taskQuantum > 0) ? taskExec->taskQuantum-- : task_yield();
	}
}

//Imprime todos os elementos de uma fila
//Usado para identificar e corrigir erros, irrelevante no contexto do trabalho
void printQueue(task_t *q){
	if(q){
		task_t *walk = q;
		char boolean = 0;
		while(walk != q || !boolean){
			if(walk == q)
				boolean = !boolean;
			printf("-> %d ", walk->id);
			walk = walk->next;
		}
		printf("\n");
	}
}

//Função auxiliar para contar o tempo de execução de uma tarefa
void tasksTime(task_t *task, char exit){
    if(exit == 1)										//Se a tarefa acabou
        task->totalTime = systime() - task->totalTime;	//Salva o tempo total da tarefa
    task->activeTime += systime() - saveActiveTime;		//Aumenta o tempo ativo (no processador)
    saveActiveTime = systime();							//Salva uma nova marca temporal
}

//Após o inicío do sistema
//Inicializa o temporizador
//Inicializa as variáveis internas do dispatcher
void after_ppos_init(){
	if(disk_cmd(DISK_CMD_INIT, 0, 0) < 0)
		exit(1);
	taskDisp->userTask = taskDisp->activeTime = taskDisp->totalTime = taskDisp->activations = 0;
    action.sa_handler = handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	if(sigaction(SIGALRM, &action, 0) < 0){
		printf("Erro no sigaction no after_ppos_init\n");
		exit(1);
	}
	timer.it_value.tv_usec = 1;
	timer.it_value.tv_sec = 0;
	timer.it_interval.tv_usec = 1000;
	timer.it_interval.tv_sec = 0;
	if(setitimer(ITIMER_REAL, &timer, 0) < 0){
		printf("Erro no setitimer do after_ppos_init\n");
		exit(1);
	}
#ifdef DEBUG
    printf("\ninit - AFTER");
#endif
}

//Depois de criar uma nova atividade
//Inicializa suas variáveis
void after_task_create (task_t *task){
	task->prioe = task->priod = 0; 	//Prioridade padrão (0)
	task->totalTime = systime();	//Tempo de ínicio da tarefa
	task->activeTime = 0;			//Tempo ativo da tarefa (0)
	task->userTask = 1;				//Marca como uma tarefa de usuário
#ifdef DEBUG
    printf("\ntask_create - AFTER - [%d]", task->id);
#endif
}

//Após uma tarefa ser finalizada
void after_task_exit(){
	tasksTime(taskExec, 1);	//Faz os últimos cálculos de tempo

	if(PRINT)	//Se o print estiver ativado, imprime as informações da task finalizada
		printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n",
			   	taskExec->id, taskExec->totalTime, taskExec->activeTime, taskExec->activations);

	//Se todas as tarefas acabaram (com exceção da main)
	if(readyQueue && readyQueue->id == 0 && readyQueue->next == readyQueue){
		tasksTime(taskDisp, 1);	//Faz os últimos cálculos de tempo para o dispatcher

		if(PRINT)	//Se o print estiver ativado, imprime as informações do dispatcher
			printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n"
					, taskDisp->id, taskDisp->totalTime, taskDisp->activeTime, taskDisp->activations);
	}
#ifdef DEBUG
    printf("\ntask_exit - AFTER- [%d]", taskExec->id);
#endif
}

//Antes de trocar a tarefa
void before_task_switch ( task_t *task ){
	task->activations++;	//Incrementa o contador de ativações (tarefa volta ao processador, ou seja, volta a executar)
#ifdef DEBUG
    printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
#endif
}

//Depois de trocar de tarefa
void after_task_switch ( task_t *task ){
	tasksTime(taskExec, 0);	//Cálculos de tempo para a última tarefa ou dispatcher
#ifdef DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]", taskExec->id, task->id);
#endif
}

//Depois da tarefa largar o processador
void after_task_yield () {
	tasksTime(taskExec, 0); //Cálculos de tempo para a última tarefa
#ifdef DEBUG
    printf("\ntask_yield - AFTER - [%d]", taskExec->id);
#endif
}

// ****************************************************************************

void before_ppos_init () {
    // put your customiization here
#ifdef DEBUG
    printf("\ninit - BEFORE");
#endif
}

void before_task_create (task_t *task ){
    // put your customization here
#ifdef DEBUG
    printf("\ntask_create - BEFORE - [%d]", task->id);
#endif
}

void before_task_exit () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_exit - BEFORE - [%d]", taskExec->id);
#endif
}

void before_task_yield () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - BEFORE - [%d]", taskExec->id);
#endif
}

void before_task_suspend( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - BEFORE - [%d]", task->id);
#endif
}

void after_task_suspend( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - AFTER - [%d]", task->id);
#endif
}

void before_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - BEFORE - [%d]", task->id);
#endif
}

void after_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - AFTER - [%d]", task->id);
#endif
}

void before_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - AFTER - [%d]", taskExec->id);
#endif
}

int before_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}
