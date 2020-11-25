CFLAGS=-g -DDEBUG

xcrutchd: xcrutchd.c aplaypop.c x11_debug.c
	gcc -W -Wall $(CFLAGS) -o xcrutchd $^ -lX11 -lXss -lasound
aplaypop: aplaypop.c
	gcc -W -Wall -DMAIN -o aplaypop aplaypop.c -lasound && ./aplaypop
aplaypop_switch: aplaypop_switch.c
	gcc -W -Wall -DMAIN -o aplaypop_switch aplaypop_switch.c -lasound && ./aplaypop_switch 1
run: xcrutchd
	./xcrutchd -d

clean:; rm -f xcrutchd aplaypop aplaypop_switch
all: xcrutchd aplaypop aplaypop_switch
