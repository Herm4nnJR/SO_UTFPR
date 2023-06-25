all:
	gcc -g -o premp pingpong-preempcao.c libppos_static.a ppos-core-aux.c -DSCHEDULER=RR -DPRINT=0
	gcc -g -o sched pingpong-scheduler.c libppos_static.a ppos-core-aux.c -DSCHEDULER=PRIOD -DPRINT=0
	gcc -g -o stres pingpong-preempcao-stress.c libppos_static.a ppos-core-aux.c -DSCHEDULER=RR -DPRINT=0
	gcc -g -o conta pingpong-contab-prio.c libppos_static.a ppos-core-aux.c -DSCHEDULER=PRIOD -DPRINT=1
	gcc -g -o disk1 -Wall pingpong-disco1.c ppos_disk.c disk.c libppos_static.a ppos-core-task-aux.c -DSCHEDULER=RR -DPRINT=1 -DDISKSCHEDULER=FCFS -lrt
	gcc -g -o disk2 -Wall pingpong-disco2.c ppos_disk.c disk.c libppos_static.a ppos-core-task-aux.c -DSCHEDULER=RR -DPRINT=1 -DDISKSCHEDULER=FCFS -lrt
test:
	gcc -g -o testFCFS -Wall pingpong-disco2.c ppos_disk.c disk.c libppos_static.a ppos-core-task-aux.c -DSCHEDULER=RR -DPRINT=1 -DDISKSCHEDULER=FCFS -lrt
	gcc -g -o testSSTF -Wall pingpong-disco2.c ppos_disk.c disk.c libppos_static.a ppos-core-task-aux.c -DSCHEDULER=RR -DPRINT=1 -DDISKSCHEDULER=SSTF -lrt
	gcc -g -o testCSCAN -Wall pingpong-disco2.c ppos_disk.c disk.c libppos_static.a ppos-core-task-aux.c -DSCHEDULER=RR -DPRINT=1 -DDISKSCHEDULER=CSCAN -lrt
	./testFCFS >> testFCFS.txt
	./testSSTF >> testSSTF.txt
	./testCSCAN >> testCSCAN.txt
	rm testFCFS testSSTF testCSCAN
