CFLAGS=-g -DDEBUG

xcrutchd: xcrutchd.c aplaypop.c x11_debug.c
	gcc -W -Wall $(CFLAGS) -o xcrutchd $^ -lX11 -lXss -lasound
run: xcrutchd
	./xcrutchd -d
