/*
 *	$XConsortium: data.h /main/12 1996/01/14 16:52:48 kaleb $
 *	$Id: data.h,v 6.2 1996/07/02 05:01:31 kagotani Rel $
 */
/*
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 *
 *                         All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Digital Equipment
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 *
 *
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifndef KTERM_NOTEK
extern TekLink *TekRefresh;
extern XPoint T_box2[];
extern XPoint T_box3[];
extern XPoint T_boxlarge[];
extern XPoint T_boxsmall[];
#endif /* !KTERM_NOTEK */
extern XPoint VTbox[];
#ifdef KTERM_MBCS
extern XPoint VTwbox[];
#endif /* KTERM_MBCS */
#ifndef KTERM_NOTEK
extern Char *Tbptr;
extern Char *Tbuffer;
extern Char *Tpushb;
extern Char *Tpushback;
#endif /* !KTERM_NOTEK */
extern Char *bptr;
#ifdef ALLOWLOGGING
extern char log_def_name[];
#endif
extern char *ptydev;
extern char *ttydev;
extern char *xterm_name;
#ifdef KTERM_MBCS
extern Char *buffer;
#else /* !KTERM_MBCS */
extern Char buffer[];
#endif /* !KTERM_MBCS */
#ifndef KTERM_NOTEK
extern int T_lastx;
extern int T_lasty;
extern int Tbcnt;
extern int Ttoggled;
#endif /* !KTERM_NOTEK */
extern int am_slave;
extern int bcnt;
#ifdef DEBUG
extern int debug;
#endif	/* DEBUG */
extern int errno;
extern int max_plus1;
extern int switchfb[];

extern int waitingForTrackInfo;

extern EventMode eventMode;

extern GC visualBellGC;

extern int VTgcFontMask;
#ifndef KTERM_NOTEK
extern int TEKgcFontMask;
#endif /* !KTERM_NOTEK */

extern XtermWidget term;
#ifndef KTERM_NOTEK
extern TekWidget tekWidget;
#endif /* !KTERM_NOTEK */

#ifdef XKB
#include <X11/extensions/XKBbells.h>
#else
#define	XkbBI_Info			0
#define	XkbBI_MinorError		1
#define	XkbBI_MajorError		2
#define	XkbBI_TerminalBell		9
#define	XkbBI_MarginBell		10
#define	XkbBI_CursorStuck		11
#endif
