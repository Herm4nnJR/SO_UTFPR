all:
	gcc -g -o premp pingpong-preempcao.c libppos_static.a ppos-core-aux.c -Dpreemp -DdeactivatePrint
	gcc -g -o sched pingpong-scheduler.c libppos_static.a ppos-core-aux.c -Dsched -DdeactivatePrint
	gcc -g -o stres pingpong-preempcao-stress.c libppos_static.a ppos-core-aux.c -Dpreemp -DdeactivatePrint
	gcc -g -o conta pingpong-contab-prio.c libppos_static.a ppos-core-aux.c -Dsched -DactivatePrint
