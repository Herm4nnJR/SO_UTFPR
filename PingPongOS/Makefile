all:
	gcc -g -o premp pingpong-preempcao.c libppos_static.a ppos-core-aux.c -DSCHEDULER=RR -DPRINT=0
	gcc -g -o sched pingpong-scheduler.c libppos_static.a ppos-core-aux.c -DSCHEDULER=PRIOD -DPRINT=0
	gcc -g -o stres pingpong-preempcao-stress.c libppos_static.a ppos-core-aux.c -DSCHEDULER=RR -DPRINT=0
	gcc -g -o conta pingpong-contab-prio.c libppos_static.a ppos-core-aux.c -DSCHEDULER=PRIOD -DPRINT=1
	gcc -g -o disk1 -Wall pingpong-disco1.c ppos_disk.c disk.c libppos_static.a ppos-core-task-aux.c -DSCHEDULER=FCFS -DPRINT=0 -lrt
	gcc -g -o disk2 -Wall pingpong-disco2.c ppos_disk.c disk.c libppos_static.a ppos-core-task-aux.c -lrt
