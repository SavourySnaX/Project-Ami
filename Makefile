
run:	memory.c memory.h main.c cpu.c cpu.h
	gcc -O0 -g main.c memory.c customchip.c cpu.c -o run
