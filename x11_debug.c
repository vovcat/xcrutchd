#include <stdio.h>  // sprintf(), vsnprintf()
#include <stdarg.h> // va_*()
#include <string.h> // strdup()
#include <stdlib.h> // malloc()

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "x11_debug.h"

extern Display *display;

const struct {
    int type;
    const char *name;
} XEventNames[] = {
#undef TAB
#define TAB(x) { x, #x }
    TAB(KeyPress),
    TAB(KeyRelease),
    TAB(ButtonPress),
    TAB(ButtonRelease),
    TAB(MotionNotify),
    TAB(EnterNotify),
    TAB(LeaveNotify),
    TAB(FocusIn),
    TAB(FocusOut),
    TAB(KeymapNotify),
    TAB(Expose),
    TAB(GraphicsExpose),
    TAB(NoExpose),
    TAB(VisibilityNotify),
    TAB(CreateNotify),
    TAB(DestroyNotify),
    TAB(UnmapNotify),
    TAB(MapNotify),
    TAB(MapRequest),
    TAB(ReparentNotify),
    TAB(ConfigureNotify),
    TAB(ConfigureRequest),
    TAB(GravityNotify),
    TAB(ResizeRequest),
    TAB(CirculateNotify),
    TAB(CirculateRequest),
    TAB(PropertyNotify),
    TAB(SelectionClear),
    TAB(SelectionRequest),
    TAB(SelectionNotify),
    TAB(ColormapNotify),
    TAB(ClientMessage),
    TAB(MappingNotify),
};

const char *debX11_event(const XEvent *event) {
    char buf[32];
    size_t i;
    for (i = 0; i <sizeof(XEventNames)/sizeof(XEventNames[0]); i++)
        if (XEventNames[i].type == event->type)
            return XEventNames[i].name;
    sprintf(buf, "Unknown(%d)", event->type);
    return strdup(buf);
}

const struct {
    Atom atom;
    const char *name;
} XAtomNames[] = {
#undef TAB
#define TAB(x) { XA_##x, #x }
    TAB(PRIMARY),
    TAB(SECONDARY),
    TAB(ARC),
    TAB(ATOM),
    TAB(BITMAP),
    TAB(CARDINAL),
    TAB(COLORMAP),
    TAB(CURSOR),
    TAB(CUT_BUFFER0),
    TAB(CUT_BUFFER1),
    TAB(CUT_BUFFER2),
    TAB(CUT_BUFFER3),
    TAB(CUT_BUFFER4),
    TAB(CUT_BUFFER5),
    TAB(CUT_BUFFER6),
    TAB(CUT_BUFFER7),
    TAB(DRAWABLE),
    TAB(FONT),
    TAB(INTEGER),
    TAB(PIXMAP),
    TAB(POINT),
    TAB(RECTANGLE),
    TAB(RESOURCE_MANAGER),
    TAB(RGB_COLOR_MAP),
    TAB(RGB_BEST_MAP),
    TAB(RGB_BLUE_MAP),
    TAB(RGB_DEFAULT_MAP),
    TAB(RGB_GRAY_MAP),
    TAB(RGB_GREEN_MAP),
    TAB(RGB_RED_MAP),
    TAB(STRING),
    TAB(VISUALID),
    TAB(WINDOW),
    TAB(WM_COMMAND),
    TAB(WM_HINTS),
    TAB(WM_CLIENT_MACHINE),
    TAB(WM_ICON_NAME),
    TAB(WM_ICON_SIZE),
    TAB(WM_NAME),
    TAB(WM_NORMAL_HINTS),
    TAB(WM_SIZE_HINTS),
    TAB(WM_ZOOM_HINTS),
    TAB(MIN_SPACE),
    TAB(NORM_SPACE),
    TAB(MAX_SPACE),
    TAB(END_SPACE),
    TAB(SUPERSCRIPT_X),
    TAB(SUPERSCRIPT_Y),
    TAB(SUBSCRIPT_X),
    TAB(SUBSCRIPT_Y),
    TAB(UNDERLINE_POSITION),
    TAB(UNDERLINE_THICKNESS),
    TAB(STRIKEOUT_ASCENT),
    TAB(STRIKEOUT_DESCENT),
    TAB(ITALIC_ANGLE),
    TAB(X_HEIGHT),
    TAB(QUAD_WIDTH),
    TAB(WEIGHT),
    TAB(POINT_SIZE),
    TAB(RESOLUTION),
    TAB(COPYRIGHT),
    TAB(NOTICE),
    TAB(FONT_NAME),
    TAB(FAMILY_NAME),
    TAB(FULL_NAME),
    TAB(CAP_HEIGHT),
    TAB(WM_CLASS),
    TAB(WM_TRANSIENT_FOR),
};

