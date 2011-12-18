CFLAGS=-g -DDEBUG

xcrutchd: xcrutchd.c aplaypop.c x11_debug.c
	gcc -W -Wall $(CFLAGS) -o xcrutchd $^ -lX11 -lXss -lasound
aplaypop: aplaypop.c
	gcc -W -Wall -DMAIN -o aplaypop aplaypop.c -lasound && ./aplaypop
run: xcrutchd
	./xcrutchd -d
clean:
	rm -f xcrutchd aplaypop
