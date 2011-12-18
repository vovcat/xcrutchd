// gcc -W -Wall -o xcrutchd xcrutchd.c aplaypop.c -lX11 -lXss -lasound && ./xcrutchd

#include <stdio.h>
#include <stdlib.h> // system()

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <X11/extensions/saver.h>
#include <X11/extensions/scrnsaver.h>
#include <X11/extensions/dpms.h>

#include "aplaypop.h"

int play_bell(int percent) {
    aplaypop_open();
    aplaypop();
    aplaypop_close();
    return 0;
}

const char *stateNames[] = { "Off", "On", "Cycle", "Disable" };
const char *kindNames[] = { "Blanked", "Internal", "External" };

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

    XFree(xss_info);

    return xss_event;
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

    Window root = DefaultRootWindow(dpy);

    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Shift_L), Mod1Mask, root, False,
             GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Alt_L), ShiftMask, root, False,
             GrabModeAsync, GrabModeAsync);

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

    //aplaypop_open();

    for (;;) {
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
        } else {
            printf("ev=%#x\n", ev.type);
        }
    }

    //aplaypop_close();
    return 0;
}