const char *debX11_atom(Atom atom) {
    if (atom == None) return "None";
    char buf[32];
    size_t i;
    for (i = 0; i < sizeof(XAtomNames)/sizeof(XAtomNames[0]); i++)
        if (XAtomNames[i].atom == atom)
            return XAtomNames[i].name;
    return XGetAtomName(display, atom);
    sprintf(buf, "Atom[%ld]", atom);
    return strdup(buf);
}

const char *debX11_win(Window w) {
    Status res;
    char out[64], buf[64];
    char *name;

    if (w == None)
        return "None";
    //if (!(res = XGetIconName(display, w, &name)) || name == NULL)
    if (!(res = XFetchName(display, w, &name)) || name == NULL)
        sprintf(name = buf, "0x%lX %d", w, res);

    sprintf(out, "win(%.12s)", name);
    return strdup(out);
}

#ifdef DEBUG
void debX11ev(Window dst, XEvent *event, const char *dir,
              Window src, const char *s, ...) {
    va_list ap;
    char msgbuf[4096];

    va_start(ap, s); vsprintf(msgbuf, s, ap); va_end(ap);

    if (src != None)
        DEBUGF("%s %s %s: %s%s{ %s } %s",
               debX11_win(dst), dir, debX11_win(src),
               debX11_event(event), event->xany.send_event ? "+": "",
               debX11_win(event->xany.window), msgbuf);
    else
        DEBUGF("%s: %s%s{ %s } %s", debX11_win(dst),
               debX11_event(event), event->xany.send_event ? "+": "",
               debX11_win(event->xany.window), msgbuf);
}

void debX11str(Window win, const char *s, ...) {
    va_list ap;
    char msgbuf[4096];

    va_start(ap, s); vsprintf(msgbuf, s, ap); va_end(ap);
    DEBUGF("%s: %s", debX11_win(win), msgbuf);
}

static char **debug_names = NULL;

void debugf(const char *filename, const char *fmt, ...) {
    if (debug_names == NULL)
        return;

    int skip = 1;
    if (skip && *debug_names == NULL) // Means debug ALL files
        skip = 0;
    char **p;
    for (p = debug_names; skip && *p != NULL; p++)
        if (!strncmp(filename, *p, strlen(*p)) || !strcmp(filename, __FILE__))
            skip = 0;
    if (skip)
        return;

    va_list ap;
    char msgbuf[4096];

    va_start(ap, fmt);
    vsnprintf(msgbuf, sizeof(msgbuf)-1, fmt, ap);
    msgbuf[sizeof(msgbuf)-1] = '\0';
    va_end(ap);

    fprintf(stderr, "%s: %s\n", filename, msgbuf);
}

void debugf_set_names(const char *arg) {
    if (arg == NULL) {
        // Means debug ALL files
        debug_names = (char **)malloc(sizeof(char *));
        *debug_names = NULL;
        return;
    }

    // count comma-separated names in arg
    char *p, *names = strdup(arg);
    int names_len = 0;
    for (p = names; p != NULL; p = strchr(p, ',')) {
        names_len++;
        p++;
    }

    debug_names = (char **)malloc(names_len * sizeof(char *));

    // split comma-separated names into debug_names array
    names_len = 0;
    p = names;
    while (*p) {
        debug_names[names_len++] = p;
        if ((p = strchr(p, ',')) == NULL)
            break;
        *p++ = '\0';
    }
    debug_names[names_len] = NULL;

    // print result
    char **pp;
    for (pp = debug_names; *pp != NULL; pp++)
        DEBUGF("debug_names[]=%s", *pp);
}

#endif
