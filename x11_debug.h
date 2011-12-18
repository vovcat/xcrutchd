/*    con_x11_debug.h
 *
 *    Copyright (c) 2003, vc@dataforce.net
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#ifndef __X11_DEBUG_H
#define __X11_DEBUG_H

#ifdef DEBUG

void debugf(const char *filename, const char *fmt, ...);
void debugf_set_names(const char *names);
#define DEBUGF(ARGS...) debugf(__FILE__, ARGS)

const char *debX11_atom(Atom atom);
const char *debX11_win(Window w);
void debX11ev(Window dst, XEvent event, const char* dir,
              Window src, const char *s, ...);
void debX11str(Window win, const char *s, ...);

#else

#define DEBUGF(ARGS...)

#define debX11ev(win, event, dir, src, s, ARGS...)
#define debX11str(win, s, ARGS...)

#endif // DEBUG

#endif // __X11_DEBUG_H
