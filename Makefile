
LD_SDL=-lglfw -lopengl32
CFLAGS=

run:	keyboard.c keyboard.h disk.c disk.h sprite.c sprite.h memory.c memory.h mgmain.c cpu.c cpu.h ciachip.c ciachip.h customchip.c customchip.h copper.h copper.c blitter.c blitter.h display.c display.h
	gcc $(CFLAGS) -O0 -g keyboard.c mgmain.c disk.c sprite.c memory.c ciachip.c customchip.c cpu.c copper.c blitter.c display.c $(LD_SDL) -o run
