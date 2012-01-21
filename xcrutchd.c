// gcc -W -Wall -o xcrutchd xcrutchd.c aplaypop.c -lX11 -lXss -lasound && ./xcrutchd

#include <stdio.h>
#include <string.h> // memset()
#include <stdlib.h> // system()
#include <unistd.h> // read(), write()
#include <signal.h> // sigaction()
#include <errno.h> // EINTR
#include <poll.h> // poll()
#include <sys/time.h> // setitimer()

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/extensions/saver.h>
#include <X11/extensions/scrnsaver.h>
#include <X11/extensions/dpms.h>

#include "aplaypop.h"
#define attr_unused __attribute__((__unused__))

static int xss_state;
static const char *stateNames[] = { "Off", "On", "Cycle", "Disable" };
static const char *kindNames[] = { "Blanked", "Internal", "External" };

int xss_printinfo(Display *dpy)
{
    int xss_event, xss_error, xss_major, xss_minor;
    Window root = DefaultRootWindow(dpy);

    if (XScreenSaverQueryExtension(dpy, &xss_event, &xss_error) != True) {
        perror("XScreenSaverQueryExtension()");
        return 0;
    }

    if (XScreenSaverQueryVersion(dpy, &xss_major, &xss_minor) != True) {
        perror("XScreenSaverQueryVersion()");
        return 0;
    }
    printf("xss_event=0x%x xss_error=0x%x xss_major=%d xss_minor=%d\n",
        xss_event, xss_error, xss_major, xss_minor);

    if (xss_major < 1 || (xss_major == 1 && xss_minor < 1)) {
        perror("Bad XScreenSaverQueryVersion");
        return 0;
    }

    XScreenSaverInfo *xss_info = XScreenSaverAllocInfo();
    if (xss_info == NULL) {
        perror("XScreenSaverAllocInfo()");
        return 0;
    }

    if (XScreenSaverQueryInfo(dpy, root, xss_info) != True) {
        perror("XScreenSaverQueryInfo()");
        return 0;
    }

    printf("XScreenSaverInfo {\n");
    printf("  Window window = %#lx;              /* screen saver window */\n", xss_info->window);
    printf("  int state = '%s';        /* ScreenSaver{Off,On,Disabled} */\n", stateNames[xss_info->state]);
    printf("  int kind = '%s';         /* ScreenSaver{Blanked,Internal,External} */\n", kindNames[xss_info->kind]);
    printf("  unsigned long til_or_since = %lu; /* ms; time til or since screen saver */\n", xss_info->til_or_since);
    printf("  unsigned long idle = %lu;         /* ms; total time since last user input */\n", xss_info->idle);
    printf("  unsigned long eventMask = %#lx;    /* selected events for this client */\n", xss_info->eventMask);
    printf("}\n");

    xss_state = xss_info->state;
    XFree(xss_info);

    return xss_event;
}

static int timer_fds[2];

static void timer_alarm(attr_unused int sig)
{
    write(timer_fds[1], "A", 1);
}

int timer_start(int ms)
{
    if (pipe(timer_fds) < 0) {
        perror("pipe()");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = timer_alarm;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGALRM, &sa, NULL) < 0) {
        perror("sigaction()");
        exit(EXIT_FAILURE);
    }

    struct itimerval it;
    memset(&it, 0, sizeof(it));
    it.it_value.tv_sec = ms / 1000;
    it.it_value.tv_usec = (ms % 1000) * 1000;
    it.it_interval.tv_sec = ms / 1000;
    it.it_interval.tv_usec = (ms % 1000) * 1000;
    if (setitimer(ITIMER_REAL, &it, NULL) < 0) {
        perror("setitimer()");
        exit(EXIT_FAILURE);
    }

    return timer_fds[0];
}

void timer_next(int fd)
{
    char c;
    read(fd, &c, 1);
}

