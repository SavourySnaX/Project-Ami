
run:	memory.c memory.h main.c cpu.c cpu.h
	gcc -O0 -g memory.c main.c cpu.c -o run