void timer_stop()
{
    struct itimerval it;
    memset(&it, 0, sizeof(it));
    if (setitimer(ITIMER_REAL, &it, NULL) < 0) {
        perror("setitimer()");
        exit(EXIT_FAILURE);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    if (sigaction(SIGALRM, &sa, NULL) < 0) {
        perror("sigaction()");
        exit(EXIT_FAILURE);
    }

    close(timer_fds[1]);
    close(timer_fds[0]);
}

static int timediff_ms(struct timeval *before, struct timeval *after)
{
    return (
            (long long) after->tv_sec * 1000000ll -
            (long long) before->tv_sec * 1000000ll +
            (long long) after->tv_usec -
            (long long) before->tv_usec
           ) / 1000;
}

static int aplaypop_idle = 5000; // ms
static struct timeval play_bell_tv;

int play_bell(attr_unused int percent) {
    aplaypop();
    if (gettimeofday(&play_bell_tv, NULL) < 0) {
        perror("gettimeofday()");
        exit(EXIT_FAILURE);
    }
    return 0;
}


Display *display;

int main()
{
    XEvent ev;

    Display *dpy = display = XOpenDisplay(NULL);
    if (!dpy) {
        perror("XOpenDisplay()");
        return 1;
    }

    int x11_fd = ConnectionNumber(dpy);
    Window root = DefaultRootWindow(dpy);

    /*
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Shift_L), Mod1Mask, root, False,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Alt_L), ShiftMask, root, False,
             GrabModeAsync, GrabModeAsync);
    */

    // Xkb
    int xkb_event, xkb_error, xkb_opcode, xkb_major, xkb_minor;

    if (XkbQueryExtension(dpy, &xkb_opcode, &xkb_event, &xkb_error,
                          &xkb_major, &xkb_minor) != True) {
        perror("XkbQueryExtension()");
        xkb_event = -1;
    }
    printf("xkb_opcode=%d xkb_event=%d xkb_error=%d xkb_major=%d xkb_minor=%d\n",
        xkb_opcode, xkb_event, xkb_error, xkb_major, xkb_minor);

    XkbSelectEvents(dpy, XkbUseCoreKbd, XkbBellNotifyMask, XkbBellNotifyMask);
    XkbChangeEnabledControls(dpy, XkbUseCoreKbd, XkbAudibleBellMask, XkbAudibleBellMask);

    unsigned int auto_ctrls, auto_values;
    auto_ctrls = auto_values = XkbAudibleBellMask;
    XkbSetAutoResetControls(dpy, XkbAudibleBellMask, &auto_ctrls, &auto_values);

    // XScreenSaver
    int xss_event = xss_printinfo(dpy);
    XScreenSaverSelectInput(dpy, root, ScreenSaverNotifyMask|ScreenSaverCycleMask);
    xss_printinfo(dpy);

    int timer_fd = timer_start(2000);

    // Event loop
    for (;;) {
        struct pollfd fds[2];

        fds[0].fd = timer_fd;
        fds[0].events = POLLIN;
        fds[1].fd = x11_fd;
        fds[1].events = POLLIN;

        int err = poll(fds, 2, -1);
        if (err < 0 && errno == EINTR) {
            continue;
        } else if (err < 0) {
            perror("poll()");
            exit(EXIT_FAILURE);
        }
        if (fds[0].revents) {    // Timer event received
            //printf("Timer\n");

            struct timeval tv;
            if (gettimeofday(&tv, NULL) < 0) {
                perror("gettimeofday()");
                exit(EXIT_FAILURE);
            }
            if (timediff_ms(&play_bell_tv, &tv) > aplaypop_idle)
                aplaypop_close();

            timer_next(fds[0].fd);
        }
        if (fds[1].revents) {    // X event received
            XNextEvent(dpy, &ev);

            if (ev.type == KeyPress) {
                printf("KeyPress\n");

            } else if (ev.type == KeyRelease) {
                printf("KeyRelease\n");

            } else if (ev.type == xkb_event) {
                XkbEvent *kev = (XkbEvent *)&ev;
                printf("ev=%#x\n", ev.type);

                const char *xkbNames[] = { "XkbNewKeyboardNotify", "XkbMapNotify", "XkbStateNotify",
                "XkbControlsNotify", "XkbIndicatorStateNotify", "XkbIndicatorMapNotify", "XkbNamesNotify",
                "XkbCompatMapNotify", "XkbBellNotify", "XkbActionMessage", "XkbAccessXNotify", "XkbExtensionDeviceNotify" };

                printf("ev.xkb_type='%s'\n", xkbNames[kev->any.xkb_type]);
                if (kev->any.xkb_type != XkbBellNotify)
                    continue;

                XkbBellNotifyEvent *bne = &kev->bell;

                printf("XkbBellNotifyEvent {\n");
                printf("  int      type = %#x;      /* XkbAnyEvent */\n", bne->type);
                printf("  ulong    serial = %lu;    /* of last req processed by server */\n", bne->serial);
                printf("  Bool     send_event = %d; /* is this from a SendEvent request? */\n", bne->send_event);
                printf("  Display *display = %p;    /* Display the event was read from */\n", bne->display);
                printf("  Time     time = %ld;      /* milliseconds */\n", bne->time);
                printf("  int      xkb_type = '%s'; /* XkbBellNotify */\n", xkbNames[bne->xkb_type]);
                printf("  int      device = %d;     /* device ID */\n", bne->device);
                printf("  int      percent = %d;    /* requested volume as a %% of maximum */\n", bne->percent);
                printf("  int      pitch = %d;      /* requested pitch in Hz */\n", bne->pitch);
                printf("  int      duration = %d;   /* requested duration in useconds */\n", bne->duration);
                printf("  int      bell_class = %d; /* (input extension) feedback class */\n", bne->bell_class);
                printf("  int      bell_id = %d;    /* (input extension) ID of feedback */\n", bne->bell_id);
                printf("  Atom     name = %ld;      /* name of requested bell */\n", bne->name);
                printf("  Window   window = %#lx;   /* window associated with event */\n", bne->window);
                printf("  Bool     event_only = %d; /* 'event only' requested */\n", bne->event_only);
                printf("}\n");

                if (play_bell(bne->percent) < 0) {
                    perror("Ringing bell failed, reverting to X11 device bell.");
                    XkbForceDeviceBell(dpy, bne->device, bne->bell_class, bne->bell_id, bne->percent);
                }

                xss_printinfo(dpy);

            } else if (ev.type == xss_event) {
                XScreenSaverNotifyEvent *se = (XScreenSaverNotifyEvent *) &ev;
                printf("XScreenSaverNotifyEvent {\n");
                printf("  int type = %#x;             /* of event */\n", se->type);
                printf("  unsigned long serial = %lu; /* # of last request processed by server */\n", se->serial);
                printf("  Bool send_event = %d;       /* true if this came frome a SendEvent request */\n", se->send_event);
                printf("  Display *display = %p;      /* Display the event was read from */\n", se->display);
                printf("  Window window = %#lx;       /* screen saver window */\n", se->window);
                printf("  Window root = %#lx;         /* root window of event screen */\n", se->root);
                printf("  int state = '%s';           /* ScreenSaver{Off,On,Disabled} */\n", stateNames[se->state]);
                printf("  int kind = '%s';            /* ScreenSaver{Blanked,Internal,External} */\n", kindNames[se->kind]);
                printf("  Bool forced = %d;           /* extents of new region */\n", se->forced);
                printf("  Time time = %ld;            /* event timestamp */\n", se->time);
                printf("}\n");

                if (se->state == ScreenSaverOn) {
                    system("bash -i -c 'FREEZE -v; exit'");
                    system("echo 7 |sudo tee '/sys/devices/LNXSYSTM:00/LNXCPU:00/thermal_cooling/cur_state'");
                } else if (se->state == ScreenSaverOff) {
                    system("bash -i -c 'FREEZE -v -CONT; exit'");
                    system("echo 0 |sudo tee '/sys/devices/LNXSYSTM:00/LNXCPU:00/thermal_cooling/cur_state'");
                }
                xss_state = se->state;

            } else {
                printf("ev=%#x\n", ev.type);
            }
        }

    }

    timer_stop(timer_fd);
    return 0;
}
