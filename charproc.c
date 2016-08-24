/*
 * $XConsortium: charproc.c /main/191 1996/01/23 11:34:26 kaleb $
 * $Id: charproc.c,v 6.5 1996/07/12 05:01:34 kagotani Rel $
 */

/*
 
Copyright (c) 1988  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.

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

/* charproc.c */

#include "ptyx.h"
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/StringDefs.h>
#include <X11/Xmu/Atoms.h>
#include <X11/Xmu/CharSet.h>
#include <X11/Xmu/Converters.h>
#include <X11/Xaw/XawImP.h>
#ifndef NO_XPOLL_H
#include <X11/Xpoll.h>
#else
#define XFD_COPYSET(src, dst) memmove((dst), (src), sizeof(fd_set))
#define Select select
#endif
#include <stdio.h>
#include <errno.h>
#include <setjmp.h>
#include <ctype.h>
#include "VTparse.h"
#include "data.h"
#include "error.h"
#include "menu.h"
#include "main.h"

/*
 * Check for both EAGAIN and EWOULDBLOCK, because some supposedly POSIX
 * systems are broken and return EWOULDBLOCK when they should return EAGAIN.
 * Note that this macro may evaluate its argument more than once.
 */
#if defined(EAGAIN) && defined(EWOULDBLOCK)
#define E_TEST(err) ((err) == EAGAIN || (err) == EWOULDBLOCK)
#else
#ifdef EAGAIN
#define E_TEST(err) ((err) == EAGAIN)
#else
#define E_TEST(err) ((err) == EWOULDBLOCK)
#endif
#endif

#ifndef KTERM_NOTEK
extern jmp_buf VTend;
#endif /* !KTERM_NOTEK */

extern XtAppContext app_con;
extern Widget toplevel;
extern void exit();
extern char *malloc();
extern char *realloc();
extern fd_set Select_mask;
extern fd_set X_mask;
extern fd_set pty_mask;

static void VTallocbuf();
static int finput();
static void dotext();
static void WriteText();
static void ToAlternate();
static void FromAlternate();
static void update_font_info();

static void bitset(), bitclr();
    
#define	DEFAULT		-1
#define	TEXT_BUF_SIZE	256
#define TRACKTIMESEC	4L
#define TRACKTIMEUSEC	0L
#define BELLSUPPRESSMSEC 200

#define XtNalwaysHighlight "alwaysHighlight"
#define XtNappcursorDefault "appcursorDefault"
#define XtNappkeypadDefault "appkeypadDefault"
#define XtNbellSuppressTime "bellSuppressTime"
#define XtNboldFont "boldFont"
#define XtNc132 "c132"
#define XtNcharClass "charClass"
#define XtNcurses "curses"
#define XtNhpLowerleftBugCompat "hpLowerleftBugCompat"
#define XtNcursorColor "cursorColor"
#ifdef KTERM_COLOR
#define XtNtextColor0 "textColor0"
#define XtNtextColor1 "textColor1"
#define XtNtextColor2 "textColor2"
#define XtNtextColor3 "textColor3"
#define XtNtextColor4 "textColor4"
#define XtNtextColor5 "textColor5"
#define XtNtextColor6 "textColor6"
#define XtNtextColor7 "textColor7"
#endif /* KTERM_COLOR */
#define XtNcutNewline "cutNewline"
#define XtNcutToBeginningOfLine "cutToBeginningOfLine"
#define XtNeightBitInput "eightBitInput"
#define XtNeightBitOutput "eightBitOutput"
#define XtNgeometry "geometry"
#define XtNtekGeometry "tekGeometry"
#define XtNinternalBorder "internalBorder"
#define XtNjumpScroll "jumpScroll"
#ifdef ALLOWLOGGING
#define XtNlogFile "logFile"
#define XtNlogging "logging"
#define XtNlogInhibit "logInhibit"
#endif
#define XtNloginShell "loginShell"
#define XtNmarginBell "marginBell"
#define XtNpointerColor "pointerColor"
#define XtNpointerColorBackground "pointerColorBackground"
#define XtNpointerShape "pointerShape"
#define XtNmultiClickTime "multiClickTime"
#define XtNmultiScroll "multiScroll"
#define XtNnMarginBell "nMarginBell"
#define XtNresizeGravity "resizeGravity"
#define XtNreverseWrap "reverseWrap"
#define XtNautoWrap "autoWrap"
#define XtNsaveLines "saveLines"
#define XtNscrollBar "scrollBar"
#define XtNscrollTtyOutput "scrollTtyOutput"
#define XtNscrollKey "scrollKey"
#define XtNscrollLines "scrollLines"
#define XtNscrollPos "scrollPos"
#define XtNsignalInhibit "signalInhibit"
#ifdef STATUSLINE
#define XtNstatusLine "statusLine"
#define XtNstatusNormal "statusNormal"
#endif /* STATUSLINE */
#define XtNtekInhibit "tekInhibit"
#define XtNtekSmall "tekSmall"
#define XtNtekStartup "tekStartup"
#define XtNtiteInhibit "titeInhibit"
#define XtNvisualBell "visualBell"
#define XtNallowSendEvents "allowSendEvents"
#ifdef KTERM
#define XtNromanKanaFont "romanKanaFont"
#define XtNromanKanaBoldFont "romanKanaBoldFont"
# ifdef KTERM_MBCS
#define XtNkanjiFont "kanjiFont"
#define XtNkanjiBoldFont "kanjiBoldFont"
# endif /* KTERM_MBCS */
# ifdef KTERM_KANJIMODE
#define XtNkanjiMode "kanjiMode"
# endif /* KTERM_KANJIMODE */
#define XtNfontList "fontList"
#define XtNboldFontList "boldFontList"
#define XtNlineSpace "lineSpace"
#endif /* KTERM */

#define XtCAlwaysHighlight "AlwaysHighlight"
#define XtCAppcursorDefault "AppcursorDefault"
#define XtCAppkeypadDefault "AppkeypadDefault"
#define XtCBellSuppressTime "BellSuppressTime"
#define XtCBoldFont "BoldFont"
#define XtCC132 "C132"
#define XtCCharClass "CharClass"
#define XtCCurses "Curses"
#define XtCHpLowerleftBugCompat "HpLowerleftBugCompat"
#define XtCCutNewline "CutNewline"
#define XtCCutToBeginningOfLine "CutToBeginningOfLine"
#define XtCEightBitInput "EightBitInput"
#define XtCEightBitOutput "EightBitOutput"
#define XtCGeometry "Geometry"
#define XtCJumpScroll "JumpScroll"
#ifdef ALLOWLOGGING
#define XtCLogfile "Logfile"
#define XtCLogging "Logging"
#define XtCLogInhibit "LogInhibit"
#endif
#define XtCLoginShell "LoginShell"
#define XtCMarginBell "MarginBell"
#define XtCMultiClickTime "MultiClickTime"
#define XtCMultiScroll "MultiScroll"
#define XtCColumn "Column"
#define XtCResizeGravity "ResizeGravity"
#define XtCReverseWrap "ReverseWrap"
#define XtCAutoWrap "AutoWrap"
#define XtCSaveLines "SaveLines"
#define XtCScrollBar "ScrollBar"
#define XtCScrollLines "ScrollLines"
#define XtCScrollPos "ScrollPos"
#define XtCScrollCond "ScrollCond"
#define XtCSignalInhibit "SignalInhibit"
#ifdef STATUSLINE
#define XtCStatusLine "StatusLine"
#define XtCStatusNormal "StatusNormal"
#endif /* STATUSLINE */
#ifndef KTERM_NOTEK
#define XtCTekInhibit "TekInhibit"
#define XtCTekSmall "TekSmall"
#define XtCTekStartup "TekStartup"
#endif /* !KTERM_NOTEK */
#define XtCTiteInhibit "TiteInhibit"
#define XtCVisualBell "VisualBell"
#define XtCAllowSendEvents "AllowSendEvents"
#ifdef KTERM
#define XtCRomanKanaFont "RomanKanaFont"
# ifdef KTERM_MBCS
#define XtCKanjiFont "KanjiFont"
# endif /* KTERM_MBCS */
# ifdef KTERM_KANJIMODE
#define XtCKanjiMode "KanjiMode"
# endif /* KTERM_KANJIMODE */
#define XtCFontList "FontList"
#define XtCLineSpace "LineSpace"
#endif /* KTERM */

#define	doinput()		(bcnt-- > 0 ? *bptr++ : in_put())

static int nparam;
static ANSI reply;
static int param[NPARAM];

static unsigned long ctotal;
static unsigned long ntotal;
static jmp_buf vtjmpbuf;

extern int groundtable[];
extern int csitable[];
extern int dectable[];
extern int eigtable[];
extern int esctable[];
extern int iestable[];
extern int igntable[];
extern int scrtable[];
extern int scstable[];
#ifdef KTERM_MBCS
extern int mbcstable[];
extern int smbcstable[];
static Char pending_byte;
#endif /* KTERM_MBCS */


/* event handlers */
extern void HandleKeyPressed(), HandleEightBitKeyPressed();
extern void HandleStringEvent();
extern void HandleEnterWindow();
extern void HandleLeaveWindow();
extern void HandleBellPropertyChange();
extern void HandleFocusChange();
static void HandleKeymapChange();
extern void HandleInsertSelection();
extern void HandleSelectStart(), HandleKeyboardSelectStart();
extern void HandleSelectExtend(), HandleSelectSet();
extern void HandleSelectEnd(), HandleKeyboardSelectEnd();
extern void HandleStartExtend(), HandleKeyboardStartExtend();
static void HandleBell();
static void HandleVisualBell();
static void HandleIgnore();
extern void HandleSecure();
extern void HandleScrollForward();
extern void HandleScrollBack();
extern void HandleCreateMenu(), HandlePopupMenu();
extern void HandleSetFont();
extern void SetVTFont();
#ifdef KTERM_XIM
extern void HandleOpenIM();
extern void HandleCloseIM();
#endif /* KTERM_XIM */
#ifdef KTERM_KINPUT2
extern void HandleBeginConversion();
#endif /* KTERM_KINPUT2 */

extern Boolean SendMousePosition();
extern void ScrnSetAttributes();

/*
 * NOTE: VTInitialize zeros out the entire ".screen" component of the 
 * XtermWidget, so make sure to add an assignment statement in VTInitialize() 
 * for each new ".screen" field added to this resource list.
 */

/* Defaults */
static  Boolean	defaultFALSE	   = FALSE;
static  Boolean	defaultTRUE	   = TRUE;
static  int	defaultBorderWidth = DEFBORDERWIDTH;
static  int	defaultIntBorder   = DEFBORDER;
static  int	defaultSaveLines   = SAVELINES;
static	int	defaultScrollLines = SCROLLLINES;
static  int	defaultNMarginBell = N_MARGINBELL;
static  int	defaultMultiClickTime = MULTICLICKTIME;
#ifdef KTERM
static  int	defaultLineSpace    = LINESPACE;
#endif /* KTERM */
static  int	defaultBellSuppressTime = BELLSUPPRESSMSEC;
static	char *	_Font_Selected_ = "yes";  /* string is arbitrary */
#ifdef KTERM_XIM
static	char *	defaultEucJPLocale = EUCJPLOCALE;
#endif /* KTERM_XIM */

/*
 * Warning, the following must be kept under 1024 bytes or else some 
 * compilers (particularly AT&T 6386 SVR3.2) will barf).  Workaround is to
 * declare a static buffer and copy in at run time (the the Athena text widget
 * does).  Yuck.
 */
static char defaultTranslations[] =
#ifdef KTERM_KINPUT2
"\
           Ctrl <KeyPress> Kanji:begin-conversion(_JAPANESE_CONVERSION) \n\
          Shift <KeyPress> Prior:scroll-back(1,halfpage) \n\
           Shift <KeyPress> Next:scroll-forw(1,halfpage) \n\
         Shift <KeyPress> Select:select-cursor-start() select-cursor-end(PRIMARY , CUT_BUFFER0) \n\
         Shift <KeyPress> Insert:insert-selection(PRIMARY, CUT_BUFFER0) \n\
                ~Meta <KeyPress>:insert-seven-bit() \n\
                 Meta <KeyPress>:insert-eight-bit() \n\
                !Ctrl <Btn1Down>:popup-menu(mainMenu) \n\
           !Lock Ctrl <Btn1Down>:popup-menu(mainMenu) \n\
 !Lock Ctrl @Num_Lock <Btn1Down>:popup-menu(mainMenu) \n\
     ! @Num_Lock Ctrl <Btn1Down>:popup-menu(mainMenu) \n\
                ~Meta <Btn1Down>:select-start() \n\
              ~Meta <Btn1Motion>:select-extend() \n\
                !Ctrl <Btn2Down>:popup-menu(vtMenu) \n\
           !Lock Ctrl <Btn2Down>:popup-menu(vtMenu) \n\
 !Lock Ctrl @Num_Lock <Btn2Down>:popup-menu(vtMenu) \n\
     ! @Num_Lock Ctrl <Btn2Down>:popup-menu(vtMenu) \n\
          ~Ctrl ~Meta <Btn2Down>:ignore() \n\
            ~Ctrl ~Meta <Btn2Up>:insert-selection(PRIMARY, CUT_BUFFER0) \n\
                !Ctrl <Btn3Down>:popup-menu(fontMenu) \n\
           !Lock Ctrl <Btn3Down>:popup-menu(fontMenu) \n\
 !Lock Ctrl @Num_Lock <Btn3Down>:popup-menu(fontMenu) \n\
     ! @Num_Lock Ctrl <Btn3Down>:popup-menu(fontMenu) \n\
          ~Ctrl ~Meta <Btn3Down>:start-extend() \n\
              ~Meta <Btn3Motion>:select-extend()      \n\
                         <BtnUp>:select-end(PRIMARY, CUT_BUFFER0) \n\
                       <BtnDown>:bell(0) \
";
#else /* !KTERM_KINPUT2 */
"\
          Shift <KeyPress> Prior:scroll-back(1,halfpage) \n\
           Shift <KeyPress> Next:scroll-forw(1,halfpage) \n\
         Shift <KeyPress> Select:select-cursor-start() select-cursor-end(PRIMARY , CUT_BUFFER0) \n\
         Shift <KeyPress> Insert:insert-selection(PRIMARY, CUT_BUFFER0) \n\
                ~Meta <KeyPress>:insert-seven-bit() \n\
                 Meta <KeyPress>:insert-eight-bit() \n\
                !Ctrl <Btn1Down>:popup-menu(mainMenu) \n\
           !Lock Ctrl <Btn1Down>:popup-menu(mainMenu) \n\
 !Lock Ctrl @Num_Lock <Btn1Down>:popup-menu(mainMenu) \n\
     ! @Num_Lock Ctrl <Btn1Down>:popup-menu(mainMenu) \n\
                ~Meta <Btn1Down>:select-start() \n\
              ~Meta <Btn1Motion>:select-extend() \n\
                !Ctrl <Btn2Down>:popup-menu(vtMenu) \n\
           !Lock Ctrl <Btn2Down>:popup-menu(vtMenu) \n\
 !Lock Ctrl @Num_Lock <Btn2Down>:popup-menu(vtMenu) \n\
     ! @Num_Lock Ctrl <Btn2Down>:popup-menu(vtMenu) \n\
          ~Ctrl ~Meta <Btn2Down>:ignore() \n\
            ~Ctrl ~Meta <Btn2Up>:insert-selection(PRIMARY, CUT_BUFFER0) \n\
                !Ctrl <Btn3Down>:popup-menu(fontMenu) \n\
           !Lock Ctrl <Btn3Down>:popup-menu(fontMenu) \n\
 !Lock Ctrl @Num_Lock <Btn3Down>:popup-menu(fontMenu) \n\
     ! @Num_Lock Ctrl <Btn3Down>:popup-menu(fontMenu) \n\
          ~Ctrl ~Meta <Btn3Down>:start-extend() \n\
              ~Meta <Btn3Motion>:select-extend()      \n\
                         <BtnUp>:select-end(PRIMARY, CUT_BUFFER0) \n\
                       <BtnDown>:bell(0) \
";
#endif /* !KTERM_KINPUT2 */

static XtActionsRec actionsList[] = { 
    { "bell",		  HandleBell },
    { "create-menu",	  HandleCreateMenu },
    { "ignore",		  HandleIgnore },
    { "insert",		  HandleKeyPressed },  /* alias for insert-seven-bit */
    { "insert-seven-bit", HandleKeyPressed },
    { "insert-eight-bit", HandleEightBitKeyPressed },
    { "insert-selection", HandleInsertSelection },
    { "keymap", 	  HandleKeymapChange },
    { "popup-menu",	  HandlePopupMenu },
    { "secure",		  HandleSecure },
    { "select-start",	  HandleSelectStart },
    { "select-extend",	  HandleSelectExtend },
    { "select-end",	  HandleSelectEnd },
    { "select-set",	  HandleSelectSet },
    { "select-cursor-start",	  HandleKeyboardSelectStart },
    { "select-cursor-end",	  HandleKeyboardSelectEnd },
    { "set-vt-font",	  HandleSetFont },
    { "start-extend",	  HandleStartExtend },
    { "start-cursor-extend",	  HandleKeyboardStartExtend },
    { "string",		  HandleStringEvent },
    { "scroll-forw",	  HandleScrollForward },
    { "scroll-back",	  HandleScrollBack },
    /* menu actions */
    { "allow-send-events",	HandleAllowSends },
    { "set-visual-bell",	HandleSetVisualBell },
#ifdef ALLOWLOGGING
    { "set-logging",		HandleLogging },
#endif
    { "redraw",			HandleRedraw },
    { "send-signal",		HandleSendSignal },
    { "quit",			HandleQuit },
    { "set-scrollbar",		HandleScrollbar },
    { "set-jumpscroll",		HandleJumpscroll },
    { "set-reverse-video",	HandleReverseVideo },
    { "set-autowrap",		HandleAutoWrap },
    { "set-reversewrap",	HandleReverseWrap },
    { "set-autolinefeed",	HandleAutoLineFeed },
    { "set-appcursor",		HandleAppCursor },
    { "set-appkeypad",		HandleAppKeypad },
    { "set-scroll-on-key",	HandleScrollKey },
    { "set-scroll-on-tty-output",	HandleScrollTtyOutput },
    { "set-allow132",		HandleAllow132 },
    { "set-cursesemul",		HandleCursesEmul },
    { "set-marginbell",		HandleMarginBell },
    { "set-altscreen",		HandleAltScreen },
    { "soft-reset",		HandleSoftReset },
    { "hard-reset",		HandleHardReset },
    { "clear-saved-lines",	HandleClearSavedLines },
#ifndef KTERM_NOTEK
    { "set-terminal-type",	HandleSetTerminalType },
    { "set-visibility",		HandleVisibility },
    { "set-tek-text",		HandleSetTekText },
    { "tek-page",		HandleTekPage },
    { "tek-reset",		HandleTekReset },
    { "tek-copy",		HandleTekCopy },
#endif /* !KTERM_NOTEK */
    { "visual-bell",		HandleVisualBell },
#ifdef STATUSLINE
    { "set-statusline",		HandleStatusLine },
    { "set-reversestatus",	HandleStatusReverse },
#endif /* STATUSLINE */
#ifdef KTERM_KANJIMODE
    { "set-kanji-mode",		HandleSetKanjiMode },
#endif /* KTERM_KANJIMODE */
#ifdef KTERM_XIM
    { "open-im",		HandleOpenIM },
    { "close-im",		HandleCloseIM },
#endif /* KTERM_XIM */
#ifdef KTERM_KINPUT2
    { "begin-conversion",	HandleBeginConversion },
#endif /* KTERM_KINPUT2 */
};

static XtResource resources[] = {
#ifdef KTERM
{XtNfont, XtCFont, XtRString, sizeof(String),
	XtOffset(XtermWidget, screen._menu_font_names[F_ISO8859_1][fontMenu_fontdefault]),
	XtRString, (XtPointer) NULL},
{XtNboldFont, XtCFont, XtRString, sizeof(String),
	XtOffset(XtermWidget, screen._menu_bfont_names[F_ISO8859_1][fontMenu_fontdefault]),
	XtRString, (XtPointer) NULL},
{XtNromanKanaFont, XtCRomanKanaFont, XtRString, sizeof(String),
	XtOffset(XtermWidget, screen._menu_font_names[F_JISX0201_0][fontMenu_fontdefault]),
	XtRString, (XtPointer) NULL},
{XtNromanKanaBoldFont, XtCRomanKanaFont, XtRString, sizeof(String),
	XtOffset(XtermWidget, screen._menu_bfont_names[F_JISX0201_0][fontMenu_fontdefault]),
	XtRString, (XtPointer) NULL},
#  ifdef KTERM_MBCS
{XtNkanjiFont, XtCKanjiFont, XtRString, sizeof(String),
	XtOffset(XtermWidget, screen._menu_font_names[F_JISX0208_0][fontMenu_fontdefault]),
	XtRString, (XtPointer) NULL},
{XtNkanjiBoldFont, XtCKanjiFont, XtRString, sizeof(String),
	XtOffset(XtermWidget, screen._menu_bfont_names[F_JISX0208_0][fontMenu_fontdefault]),
	XtRString, (XtPointer) NULL},
#  endif /* KTERM_MBCS */
#  ifdef KTERM_KANJIMODE
{XtNkanjiMode, XtCKanjiMode, XtRString, sizeof(char *),
	XtOffset(XtermWidget, misc.k_m), XtRString,
	(XtPointer) NULL},
#  endif /* KTERM_KANJIMODE */
{XtNfontList, XtCFontList, XtRString, sizeof(String),
	XtOffset(XtermWidget, screen.menu_font_list[fontMenu_fontdefault]), XtRString,
	(XtPointer) NULL},
{XtNboldFontList, XtCFontList, XtRString, sizeof(String),
	XtOffset(XtermWidget, screen.menu_bfont_list[fontMenu_fontdefault]), XtRString,
	(XtPointer) NULL},
{XtNlineSpace, XtCLineSpace, XtRInt, sizeof(int),
	XtOffset(XtermWidget, screen.linespace),
	XtRInt, (XtPointer) &defaultLineSpace},
#else /* !KTERM */
{XtNfont, XtCFont, XtRString, sizeof(char *),
	XtOffsetOf(XtermWidgetRec, misc.f_n), XtRString,
	DEFFONT},
{XtNboldFont, XtCBoldFont, XtRString, sizeof(char *),
	XtOffsetOf(XtermWidgetRec, misc.f_b), XtRString,
	DEFBOLDFONT},
#endif /* !KTERM */
{XtNc132, XtCC132, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, screen.c132),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNcharClass, XtCCharClass, XtRString, sizeof(char *),
	XtOffsetOf(XtermWidgetRec, screen.charClass),
	XtRString, (XtPointer) NULL},
{XtNcurses, XtCCurses, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, screen.curses),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNcutNewline, XtCCutNewline, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, screen.cutNewline),
	XtRBoolean, (XtPointer) &defaultTRUE},
{XtNcutToBeginningOfLine, XtCCutToBeginningOfLine, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, screen.cutToBeginningOfLine),
	XtRBoolean, (XtPointer) &defaultTRUE},
{XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
	XtOffsetOf(XtermWidgetRec, core.background_pixel),
	XtRString, "XtDefaultBackground"},
{XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
	XtOffsetOf(XtermWidgetRec, screen.foreground),
	XtRString, "XtDefaultForeground"},
{XtNcursorColor, XtCForeground, XtRPixel, sizeof(Pixel),
	XtOffsetOf(XtermWidgetRec, screen.cursorcolor),
	XtRString, "XtDefaultForeground"},
#ifdef KTERM_COLOR
{XtNtextColor0, XtCForeground, XtRPixel, sizeof(Pixel),
	XtOffsetOf(XtermWidgetRec, screen.textcolor[0]),
	XtRString, "Black"},
{XtNtextColor1, XtCForeground, XtRPixel, sizeof(Pixel),
	XtOffsetOf(XtermWidgetRec, screen.textcolor[1]),
	XtRString, "Red"},
{XtNtextColor2, XtCForeground, XtRPixel, sizeof(Pixel),
	XtOffsetOf(XtermWidgetRec, screen.textcolor[2]),
	XtRString, "Green"},
{XtNtextColor3, XtCForeground, XtRPixel, sizeof(Pixel),
	XtOffsetOf(XtermWidgetRec, screen.textcolor[3]),
	XtRString, "Yellow"},
{XtNtextColor4, XtCForeground, XtRPixel, sizeof(Pixel),
	XtOffsetOf(XtermWidgetRec, screen.textcolor[4]),
	XtRString, "Blue"},
{XtNtextColor5, XtCForeground, XtRPixel, sizeof(Pixel),
	XtOffsetOf(XtermWidgetRec, screen.textcolor[5]),
	XtRString, "Magenta"},
{XtNtextColor6, XtCForeground, XtRPixel, sizeof(Pixel),
	XtOffsetOf(XtermWidgetRec, screen.textcolor[6]),
	XtRString, "Cyan"},
{XtNtextColor7, XtCForeground, XtRPixel, sizeof(Pixel),
	XtOffsetOf(XtermWidgetRec, screen.textcolor[7]),
	XtRString, "White"},
#endif /* KTERM_COLOR */
{XtNeightBitInput, XtCEightBitInput, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, screen.input_eight_bits), 
	XtRBoolean, (XtPointer) &defaultTRUE},
{XtNeightBitOutput, XtCEightBitOutput, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, screen.output_eight_bits), 
	XtRBoolean, (XtPointer) &defaultTRUE},
{XtNgeometry,XtCGeometry, XtRString, sizeof(char *),
	XtOffsetOf(XtermWidgetRec, misc.geo_metry),
	XtRString, (XtPointer) NULL},
{XtNalwaysHighlight,XtCAlwaysHighlight,XtRBoolean,
        sizeof(Boolean),XtOffsetOf(XtermWidgetRec, screen.always_highlight),
        XtRBoolean, (XtPointer) &defaultFALSE},
{XtNappcursorDefault,XtCAppcursorDefault,XtRBoolean,
        sizeof(Boolean),XtOffsetOf(XtermWidgetRec, misc.appcursorDefault),
        XtRBoolean, (XtPointer) &defaultFALSE},
{XtNappkeypadDefault,XtCAppkeypadDefault,XtRBoolean,
        sizeof(Boolean),XtOffsetOf(XtermWidgetRec, misc.appkeypadDefault),
        XtRBoolean, (XtPointer) &defaultFALSE},
{XtNbellSuppressTime, XtCBellSuppressTime, XtRInt, sizeof(int),
        XtOffsetOf(XtermWidgetRec, screen.bellSuppressTime),
        XtRInt, (XtPointer) &defaultBellSuppressTime},
{XtNtekGeometry,XtCGeometry, XtRString, sizeof(char *),
	XtOffsetOf(XtermWidgetRec, misc.T_geometry),
	XtRString, (XtPointer) NULL},
{XtNinternalBorder,XtCBorderWidth,XtRInt, sizeof(int),
	XtOffsetOf(XtermWidgetRec, screen.border),
	XtRInt, (XtPointer) &defaultIntBorder},
{XtNjumpScroll, XtCJumpScroll, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, screen.jumpscroll),
	XtRBoolean, (XtPointer) &defaultTRUE},
#ifdef ALLOWLOGGING
{XtNlogFile, XtCLogfile, XtRString, sizeof(char *),
	XtOffsetOf(XtermWidgetRec, screen.logfile),
	XtRString, (XtPointer) NULL},
{XtNlogging, XtCLogging, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, misc.log_on),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNlogInhibit, XtCLogInhibit, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, misc.logInhibit),
	XtRBoolean, (XtPointer) &defaultFALSE},
#endif
{XtNloginShell, XtCLoginShell, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, misc.login_shell),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNmarginBell, XtCMarginBell, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, screen.marginbell),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNpointerColor, XtCForeground, XtRPixel, sizeof(Pixel),
	XtOffsetOf(XtermWidgetRec, screen.mousecolor),
	XtRString, "XtDefaultForeground"},
{XtNpointerColorBackground, XtCBackground, XtRPixel, sizeof(Pixel),
	XtOffsetOf(XtermWidgetRec, screen.mousecolorback),
	XtRString, "XtDefaultBackground"},
{XtNpointerShape,XtCCursor, XtRCursor, sizeof(Cursor),
	XtOffsetOf(XtermWidgetRec, screen.pointer_cursor),
	XtRString, (XtPointer) "xterm"},
{XtNmultiClickTime,XtCMultiClickTime, XtRInt, sizeof(int),
	XtOffsetOf(XtermWidgetRec, screen.multiClickTime),
	XtRInt, (XtPointer) &defaultMultiClickTime},
{XtNmultiScroll,XtCMultiScroll, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, screen.multiscroll),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNnMarginBell,XtCColumn, XtRInt, sizeof(int),
	XtOffsetOf(XtermWidgetRec, screen.nmarginbell),
	XtRInt, (XtPointer) &defaultNMarginBell},
{XtNreverseVideo,XtCReverseVideo,XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, misc.re_verse),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNresizeGravity, XtCResizeGravity, XtRGravity, sizeof(XtGravity),
	XtOffsetOf(XtermWidgetRec, misc.resizeGravity),
	XtRImmediate, (XtPointer) SouthWestGravity},
{XtNreverseWrap,XtCReverseWrap, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, misc.reverseWrap),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNautoWrap,XtCAutoWrap, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, misc.autoWrap),
	XtRBoolean, (XtPointer) &defaultTRUE},
{XtNsaveLines, XtCSaveLines, XtRInt, sizeof(int),
	XtOffsetOf(XtermWidgetRec, screen.savelines),
	XtRInt, (XtPointer) &defaultSaveLines},
{XtNscrollBar, XtCScrollBar, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, misc.scrollbar),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNscrollTtyOutput,XtCScrollCond, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, screen.scrollttyoutput),
	XtRBoolean, (XtPointer) &defaultTRUE},
{XtNscrollKey, XtCScrollCond, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, screen.scrollkey),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNscrollLines, XtCScrollLines, XtRInt, sizeof(int),
	XtOffsetOf(XtermWidgetRec, screen.scrolllines),
	XtRInt, (XtPointer) &defaultScrollLines},
{XtNsignalInhibit,XtCSignalInhibit,XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, misc.signalInhibit),
	XtRBoolean, (XtPointer) &defaultFALSE},
#ifdef STATUSLINE
{XtNstatusLine, XtCStatusLine, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, misc.statusline),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNstatusNormal, XtCStatusNormal, XtRBoolean, sizeof(Boolean),
	XtOffset(XtermWidget, misc.statusnormal),
	XtRBoolean, (XtPointer) &defaultFALSE},
#endif /* STATUSLINE */
#ifndef KTERM_NOTEK
{XtNtekInhibit, XtCTekInhibit, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, misc.tekInhibit),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNtekSmall, XtCTekSmall, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, misc.tekSmall),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNtekStartup, XtCTekStartup, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, screen.TekEmu),
	XtRBoolean, (XtPointer) &defaultFALSE},
#endif /* !KTERM_NOTEK */
{XtNtiteInhibit, XtCTiteInhibit, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, misc.titeInhibit),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNvisualBell, XtCVisualBell, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, screen.visualbell),
	XtRBoolean, (XtPointer) &defaultFALSE},
{XtNallowSendEvents, XtCAllowSendEvents, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, screen.allowSendEvents),
	XtRBoolean, (XtPointer) &defaultFALSE},
#ifdef KTERM
{"dynamicFontLoad", "DynamicFontLoad", XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, screen.dynamic_font_load),
	XtRBoolean, (XtPointer) &defaultTRUE},
{"fontList1", "FontList1", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_font_list[fontMenu_font1]),
	XtRString, (XtPointer) NULL},
{"fontList2", "FontList2", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_font_list[fontMenu_font2]),
	XtRString, (XtPointer) NULL},
{"fontList3", "FontList3", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_font_list[fontMenu_font3]),
	XtRString, (XtPointer) NULL},
{"fontList4", "FontList4", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_font_list[fontMenu_font4]),
	XtRString, (XtPointer) NULL},
{"fontList5", "FontList5", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_font_list[fontMenu_font5]),
	XtRString, (XtPointer) NULL},
{"fontList6", "FontList6", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_font_list[fontMenu_font6]),
	XtRString, (XtPointer) NULL},
{"boldFontList1", "BoldFontList1", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_bfont_list[fontMenu_font1]),
	XtRString, (XtPointer) NULL},
{"boldFontList2", "BoldFontList2", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_bfont_list[fontMenu_font2]),
	XtRString, (XtPointer) NULL},
{"boldFontList3", "BoldFontList3", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_bfont_list[fontMenu_font3]),
	XtRString, (XtPointer) NULL},
{"boldFontList4", "BoldFontList4", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_bfont_list[fontMenu_font4]),
	XtRString, (XtPointer) NULL},
{"boldFontList5", "BoldFontList5", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_bfont_list[fontMenu_font5]),
	XtRString, (XtPointer) NULL},
{"boldFontList6", "BoldFontList6", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_bfont_list[fontMenu_font6]),
	XtRString, (XtPointer) NULL},
{"font1", "Font1", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_ISO8859_1][fontMenu_font1]),
	XtRString, (XtPointer) NULL},
{"font2", "Font2", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_ISO8859_1][fontMenu_font2]),
	XtRString, (XtPointer) NULL},
{"font3", "Font3", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_ISO8859_1][fontMenu_font3]),
	XtRString, (XtPointer) NULL},
{"font4", "Font4", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_ISO8859_1][fontMenu_font4]),
	XtRString, (XtPointer) NULL},
{"font5", "Font5", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_ISO8859_1][fontMenu_font5]),
	XtRString, (XtPointer) NULL},
{"font6", "Font6", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_ISO8859_1][fontMenu_font6]),
	XtRString, (XtPointer) NULL},
{"boldFont1", "BoldFont1", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_ISO8859_1][fontMenu_font1]),
	XtRString, (XtPointer) NULL},
{"boldFont2", "BoldFont2", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_ISO8859_1][fontMenu_font2]),
	XtRString, (XtPointer) NULL},
{"boldFont3", "BoldFont3", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_ISO8859_1][fontMenu_font3]),
	XtRString, (XtPointer) NULL},
{"boldFont4", "BoldFont4", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_ISO8859_1][fontMenu_font4]),
	XtRString, (XtPointer) NULL},
{"boldFont5", "BoldFont5", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_ISO8859_1][fontMenu_font5]),
	XtRString, (XtPointer) NULL},
{"boldFont6", "BoldFont6", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_ISO8859_1][fontMenu_font6]),
	XtRString, (XtPointer) NULL},
{"romanKanaFont1", "RomanKanaFont1", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_JISX0201_0][fontMenu_font1]),
	XtRString, (XtPointer) NULL},
{"romanKanaFont2", "RomanKanaFont2", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_JISX0201_0][fontMenu_font2]),
	XtRString, (XtPointer) NULL},
{"romanKanaFont3", "RomanKanaFont3", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_JISX0201_0][fontMenu_font3]),
	XtRString, (XtPointer) NULL},
{"romanKanaFont4", "RomanKanaFont4", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_JISX0201_0][fontMenu_font4]),
	XtRString, (XtPointer) NULL},
{"romanKanaFont5", "RomanKanaFont5", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_JISX0201_0][fontMenu_font5]),
	XtRString, (XtPointer) NULL},
{"romanKanaFont6", "RomanKanaFont6", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_JISX0201_0][fontMenu_font6]),
	XtRString, (XtPointer) NULL},
{"romanKanaBoldFont1", "RomanKanaBoldFont1", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_JISX0201_0][fontMenu_font1]),
	XtRString, (XtPointer) NULL},
{"romanKanaBoldFont2", "RomanKanaBoldFont2", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_JISX0201_0][fontMenu_font2]),
	XtRString, (XtPointer) NULL},
{"romanKanaBoldFont3", "RomanKanaBoldFont3", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_JISX0201_0][fontMenu_font3]),
	XtRString, (XtPointer) NULL},
{"romanKanaBoldFont4", "RomanKanaBoldFont4", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_JISX0201_0][fontMenu_font4]),
	XtRString, (XtPointer) NULL},
{"romanKanaBoldFont5", "RomanKanaBoldFont5", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_JISX0201_0][fontMenu_font5]),
	XtRString, (XtPointer) NULL},
{"romanKanaBoldFont6", "RomanKanaBoldFont6", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_JISX0201_0][fontMenu_font6]),
	XtRString, (XtPointer) NULL},
# ifdef KTERM_MBCS
{"kanjiFont1", "KanjiFont1", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_JISX0208_0][fontMenu_font1]),
	XtRString, (XtPointer) NULL},
{"kanjiFont2", "KanjiFont2", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_JISX0208_0][fontMenu_font2]),
	XtRString, (XtPointer) NULL},
{"kanjiFont3", "KanjiFont3", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_JISX0208_0][fontMenu_font3]),
	XtRString, (XtPointer) NULL},
{"kanjiFont4", "KanjiFont4", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_JISX0208_0][fontMenu_font4]),
	XtRString, (XtPointer) NULL},
{"kanjiFont5", "KanjiFont5", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_JISX0208_0][fontMenu_font5]),
	XtRString, (XtPointer) NULL},
{"kanjiFont6", "KanjiFont6", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_font_names[F_JISX0208_0][fontMenu_font6]),
	XtRString, (XtPointer) NULL},
{"kanjiBoldFont1", "KanjiBoldFont1", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_JISX0208_0][fontMenu_font1]),
	XtRString, (XtPointer) NULL},
{"kanjiBoldFont2", "KanjiBoldFont2", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_JISX0208_0][fontMenu_font2]),
	XtRString, (XtPointer) NULL},
{"kanjiBoldFont3", "KanjiBoldFont3", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_JISX0208_0][fontMenu_font3]),
	XtRString, (XtPointer) NULL},
{"kanjiBoldFont4", "KanjiBoldFont4", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_JISX0208_0][fontMenu_font4]),
	XtRString, (XtPointer) NULL},
{"kanjiBoldFont5", "KanjiBoldFont5", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_JISX0208_0][fontMenu_font5]),
	XtRString, (XtPointer) NULL},
{"kanjiBoldFont6", "KanjiBoldFont6", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen._menu_bfont_names[F_JISX0208_0][fontMenu_font6]),
	XtRString, (XtPointer) NULL},
# endif /* KTERM_MBCS */
#else /* !KTERM */
{"font1", "Font1", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_font_names[fontMenu_font1]),
	XtRString, (XtPointer) NULL},
{"font2", "Font2", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_font_names[fontMenu_font2]),
	XtRString, (XtPointer) NULL},
{"font3", "Font3", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_font_names[fontMenu_font3]),
	XtRString, (XtPointer) NULL},
{"font4", "Font4", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_font_names[fontMenu_font4]),
	XtRString, (XtPointer) NULL},
{"font5", "Font5", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_font_names[fontMenu_font5]),
	XtRString, (XtPointer) NULL},
{"font6", "Font6", XtRString, sizeof(String),
	XtOffsetOf(XtermWidgetRec, screen.menu_font_names[fontMenu_font6]),
	XtRString, (XtPointer) NULL},
#endif /* !KTERM */
{XtNinputMethod, XtCInputMethod, XtRString, sizeof(char*),
	XtOffsetOf(XtermWidgetRec, misc.input_method),
	XtRString, (XtPointer)NULL},
#ifdef KTERM_XIM
{XtNpreeditType, XtCPreeditType, XtRString, sizeof(char*),
	XtOffsetOf(XtermWidgetRec, misc.preedit_type),
	XtRString, (XtPointer)"OverTheSpot,Root"},
{XtNopenIm, XtCOpenIm, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, misc.open_im),
	XtRImmediate, (XtPointer)FALSE},
{"eucJPLocale", "EucJPLocale", XtRString, sizeof(char*),
	XtOffsetOf(XtermWidgetRec, misc.eucjp_locale),
	XtRString, (XtPointer)EUCJPLOCALE},
#else /* !KTERM_XIM */
{XtNpreeditType, XtCPreeditType, XtRString, sizeof(char*),
	XtOffsetOf(XtermWidgetRec, misc.preedit_type),
	XtRString, (XtPointer)"Root"},
{XtNopenIm, XtCOpenIm, XtRBoolean, sizeof(Boolean),
	XtOffsetOf(XtermWidgetRec, misc.open_im),
	XtRImmediate, (XtPointer)TRUE},
#endif /* !KTERM_XIM */
};

static void VTClassInit();
static void VTInitialize();
static void VTRealize();
static void VTExpose();
static void VTResize();
static void VTDestroy();
static Boolean VTSetValues();
static void VTInitI18N();

static WidgetClassRec xtermClassRec = {
  {
/* core_class fields */	
    /* superclass	  */	(WidgetClass) &widgetClassRec,
    /* class_name	  */	"VT100",
    /* widget_size	  */	sizeof(XtermWidgetRec),
    /* class_initialize   */    VTClassInit,
    /* class_part_initialize */ NULL,
    /* class_inited       */	FALSE,
    /* initialize	  */	VTInitialize,
    /* initialize_hook    */    NULL,				
    /* realize		  */	VTRealize,
    /* actions		  */	actionsList,
    /* num_actions	  */	XtNumber(actionsList),
    /* resources	  */	resources,
    /* num_resources	  */	XtNumber(resources),
    /* xrm_class	  */	NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	FALSE,
    /* compress_enterleave */   TRUE,
    /* visible_interest	  */	FALSE,
    /* destroy		  */	VTDestroy,
    /* resize		  */	VTResize,
    /* expose		  */	VTExpose,
    /* set_values	  */	VTSetValues,
    /* set_values_hook    */    NULL,
    /* set_values_almost  */    NULL,
    /* get_values_hook    */    NULL,
    /* accept_focus	  */	NULL,
    /* version            */    XtVersion,
    /* callback_offsets   */    NULL,
    /* tm_table           */    defaultTranslations,
    /* query_geometry     */    XtInheritQueryGeometry,
    /* display_accelerator*/    XtInheritDisplayAccelerator,
    /* extension          */    NULL
  }
};

WidgetClass xtermWidgetClass = (WidgetClass)&xtermClassRec;

#ifdef KTERM
doSS(gset)
short gset;
{
	register Char c, min, max, print_min, print_max;
	register Char *cp = bptr;

	min = 0x20 | *cp & 0x80;
	max = 0x7f | *cp & 0x80;
	if (gset & CS96) {
		print_min = min;
		print_max = max;
	} else {
		print_min = min + 1;
		print_max = max - 1;
	}
# ifdef KTERM_MBCS
	if (gset & MBCS) {
		if (bcnt > 0) {
			/* first byte */
			c = *cp++;
			if (bcnt == 1) {
				/*
				 * Incomplete multi-byte character.
				 * Preserve and skip its first byte.
				 */
				pending_byte = c;
				goto donottext;
			}
			if (c < print_min || print_max < c) {
				/*
				 * Illegal multi-byte character. Skip it.
				 */
				cp++;
				goto donottext;
			}
			/* second byte */
			c = *cp++;
			if (c < print_min || print_max < c) {
				/*
				 * Illegal multi-byte character. Skip it.
				 */
				goto donottext;
			}
		}
	} else
# endif /* KTERM_MBCS */
	{
		if (bcnt > 0) {
			c = *cp++;
			if (c < print_min || print_max < c)
				goto donottext;
		}
	}
	dotext(&term->screen, term->flags, gset, bptr, cp);
    donottext:
	bcnt -= cp - bptr;
	bptr = cp;
}

doLS(gset)
short gset;
{
	register Char c, min, max, print_min, print_max;
	register Char *cp = bptr;
	register int cnt = bcnt > TEXT_BUF_SIZE ? TEXT_BUF_SIZE : bcnt;
				/* TEXT_BUF_SIZE must be an even number */
	int skip = 0;

	min = 0x20 | *cp & 0x80;
	max = 0x7f | *cp & 0x80;
	if (gset & CS96) {
		print_min = min;
		print_max = max;
	} else {
		print_min = min + 1;
		print_max = max - 1;
	}
# ifdef KTERM_MBCS
	if (gset & MBCS) {
		while (cnt > 0) {
			/* first byte */
			c = *cp;
			if (c < print_min || print_max < c)
				break;
			cnt--;
			cp++;
			if (cnt == 0) { /* must be (bcnt == cp-bptr) */
				/*
				 * Incomplete multi-byte character.
				 * Preserve and skip its first byte.
				 */
				pending_byte = c;
				skip = 1;
				break;
			}
			/* second byte */
			c = *cp;
			cnt--;
			cp++;
			if (c < print_min || print_max < c) {
				/*
				 * Illegal multi-byte character. Skip it.
				 */
				skip = 2;
				break;
			}
		}
	} else
# endif /* KTERM_MBCS */
	{
		if (!(gset & CS96) && !(*cp & 0x80)) { /* SBCS&CS94:GL */
			print_min = min; /* SPACE is printable */
		}
		while (cnt > 0) {
			c = *cp;
			if (c < print_min || print_max < c)
				break;
			cnt--;
			cp++;
		}
	}
	dotext(&term->screen, term->flags, gset, bptr, cp-skip);

# ifdef KTERM_MBCS
	/*
	 * Print printable SPCs in MBCS and CS94 and GL
	 */
	if (print_min == 0x21) { /* implies MBCS and CS94 and GL */
		/*
		 * Print SPACEs left
		 */
		bcnt -= cp - bptr;
		bptr = cp;
		while (cnt > 0) {
			c = *cp;
			if (c != 0x20)
				break;
			cnt--;
			cp++;
		}
		dotext(&term->screen, term->flags, GSET_ASCII, bptr, cp);
	}
# endif /* KTERM_MBCS */

	/*
	 * Skip unprintable SPCs and DELs
	 */
	while (cnt > 0) {
		c = *cp;
		if (c < min || print_min <= c && c <= print_max || max < c)
			break;
		cnt--;
		cp++;
	}
	bcnt -= cp - bptr;
	bptr = cp;
}

# ifdef KTERM_KANJIMODE
doSJIS()
{
	Char dotextbuf[TEXT_BUF_SIZE];
	register Char c1, c2;
	register Char *cp = bptr;
	register Char *dcp = dotextbuf;
	register int cnt = bcnt > TEXT_BUF_SIZE ? TEXT_BUF_SIZE : bcnt;
				/* TEXT_BUF_SIZE must be an even number */

#  define SJIS1(c) ((0x81 <= c && c <= 0x9F) || (0xE0 <= c && c <= 0xEF))
#  define SJIS2(c) (0x40 <= c && c <= 0xFC && c != 0x7F)
	while (cnt > 0) {
		/* first byte */
		c1 = *cp;
		if (!SJIS1(c1))
			break;
		cnt--;
		cp++;
		if (cnt == 0) { /* must be (bcnt == cp-bptr) */
			/*
			 * Incomplete multi-byte character.
			 * Preserve and skip its first byte.
			 */
			pending_byte = c1;
			break;
		}
		/* second byte */
		c2 = *cp;
		if (!SJIS2(c2)) {
			/*
			 * Illegal shift-jis character. Skip it.
			 */
			break;
		}
		cnt--;
		cp++;
		/* SJIS to JIS code conversion */
		if (c1 <= 0x9f)	c1 = (c1-0x81)*2 + 0x21;
		else		c1 = (c1-0xc1)*2 + 0x21;
		if (c2 <= 0x7e)	     c2 -= 0x1f;
		else if (c2 <= 0x9e) c2 -= 0x20;
		else		     c2 -= 0x7e, c1 += 1;
		/* copy to buffer */
		*dcp++ = c1;
		*dcp++ = c2;
	}
	dotext(&term->screen, term->flags, GSET_KANJI, dotextbuf, dcp);
	bcnt -= cp - bptr;
	bptr = cp;
}
# endif /* KTERM_KANJIMODE */
#endif /* KTERM */

static void VTparse()
{
	register TScreen *screen = &term->screen;
	register int *parsestate = groundtable;
	register unsigned int c;
	register unsigned char *cp;
	register int row, col, top, bot, scstype;
#ifdef KTERM
	register Bchr *xp;
	Char cs96;
# ifdef KTERM_MBCS
	Char mbcs;
# endif /* KTERM_MBCS */
#endif /* KTERM */
	extern int TrackMouse();

	if(setjmp(vtjmpbuf))
		parsestate = groundtable;
#ifdef KTERM_MBCS
	pending_byte = 0;
#endif /* KTERM_MBCS */
	for( ; ; ) {
#ifdef KTERM
		if (cs96
# ifdef KTERM_MBCS
		  | mbcs
		 && parsestate != mbcstable
		 && parsestate != smbcstable
# endif /* KTERM_MBCS */
		 && parsestate != scstable) {
			cs96 = 0;
# ifdef KTERM_MBCS
			mbcs = 0;
# endif /* KTERM_MBCS */
		}
#endif /* KTERM */
#ifdef KTERM_KANJIMODE
	        c = doinput();
		if (term->flags & SJIS_KANJI && SJIS1(c)) {
			bcnt++;
			*--bptr = c;
			doSJIS();
			screen->curss = 0;
			continue;
		}
	        switch (parsestate[c]) {
#else /* !KTERM_KANJIMODE */
	        switch (parsestate[c = doinput()]) {
#endif /* !KTERM_KANJIMODE */
		 case CASE_PRINT:
			/* printable characters */
#ifdef KTERM
			bcnt++;
			*--bptr = c;
			if(screen->curss) {
				doSS(screen->gsets[screen->curss]);
# ifdef KTERM_MBCS
				if (pending_byte == 0)
# endif /* KTERM_MBCS */
					screen->curss = 0;
				break; /* switch */
			}
			doLS(screen->gsets[c & 0x80 ? screen->curgr
						    : screen->curgl]);
#else /* !KTERM */
			top = bcnt > TEXT_BUF_SIZE ? TEXT_BUF_SIZE : bcnt;
			cp = bptr;
			*--bptr = c;
			while(top > 0 && isprint(*cp & 0x7f)) {
				top--;
				bcnt--;
				cp++;
			}
			if(screen->curss) {
				dotext(screen, term->flags,
				 screen->gsets[screen->curss], bptr, bptr + 1);
				screen->curss = 0;
				bptr++;
			}
			if(bptr < cp)
				dotext(screen, term->flags,
				 screen->gsets[screen->curgl], bptr, cp);
			bptr = cp;
#endif /* !KTERM */
			break;

		 case CASE_GROUND_STATE:
			/* exit ignore mode */
			parsestate = groundtable;
			break;

		 case CASE_IGNORE_STATE:
			/* Ies: ignore anything else */
			parsestate = igntable;
			break;

		 case CASE_IGNORE_ESC:
			/* Ign: escape */
			parsestate = iestable;
			break;

		 case CASE_IGNORE:
			/* Ignore character */
			break;

		 case CASE_BELL:
			/* bell */
			Bell(XkbBI_TerminalBell,0);
			break;

		 case CASE_BS:
			/* backspace */
			CursorBack(screen, 1);
			break;

		 case CASE_CR:
			/* carriage return */
			CarriageReturn(screen);
			parsestate = groundtable;
			break;

		 case CASE_ESC:
			/* escape */
			parsestate = esctable;
			break;

		 case CASE_VMOT:
			/*
			 * form feed, line feed, vertical tab
			 */
#ifdef STATUSLINE
		      if (!screen->instatus)
#endif /* STATUSLINE */
			Index(screen, 1);
			if (term->flags & LINEFEED)
				CarriageReturn(screen);
#ifdef KTERM_XAW3D
			if (XtAppPending(app_con) > 0 ||
#else /* !KTERM_XAW3D */
			if (QLength(screen->display) > 0 ||
#endif /* !KTERM_XAW3D */
			    GetBytesAvailable (ConnectionNumber(screen->display)) > 0)
			  xevents();
			parsestate = groundtable;
			break;

		 case CASE_TAB:
			/* tab */
			screen->cur_col = TabNext(term->tabs, screen->cur_col);
			if (screen->cur_col > screen->max_col)
				screen->cur_col = screen->max_col;
			break;

		 case CASE_SI:
			screen->curgl = 0;
			break;

		 case CASE_SO:
			screen->curgl = 1;
			break;

		 case CASE_SCR_STATE:
			/* enter scr state */
			parsestate = scrtable;
			break;

#ifdef KTERM_MBCS
		 case CASE_MBCS:
			/* enter mbcs state */
			mbcs = MBCS;
			scstype = 0; /* for ESC-$-F */
			parsestate = mbcstable;
			break;
#endif /* KTERM_MBCS */

#ifdef KTERM
		 case CASE_SCS_STATE:
			/* enter scs state */
			if (c >= ',') {	/* , - . / */
				cs96 = CS96;
				scstype = c - ',';
			} else {	/* ( ) * + */
				scstype = c - '(';
			}
# ifdef KTERM_MBCS
			if (mbcs)
				parsestate = smbcstable;
			else
# endif /* KTERM_MBCS */
			parsestate = scstable;
			break;
#else /* !KTERM */
		 case CASE_SCS0_STATE:
			/* enter scs state 0 */
			scstype = 0;
			parsestate = scstable;
			break;

		 case CASE_SCS1_STATE:
			/* enter scs state 1 */
			scstype = 1;
			parsestate = scstable;
			break;

		 case CASE_SCS2_STATE:
			/* enter scs state 2 */
			scstype = 2;
			parsestate = scstable;
			break;

		 case CASE_SCS3_STATE:
			/* enter scs state 3 */
			scstype = 3;
			parsestate = scstable;
			break;
#endif /* !KTERM */

		 case CASE_ESC_IGNORE:
			/* unknown escape sequence */
			parsestate = eigtable;
			break;

		 case CASE_ESC_DIGIT:
			/* digit in csi or dec mode */
			if((row = param[nparam - 1]) == DEFAULT)
				row = 0;
			param[nparam - 1] = 10 * row + (c - '0');
			break;

		 case CASE_ESC_SEMI:
			/* semicolon in csi or dec mode */
			if (nparam < NPARAM)
			    param[nparam++] = DEFAULT;
			break;

		 case CASE_DEC_STATE:
			/* enter dec mode */
			parsestate = dectable;
			break;

		 case CASE_ICH:
			/* ICH */
			if((row = param[0]) < 1)
				row = 1;
			InsertChar(screen, row);
			parsestate = groundtable;
			break;

		 case CASE_CUU:
			/* CUU */
			if((row = param[0]) < 1)
				row = 1;
#ifdef STATUSLINE
		      if (!screen->instatus)
#endif /* STATUSLINE */
			CursorUp(screen, row);
			parsestate = groundtable;
			break;

		 case CASE_CUD:
			/* CUD */
			if((row = param[0]) < 1)
				row = 1;
#ifdef STATUSLINE
		      if (!screen->instatus)
#endif /* STATUSLINE */
			CursorDown(screen, row);
			parsestate = groundtable;
			break;

		 case CASE_CUF:
			/* CUF */
			if((row = param[0]) < 1)
				row = 1;
			CursorForward(screen, row);
			parsestate = groundtable;
			break;

		 case CASE_CUB:
			/* CUB */
			if((row = param[0]) < 1)
				row = 1;
			CursorBack(screen, row);
			parsestate = groundtable;
			break;

		 case CASE_CUP:
			/* CUP | HVP */
			if((row = param[0]) < 1)
				row = 1;
			if(nparam < 2 || (col = param[1]) < 1)
				col = 1;
#ifdef STATUSLINE
			if (screen->instatus) {
			    col--;
			    if (col > screen->cur_col)
				CursorForward(screen, col - screen->cur_col);
			    else if (col < screen->cur_col)
				CursorBack(screen, screen->cur_col - col);
			} else
#endif /* STATUSLINE */
			CursorSet(screen, row-1, col-1, term->flags);
			parsestate = groundtable;
			break;

		 case CASE_HP_BUGGY_LL:
			/* Some HP-UX applications have the bug that they
			   assume ESC F goes to the lower left corner of
			   the screen, regardless of what terminfo says. */
			if (screen->hp_ll_bc)
			    CursorSet(screen, screen->max_row, 0, term->flags);
			parsestate = groundtable;
			break;

		 case CASE_ED:
			/* ED */
			switch (param[0]) {
			 case DEFAULT:
			 case 0:
#ifdef STATUSLINE
			      if (screen->instatus)
				ClearRight(screen);
			      else
#endif /* STATUSLINE */
				ClearBelow(screen);
				break;

			 case 1:
#ifdef STATUSLINE
			      if (screen->instatus)
				ClearLeft(screen);
			      else
#endif /* STATUSLINE */
				ClearAbove(screen);
				break;

			 case 2:
#ifdef STATUSLINE
			      if (screen->instatus)
				ClearLine(screen);
			      else
#endif /* STATUSLINE */
				ClearScreen(screen);
				break;
			}
			parsestate = groundtable;
			break;

		 case CASE_EL:
			/* EL */
			switch (param[0]) {
			 case DEFAULT:
			 case 0:
				ClearRight(screen);
				break;
			 case 1:
				ClearLeft(screen);
				break;
			 case 2:
				ClearLine(screen);
				break;
			}
			parsestate = groundtable;
			break;

		 case CASE_IL:
			/* IL */
			if((row = param[0]) < 1)
				row = 1;
#ifdef STATUSLINE
		      if (!screen->instatus)
#endif /* STATUSLINE */
			InsertLine(screen, row);
			parsestate = groundtable;
			break;

		 case CASE_DL:
			/* DL */
			if((row = param[0]) < 1)
				row = 1;
#ifdef STATUSLINE
		      if (!screen->instatus)
#endif /* STATUSLINE */
			DeleteLine(screen, row);
			parsestate = groundtable;
			break;

		 case CASE_DCH:
			/* DCH */
			if((row = param[0]) < 1)
				row = 1;
			DeleteChar(screen, row);
			parsestate = groundtable;
			break;

		 case CASE_TRACK_MOUSE:
		 	/* Track mouse as long as in window and between
			   specified rows */
			TrackMouse(param[0], param[2]-1, param[1]-1,
			 param[3]-1, param[4]-2);
			break;

		 case CASE_DECID:
			param[0] = -1;		/* Default ID parameter */
			/* Fall through into ... */
		 case CASE_DA1:
			/* DA1 */
			if (param[0] <= 0) {	/* less than means DEFAULT */
				reply.a_type   = CSI;
				reply.a_pintro = '?';
				reply.a_nparam = 2;
				reply.a_param[0] = 1;		/* VT102 */
				reply.a_param[1] = 2;		/* VT102 */
				reply.a_inters = 0;
				reply.a_final  = 'c';
				unparseseq(&reply, screen->respond);
			}
			parsestate = groundtable;
			break;

		 case CASE_TBC:
			/* TBC */
			if ((row = param[0]) <= 0) /* less than means default */
				TabClear(term->tabs, screen->cur_col);
			else if (row == 3)
				TabZonk(term->tabs);
			parsestate = groundtable;
			break;

		 case CASE_SET:
			/* SET */
			ansi_modes(term, bitset);
			parsestate = groundtable;
			break;

		 case CASE_RST:
			/* RST */
			ansi_modes(term, bitclr);
			parsestate = groundtable;
			break;

		 case CASE_SGR:
			/* SGR */
			for (row=0; row<nparam; ++row) {
				switch (param[row]) {
				 case DEFAULT:
				 case 0:
#ifdef KTERM_COLOR
					term->flags &= ~(INVERSE|BOLD|UNDERLINE|FORECOLORED|BACKCOLORED);
#else /* !KTERM_COLOR */
					term->flags &= ~(INVERSE|BOLD|UNDERLINE);
#endif /* !KTERM_COLOR */
					break;
				 case 1:
				 case 5:	/* Blink, really.	*/
					term->flags |= BOLD;
					break;
				 case 4:	/* Underscore		*/
					term->flags |= UNDERLINE;
					break;
				 case 7:
					term->flags |= INVERSE;
					break;
#ifdef KTERM_COLOR
				 case 30:
				 case 31:
				 case 32:
				 case 33:
				 case 34:
				 case 35:
				 case 36:
				 case 37:
					term->flags &= ~FORECOLORMASK;
					term->flags |= FORECOLORED|FORECOLOR(param[row]-30);
					break;
				 case 39:
					term->flags &= ~FORECOLORED;
					break;
				 case 40:
				 case 41:
				 case 42:
				 case 43:
				 case 44:
				 case 45:
				 case 46:
				 case 47:
					term->flags &= ~BACKCOLORMASK;
					term->flags |= BACKCOLORED|BACKCOLOR(param[row]-40);
					break;
				 case 49:
					term->flags &= ~BACKCOLORED;
					break;
#endif /* KTERM_COLOR */
				}
			}
			parsestate = groundtable;
			break;

		 case CASE_CPR:
			/* CPR */
			if ((row = param[0]) == 5) {
				reply.a_type = CSI;
				reply.a_pintro = 0;
				reply.a_nparam = 1;
				reply.a_param[0] = 0;
				reply.a_inters = 0;
				reply.a_final  = 'n';
				unparseseq(&reply, screen->respond);
			} else if (row == 6) {
				reply.a_type = CSI;
				reply.a_pintro = 0;
				reply.a_nparam = 2;
				reply.a_param[0] = screen->cur_row+1;
				reply.a_param[1] = screen->cur_col+1;
				reply.a_inters = 0;
				reply.a_final  = 'R';
				unparseseq(&reply, screen->respond);
			}
			parsestate = groundtable;
			break;

		 case CASE_HP_MEM_LOCK:
		 case CASE_HP_MEM_UNLOCK:
			if(screen->scroll_amt)
			    FlushScroll(screen);
			if (parsestate[c] == CASE_HP_MEM_LOCK)
			    screen->top_marg = screen->cur_row;
			else
			    screen->top_marg = 0;
			parsestate = groundtable;
			break;

		 case CASE_DECSTBM:
			/* DECSTBM - set scrolling region */
			if((top = param[0]) < 1)
				top = 1;
			if(nparam < 2 || (bot = param[1]) == DEFAULT
			   || bot > screen->max_row + 1
			   || bot == 0)
				bot = screen->max_row+1;
			if (bot > top) {
				if(screen->scroll_amt)
					FlushScroll(screen);
				screen->top_marg = top-1;
				screen->bot_marg = bot-1;
#ifdef STATUSLINE
			      if (!screen->instatus)
#endif /* STATUSLINE */
				CursorSet(screen, 0, 0, term->flags);
			}
			parsestate = groundtable;
			break;

		 case CASE_DECREQTPARM:
			/* DECREQTPARM */
			if ((row = param[0]) == DEFAULT)
				row = 0;
			if (row == 0 || row == 1) {
				reply.a_type = CSI;
				reply.a_pintro = 0;
				reply.a_nparam = 7;
				reply.a_param[0] = row + 2;
				reply.a_param[1] = 1;	/* no parity */
				reply.a_param[2] = 1;	/* eight bits */
				reply.a_param[3] = 112;	/* transmit 9600 baud */
				reply.a_param[4] = 112;	/* receive 9600 baud */
				reply.a_param[5] = 1;	/* clock multiplier ? */
				reply.a_param[6] = 0;	/* STP flags ? */
				reply.a_inters = 0;
				reply.a_final  = 'x';
				unparseseq(&reply, screen->respond);
			}
			parsestate = groundtable;
			break;

		 case CASE_DECSET:
			/* DECSET */
			dpmodes(term, bitset);
			parsestate = groundtable;
#ifndef KTERM_NOTEK
			if(screen->TekEmu)
				return;
#endif /* !KTERM_NOTEK */
			break;

		 case CASE_DECRST:
			/* DECRST */
			dpmodes(term, bitclr);
			parsestate = groundtable;
			break;

		 case CASE_DECALN:
			/* DECALN */
			if(screen->cursor_state)
				HideCursor();
			for(row = screen->max_row ; row >= 0 ; row--) {
#ifdef KTERM
				col = screen->max_col + 1;
				for(xp = screen->buf[row] ; col > 0 ; col--) {
					xp->code = (unsigned char) 'E';
					xp->gset = GSET_ASCII;
					xp->attr = 0;
					xp++;
				}
#else /* !KTERM */
				bzero(screen->buf[2 * row + 1],
				 col = screen->max_col + 1);
				for(cp = (unsigned char *)screen->buf[2 * row] ; col > 0 ; col--)
					*cp++ = (unsigned char) 'E';
#endif /* !KTERM */
			}
			ScrnRefresh(screen, 0, 0, screen->max_row + 1,
			 screen->max_col + 1, False);
			parsestate = groundtable;
			break;

		 case CASE_GSETS:
#ifdef KTERM
# ifdef KTERM_MBCS
			screen->gsets[scstype] = GSET(c) | cs96 | mbcs;
# else /* !KTERM_MBCS */
			screen->gsets[scstype] = GSET(c) | cs96;
# endif /* !KTERM_MBCS */
#else /* !KTERM */
			screen->gsets[scstype] = c;
#endif /* !KTERM */
			parsestate = groundtable;
			break;

		 case CASE_DECSC:
			/* DECSC */
			CursorSave(term, &screen->sc);
			parsestate = groundtable;
			break;

		 case CASE_DECRC:
			/* DECRC */
			CursorRestore(term, &screen->sc);
			parsestate = groundtable;
			break;

		 case CASE_DECKPAM:
			/* DECKPAM */
			term->keyboard.flags |= KYPD_APL;
			update_appkeypad();
			parsestate = groundtable;
			break;

		 case CASE_DECKPNM:
			/* DECKPNM */
			term->keyboard.flags &= ~KYPD_APL;
			update_appkeypad();
			parsestate = groundtable;
			break;

		 case CASE_IND:
			/* IND */
#ifdef STATUSLINE
		      if (!screen->instatus)
#endif /* STATUSLINE */
			Index(screen, 1);
#ifdef KTERM_XAW3D
			if (XtAppPending(app_con) > 0 ||
#else /* !KTERM_XAW3D */
			if (QLength(screen->display) > 0 ||
#endif /* !KTERM_XAW3D */
			    GetBytesAvailable (ConnectionNumber(screen->display)) > 0)
			  xevents();
			parsestate = groundtable;
			break;

		 case CASE_NEL:
			/* NEL */
#ifdef STATUSLINE
		      if (!screen->instatus)
#endif /* STATUSLINE */
			Index(screen, 1);
			CarriageReturn(screen);
			
#ifdef KTERM_XAW3D
			if (XtAppPending(app_con) > 0 ||
#else /* !KTERM_XAW3D */
			if (QLength(screen->display) > 0 ||
#endif /* !KTERM_XAW3D */
			    GetBytesAvailable (ConnectionNumber(screen->display)) > 0)
			  xevents();
			parsestate = groundtable;
			break;

		 case CASE_HTS:
			/* HTS */
			TabSet(term->tabs, screen->cur_col);
			parsestate = groundtable;
			break;

		 case CASE_RI:
			/* RI */
#ifdef STATUSLINE
		      if (!screen->instatus)
#endif /* STATUSLINE */
			RevIndex(screen, 1);
			parsestate = groundtable;
			break;

		 case CASE_SS2:
			/* SS2 */
			screen->curss = 2;
			parsestate = groundtable;
			break;

		 case CASE_SS3:
			/* SS3 */
			screen->curss = 3;
			parsestate = groundtable;
			break;

		 case CASE_CSI_STATE:
			/* enter csi state */
			nparam = 1;
			param[0] = DEFAULT;
			parsestate = csitable;
			break;

		 case CASE_OSC:
			/* Operating System Command: ESC ] */
			do_osc(finput);
			parsestate = groundtable;
			break;

		 case CASE_RIS:
			/* RIS */
			VTReset(TRUE);
			parsestate = groundtable;
			break;

		 case CASE_LS2:
			/* LS2 */
			screen->curgl = 2;
			parsestate = groundtable;
			break;

		 case CASE_LS3:
			/* LS3 */
			screen->curgl = 3;
			parsestate = groundtable;
			break;

		 case CASE_LS3R:
			/* LS3R */
			screen->curgr = 3;
			parsestate = groundtable;
			break;

		 case CASE_LS2R:
			/* LS2R */
			screen->curgr = 2;
			parsestate = groundtable;
			break;

		 case CASE_LS1R:
			/* LS1R */
			screen->curgr = 1;
			parsestate = groundtable;
			break;

#ifdef STATUSLINE
		 case CASE_TO_STATUS:
			if((col = param[0]) < 1)
				col = 1;
			ToStatus(col-1);
			parsestate = groundtable;
			break;

		 case CASE_FROM_STATUS:
			FromStatus();
			parsestate = groundtable;
			break;

		 case CASE_SHOW_STATUS:
			ShowStatus();
			parsestate = groundtable;
			break;

		 case CASE_HIDE_STATUS:
			HideStatus();
			parsestate = groundtable;
			break;

		 case CASE_ERASE_STATUS:
			EraseStatus();
			parsestate = groundtable;
			break;
#endif /* STATUSLINE */

		 case CASE_XTERM_SAVE:
			savemodes(term);
			parsestate = groundtable;
			break;

		 case CASE_XTERM_RESTORE:
			restoremodes(term);
			parsestate = groundtable;
			break;
#ifdef KTERM_KANJIMODE
		}
#else /* !KTERM_KANJIMODE */
		}
#endif /* !KTERM_KANJIMODE */
	}
}

static finput()
{
	return(doinput());
}


static char *v_buffer;		/* pointer to physical buffer */
static char *v_bufstr = NULL;	/* beginning of area to write */
static char *v_bufptr;		/* end of area to write */
static char *v_bufend;		/* end of physical buffer */
#define	ptymask()	(v_bufptr > v_bufstr ? pty_mask : 0)

/* Write data to the pty as typed by the user, pasted with the mouse,
   or generated by us in response to a query ESC sequence. */

v_write(f, d, len)
    int f;
    char *d;
    int len;
{
	int riten;
	int c = len;

	if (v_bufstr == NULL  &&  len > 0) {
	        v_buffer = XtMalloc(len);
		v_bufstr = v_buffer;
		v_bufptr = v_buffer;
		v_bufend = v_buffer + len;
	}
#ifdef DEBUG
	if (debug) {
	    fprintf(stderr, "v_write called with %d bytes (%d left over)",
		    len, v_bufptr - v_bufstr);
	    if (len > 1  &&  len < 10) fprintf(stderr, " \"%.*s\"", len, d);
	    fprintf(stderr, "\n");
	}
#endif

	if (!FD_ISSET (f, &pty_mask))
		return(write(f, d, len));

	/*
	 * Append to the block we already have.
	 * Always doing this simplifies the code, and
	 * isn't too bad, either.  If this is a short
	 * block, it isn't too expensive, and if this is
	 * a long block, we won't be able to write it all
	 * anyway.
	 */

	if (len > 0) {
	    if (v_bufend < v_bufptr + len) { /* we've run out of room */
		if (v_bufstr != v_buffer) {
		    /* there is unused space, move everything down */
		    /* possibly overlapping memmove here */
#ifdef DEBUG
		    if (debug)
			fprintf(stderr, "moving data down %d\n",
				v_bufstr - v_buffer);
#endif
		    memmove( v_buffer, v_bufstr, v_bufptr - v_bufstr);
		    v_bufptr -= v_bufstr - v_buffer;
		    v_bufstr = v_buffer;
		}
		if (v_bufend < v_bufptr + len) {
		    /* still won't fit: get more space */
		    /* Don't use XtRealloc because an error is not fatal. */
		    int size = v_bufptr - v_buffer; /* save across realloc */
		    v_buffer = realloc(v_buffer, size + len);
		    if (v_buffer) {
#ifdef DEBUG
			if (debug)
			    fprintf(stderr, "expanded buffer to %d\n",
				    size + len);
#endif
			v_bufstr = v_buffer;
			v_bufptr = v_buffer + size;
			v_bufend = v_bufptr + len;
		    } else {
			/* no memory: ignore entire write request */
			fprintf(stderr, "%s: cannot allocate buffer space\n",
				xterm_name);
			v_buffer = v_bufstr; /* restore clobbered pointer */
			c = 0;
		    }
		}
	    }
	    if (v_bufend >= v_bufptr + len) {
		/* new stuff will fit */
		memmove( v_bufptr, d, len);
		v_bufptr += len;
	    }
	}

	/*
	 * Write out as much of the buffer as we can.
	 * Be careful not to overflow the pty's input silo.
	 * We are conservative here and only write
	 * a small amount at a time.
	 *
	 * If we can't push all the data into the pty yet, we expect write
	 * to return a non-negative number less than the length requested
	 * (if some data written) or -1 and set errno to EAGAIN,
	 * EWOULDBLOCK, or EINTR (if no data written).
	 *
	 * (Not all systems do this, sigh, so the code is actually
	 * a little more forgiving.)
	 */

#define MAX_PTY_WRITE 128	/* 1/2 POSIX minimum MAX_INPUT */

	if (v_bufptr > v_bufstr) {
	    riten = write(f, v_bufstr, v_bufptr - v_bufstr <= MAX_PTY_WRITE ?
			  	       v_bufptr - v_bufstr : MAX_PTY_WRITE);
	    if (riten < 0) {
#ifdef DEBUG
		if (debug) perror("write");
#endif
		riten = 0;
	    }
#ifdef DEBUG
	    if (debug)
		fprintf(stderr, "write called with %d, wrote %d\n",
			v_bufptr - v_bufstr <= MAX_PTY_WRITE ?
			v_bufptr - v_bufstr : MAX_PTY_WRITE,
			riten);
#endif
	    v_bufstr += riten;
	    if (v_bufstr >= v_bufptr) /* we wrote it all */
		v_bufstr = v_bufptr = v_buffer;
	}

	/*
	 * If we have lots of unused memory allocated, return it
	 */
	if (v_bufend - v_bufptr > 1024) { /* arbitrary hysteresis */
	    /* save pointers across realloc */
	    int start = v_bufstr - v_buffer;
	    int size = v_bufptr - v_buffer;
	    int allocsize = size ? size : 1;
	    
	    v_buffer = realloc(v_buffer, allocsize);
	    if (v_buffer) {
		v_bufstr = v_buffer + start;
		v_bufptr = v_buffer + size;
		v_bufend = v_buffer + allocsize;
#ifdef DEBUG
		if (debug)
		    fprintf(stderr, "shrunk buffer to %d\n", allocsize);
#endif
	    } else {
		/* should we print a warning if couldn't return memory? */
		v_buffer = v_bufstr - start; /* restore clobbered pointer */
	    }
	}
	return(c);
}

static fd_set select_mask;
static fd_set write_mask;
static int pty_read_bytes;

in_put()
{
    register TScreen *screen = &term->screen;
    register int i;
    static struct timeval select_timeout;

    for( ; ; ) {
	if (FD_ISSET (screen->respond, &select_mask) && eventMode == NORMAL) {
#ifdef ALLOWLOGGING
	    if (screen->logging)
		FlushLog(screen);
#endif
	    bcnt = read(screen->respond, (char *)(bptr = buffer), BUF_SIZE);
	    if (bcnt < 0) {
		if (errno == EIO)
		    Cleanup (0);
		else if (!E_TEST(errno))
		    Panic(
			  "input: read returned unexpected error (%d)\n",
			  errno);
	    } else if (bcnt == 0)
		Panic("input: read returned zero\n", 0);
	    else {
		/* read from pty was successful */
#ifdef KTERM_MBCS
		if (pending_byte) {
		    /*
		     * restore pending_byte to the top of
		     * the text which just read.
		     */
		    *--bptr = pending_byte;
		    bcnt++;
		    pending_byte = 0;
		}
#endif /* KTERM_MBCS */
		if (!screen->output_eight_bits) {
		    register int bc = bcnt;
		    register Char *b = bptr;

		    for (; bc > 0; bc--, b++) {
			*b &= (Char) 0x7f;
		    }
		}
		if ( screen->scrollWidget && screen->scrollttyoutput &&
		     screen->topline < 0)
		    WindowScroll(screen, 0);  /* Scroll to bottom */
		pty_read_bytes += bcnt;
		/* stop speed reading at some point to look for X stuff */
		/* (4096 is just a random large number.) */
		if (pty_read_bytes > 4096)
		    FD_CLR (screen->respond, &select_mask);
		break;
	    }
	}
	pty_read_bytes = 0;
	/* update the screen */
	if (screen->scroll_amt)
	    FlushScroll(screen);
	if (screen->cursor_set && (screen->cursor_col != screen->cur_col
				   || screen->cursor_row != screen->cur_row)) {
	    if (screen->cursor_state)
		HideCursor();
	    ShowCursor();
	} else if (screen->cursor_set != screen->cursor_state) {
	    if (screen->cursor_set)
		ShowCursor();
	    else
		HideCursor();
	}

	XFlush(screen->display); /* always flush writes before waiting */

	/* Update the masks and, unless X events are already in the queue,
	   wait for I/O to be possible. */
	XFD_COPYSET (&Select_mask, &select_mask);
	if (v_bufptr > v_bufstr) {
	    XFD_COPYSET (&pty_mask, &write_mask);
	} else
	    FD_ZERO (&write_mask);
	select_timeout.tv_sec = 0;
#ifdef KTERM_XAW3D
	/*
	 * if there's either an XEvent or an XtTimeout pending, just take
	 * a quick peek, i.e. timeout from the select() immediately.  If
	 * there's nothing pending, let select() block a little while, but
	 * for a shorter interval than the arrow-style scrollbar timeout.
	 */
	if (XtAppPending(app_con))
		select_timeout.tv_usec = 0;
	else
		select_timeout.tv_usec = 50000;
	i = Select(max_plus1, &select_mask, &write_mask, NULL, &select_timeout);
#else /* !KTERM_XAW3D */
	select_timeout.tv_usec = 0;
	i = Select(max_plus1, &select_mask, &write_mask, NULL,
		   QLength(screen->display) ? &select_timeout : NULL);
#endif /* !KTERM_XAW3D */
	if (i < 0) {
	    if (errno != EINTR)
		SysError(ERROR_SELECT);
	    continue;
	} 

	/* if there is room to write more data to the pty, go write more */
	if (FD_ISSET (screen->respond, &write_mask)) {
	    v_write(screen->respond, 0, 0); /* flush buffer */
	}

	/* if there are X events already in our queue, it
	   counts as being readable */
#ifdef KTERM_XAW3D
	if (XtAppPending(app_con) || 
#else /* !KTERM_XAW3D */
	if (QLength(screen->display) || 
#endif /* !KTERM_XAW3D */
	    FD_ISSET (ConnectionNumber(screen->display), &select_mask)) {
	    xevents();
	}

    }
    bcnt--;
    return(*bptr++);
}

/*
 * process a string of characters according to the character set indicated
 * by charset.  worry about end of line conditions (wraparound if selected).
 */
static void
dotext(screen, flags, charset, buf, ptr)
    register TScreen	*screen;
    unsigned	flags;
#ifdef KTERM
    Char	charset;
    Char	*buf;		/* start of characters to process */
    Char	*ptr;		/* end */
#else /* !KTERM */
    char	charset;
    char	*buf;		/* start of characters to process */
    char	*ptr;		/* end */
#endif /* !KTERM */
{
#ifdef KTERM
	int	do_wrap = 0;
#else /* !KTERM */
	register char	*s;
#endif /* !KTERM */
	register int	len;
	register int	n;
	register int	next_col;

#ifndef KTERM
	switch (charset) {
	case 'A':	/* United Kingdom set			*/
		for (s=buf; s<ptr; ++s)
			if (*s == '#')
				*s = '\036';	/* UK pound sign*/
		break;

	case 'B':	/* ASCII set				*/
		break;

	case '0':	/* special graphics (line drawing)	*/
		for (s=buf; s<ptr; ++s)
			if (*s>=0x5f && *s<=0x7e)
				*s = *s == 0x5f ? 0x7f : *s - 0x5f;
		break;

	default:	/* any character sets we don't recognize*/
		return;
	}
#endif /* !KTERM */

	len = ptr - buf; 
	ptr = buf;
	while (len > 0) {
		n = screen->max_col - screen->cur_col +1;
		if (n <= 1) {
#ifdef STATUSLINE
			if (screen->do_wrap && (flags&WRAPAROUND) &&
			    !screen->instatus) {
#else /* !STATUSLINE */
			if (screen->do_wrap && (flags&WRAPAROUND)) {
#endif /* !STATUSLINE */
			    /* mark that we had to wrap this line */
			    ScrnSetAttributes(screen, screen->cur_row, 0,
					      LINEWRAPPED, LINEWRAPPED, 1);
			    Index(screen, 1);
			    screen->cur_col = 0;
			    screen->do_wrap = 0;
			    n = screen->max_col+1;
			} else
			    n = 1;
		}
		if (len < n)
			n = len;
#ifdef KTERM_MBCS
		if (charset & MBCS) {
			if (n == 1) {
				if (flags & WRAPAROUND) {
					n--; do_wrap = 1;
				} else
					n++;
			} else
				if (n & 1)
					n--;
		}
#endif /* KTERM_MBCS */
		next_col = screen->cur_col + n;
#ifdef KTERM
		WriteText(screen, ptr, n, flags, charset);
#else /* !KTERM */
		WriteText(screen, ptr, n, flags);
#endif /* !KTERM */
		/*
		 * the call to WriteText updates screen->cur_col.
		 * If screen->cur_col != next_col, we must have
		 * hit the right margin, so set the do_wrap flag.
		 */
		screen->do_wrap = (screen->cur_col < next_col);
#ifdef KTERM_MBCS
		screen->do_wrap |= do_wrap;
#endif /* KTERM_MBCS */
		len -= n;
		ptr += n;
	}
}
 
/*
 * write a string str of length len onto the screen at
 * the current cursor position.  update cursor position.
 */
#ifdef KTERM
static void
WriteText(screen, str, len, flags, gset)
    register TScreen	*screen;
    register char	*str;
    register int	len;
    unsigned		flags;
    register Char	gset;
{
	if(screen->cursor_state)
		HideCursor();
	if (flags & INSERT)
		InsertChar(screen, len);
# ifdef KTERM_MBCS
	BreakMBchar(screen);
	screen->cur_col += len;
	BreakMBchar(screen);
	screen->cur_col -= len;
# endif /* KTERM_MBCS */
	ScreenWrite(screen, str, flags, gset, len);

	if (screen->cur_row - screen->topline <= screen->max_row) {
		if (!(AddToRefresh(screen))) {
			if(screen->scroll_amt)
				FlushScroll(screen);
			ScreenDraw(screen, screen->cur_row, screen->cur_col,
				screen->cur_col+len, flags, False);
		}
# ifdef STATUSLINE
	} else if (screen->instatus) {
		ScreenDraw(screen, screen->max_row + 1, screen->cur_col,
			screen->cur_col+len, flags, False);
# endif /* STATUSLINE */
	}

	CursorForward(screen, len);
}
#else /* !KTERM */
static void
WriteText(screen, str, len, flags)
    register TScreen	*screen;
    register char	*str;
    register int	len;
    unsigned		flags;
{
	register int cx, cy;
	register unsigned fgs = flags;
	GC	currentGC;
 
#ifndef ENBUG /* kagotai */
	if (fgs & INSERT)
		InsertChar(screen, len);
#endif
#ifdef STATUSLINE
   if(screen->instatus && screen->reversestatus)
	fgs ^= INVERSE;
   if(screen->cur_row - screen->topline <= screen->max_row ||
      screen->instatus) {
#else /* !STATUSLINE */
   if(screen->cur_row - screen->topline <= screen->max_row) {
#endif /* !STATUSLINE */
	/*
	if(screen->cur_row == screen->cursor_row && screen->cur_col <=
	 screen->cursor_col && screen->cursor_col <= screen->cur_col + len - 1)
		screen->cursor_state = OFF;
	 */
	if(screen->cursor_state)
		HideCursor();

	/*
	 *	make sure that the correct GC is current
	 */

	if (fgs & BOLD)
		if (fgs & INVERSE)
			currentGC = screen->reverseboldGC;
		else	currentGC = screen->normalboldGC;
	else  /* not bold */
		if (fgs & INVERSE)
			currentGC = screen->reverseGC;
		else	currentGC = screen->normalGC;

#ifdef ENBUG /* kagotai */
	if (fgs & INSERT)
		InsertChar(screen, len);
#endif
      if (!(AddToRefresh(screen))) {
		if(screen->scroll_amt)
			FlushScroll(screen);
	cx = CursorX(screen, screen->cur_col);
	cy = CursorY(screen, screen->cur_row)+screen->fnt_norm->ascent;
 	XDrawImageString(screen->display, TextWindow(screen), currentGC,
			cx, cy, str, len);

	if((fgs & BOLD) && screen->enbolden) 
		if (currentGC == screen->normalGC || screen->reverseGC)
			XDrawString(screen->display, TextWindow(screen),
			      	currentGC,cx + 1, cy, str, len);

	if(fgs & UNDERLINE) 
		XDrawLine(screen->display, TextWindow(screen), currentGC,
			cx, cy+1,
			cx + len * FontWidth(screen), cy+1);
	/*
	 * the following statements compile data to compute the average 
	 * number of characters written on each call to XText.  The data
	 * may be examined via the use of a "hidden" escape sequence.
	 */
	ctotal += len;
	++ntotal;
      }
    }
	ScreenWrite(screen, str, flags, len);
	CursorForward(screen, len);
}
#endif /* !KTERM */
 
/*
 * process ANSI modes set, reset
 */
ansi_modes(termw, func)
    XtermWidget	termw;
    void (*func)();
{
	register int	i;

	for (i=0; i<nparam; ++i) {
		switch (param[i]) {
		case 4:			/* IRM				*/
			(*func)(&termw->flags, INSERT);
			break;

		case 20:		/* LNM				*/
			(*func)(&termw->flags, LINEFEED);
			update_autolinefeed();
			break;
		}
	}
}

/*
 * process DEC private modes set, reset
 */
dpmodes(termw, func)
    XtermWidget	termw;
    void (*func)();
{
	register TScreen	*screen	= &termw->screen;
	register int	i, j;

	for (i=0; i<nparam; ++i) {
		switch (param[i]) {
		case 1:			/* DECCKM			*/
			(*func)(&termw->keyboard.flags, CURSOR_APL);
			update_appcursor();
			break;
		case 2:			/* ANSI/VT52 mode		*/
			if (func == bitset) {
				screen->gsets[0] =
					screen->gsets[1] =
					screen->gsets[2] =
					screen->gsets[3] = 'B';
				screen->curgl = 0;
				screen->curgr = 2;
			}
			break;
		case 3:			/* DECCOLM			*/
			if(screen->c132) {
				ClearScreen(screen);
				CursorSet(screen, 0, 0, termw->flags);
				if((j = func == bitset ? 132 : 80) !=
				 ((termw->flags & IN132COLUMNS) ? 132 : 80) ||
				 j != screen->max_col + 1) {
				        Dimension replyWidth, replyHeight;
					XtGeometryResult status;

					status = XtMakeResizeRequest (
					    (Widget) termw, 
					    (Dimension) FontWidth(screen) * j
					        + 2*screen->border
						+ screen->scrollbar,
					    (Dimension) FontHeight(screen)
						* (screen->max_row + 1)
#ifdef STATUSLINE
						+ screen->statusheight
#endif /* STATUSLINE */
						+ 2 * screen->border,
					    &replyWidth, &replyHeight);

					if (status == XtGeometryYes ||
					    status == XtGeometryDone) {
					    ScreenResize (&termw->screen,
							  replyWidth,
							  replyHeight,
							  &termw->flags);
					}
				}
				(*func)(&termw->flags, IN132COLUMNS);
			}
			break;
		case 4:			/* DECSCLM (slow scroll)	*/
			if (func == bitset) {
				screen->jumpscroll = 0;
				if (screen->scroll_amt)
					FlushScroll(screen);
			} else
				screen->jumpscroll = 1;
			(*func)(&termw->flags, SMOOTHSCROLL);
			update_jumpscroll();
			break;
		case 5:			/* DECSCNM			*/
			j = termw->flags;
			(*func)(&termw->flags, REVERSE_VIDEO);
			if ((termw->flags ^ j) & REVERSE_VIDEO)
				ReverseVideo(termw);
			/* update_reversevideo done in RevVid */
			break;

		case 6:			/* DECOM			*/
			(*func)(&termw->flags, ORIGIN);
			CursorSet(screen, 0, 0, termw->flags);
			break;

		case 7:			/* DECAWM			*/
			(*func)(&termw->flags, WRAPAROUND);
			update_autowrap();
			break;
		case 8:			/* DECARM			*/
			/* ignore autorepeat */
			break;
		case 9:			/* MIT bogus sequence		*/
			if(func == bitset)
				screen->send_mouse_pos = 1;
			else
				screen->send_mouse_pos = 0;
			break;
#ifndef KTERM_NOTEK
		case 38:		/* DECTEK			*/
			if(func == bitset && !(screen->inhibit & I_TEK)) {
#ifdef ALLOWLOGGING
				if(screen->logging) {
					FlushLog(screen);
					screen->logstart = Tbuffer;
				}
#endif
				screen->TekEmu = TRUE;
			}
			break;
#endif /* !KTERM_NOTEK */
		case 40:		/* 132 column mode		*/
			screen->c132 = (func == bitset);
			update_allow132();
			break;
		case 41:		/* curses hack			*/
			screen->curses = (func == bitset);
			update_cursesemul();
			break;
		case 44:		/* margin bell			*/
			screen->marginbell = (func == bitset);
			if(!screen->marginbell)
				screen->bellarmed = -1;
			update_marginbell();
			break;
		case 45:		/* reverse wraparound	*/
			(*func)(&termw->flags, REVERSEWRAP);
			update_reversewrap();
			break;
#ifdef ALLOWLOGGING
		case 46:		/* logging		*/
#ifdef ALLOWLOGFILEONOFF
			/*
			 * if this feature is enabled, logging may be 
			 * enabled and disabled via escape sequences.
			 */
			if(func == bitset)
				StartLog(screen);
			else
				CloseLog(screen);
#else
			Bell(XkbBI_Info,0);
			Bell(XkbBI_Info,0);
#endif /* ALLOWLOGFILEONOFF */
			break;
#endif
		case 47:		/* alternate buffer */
			if (!termw->misc.titeInhibit) {
			    if(func == bitset)
				ToAlternate(screen);
			    else
				FromAlternate(screen);
			}
			break;
#ifdef STATUSLINE
		case 48:		/* reverse statusline		*/
			j = screen->reversestatus;
			if(func == bitset)
				screen->reversestatus = TRUE;
			else
				screen->reversestatus = FALSE;
			if (j != screen->reversestatus)
				ScrnRefresh(screen, screen->max_row + 1, 0, 1,
					screen->max_col + 1, False);
			break;
#endif /* STATUSLINE */
		case 1000:		/* xterm bogus sequence		*/
			if(func == bitset)
				screen->send_mouse_pos = 2;
			else
				screen->send_mouse_pos = 0;
			break;
		case 1001:		/* xterm sequence w/hilite tracking */
			if(func == bitset)
				screen->send_mouse_pos = 3;
			else
				screen->send_mouse_pos = 0;
			break;
		}
	}
}

/*
 * process xterm private modes save
 */
savemodes(termw)
    XtermWidget termw;
{
	register TScreen	*screen	= &termw->screen;
	register int i;

	for (i = 0; i < nparam; i++) {
		switch (param[i]) {
		case 1:			/* DECCKM			*/
			screen->save_modes[0] = termw->keyboard.flags &
			 CURSOR_APL;
			break;
		case 3:			/* DECCOLM			*/
			if(screen->c132)
			    screen->save_modes[1] = termw->flags & IN132COLUMNS;
			break;
		case 4:			/* DECSCLM (slow scroll)	*/
			screen->save_modes[2] = termw->flags & SMOOTHSCROLL;
			break;
		case 5:			/* DECSCNM			*/
			screen->save_modes[3] = termw->flags & REVERSE_VIDEO;
			break;
		case 6:			/* DECOM			*/
			screen->save_modes[4] = termw->flags & ORIGIN;
			break;

		case 7:			/* DECAWM			*/
			screen->save_modes[5] = termw->flags & WRAPAROUND;
			break;
		case 8:			/* DECARM			*/
			/* ignore autorepeat */
			break;
		case 9:			/* mouse bogus sequence */
			screen->save_modes[7] = screen->send_mouse_pos;
			break;
		case 40:		/* 132 column mode		*/
			screen->save_modes[8] = screen->c132;
			break;
		case 41:		/* curses hack			*/
			screen->save_modes[9] = screen->curses;
			break;
		case 44:		/* margin bell			*/
			screen->save_modes[12] = screen->marginbell;
			break;
		case 45:		/* reverse wraparound	*/
			screen->save_modes[13] = termw->flags & REVERSEWRAP;
			break;
#ifdef ALLOWLOGGING
		case 46:		/* logging		*/
			screen->save_modes[14] = screen->logging;
			break;
#endif
		case 47:		/* alternate buffer		*/
			screen->save_modes[15] = screen->alternate;
			break;
#ifdef STATUSLINE
		case 48:		/* reverse statusline		*/
			screen->save_modes[16] = screen->reversestatus;
			break;
#endif /* STATUSLINE */
		case 1000:		/* mouse bogus sequence		*/
		case 1001:
			screen->save_modes[7] = screen->send_mouse_pos;
			break;
		}
	}
}

/*
 * process xterm private modes restore
 */
restoremodes(termw)
    XtermWidget termw;
{
	register TScreen	*screen	= &termw->screen;
	register int i, j;

	for (i = 0; i < nparam; i++) {
		switch (param[i]) {
		case 1:			/* DECCKM			*/
			termw->keyboard.flags &= ~CURSOR_APL;
			termw->keyboard.flags |= screen->save_modes[0] &
			 CURSOR_APL;
			update_appcursor();
			break;
		case 3:			/* DECCOLM			*/
			if(screen->c132) {
				ClearScreen(screen);
				CursorSet(screen, 0, 0, termw->flags);
				if((j = (screen->save_modes[1] & IN132COLUMNS)
				 ? 132 : 80) != ((termw->flags & IN132COLUMNS)
				 ? 132 : 80) || j != screen->max_col + 1) {
				        Dimension replyWidth, replyHeight;
					XtGeometryResult status;
					status = XtMakeResizeRequest (
					    (Widget) termw,
					    (Dimension) FontWidth(screen) * j 
						+ 2*screen->border
						+ screen->scrollbar,
					    (Dimension) FontHeight(screen)
						* (screen->max_row + 1)
						+ 2*screen->border,
					    &replyWidth, &replyHeight);

					if (status == XtGeometryYes ||
					    status == XtGeometryDone) {
					    ScreenResize (&termw->screen,
							  replyWidth,
							  replyHeight,
							  &termw->flags);
					}
				}
				termw->flags &= ~IN132COLUMNS;
				termw->flags |= screen->save_modes[1] &
				 IN132COLUMNS;
			}
			break;
		case 4:			/* DECSCLM (slow scroll)	*/
			if (screen->save_modes[2] & SMOOTHSCROLL) {
				screen->jumpscroll = 0;
				if (screen->scroll_amt)
					FlushScroll(screen);
			} else
				screen->jumpscroll = 1;
			termw->flags &= ~SMOOTHSCROLL;
			termw->flags |= screen->save_modes[2] & SMOOTHSCROLL;
			update_jumpscroll();
			break;
		case 5:			/* DECSCNM			*/
			if((screen->save_modes[3] ^ termw->flags) & REVERSE_VIDEO) {
				termw->flags &= ~REVERSE_VIDEO;
				termw->flags |= screen->save_modes[3] & REVERSE_VIDEO;
				ReverseVideo(termw);
				/* update_reversevideo done in RevVid */
			}
			break;
		case 6:			/* DECOM			*/
			termw->flags &= ~ORIGIN;
			termw->flags |= screen->save_modes[4] & ORIGIN;
			CursorSet(screen, 0, 0, termw->flags);
			break;

		case 7:			/* DECAWM			*/
			termw->flags &= ~WRAPAROUND;
			termw->flags |= screen->save_modes[5] & WRAPAROUND;
			update_autowrap();
			break;
		case 8:			/* DECARM			*/
			/* ignore autorepeat */
			break;
		case 9:			/* MIT bogus sequence		*/
			screen->send_mouse_pos = screen->save_modes[7];
			break;
		case 40:		/* 132 column mode		*/
			screen->c132 = screen->save_modes[8];
			update_allow132();
			break;
		case 41:		/* curses hack			*/
			screen->curses = screen->save_modes[9];
			update_cursesemul();
			break;
		case 44:		/* margin bell			*/
			if(!(screen->marginbell = screen->save_modes[12]))
				screen->bellarmed = -1;
			update_marginbell();
			break;
		case 45:		/* reverse wraparound	*/
			termw->flags &= ~REVERSEWRAP;
			termw->flags |= screen->save_modes[13] & REVERSEWRAP;
			update_reversewrap();
			break;
#ifdef ALLOWLOGGING
		case 46:		/* logging		*/
#ifdef ALLOWLOGFILEONOFF
			if(screen->save_modes[14])
				StartLog(screen);
			else
				CloseLog(screen);
#endif /* ALLOWLOGFILEONOFF */
			/* update_logging done by StartLog and CloseLog */
			break;
#endif
		case 47:		/* alternate buffer */
			if (!termw->misc.titeInhibit) {
			    if(screen->save_modes[15])
				ToAlternate(screen);
			    else
				FromAlternate(screen);
			    /* update_altscreen done by ToAlt and FromAlt */
			}
			break;
#ifdef STATUSLINE
		case 48:		/* reverse statusline		*/
			if (screen->save_modes[16] != screen->reversestatus) {
				screen->reversestatus = screen->save_modes[16];
				ScrnRefresh(screen, screen->max_row + 1, 0, 1,
					    screen->max_col + 1, False);
			}
			break;
#endif /* STATUSLINE */
		case 1000:		/* mouse bogus sequence		*/
		case 1001:
			screen->send_mouse_pos = screen->save_modes[7];
			break;
		}
	}
}

/*
 * set a bit in a word given a pointer to the word and a mask.
 */
static void bitset(p, mask)
    unsigned *p;
    int mask;
{
	*p |= mask;
}

/*
 * clear a bit in a word given a pointer to the word and a mask.
 */
static void bitclr(p, mask)
    unsigned *p;
    int mask;
{
	*p &= ~mask;
}

unparseseq(ap, fd)
    register ANSI *ap;
    int fd;
{
	register int	c;
	register int	i;
	register int	inters;

	c = ap->a_type;
	if (c>=0x80 && c<=0x9F) {
		unparseputc(ESC, fd);
		c -= 0x40;
	}
	unparseputc(c, fd);
	c = ap->a_type;
	if (c==ESC || c==DCS || c==CSI || c==OSC || c==PM || c==APC) {
		if (ap->a_pintro != 0)
			unparseputc((char) ap->a_pintro, fd);
		for (i=0; i<ap->a_nparam; ++i) {
			if (i != 0)
				unparseputc(';', fd);
			unparseputn((unsigned int) ap->a_param[i], fd);
		}
		inters = ap->a_inters;
		for (i=3; i>=0; --i) {
			c = (inters >> (8*i)) & 0xff;
			if (c != 0)
				unparseputc(c, fd);
		}
		unparseputc((char) ap->a_final, fd);
	}
}

unparseputn(n, fd)
unsigned int	n;
int fd;
{
	unsigned int	q;

	q = n/10;
	if (q != 0)
		unparseputn(q, fd);
	unparseputc((char) ('0' + (n%10)), fd);
}

unparseputc(c, fd)
char c;
int fd;
{
	char	buf[2];
	register i = 1;
	extern XtermWidget term;

	if((buf[0] = c) == '\r' && (term->flags & LINEFEED)) {
		buf[1] = '\n';
		i++;
	}
	v_write(fd, buf, i);
}

unparsefputs (s, fd)
    register char *s;
    int fd;
{
    if (s) {
	while (*s) unparseputc (*s++, fd);
    }
}

static void SwitchBufs();

static void
ToAlternate(screen)
    register TScreen *screen;
{
	extern ScrnBuf Allocate();

	if(screen->alternate)
		return;
	if(!screen->altbuf)
		screen->altbuf = Allocate(screen->max_row + 1, screen->max_col
		 + 1, &screen->abuf_address);
	SwitchBufs(screen);
	screen->alternate = TRUE;
	update_altscreen();
}

static void
FromAlternate(screen)
    register TScreen *screen;
{
	if(!screen->alternate)
		return;
	screen->alternate = FALSE;
	SwitchBufs(screen);
	update_altscreen();
}

static void
SwitchBufs(screen)
    register TScreen *screen;
{
	register int rows, top;

	if(screen->cursor_state)
		HideCursor();
	rows = screen->max_row + 1;
	SwitchBufPtrs(screen);
	TrackText(0, 0, 0, 0);	/* remove any highlighting */
	if((top = -screen->topline) <= screen->max_row) {
		if(screen->scroll_amt)
			FlushScroll(screen);
#ifdef STATUSLINE
		if(top == 0 && !screen->statusheight)
#else /* !STATUSLINE */
		if(top == 0)
#endif /* !STATUSLINE */
			XClearWindow(screen->display, TextWindow(screen));
		else
			XClearArea(
			    screen->display,
			    TextWindow(screen),
			    (int) screen->border + screen->scrollbar,
			    (int) top * FontHeight(screen) + screen->border,
			    (unsigned) Width(screen),
			    (unsigned) (screen->max_row - top + 1)
				* FontHeight(screen),
			    FALSE);
	}
	ScrnRefresh(screen, 0, 0, rows, screen->max_col + 1, False);
}

/* swap buffer line pointers between alt and regular screens */

SwitchBufPtrs(screen)
    register TScreen *screen;
{
    register int rows = screen->max_row + 1;
#ifdef KTERM
    Bchr *savebuf [MAX_ROWS];
    Bchr **save;
    save = rows > MAX_ROWS ? (Bchr **)XtMalloc(sizeof(Bchr *) * rows) : savebuf;

    memmove( (char *)save, (char *)screen->buf, sizeof(Bchr *) * rows);
    memmove( (char *)screen->buf, (char *)screen->altbuf, 
	  sizeof(Bchr *) * rows);
    memmove( (char *)screen->altbuf, (char *)save, sizeof(Bchr *) * rows);

    if (save != savebuf) XtFree((char *)save);
#else /* !KTERM */
    char *save [2 * MAX_ROWS];

    memmove( (char *)save, (char *)screen->buf, 2 * sizeof(char *) * rows);
    memmove( (char *)screen->buf, (char *)screen->altbuf, 
	  2 * sizeof(char *) * rows);
    memmove( (char *)screen->altbuf, (char *)save, 2 * sizeof(char *) * rows);
#endif /* !KTERM */
}

VTRun()
{
	register TScreen *screen = &term->screen;
	register int i;
	
#ifdef KTERM_NOTEK
	VTInit ();
	XtMapWidget (term->core.parent);
#else /* !KTERM_NOTEK */
	if (!screen->Vshow) {
	    set_vt_visibility (TRUE);
	} 
	update_vttekmode();
	update_vtshow();
	update_tekshow();
	set_vthide_sensitivity();
#endif /* !KTERM_NOTEK */

	if (screen->allbuf == NULL) VTallocbuf ();

	screen->cursor_state = OFF;
	screen->cursor_set = ON;

	bcnt = 0;
	bptr = buffer;
#ifndef KTERM_NOTEK
	while(Tpushb > Tpushback) {
		*bptr++ = *--Tpushb;
		bcnt++;
	}
	bcnt += (i = Tbcnt);
	for( ; i > 0 ; i--)
		*bptr++ = *Tbptr++;
#endif /* !KTERM_NOTEK */
	bptr = buffer;
#ifndef KTERM_NOTEK
	if(!setjmp(VTend))
#endif /* !KTERM_NOTEK */
		VTparse();
	HideCursor();
	screen->cursor_set = OFF;
}

/*ARGSUSED*/
static void VTExpose(w, event, region)
    Widget w;
    XEvent *event;
    Region region;
{
	register TScreen *screen = &term->screen;

#ifdef DEBUG
	if(debug)
		fputs("Expose\n", stderr);
#endif	/* DEBUG */
	if (event->type == Expose)
		HandleExposure (screen, event);
}

static void VTGraphicsOrNoExpose (event)
    XEvent *event;
{
	register TScreen *screen = &term->screen;
	if (screen->incopy <= 0) {
		screen->incopy = 1;
		if (screen->scrolls > 0)
			screen->scrolls--;
	}
	if (event->type == GraphicsExpose)
	  if (HandleExposure (screen, event))
		screen->cursor_state = OFF;
	if ((event->type == NoExpose) || ((XGraphicsExposeEvent *)event)->count == 0) {
		if (screen->incopy <= 0 && screen->scrolls > 0)
			screen->scrolls--;
		if (screen->scrolls)
			screen->incopy = -1;
		else
			screen->incopy = 0;
	}
}

/*ARGSUSED*/
static void VTNonMaskableEvent (w, closure, event, cont)
Widget w;			/* unused */
XtPointer closure;		/* unused */
XEvent *event;
Boolean *cont;			/* unused */
{
    switch (event->type) {
       case GraphicsExpose:
       case NoExpose:
	  VTGraphicsOrNoExpose (event);
	  break;
      }
}




static void VTResize(w)
    Widget w;
{
    if (XtIsRealized(w))
      ScreenResize (&term->screen, term->core.width, term->core.height,
		    &term->flags);
}

				
extern Atom wm_delete_window;	/* for ICCCM delete window */

static String xterm_trans =
    "<ClientMessage>WM_PROTOCOLS: DeleteWindow()\n\
     <MappingNotify>: KeyboardMapping()\n";

int VTInit ()
{
    register TScreen *screen = &term->screen;
    Widget vtparent = term->core.parent;

    XtRealizeWidget (vtparent);
    XtOverrideTranslations(vtparent, XtParseTranslationTable(xterm_trans));
    (void) XSetWMProtocols (XtDisplay(vtparent), XtWindow(vtparent),
			    &wm_delete_window, 1);

    if (screen->allbuf == NULL) VTallocbuf ();
    return (1);
}

static void VTallocbuf ()
{
    register TScreen *screen = &term->screen;
    int nrows = screen->max_row + 1;
    extern ScrnBuf Allocate();

    /* allocate screen buffer now, if necessary. */
    if (screen->scrollWidget)
      nrows += screen->savelines;
    screen->allbuf = Allocate (nrows, screen->max_col + 1,
     &screen->sbuf_address);
    if (screen->scrollWidget)
#ifdef KTERM
      screen->buf = &screen->allbuf[screen->savelines];
#else /* !KTERM */
      screen->buf = &screen->allbuf[2 * screen->savelines];
#endif /* !KTERM */
    else
      screen->buf = screen->allbuf;
    return;
}

static void VTClassInit ()
{
    XtAddConverter(XtRString, XtRGravity, XmuCvtStringToGravity,
		   (XtConvertArgList) NULL, (Cardinal) 0);
}


/* ARGSUSED */
static void VTInitialize (wrequest, wnew, args, num_args)
   Widget wrequest, wnew;
   ArgList args;
   Cardinal *num_args;
{
   XtermWidget request = (XtermWidget) wrequest;
   XtermWidget new     = (XtermWidget) wnew;
   int i;
#ifdef KTERM
   int fnum;
#endif /* KTERM */

   /* Zero out the entire "screen" component of "new" widget,
      then do field-by-field assigment of "screen" fields
      that are named in the resource list. */

   bzero ((char *) &new->screen, sizeof(new->screen));
   new->screen.c132 = request->screen.c132;
   new->screen.curses = request->screen.curses;
   new->screen.hp_ll_bc = request->screen.hp_ll_bc;
   new->screen.foreground = request->screen.foreground;
   new->screen.cursorcolor = request->screen.cursorcolor;
#ifdef KTERM_COLOR
   memmove( new->screen.textcolor, request->screen.textcolor, 
	 sizeof new->screen.textcolor);
#endif /* KTERM_COLOR */
   new->screen.border = request->screen.border;
   new->screen.jumpscroll = request->screen.jumpscroll;
#ifdef ALLOWLOGGING
   new->screen.logfile = request->screen.logfile;
#endif
   new->screen.marginbell = request->screen.marginbell;
   new->screen.mousecolor = request->screen.mousecolor;
   new->screen.mousecolorback = request->screen.mousecolorback;
   new->screen.multiscroll = request->screen.multiscroll;
   new->screen.nmarginbell = request->screen.nmarginbell;
   new->screen.savelines = request->screen.savelines;
   new->screen.scrolllines = request->screen.scrolllines;
   new->screen.scrollttyoutput = request->screen.scrollttyoutput;
   new->screen.scrollkey = request->screen.scrollkey;
   new->screen.visualbell = request->screen.visualbell;
#ifndef KTERM_NOTEK
   new->screen.TekEmu = request->screen.TekEmu;
#endif /* !KTERM_NOTEK */
   new->misc.re_verse = request->misc.re_verse;
   new->screen.multiClickTime = request->screen.multiClickTime;
   new->screen.bellSuppressTime = request->screen.bellSuppressTime;
   new->screen.charClass = request->screen.charClass;
   new->screen.cutNewline = request->screen.cutNewline;
   new->screen.cutToBeginningOfLine = request->screen.cutToBeginningOfLine;
   new->screen.always_highlight = request->screen.always_highlight;
   new->screen.pointer_cursor = request->screen.pointer_cursor;
   new->screen.input_eight_bits = request->screen.input_eight_bits;
   new->screen.output_eight_bits = request->screen.output_eight_bits;
   new->screen.allowSendEvents = request->screen.allowSendEvents;
   new->misc.titeInhibit = request->misc.titeInhibit;
#ifdef STATUSLINE
   new->misc.statusline = request->misc.statusline;
   new->misc.statusnormal = request->misc.statusnormal;
#endif /* STATUSLINE */
#ifdef KTERM
   new->screen.dynamic_font_load = request->screen.dynamic_font_load;
   for (fnum = F_ISO8859_1; fnum < FCNT; fnum++) {
    if (fnum == F_ISO8859_1 || fnum == F_JISX0201_0 || fnum == F_JISX0208_0) {
     for (i = fontMenu_fontdefault; i <= fontMenu_lastBuiltin; i++) {
       new->screen.menu_font_names[i] = request->screen.menu_font_names[i];
       new->screen.menu_bfont_names[i] = request->screen.menu_bfont_names[i];
     }
    } else {
     for (i = fontMenu_fontdefault; i <= fontMenu_lastBuiltin; i++) {
       new->screen.menu_font_names[i] = NULL;
       new->screen.menu_bfont_names[i] = NULL;
     }
    }
    /* set default in realize proc */
    new->screen.menu_font_names[fontMenu_fontescape] = NULL;
    new->screen.menu_font_names[fontMenu_fontsel] = NULL;
    new->screen.menu_bfont_names[fontMenu_fontescape] = NULL;
    new->screen.menu_bfont_names[fontMenu_fontsel] = NULL;
   }
   for (i = fontMenu_fontdefault; i <= fontMenu_lastBuiltin; i++) {
       new->screen.menu_font_list[i] = request->screen.menu_font_list[i];
       new->screen.menu_bfont_list[i] = request->screen.menu_bfont_list[i];
   }
   /* set default in realize proc */
   new->screen.menu_font_list[fontMenu_fontescape] = NULL;
   new->screen.menu_font_list[fontMenu_fontsel] = NULL;
   new->screen.menu_bfont_list[fontMenu_fontescape] = NULL;
   new->screen.menu_bfont_list[fontMenu_fontsel] = NULL;
#else /* !KTERM */
   for (i = fontMenu_font1; i <= fontMenu_lastBuiltin; i++) {
       new->screen.menu_font_names[i] = request->screen.menu_font_names[i];
   }
   /* set default in realize proc */
   new->screen.menu_font_names[fontMenu_fontdefault] = NULL;
   new->screen.menu_font_names[fontMenu_fontescape] = NULL;
   new->screen.menu_font_names[fontMenu_fontsel] = NULL;
#endif /* !KTERM */
   new->screen.menu_font_number = fontMenu_fontdefault;
#ifdef KTERM
   new->screen.linespace = request->screen.linespace;
#endif /* KTERM */

    /*
     * The definition of -rv now is that it changes the definition of 
     * XtDefaultForeground and XtDefaultBackground.  So, we no longer
     * need to do anything special.
     */
   new->keyboard.flags = 0;
   new->screen.display = new->core.screen->display;
   new->core.height = new->core.width = 1;
      /* dummy values so that we don't try to Realize the parent shell 
	 with height or width of 0, which is illegal in X.  The real
	 size is computed in the xtermWidget's Realize proc,
	 but the shell's Realize proc is called first, and must see
	 a valid size. */

   /* look for focus related events on the shell, because we need
    * to care about the shell's border being part of our focus.
    */
   XtAddEventHandler(XtParent(new), EnterWindowMask, FALSE,
		HandleEnterWindow, (Opaque)NULL);
   XtAddEventHandler(XtParent(new), LeaveWindowMask, FALSE,
		HandleLeaveWindow, (Opaque)NULL);
   XtAddEventHandler(XtParent(new), FocusChangeMask, FALSE,
		HandleFocusChange, (Opaque)NULL);
   XtAddEventHandler((Widget)new, 0L, TRUE,
		VTNonMaskableEvent, (Opaque)NULL);
   XtAddEventHandler((Widget)new, PropertyChangeMask, FALSE,
		     HandleBellPropertyChange, (Opaque)NULL);
   new->screen.bellInProgress = FALSE;

   set_character_class (new->screen.charClass);

   /* create it, but don't realize it */
   ScrollBarOn (new, TRUE, FALSE);

   /* make sure that the resize gravity acceptable */
   if ( new->misc.resizeGravity != NorthWestGravity &&
        new->misc.resizeGravity != SouthWestGravity) {
       extern XtAppContext app_con;
       Cardinal nparams = 1;

       XtAppWarningMsg(app_con, "rangeError", "resizeGravity", "XTermError",
		       "unsupported resizeGravity resource value (%d)",
		       (String *) &(new->misc.resizeGravity), &nparams);
       new->misc.resizeGravity = SouthWestGravity;
   }

   return;
}


static void VTDestroy (w)
Widget w;
{
#ifdef KTERM
    XtFree((char *)(((XtermWidget)w)->screen.selection));
#else /* !KTERM */
    XtFree(((XtermWidget)w)->screen.selection);
#endif /* !KTERM */
}

/*ARGSUSED*/
static void VTRealize (w, valuemask, values)
    Widget w;
    XtValueMask *valuemask;
    XSetWindowAttributes *values;
{
	unsigned int width, height;
	register TScreen *screen = &term->screen;
	int xpos, ypos, pr;
	XSizeHints		sizehints;
	int scrollbar_width;
#ifdef KTERM
	int fnum;
#endif /* KTERM */

	TabReset (term->tabs);

#ifdef KTERM
	for (fnum = F_ISO8859_1; fnum < FCNT; fnum ++) {
	    screen->fnt_norm = screen->fnt_bold = NULL;
	}
	fnum = F_ISO8859_1;
	if (!LoadNewFont(screen, False, 0)) {
	    if (screen->menu_font_names[fontMenu_fontdefault] == NULL
	     || XmuCompareISOLatin1(screen->menu_font_names[fontMenu_fontdefault], "fixed") != 0) {
		fprintf (stderr, 
		     "%s:  unable to open font \"%s\", trying \"fixed\"....\n",
		     xterm_name,
		     screen->menu_font_names[fontMenu_fontdefault]
		       ? screen->menu_font_names[fontMenu_fontdefault]
		       : "");
		screen->menu_font_names[fontMenu_fontdefault] = "fixed";
		(void) LoadNewFont (screen, False, 0);
	    }
	}
#else /* !KTERM */
	screen->menu_font_names[fontMenu_fontdefault] = term->misc.f_n;
	screen->fnt_norm = screen->fnt_bold = NULL;
	if (!LoadNewFont(screen, term->misc.f_n, term->misc.f_b, False, 0)) {
	    if (XmuCompareISOLatin1(term->misc.f_n, "fixed") != 0) {
		fprintf (stderr, 
		     "%s:  unable to open font \"%s\", trying \"fixed\"....\n",
		     xterm_name, term->misc.f_n);
		(void) LoadNewFont (screen, "fixed", NULL, False, 0);
		screen->menu_font_names[fontMenu_fontdefault] = "fixed";
	    }
	}
#endif /* !KTERM */

	/* really screwed if we couldn't open default font */
	if (!screen->fnt_norm) {
	    fprintf (stderr, "%s:  unable to locate a suitable font\n",
		     xterm_name);
	    Exit (1);
	}

	/* making cursor */
	if (!screen->pointer_cursor) 
	  screen->pointer_cursor = make_colored_cursor(XC_xterm, 
						       screen->mousecolor,
						       screen->mousecolorback);
	else 
	  recolor_cursor (screen->pointer_cursor, 
			  screen->mousecolor, screen->mousecolorback);

	scrollbar_width = (term->misc.scrollbar ?
			   screen->scrollWidget->core.width /* +
			   screen->scrollWidget->core.border_width */ : 0);
#ifdef STATUSLINE
	if (term->misc.statusline) screen->statusheight = -1;
#endif /* STATUSLINE */

	/* set defaults */
	xpos = 1; ypos = 1; width = 80; height = 24;
	pr = XParseGeometry (term->misc.geo_metry, &xpos, &ypos,
			     &width, &height);
	screen->max_col = (width - 1);	/* units in character cells */
	screen->max_row = (height - 1);	/* units in character cells */
	update_font_info (&term->screen, False);

	width = screen->fullVwin.fullwidth;
	height = screen->fullVwin.fullheight;

	if ((pr & XValue) && (XNegative&pr)) 
	  xpos += DisplayWidth(screen->display, DefaultScreen(screen->display))
			- width - (term->core.parent->core.border_width * 2);
	if ((pr & YValue) && (YNegative&pr))
	  ypos += DisplayHeight(screen->display,DefaultScreen(screen->display))
			- height - (term->core.parent->core.border_width * 2);

	/* set up size hints for window manager; min 1 char by 1 char */
	sizehints.base_width = 2 * screen->border + scrollbar_width;
	sizehints.base_height = 2 * screen->border;
	sizehints.width_inc = FontWidth(screen);
	sizehints.height_inc = FontHeight(screen);
	sizehints.min_width = sizehints.base_width + sizehints.width_inc;
	sizehints.min_height = sizehints.base_height + sizehints.height_inc;
	sizehints.flags = (PBaseSize|PMinSize|PResizeInc);
	sizehints.x = xpos;
	sizehints.y = ypos;
	if ((XValue&pr) || (YValue&pr)) {
	    sizehints.flags |= USSize|USPosition;
	    sizehints.flags |= PWinGravity;
	    switch (pr & (XNegative | YNegative)) {
	      case 0:
		sizehints.win_gravity = NorthWestGravity;
		break;
	      case XNegative:
		sizehints.win_gravity = NorthEastGravity;
		break;
	      case YNegative:
		sizehints.win_gravity = SouthWestGravity;
		break;
	      default:
		sizehints.win_gravity = SouthEastGravity;
		break;
	    }
	} else {
	    /* set a default size, but do *not* set position */
	    sizehints.flags |= PSize;
	}
	sizehints.width = width;
	sizehints.height = height;
	if ((WidthValue&pr) || (HeightValue&pr)) 
	  sizehints.flags |= USSize;
	else sizehints.flags |= PSize;
#ifdef STATUSLINE
	sizehints.base_height += screen->statusheight;
	sizehints.min_height += screen->statusheight;
#endif /* STATUSLINE */

	(void) XtMakeResizeRequest((Widget) term,
				   (Dimension)width, (Dimension)height,
				   &term->core.width, &term->core.height);

	/* XXX This is bogus.  We are parsing geometries too late.  This
	 * is information that the shell widget ought to have before we get
	 * realized, so that it can do the right thing.
	 */
        if (sizehints.flags & USPosition)
	    XMoveWindow (XtDisplay(term), term->core.parent->core.window,
			 sizehints.x, sizehints.y);

	XSetWMNormalHints (XtDisplay(term), term->core.parent->core.window,
			   &sizehints);
	XFlush (XtDisplay(term));	/* get it out to window manager */

	/* use ForgetGravity instead of SouthWestGravity because translating
	   the Expose events for ConfigureNotifys is too hard */
	values->bit_gravity = term->misc.resizeGravity == NorthWestGravity ?
	    NorthWestGravity : ForgetGravity;
	term->screen.fullVwin.window = term->core.window =
	  XCreateWindow(XtDisplay(term), XtWindow(term->core.parent),
		term->core.x, term->core.y,
		term->core.width, term->core.height, term->core.border_width,
		(int) term->core.depth,
		InputOutput, CopyFromParent,	
		*valuemask|CWBitGravity, values);

	VTInitI18N();

#ifdef KTERM
	set_cursor_gcs (screen, F_ISO8859_1);
#else /* !KTERM */
	set_cursor_gcs (screen);
#endif /* !KTERM */

	/* Reset variables used by ANSI emulation. */

#ifdef KTERM
	setupgset();
	screen->gsets[0] = GSET_ASCII;
# ifdef KTERM_KANJIMODE
	screen->gsets[1] = (term->flags & EUC_KANJI) ? GSET_KANJI : GSET_KANA;
	screen->gsets[2] = (term->flags & EUC_KANJI) ? GSET_KANA : GSET_ASCII;
	screen->gsets[3] = (term->flags & EUC_KANJI) ? GSET_HOJOKANJI : GSET_ASCII;
# else /* !KTERM_KANJIMODE */
	screen->gsets[1] = GSET_KANA;
	screen->gsets[2] = GSET_ASCII;
	screen->gsets[3] = GSET_ASCII;
# endif /* !KTERM_KANJIMODE */
#else /* !KTERM */
	screen->gsets[0] = 'B';			/* ASCII_G		*/
	screen->gsets[1] = 'B';
	screen->gsets[2] = 'B';			/* DEC supplemental.	*/
	screen->gsets[3] = 'B';
#endif /* !KTERM */
	screen->curgl = 0;			/* G0 => GL.		*/
#ifdef KTERM
	screen->curgr = 1;			/* G1 => GR.		*/
#else /* !KTERM */
	screen->curgr = 2;			/* G2 => GR.		*/
#endif /* !KTERM */
	screen->curss = 0;			/* No single shift.	*/

	XDefineCursor(screen->display, VShellWindow, screen->pointer_cursor);

        screen->cur_col = screen->cur_row = 0;
#ifdef KTERM
	screen->max_col = Width(screen)/FontWidth(screen) - 1;
	screen->top_marg = 0;
	screen->bot_marg = screen->max_row = Height(screen) /
				FontHeight(screen) - 1;
#else /* !KTERM */
	screen->max_col = Width(screen)/screen->fullVwin.f_width - 1;
	screen->top_marg = 0;
	screen->bot_marg = screen->max_row = Height(screen) /
				screen->fullVwin.f_height - 1;
#endif /* !KTERM */

	screen->sc.row = screen->sc.col = screen->sc.flags = 0;

	/* Mark screen buffer as unallocated.  We wait until the run loop so
	   that the child process does not fork and exec with all the dynamic
	   memory it will never use.  If we were to do it here, the
	   swap space for new process would be huge for huge savelines. */
#ifndef KTERM_NOTEK /* XXX */
	if (!tekWidget)			/* if not called after fork */
#endif /* !KTERM_NOTEK */
	  screen->buf = screen->allbuf = NULL;

	screen->do_wrap = 0;
	screen->scrolls = screen->incopy = 0;
	set_vt_box (screen);

	screen->savedlines = 0;
#ifdef STATUSLINE
	screen->reversestatus = !term->misc.statusnormal;
	update_reversestatus();
#endif /* STATUSLINE */

	if (term->misc.scrollbar) {
		screen->scrollbar = 0;
		ScrollBarOn (term, FALSE, TRUE);
	}
	CursorSave (term, &screen->sc);
	return;
}

static void VTInitI18N()
{
#ifdef KTERM_XIM
    extern void SetLocale();
    extern void SetLocaleModifiers();
    extern void OpenIM();
#else /* !KTERM_XIM */
    int		i;
    char       *p,
	       *s,
	       *ns,
	       *end,
		tmp[1024],
	  	buf[32];
    XIM		xim = (XIM) NULL;
    XIMStyles  *xim_styles;
    XIMStyle	input_style = 0;
    Boolean	found;
#endif /* !KTERM_XIM */

    term->screen.xic = NULL;

#ifdef KTERM_XIM
    SetLocale(term->misc.eucjp_locale);
    if (term->misc.input_method)
	SetLocaleModifiers(&term->misc.input_method, 1);
    else
	SetLocaleModifiers(NULL, 0);
#endif /* KTERM_XIM */

    if (!term->misc.open_im) return;

#ifdef KTERM_XIM
    OpenIM(&term->screen);
#else /* !KTERM_XIM */
    if (!term->misc.input_method || !*term->misc.input_method) {
	if ((p = XSetLocaleModifiers("@im=none")) != NULL && *p)
	    xim = XOpenIM(XtDisplay(term), NULL, NULL, NULL);
    } else {
	strcpy(tmp, term->misc.input_method);
	for(ns=s=tmp; ns && *s;) {
	    while (*s && isspace(*s)) s++;
	    if (!*s) break;
	    if ((ns = end = index(s, ',')) == 0)
		end = s + strlen(s);
	    while (isspace(*end)) end--;
	    *end = '\0';

	    strcpy(buf, "@im=");
	    strcat(buf, s);
	    if ((p = XSetLocaleModifiers(buf)) != NULL && *p
		&& (xim = XOpenIM(XtDisplay(term), NULL, NULL, NULL)) != NULL)
		break;

	    s = ns + 1;
	}
    }

    if (xim == NULL && (p = XSetLocaleModifiers("")) != NULL && *p)
	xim = XOpenIM(XtDisplay(term), NULL, NULL, NULL);

    if (!xim) {
	fprintf(stderr, "Failed to open input method");
	return;
    }

#ifdef KTERM_DEBUG
    printf("modifiers=%s\n",XSetLocaleModifiers(NULL));
#endif

    if (XGetIMValues(xim, XNQueryInputStyle, &xim_styles, NULL)
        || !xim_styles) {
	fprintf(stderr, "input method doesn't support any style\n");
        XCloseIM(xim);
        return;
    }

    found = False;
    strcpy(tmp, term->misc.preedit_type);
    for(s = tmp; s && !found;) {
	while (*s && isspace(*s)) s++;
	if (!*s) break;
	if (ns = end = index(s, ','))
	    ns++;
	else
	    end = s + strlen(s);
	while (isspace(*end)) end--;
	*end = '\0';

	if (!strcmp(s, "OverTheSpot")) {
	    input_style = (XIMPreeditPosition | XIMStatusArea);
	} else if (!strcmp(s, "OffTheSpot")) {
	    input_style = (XIMPreeditArea | XIMStatusArea);
	} else if (!strcmp(s, "Root")) {
	    input_style = (XIMPreeditNothing | XIMStatusNothing);
	}
	for (i = 0; (unsigned short)i < xim_styles->count_styles; i++)
	    if (input_style == xim_styles->supported_styles[i]) {
		found = True;
		break;
	    }

	s = ns;
    }
    XFree(xim_styles);

    if (!found) {
	fprintf(stderr, "input method doesn't support my preedit type\n");
	XCloseIM(xim);
	return;
    }

    /*
     * This program only understands the Root preedit_style yet
     * Then misc.preedit_type should default to:
     *		"OverTheSpot,OffTheSpot,Root"
     *
     *	/MaF
     */
    if (input_style != (XIMPreeditNothing | XIMStatusNothing)) {
	fprintf(stderr,"This program only supports the 'Root' preedit type\n");
	XCloseIM(xim);
	return;
    }

    term->screen.xic = XCreateIC(xim, XNInputStyle, input_style,
				      XNClientWindow, term->core.window,
				      XNFocusWindow, term->core.window,
				      NULL);

    if (!term->screen.xic) {
	fprintf(stderr,"Failed to create input context\n");
	XCloseIM(xim);
    }
#endif /* !KTERM_XIM */

    return;
}


static Boolean VTSetValues (cur, request, new, args, num_args)
    Widget cur, request, new;
    ArgList args;
    Cardinal *num_args;
{
    XtermWidget curvt = (XtermWidget) cur;
    XtermWidget newvt = (XtermWidget) new; 
    Boolean refresh_needed = FALSE;
    Boolean fonts_redone = FALSE;
#ifdef KTERM
    int fnum = F_ISO8859_1;
#endif /* KTERM */

#ifdef KTERM
    if(curvt->core.background_pixel != newvt->core.background_pixel
       || curvt->screen.foreground != newvt->screen.foreground
       || curvt->screen.menu_font_names[curvt->screen.menu_font_number]
          != newvt->screen.menu_font_names[newvt->screen.menu_font_number]
       || curvt->screen.menu_font_names[fontMenu_fontdefault] != newvt->screen.menu_font_names[fontMenu_fontdefault]) {
	if(curvt->screen.menu_font_names[fontMenu_fontdefault] != newvt->screen.menu_font_names[fontMenu_fontdefault])
	    newvt->screen.menu_font_names[fontMenu_fontdefault] = newvt->screen.menu_font_names[fontMenu_fontdefault];
	fprintf(stderr, "kterm(VTSetValues): Font changing to %s\n",
		newvt->screen.menu_font_names[curvt->screen.menu_font_number]);
	if (LoadNewFont(&newvt->screen, TRUE, newvt->screen.menu_font_number)) {
#else /* !KTERM */
    if(curvt->core.background_pixel != newvt->core.background_pixel
       || curvt->screen.foreground != newvt->screen.foreground
       || curvt->screen.menu_font_names[curvt->screen.menu_font_number]
          != newvt->screen.menu_font_names[newvt->screen.menu_font_number]
       || curvt->misc.f_n != newvt->misc.f_n) {
	if(curvt->misc.f_n != newvt->misc.f_n)
	    newvt->screen.menu_font_names[fontMenu_fontdefault] = newvt->misc.f_n;
	if (LoadNewFont(&newvt->screen,
			newvt->screen.menu_font_names[curvt->screen.menu_font_number],
			newvt->screen.menu_font_names[curvt->screen.menu_font_number],
			TRUE, newvt->screen.menu_font_number)) {
#endif /* !KTERM */
	    /* resizing does the redisplay, so don't ask for it here */
	    refresh_needed = TRUE;
	    fonts_redone = TRUE;
	} else
#ifdef KTERM
	    if(curvt->screen.menu_font_names[fontMenu_fontdefault] != newvt->screen.menu_font_names[fontMenu_fontdefault])
		newvt->screen.menu_font_names[fontMenu_fontdefault] = curvt->screen.menu_font_names[fontMenu_fontdefault];
#else /* !KTERM */
	    if(curvt->misc.f_n != newvt->misc.f_n)
		newvt->screen.menu_font_names[fontMenu_fontdefault] = curvt->misc.f_n;
#endif /* !KTERM */
    }
    if(!fonts_redone
       && curvt->screen.cursorcolor != newvt->screen.cursorcolor) {
#ifdef KTERM
	for (fnum = F_ISO8859_1; fnum < FCNT; fnum++)
	  set_cursor_gcs(&newvt->screen, fnum);
#else /* !KTERM */
	set_cursor_gcs(&newvt->screen);
#endif /* !KTERM */
	refresh_needed = TRUE;
    }
    if(curvt->misc.re_verse != newvt->misc.re_verse) {
	newvt->flags ^= REVERSE_VIDEO;
	ReverseVideo(newvt);
	newvt->misc.re_verse = !newvt->misc.re_verse; /* ReverseVideo toggles */
	refresh_needed = TRUE;
    }
    if(curvt->screen.mousecolor != newvt->screen.mousecolor
       || curvt->screen.mousecolorback != newvt->screen.mousecolorback) {
	recolor_cursor (newvt->screen.pointer_cursor, 
			newvt->screen.mousecolor,
			newvt->screen.mousecolorback);
	refresh_needed = TRUE;
    }
    if (curvt->misc.scrollbar != newvt->misc.scrollbar) {
	if (newvt->misc.scrollbar) {
	    ScrollBarOn (newvt, FALSE, FALSE);
	} else {
	    ScrollBarOff (&newvt->screen);
	}
	update_scrollbar();
    }
#ifdef STATUSLINE
    if (curvt->misc.statusnormal != newvt->misc.statusnormal) {
	newvt->screen.reversestatus = !term->misc.statusnormal;
	if (newvt->screen.statusheight)
	    ScrnRefresh(newvt->screen, newvt->screen.max_row + 1, 0, 1, newvt->screen.max_col + 1, False);
	update_reversestatus();
    }
    if (curvt->misc.statusline != newvt->misc.statusline) {
	if (newvt->misc.statusline) {
	    ShowStatus(&newvt->screen);
	} else {
	    HideStatus(&newvt->screen);
	}
	update_statusline();
    }
#endif /* STATUSLINE */

    return refresh_needed;
}

/*
 * Shows cursor at new cursor position in screen.
 */
ShowCursor()
{
	register TScreen *screen = &term->screen;
	register int x, y, flags;
#ifdef KTERM
	Char gset;
#else /* !KTERM */
	Char c;
	GC	currentGC;
#endif /* !KTERM */
	Boolean	in_selection;

	if (eventMode != NORMAL) return;

#ifdef STATUSLINE
	if (screen->cur_row - screen->topline > screen->max_row
	 && !screen->instatus)
#else /* !STATUSLINE */
	if (screen->cur_row - screen->topline > screen->max_row)
#endif /* !STATUSLINE */
		return;
#ifdef KTERM
	gset = screen->buf[y = screen->cursor_row = screen->cur_row]
				[x = screen->cursor_col = screen->cur_col].gset;
	if (gset == MBC2) {
		gset = screen->buf[y][x-1].gset;
		x--;
	}
	flags = screen->buf[y][x].attr;
#else /* !KTERM */
	c = screen->buf[y = 2 * (screen->cursor_row = screen->cur_row)]
	 [x = screen->cursor_col = screen->cur_col];
	flags = screen->buf[y + 1][x];
	if (c == 0)
		c = ' ';
# ifdef STATUSLINE
	if (screen->instatus && screen->reversestatus)
		flags ^= INVERSE;
# endif /* STATUSLINE */
#endif /* !KTERM */

	if (screen->cur_row > screen->endHRow ||
	    (screen->cur_row == screen->endHRow &&
	     screen->cur_col >= screen->endHCol) ||
	    screen->cur_row < screen->startHRow ||
	    (screen->cur_row == screen->startHRow &&
	     screen->cur_col < screen->startHCol))
	    in_selection = False;
	else
	    in_selection = True;

#ifdef KTERM
	if ((screen->select || screen->always_highlight) ^ in_selection)
		flags ^= INVERSE;
	ScreenDraw(screen, y, x, x+(gset&MBCS?2:1), flags, True);
#else /* !KTERM */
	if(screen->select || screen->always_highlight) {
		if (( (flags & INVERSE) && !in_selection) ||
		    (!(flags & INVERSE) &&  in_selection)){
		    /* text is reverse video */
		    if (screen->cursorGC) {
			currentGC = screen->cursorGC;
		    } else {
			if (flags & BOLD) {
				currentGC = screen->normalboldGC;
			} else {
				currentGC = screen->normalGC;
			}
		    }
		} else { /* normal video */
		    if (screen->reversecursorGC) {
			currentGC = screen->reversecursorGC;
		    } else {
			if (flags & BOLD) {
				currentGC = screen->reverseboldGC;
			} else {
				currentGC = screen->reverseGC;
			}
		    }
		}
	} else { /* not selected */
		if (( (flags & INVERSE) && !in_selection) ||
		    (!(flags & INVERSE) &&  in_selection)) {
		    /* text is reverse video */
			currentGC = screen->reverseGC;
		} else { /* normal video */
			currentGC = screen->normalGC;
		}
	    
	}

	x = CursorX (screen, screen->cur_col);
	y = CursorY(screen, screen->cur_row) + 
	  screen->fnt_norm->ascent;
	XDrawImageString(screen->display, TextWindow(screen), currentGC,
		x, y, (char *) &c, 1);

	if((flags & BOLD) && screen->enbolden) /* no bold font */
		XDrawString(screen->display, TextWindow(screen), currentGC,
			x + 1, y, (char *) &c, 1);
	if(flags & UNDERLINE) 
		XDrawLine(screen->display, TextWindow(screen), currentGC,
			x, y+1, x + FontWidth(screen), y+1);
	if (!screen->select && !screen->always_highlight) {
		screen->box->x = x;
		screen->box->y = y - screen->fnt_norm->ascent;
		XDrawLines (screen->display, TextWindow(screen), 
			    screen->cursoroutlineGC ? screen->cursoroutlineGC 
			    			    : currentGC,
			    screen->box, NBOX, CoordModePrevious);
	}
#endif /* !KTERM */
	screen->cursor_state = ON;
#ifdef KTERM_XIM
	IMSendSpot(screen);
#endif /* KTERM_XIM */
#ifdef KTERM_KINPUT2
	Kinput2SendSpot();
#endif /* KTERM_KINPUT2 */
}

/*
 * hide cursor at previous cursor position in screen.
 */
HideCursor()
{
	register TScreen *screen = &term->screen;
#ifndef KTERM
	GC	currentGC;
#endif /* !KTERM */
	register int x, y, flags;
#ifdef KTERM
	Char gset;
#else /* !KTERM */
	char c;
#endif /* !KTERM */
	Boolean	in_selection;

#ifdef STATUSLINE
	Boolean in_status = (screen->cursor_row > screen->max_row);

	if(screen->cursor_row - screen->topline > screen->max_row && !in_status)
#else /* !STATUSLINE */
	if(screen->cursor_row - screen->topline > screen->max_row)
#endif /* !STATUSLINE */
		return;
#ifdef KTERM
	gset = screen->buf[y = screen->cursor_row][x = screen->cursor_col].gset;
	if (gset == MBC2) {
		gset = screen->buf[y][x-1].gset;
		x--;
	}
	flags = screen->buf[y][x].attr;
#else /* !KTERM */
	c = screen->buf[y = 2 * screen->cursor_row][x = screen->cursor_col];
	flags = screen->buf[y + 1][x];
# ifdef STATUSLINE
	if (in_status && screen->reversestatus)
		flags ^= INVERSE;
# endif /* STATUSLINE */
#endif /* !KTERM */

	if (screen->cursor_row > screen->endHRow ||
	    (screen->cursor_row == screen->endHRow &&
	     screen->cursor_col >= screen->endHCol) ||
	    screen->cursor_row < screen->startHRow ||
	    (screen->cursor_row == screen->startHRow &&
	     screen->cursor_col < screen->startHCol))
	    in_selection = False;
	else
	    in_selection = True;

#ifdef KTERM
	if (in_selection)
		flags ^= INVERSE;
	ScreenDraw(screen, y, x, x+(gset&MBCS?2:1), flags, False);
#else /* !KTERM */
	if (( (flags & INVERSE) && !in_selection) ||
	    (!(flags & INVERSE) &&  in_selection)) {
		if(flags & BOLD) {
			currentGC = screen->reverseboldGC;
		} else {
			currentGC = screen->reverseGC;
		}
	} else {
		if(flags & BOLD) {
			currentGC = screen->normalboldGC;
		} else {
			currentGC = screen->normalGC;
		}
	}

	if (c == 0)
		c = ' ';
	x = CursorX (screen, screen->cursor_col);
# ifdef STATUSLINE
	y = CursorY (screen, screen->cursor_row);
# else /* !STATUSLINE */
	y = (((screen->cursor_row - screen->topline) * FontHeight(screen))) +
	 screen->border;
# endif /* !STATUSLINE */
	y = y+screen->fnt_norm->ascent;
	XDrawImageString(screen->display, TextWindow(screen), currentGC,
		x, y, &c, 1);
	if((flags & BOLD) && screen->enbolden)
		XDrawString(screen->display, TextWindow(screen), currentGC,
			x + 1, y, &c, 1);
	if(flags & UNDERLINE) 
		XDrawLine(screen->display, TextWindow(screen), currentGC,
			x, y+1, x + FontWidth(screen), y+1);
#endif /* !KTERM */
	screen->cursor_state = OFF;
}

VTReset(full)
    Boolean full;
{
	register TScreen *screen = &term->screen;

	/* reset scrolling region */
	screen->top_marg = 0;
	screen->bot_marg = screen->max_row;
	term->flags &= ~ORIGIN;
	if(full) {
		TabReset (term->tabs);
		term->keyboard.flags = 0;
		update_appcursor();
		update_appkeypad();
#ifdef KTERM
		screen->gsets[0] = GSET_ASCII;
# ifdef KTERM_KANJIMODE
		screen->gsets[1] = (term->flags & EUC_KANJI)
					? GSET_KANJI : GSET_KANA;
		screen->gsets[2] = (term->flags & EUC_KANJI)
					? GSET_KANA : GSET_ASCII;
		screen->gsets[3] = (term->flags & EUC_KANJI)
					? GSET_HOJOKANJI : GSET_ASCII;
# else /* !KTERM_KANJIMODE */
		screen->gsets[1] = GSET_KANA;
		screen->gsets[2] = GSET_ASCII;
		screen->gsets[3] = GSET_ASCII;
# endif /* !KTERM_KANJIMODE */
		screen->curgl = 0;
		screen->curgr = 1;
#else /* !KTERM */
		screen->gsets[0] = 'B';
		screen->gsets[1] = 'B';
		screen->gsets[2] = 'B';
		screen->gsets[3] = 'B';
		screen->curgl = 0;
		screen->curgr = 2;
#endif /* !KTERM */
		screen->curss = 0;
		FromAlternate(screen);
		ClearScreen(screen);
#ifdef STATUSLINE
		EraseStatus();
#endif /* STATUSLINE */
		screen->cursor_state = OFF;
		if (term->flags & REVERSE_VIDEO)
			ReverseVideo(term);

		term->flags = term->initflags;
		update_reversevideo();
		update_autowrap();
		update_reversewrap();
		update_autolinefeed();
		screen->jumpscroll = !(term->flags & SMOOTHSCROLL);
		update_jumpscroll();
		if(screen->c132 && (term->flags & IN132COLUMNS)) {
		        Dimension junk;
			XtMakeResizeRequest(
			    (Widget) term,
			    (Dimension) 80*FontWidth(screen)
				+ 2 * screen->border + screen->scrollbar,
#ifdef STATUSLINE
			    (Dimension) screen->statusheight +
#endif /* STATUSLINE */
			    (Dimension) FontHeight(screen)
			        * (screen->max_row + 1) + 2 * screen->border,
			    &junk, &junk);
			XSync(screen->display, FALSE);	/* synchronize */
#ifdef KTERM_XAW3D
			if(XtAppPending(app_con))
#else /* !KTERM_XAW3D */
			if(QLength(screen->display) > 0)
#endif /* !KTERM_XAW3D */
				xevents();
		}
		CursorSet(screen, 0, 0, term->flags);
	}
	longjmp(vtjmpbuf, 1);	/* force ground state in parser */
}


#ifdef STATUSLINE
ToStatus(col)
int col;
{
	register TScreen *screen = &term->screen;

	if (screen->cursor_state)
		HideCursor();
	if (col > screen->max_col)
		col = screen->max_col;
	if (!screen->instatus) {
		if (!screen->statusheight)
			ShowStatus();
		CursorSave(term, &screen->statussc);
		screen->instatus = TRUE;
		screen->cur_row = screen->max_row + 1;
	}
	screen->cur_col = col;
}

FromStatus()
{
	register TScreen *screen = &term->screen;

	if (!screen->instatus)
		return;
	screen->instatus = FALSE;
	CursorRestore(term, &screen->statussc);
}

ShowStatus()
{
	register TScreen *screen = &term->screen;

	if (screen->statusheight)
		return;
	screen->statusheight = FontHeight(screen) + 2;
	DoResizeScreen(term);
	if (screen->scrollWidget)
		ResizeScrollBar(screen->scrollWidget, -1, -1,
		  Height(screen) + screen->border * 2 + screen->statusheight);
}

HideStatus()
{
	register TScreen *screen = &term->screen;
# ifndef KTERM
	register int i, j;
# endif /* !KTERM */

	if (!screen->statusheight)
		return;
	if (screen->instatus)
		FromStatus();
	screen->statusheight = 0;
# ifdef KTERM
	bzero(screen->buf[screen->max_row + 1],
		sizeof(Bchr) * (screen->max_col+1));
# else /* !KTERM */
	bzero(screen->buf[i = 2 * (screen->max_row + 1)],
		j = screen->max_col + 1);
	bzero(screen->buf[i + 1], j);
# endif /* !KTERM */
	DoResizeScreen(term);
	if (screen->scrollWidget)
		ResizeScrollBar(screen->scrollWidget, -1, -1,
		  Height(screen) + screen->border * 2);
}

EraseStatus()
{
	register TScreen *screen = &term->screen;
	register int j, pix;
# ifdef KTERM
	int fnum = F_ISO8859_1; /* *GC */
# else /* !KTERM */
	register int i;
# endif /* !KTERM */

	if (!screen->statusheight)
		return;
# ifdef KTERM
	bzero(screen->buf[screen->max_row + 1],
		j = sizeof(Bchr) * (screen->max_col+1));
# else /* !KTERM */
	bzero(screen->buf[i = 2 * (screen->max_row + 1)],
		j = screen->max_col + 1);
	bzero(screen->buf[i + 1], j) ;
# endif /* !KTERM */
	XFillRectangle(screen->display, TextWindow(screen),
		screen->reversestatus ? screen->normalGC : screen->reverseGC,
		screen->border - 1 + screen->scrollbar,
		Height(screen) + screen->border * 2 + 1,
		Width(screen),
		screen->statusheight - 2);
}

StatusBox(screen)
register TScreen *screen;
{
# ifdef KTERM
	int fnum = F_ISO8859_1; /* *GC */
# endif /* KTERM */
	XDrawRectangle(screen->display, TextWindow(screen),
		       screen->normalGC,
		       screen->scrollbar,
		       Height(screen) + screen->border * 2,
		       Width(screen) + screen->border * 2 - 1,
		       screen->statusheight - 1);
}
#endif /* STATUSLINE */

/*
 * set_character_class - takes a string of the form
 * 
 *                 low[-high]:val[,low[-high]:val[...]]
 * 
 * and sets the indicated ranges to the indicated values.
 */

int set_character_class (s)
    register char *s;
{
    register int i;			/* iterator, index into s */
    int len;				/* length of s */
    int acc;				/* accumulator */
    int low, high;			/* bounds of range [0..127] */
    int base;				/* 8, 10, 16 (octal, decimal, hex) */
    int numbers;			/* count of numbers per range */
    int digits;				/* count of digits in a number */
    static char *errfmt = "%s:  %s in range string \"%s\" (position %d)\n";
    extern char *ProgramName;

    if (!s || !s[0]) return -1;

    base = 10;				/* in case we ever add octal, hex */
    low = high = -1;			/* out of range */

    for (i = 0, len = strlen (s), acc = 0, numbers = digits = 0;
	 i < len; i++) {
	char c = s[i];

	if (isspace(c)) {
	    continue;
	} else if (isdigit(c)) {
	    acc = acc * base + (c - '0');
	    digits++;
	    continue;
	} else if (c == '-') {
	    low = acc;
	    acc = 0;
	    if (digits == 0) {
		fprintf (stderr, errfmt, ProgramName, "missing number", s, i);
		return (-1);
	    }
	    digits = 0;
	    numbers++;
	    continue;
	} else if (c == ':') {
	    if (numbers == 0)
	      low = acc;
	    else if (numbers == 1)
	      high = acc;
	    else {
		fprintf (stderr, errfmt, ProgramName, "too many numbers",
			 s, i);
		return (-1);
	    }
	    digits = 0;
	    numbers++;
	    acc = 0;
	    continue;
	} else if (c == ',') {
	    /*
	     * now, process it
	     */

	    if (high < 0) {
		high = low;
		numbers++;
	    }
	    if (numbers != 2) {
		fprintf (stderr, errfmt, ProgramName, "bad value number", 
			 s, i);
	    } else if (SetCharacterClassRange (low, high, acc) != 0) {
		fprintf (stderr, errfmt, ProgramName, "bad range", s, i);
	    }

	    low = high = -1;
	    acc = 0;
	    digits = 0;
	    numbers = 0;
	    continue;
	} else {
	    fprintf (stderr, errfmt, ProgramName, "bad character", s, i);
	    return (-1);
	}				/* end if else if ... else */

    }

    if (low < 0 && high < 0) return (0);

    /*
     * now, process it
     */

    if (high < 0) high = low;
    if (numbers < 1 || numbers > 2) {
	fprintf (stderr, errfmt, ProgramName, "bad value number", s, i);
    } else if (SetCharacterClassRange (low, high, acc) != 0) {
	fprintf (stderr, errfmt, ProgramName, "bad range", s, i);
    }

    return (0);
}

/* ARGSUSED */
static void HandleKeymapChange(w, event, params, param_count)
    Widget w;
    XEvent *event;
    String *params;
    Cardinal *param_count;
{
    static XtTranslations keymap, original;
    static XtResource key_resources[] = {
	{ XtNtranslations, XtCTranslations, XtRTranslationTable,
	      sizeof(XtTranslations), 0, XtRTranslationTable, (XtPointer)NULL}
    };
    char mapName[1000];
    char mapClass[1000];

    if (*param_count != 1) return;

    if (original == NULL) original = w->core.tm.translations;

    if (strcmp(params[0], "None") == 0) {
	XtOverrideTranslations(w, original);
	return;
    }
    (void) sprintf( mapName, "%sKeymap", params[0] );
    (void) strcpy( mapClass, mapName );
    if (islower(mapClass[0])) mapClass[0] = toupper(mapClass[0]);
    XtGetSubresources( w, (XtPointer)&keymap, mapName, mapClass,
		       key_resources, (Cardinal)1, NULL, (Cardinal)0 );
    if (keymap != NULL)
	XtOverrideTranslations(w, keymap);
}


/* ARGSUSED */
static void HandleBell(w, event, params, param_count)
    Widget w;
    XEvent *event;		/* unused */
    String *params;		/* [0] = volume */
    Cardinal *param_count;	/* 0 or 1 */
{
    int percent = (*param_count) ? atoi(params[0]) : 0;

#ifdef XKB
    int which= XkbBI_TerminalBell;
    XkbStdBell(XtDisplay(w),XtWindow(w),percent,which);
#else
    XBell( XtDisplay(w), percent );
#endif
}


/* ARGSUSED */
static void HandleVisualBell(w, event, params, param_count)
    Widget w;
    XEvent *event;		/* unused */
    String *params;		/* unused */
    Cardinal *param_count;	/* unused */
{
    VisualBell();
}


/* ARGSUSED */
static void HandleIgnore(w, event, params, param_count)
    Widget w;
    XEvent *event;		/* unused */
    String *params;		/* unused */
    Cardinal *param_count;	/* unused */
{
    /* do nothing, but check for funny escape sequences */
    (void) SendMousePosition(w, event);
}


/* ARGSUSED */
static void
DoSetSelectedFont(w, client_data, selection, type, value, length, format)
    Widget w;
    XtPointer client_data;
    Atom *selection, *type;
    XtPointer value;
    unsigned long *length;
    int *format;
{
    char *val = (char *)value;
    int len;
    if (*type != XA_STRING  ||  *format != 8) {
	Bell(XkbBI_MinorError,0);
	return;
    }
    len = strlen(val);
    if (len > 0) {
	if (val[len-1] == '\n') val[len-1] = '\0';
	/* Do some sanity checking to avoid sending a long selection
	   back to the server in an OpenFont that is unlikely to succeed.
	   XLFD allows up to 255 characters and no control characters;
	   we are a little more liberal here. */
	if (len > 1000  ||  strchr(val, '\n'))
	    return;
#ifdef KTERM
/*
	if (term->screen.menu_font_list[fontMenu_fontsel])
		XtFree(term->screen.menu_font_list[fontMenu_fontsel]);
	term->screen.menu_font_list[fontMenu_fontsel] = XtMalloc(len + 1);
	strcpy(term->screen.menu_font_list[fontMenu_fontsel], val);
*/
	term->screen.menu_font_list[fontMenu_fontsel] = val; /* XXX */
	if (!LoadNewFont (&term->screen, True, fontMenu_fontsel))
#else /* !KTERM */
	if (!LoadNewFont (&term->screen, val, NULL, True, fontMenu_fontsel))
#endif /* !KTERM */
	    Bell(XkbBI_MinorError,0);
    }
}

void FindFontSelection (atom_name, justprobe)
    char *atom_name;
    Bool justprobe;
{
    static AtomPtr *atoms;
    static int atomCount = 0;
    AtomPtr *pAtom;
    int a;
    Atom target;

    if (!atom_name) atom_name = "PRIMARY";

    for (pAtom = atoms, a = atomCount; a; a--, pAtom++) {
	if (strcmp(atom_name, XmuNameOfAtom(*pAtom)) == 0) break;
    }
    if (!a) {
	atoms = (AtomPtr*) XtRealloc ((char *)atoms,
				      sizeof(AtomPtr)*(atomCount+1));
	*(pAtom = &atoms[atomCount++]) = XmuMakeAtom(atom_name);
    }

    target = XmuInternAtom(XtDisplay(term), *pAtom);
    if (justprobe) {
#ifdef KTERM
	term->screen.menu_font_list[fontMenu_fontsel] = 
#else /* !KTERM */
	term->screen.menu_font_names[fontMenu_fontsel] = 
#endif /* KTERM */
	  XGetSelectionOwner(XtDisplay(term), target) ? _Font_Selected_ : NULL;
    } else {
	XtGetSelectionValue((Widget)term, target, XA_STRING,
			    DoSetSelectedFont, NULL,
			    XtLastTimestampProcessed(XtDisplay(term)));
    }
    return;
}


/* ARGSUSED */
void HandleSetFont(w, event, params, param_count)
    Widget w;
    XEvent *event;		/* unused */
    String *params;		/* unused */
    Cardinal *param_count;	/* unused */
{
    int fontnum;
    char *name1 = NULL, *name2 = NULL;

    if (*param_count == 0) {
	fontnum = fontMenu_fontdefault;
    } else {
	int maxparams = 1;		/* total number of params allowed */

	switch (params[0][0]) {
	  case 'd': case 'D': case '0':
	    fontnum = fontMenu_fontdefault; break;
	  case '1':
	    fontnum = fontMenu_font1; break;
	  case '2':
	    fontnum = fontMenu_font2; break;
	  case '3':
	    fontnum = fontMenu_font3; break;
	  case '4':
	    fontnum = fontMenu_font4; break;
	  case '5':
	    fontnum = fontMenu_font5; break;
	  case '6':
	    fontnum = fontMenu_font6; break;
	  case 'e': case 'E':
	    fontnum = fontMenu_fontescape; maxparams = 3; break;
	  case 's': case 'S':
	    fontnum = fontMenu_fontsel; maxparams = 2; break;
	  default:
	    Bell(XkbBI_MinorError,0);
	    return;
	}
	if (*param_count > maxparams) {	 /* see if extra args given */
	    Bell(XkbBI_MinorError,0);
	    return;
	}
	switch (*param_count) {		/* assign 'em */
	  case 3:
	    name2 = params[2];
	    /* fall through */
	  case 2:
	    name1 = params[1];
	    break;
	}
    }

    SetVTFont (fontnum, True, name1, name2);
}


void SetVTFont (i, doresize, name1, name2)
    int i;
    Bool doresize;
    char *name1, *name2;
{
    TScreen *screen = &term->screen;

    if (i < 0 || i >= NMENUFONTS) {
	Bell(XkbBI_MinorError,0);
	return;
    }
    if (i == fontMenu_fontsel) {	/* go get the selection */
	FindFontSelection (name1, False);  /* name1 = atom, name2 is ignored */
	return;
    }
#ifdef KTERM
    if (i == fontMenu_fontescape) {
	if (name1) {
	    if (screen->menu_font_list[i]) XtFree(screen->menu_font_list[i]);
	    screen->menu_font_list[i] = XtMalloc(strlen(name1) + 1);
	    strcpy(screen->menu_font_list[i], name1);
	}
	if (name2) {
	    if (screen->menu_bfont_list[i]) XtFree(screen->menu_bfont_list[i]);
	    screen->menu_bfont_list[i] = XtMalloc(strlen(name2) + 1);
	    strcpy(screen->menu_bfont_list[i], name2);
	}
    }
    if (!LoadNewFont(screen, doresize, i)) {
	Bell(XkbBI_MinorError,0);
    }
#else /* !KTERM */
    if (!name1) name1 = screen->menu_font_names[i];
    if (!LoadNewFont(screen, name1, name2, doresize, i)) {
	Bell(XkbBI_MinorError,0);
    }
#endif /* !KTERM */
    return;
}

#ifdef KTERM
static char **
ListFonts(display, pattern, maxnames, actual_count_return)
Display *display;
char *pattern;
int maxnames;
int *actual_count_return;
{
	static struct fontlist_t {
		char	*pattern;
		char	**list;
		int	count;
	} *fontlists;
	static int fontlistscnt;
	static int fontlistslen;

	int i, count;

	for (i = 0; i < fontlistscnt; i++) {
		if (!XmuCompareISOLatin1(fontlists[i].pattern, pattern)) {
			if (actual_count_return)
				*actual_count_return = fontlists[i].count;
			return fontlists[i].list;
		}
	}

	if (fontlistscnt == fontlistslen) {
		fontlistslen += 10;
		if (fontlists) {
			fontlists = (struct fontlist_t *)XtRealloc((char *)fontlists, fontlistslen * sizeof(struct fontlist_t));
		} else {
			fontlists = (struct fontlist_t *)XtMalloc(fontlistslen * sizeof(struct fontlist_t));
		}
	}

	fontlistscnt++;
	fontlists[i].pattern = XtMalloc(strlen(pattern) + 1);
	strcpy(fontlists[i].pattern, pattern);
	fontlists[i].list = XListFonts(display, pattern, maxnames, &count);
	fontlists[i].count = count;

	if (actual_count_return)
		*actual_count_return = fontlists[i].count;
	return fontlists[i].list;
}

static char *
search_font_matching(screen, xlfdlist, fnum)
TScreen *screen;
char *xlfdlist;
int fnum;
{
	extern char **ParseList();
	extern char **csnames();
	char **list;
	char **parsed_xlfdlist, **xlfd_p;
	char **tail_p;
	char *tmptail;
	int count, i;
	int taillen;

	if (!xlfdlist) return NULL;

	parsed_xlfdlist = ParseList(xlfdlist);

	for (tail_p = csnames(fnum); *tail_p; tail_p++) {
		taillen = strlen(*tail_p);

		for (xlfd_p = parsed_xlfdlist; *xlfd_p; xlfd_p++) {
			list = ListFonts(screen->display, *xlfd_p, 1000, &count);
			if (!list) continue;

			for (i = 0; i < count; i ++) {
				tmptail = list[i]+strlen(list[i])-taillen;
				if (tmptail[-1] == '-'
				 && !XmuCompareISOLatin1(tmptail, *tail_p)) {
					return list[i];
				}
			}
		}
	}
	return NULL;
}

int
LoadOneFont(screen, doresize, fontnum, fnum, bold)
TScreen *screen;
int doresize; /* -1: recursive call */
int fontnum;
int fnum;
Boolean bold;
{
    XFontStruct *fs = NULL;
    XFontStruct *normalfs = screen->fnt_norm;
    XFontStruct *asciinfs = screen->_fnt_norm[F_ISO8859_1];
    XGCValues xgcv;
    unsigned long mask;
    GC new_normalGC = NULL;
    GC new_reverseGC = NULL;
    char *fontname;
    char *fn = (bold?screen->menu_bfont_names:screen->menu_font_names)[fontnum];
    char *fl = (bold?screen->menu_bfont_list:screen->menu_font_list)[fontnum];
    int f;

    if (fontname = fn)
	fs = XLoadQueryFont (screen->display, fontname);

    if (!fs && (fontname = search_font_matching(screen, fl, fnum)))
	fs = XLoadQueryFont (screen->display, fontname);

    if (!fs) {
	if (bold) {
	    if (!normalfs) {
		LoadOneFont(screen, -1, fontnum, fnum, False);
		normalfs = screen->fnt_norm;
	    }
	    fs = normalfs;
	} else if (fnum != F_ISO8859_1) {
	    fs = asciinfs;
	}
    }
    if (!fs) goto bad;

    if (bold && fs == normalfs) {
	new_normalGC = screen->normalGC;
	new_reverseGC = screen->reverseGC;
    } else {
	if (fs->ascent + fs->descent == 0  ||  fs->max_bounds.width == 0)
	    goto bad;		/* can't use a 0-sized font */

	mask = (GCFont | GCForeground | GCBackground | GCGraphicsExposures |
		GCFunction);

	xgcv.font = fs->fid;
	xgcv.foreground = screen->foreground;
	xgcv.background = term->core.background_pixel;
	xgcv.graphics_exposures = FALSE;
	xgcv.function = GXcopy;

	new_normalGC = XtGetGC((Widget)term, mask, &xgcv);
	if (!new_normalGC) goto bad;

	xgcv.foreground = term->core.background_pixel;
	xgcv.background = screen->foreground;

	new_reverseGC = XtGetGC((Widget)term, mask, &xgcv);
	if (!new_reverseGC) goto bad;
    }

    if (bold) {
	screen->normalboldGC = new_normalGC;
	screen->reverseboldGC = new_reverseGC;
	screen->fnt_bold = fs;
    } else {
	screen->normalGC = new_normalGC;
	screen->reverseGC = new_reverseGC;
	screen->fnt_norm = fs;
    }

    if (!bold) {
	set_cursor_gcs (screen, fnum);
    }
    if (doresize != -1)
	update_font_info (screen, doresize);
    return 1;

  bad:
    if (new_normalGC)
      XtReleaseGC ((Widget) term, new_normalGC);
    if (new_reverseGC)
      XtReleaseGC ((Widget) term, new_reverseGC);
    if (fs && (fnum == F_ISO8859_1 || fs != asciinfs))
      XFreeFont (screen->display, fs);
    return 0;
}

int
LoadNewFont(screen, doresize, fontnum)
TScreen *screen;
Boolean doresize;
int fontnum;
{
    int fnum, f, succ = 0;
    GC nGC[FCNT], rGC[FCNT], nbGC[FCNT], rbGC[FCNT];
    XFontStruct *fn[FCNT], *fb[FCNT], *fs;
    int enb[FCNT];

    for (fnum = F_ISO8859_1; fnum < FCNT; fnum++) {
	nGC[fnum] = screen->_normalGC[fnum];
	screen->_normalGC[fnum] = NULL;
	rGC[fnum] = screen->_reverseGC[fnum];
	screen->_reverseGC[fnum] = NULL;
	fn[fnum] = screen->_fnt_norm[fnum];
	screen->_fnt_norm[fnum] = NULL;

	nbGC[fnum] = screen->_normalboldGC[fnum];
	screen->_normalboldGC[fnum] = NULL;
	rbGC[fnum] = screen->_reverseboldGC[fnum];
	screen->_reverseboldGC[fnum] = NULL;
	fb[fnum] = screen->_fnt_bold[fnum];
	screen->_fnt_bold[fnum] = NULL;
    }

    for (fnum = F_ISO8859_1; fnum < FCNT; fnum++) {
	if (!screen->dynamic_font_load || fn[fnum] || fnum == F_ISO8859_1) {
	    succ += LoadOneFont(screen, -1, fontnum, fnum, False);
	}
	if (!screen->dynamic_font_load || fb[fnum]) {
	    succ += LoadOneFont(screen, -1, fontnum, fnum, True);
	}
    }

    if (!succ) goto bad;

    for (fnum = F_ISO8859_1; fnum < FCNT; fnum++) {
	if (fs = fn[fnum]) {
	    XtReleaseGC((Widget)term, nGC[fnum]);
	    XtReleaseGC((Widget)term, rGC[fnum]);
	    XFreeFontInfo(NULL, fn[fnum], 1);
	    for (f = F_ISO8859_1; f < FCNT; f++) {
		if (fn[f] == fs) fn[f] = NULL;
		if (fb[f] == fs) fb[f] = NULL;
	    }
	}

	if (fs = fb[fnum]) {
	    XtReleaseGC((Widget)term, nbGC[fnum]);
	    XtReleaseGC((Widget)term, rbGC[fnum]);
	    XFreeFontInfo(NULL, fb[fnum], 1);
	    for (f = F_ISO8859_1; f < FCNT; f++) {
		if (fn[f] == fs) fn[f] = NULL;
		if (fb[f] == fs) fb[f] = NULL;
	    }
	}
    }

    set_menu_font (False);
    screen->menu_font_number = fontnum;
    set_menu_font (True);
    if (fontnum == fontMenu_fontescape) {
	set_sensitivity (term->screen.fontMenu,
			 fontMenuEntries[fontMenu_fontescape].widget,
			 TRUE);
    }
#ifdef KTERM_XIM
    IMSendFonts(screen);
#endif /* KTERM_XIM */
#ifdef KTERM_KINPUT2
    Kinput2SendFonts();
#endif /* KTERM_KINPUT2 */
    update_font_info (screen, doresize);
    return 1;

  bad:
    for (fnum = F_ISO8859_1; fnum < FCNT; fnum++) {
	screen->_normalGC[fnum] = nGC[fnum];
	screen->_reverseGC[fnum] = rGC[fnum];
	screen->_fnt_norm[fnum] = fn[fnum];

	screen->_normalboldGC[fnum] = nbGC[fnum];
	screen->_reverseboldGC[fnum] = rbGC[fnum];
	screen->_fnt_bold[fnum] = fb[fnum];
    }
    return 0;
}
#else /* !KTERM */
int LoadNewFont (screen, nfontname, bfontname, doresize, fontnum)
    TScreen *screen;
    char *nfontname, *bfontname;
    Bool doresize;
    int fontnum;
{
    XFontStruct *nfs = NULL, *bfs = NULL;
    XGCValues xgcv;
    unsigned long mask;
    GC new_normalGC = NULL, new_normalboldGC = NULL;
    GC new_reverseGC = NULL, new_reverseboldGC = NULL;
    char *tmpname = NULL;

    if (!nfontname) return 0;

    if (fontnum == fontMenu_fontescape &&
	nfontname != screen->menu_font_names[fontnum]) {
	tmpname = (char *) malloc (strlen(nfontname) + 1);
	if (!tmpname) return 0;
	strcpy (tmpname, nfontname);
    }

    if (!(nfs = XLoadQueryFont (screen->display, nfontname))) goto bad;
    if (nfs->ascent + nfs->descent == 0  ||  nfs->max_bounds.width == 0)
	goto bad;		/* can't use a 0-sized font */

    if (!(bfontname && 
	  (bfs = XLoadQueryFont (screen->display, bfontname))))
      bfs = nfs;
    else
	if (bfs->ascent + bfs->descent == 0  ||  bfs->max_bounds.width == 0)
	    goto bad;		/* can't use a 0-sized font */

    mask = (GCFont | GCForeground | GCBackground | GCGraphicsExposures |
	    GCFunction);

    xgcv.font = nfs->fid;
    xgcv.foreground = screen->foreground;
    xgcv.background = term->core.background_pixel;
    xgcv.graphics_exposures = TRUE;	/* default */
    xgcv.function = GXcopy;

    new_normalGC = XtGetGC((Widget)term, mask, &xgcv);
    if (!new_normalGC) goto bad;

    if (nfs == bfs) {			/* there is no bold font */
	new_normalboldGC = new_normalGC;
    } else {
	xgcv.font = bfs->fid;
	new_normalboldGC = XtGetGC((Widget)term, mask, &xgcv);
	if (!new_normalboldGC) goto bad;
    }

    xgcv.font = nfs->fid;
    xgcv.foreground = term->core.background_pixel;
    xgcv.background = screen->foreground;
    new_reverseGC = XtGetGC((Widget)term, mask, &xgcv);
    if (!new_reverseGC) goto bad;

    if (nfs == bfs) {			/* there is no bold font */
	new_reverseboldGC = new_reverseGC;
    } else {
	xgcv.font = bfs->fid;
	new_reverseboldGC = XtGetGC((Widget)term, mask, &xgcv);
	if (!new_reverseboldGC) goto bad;
    }

    if (screen->normalGC != screen->normalboldGC)
	XtReleaseGC ((Widget) term, screen->normalboldGC);
    XtReleaseGC ((Widget) term, screen->normalGC);
    if (screen->reverseGC != screen->reverseboldGC)
	XtReleaseGC ((Widget) term, screen->reverseboldGC);
    XtReleaseGC ((Widget) term, screen->reverseGC);
    screen->normalGC = new_normalGC;
    screen->normalboldGC = new_normalboldGC;
    screen->reverseGC = new_reverseGC;
    screen->reverseboldGC = new_reverseboldGC;
    screen->fnt_norm = nfs;
    screen->fnt_bold = bfs;
    screen->enbolden = (nfs == bfs);
    set_menu_font (False);
    screen->menu_font_number = fontnum;
    set_menu_font (True);
    if (tmpname) {			/* if setting escape or sel */
	if (screen->menu_font_names[fontnum])
	  free (screen->menu_font_names[fontnum]);
	screen->menu_font_names[fontnum] = tmpname;
	if (fontnum == fontMenu_fontescape) {
	    set_sensitivity (term->screen.fontMenu,
			     fontMenuEntries[fontMenu_fontescape].widget,
			     TRUE);
	}
    }
    set_cursor_gcs (screen);
    update_font_info (screen, doresize);
    return 1;

  bad:
    if (tmpname) free (tmpname);
    if (new_normalGC)
      XtReleaseGC ((Widget) term, screen->normalGC);
    if (new_normalGC && new_normalGC != new_normalboldGC)
      XtReleaseGC ((Widget) term, new_normalboldGC);
    if (new_reverseGC)
      XtReleaseGC ((Widget) term, new_reverseGC);
    if (new_reverseGC && new_reverseGC != new_reverseboldGC)
      XtReleaseGC ((Widget) term, new_reverseboldGC);
    if (nfs) XFreeFont (screen->display, nfs);
    if (bfs && nfs != bfs) XFreeFont (screen->display, bfs);
    return 0;
}
#endif /* !KTERM */


static void
update_font_info (screen, doresize)
    TScreen *screen;
    Bool doresize;
{
    int i, j, width, height, scrollbar_width;
#ifdef KTERM
    int fnum; /* fnt_norm, fnt_bold */
    int max_ascent = 0, max_descent = 0, max_width = 0;

    for (fnum = F_ISO8859_1; fnum < FCNT; fnum ++) {
	if (screen->fnt_norm) {
# ifdef KTERM_MBCS
	  if (screen->fnt_norm->max_byte1 > 0) /* MB font */
	    max_width = Max(max_width, screen->fnt_norm->max_bounds.width/2);
	  else
# endif /* KTERM_MBCS */
	    max_width = Max(max_width, screen->fnt_norm->max_bounds.width);
	  max_ascent = Max(max_ascent, screen->fnt_norm->ascent);
	  max_descent = Max(max_descent, screen->fnt_norm->descent);
	}
	if (screen->fnt_bold) {
# ifdef KTERM_MBCS
	  if (screen->fnt_bold->max_byte1 > 0) /* MB font */
	    max_width = Max(max_width, screen->fnt_bold->max_bounds.width/2);
	  else
# endif /* KTERM_MBCS */
	    max_width = Max(max_width, screen->fnt_bold->max_bounds.width);
	  max_ascent = Max(max_ascent, screen->fnt_bold->ascent);
	  max_descent = Max(max_descent, screen->fnt_bold->descent);
	}
    }
    doresize = (doresize
	     && (screen->fullVwin.f_width != max_width
	      || screen->max_ascent != max_ascent
	      || screen->max_descent != max_descent));
    screen->fullVwin.f_width = max_width;
    screen->max_ascent = max_ascent;
    screen->max_descent = max_descent;
    screen->fullVwin.f_height = max_ascent + max_descent;
#else /* !KTERM */

    screen->fullVwin.f_width = screen->fnt_norm->max_bounds.width;
    screen->fullVwin.f_height = (screen->fnt_norm->ascent +
				 screen->fnt_norm->descent);
#endif /* !KTERM */
    scrollbar_width = (term->misc.scrollbar ? 
		       screen->scrollWidget->core.width +
		       screen->scrollWidget->core.border_width : 0);
    i = 2 * screen->border + scrollbar_width;
    j = 2 * screen->border;
#ifdef STATUSLINE
    if (screen->statusheight)
	j += (screen->statusheight = FontHeight(screen) + 2);
#endif /* STATUSLINE */
#ifdef KTERM
    width = (screen->max_col + 1) * FontWidth(screen) + i;
    height = (screen->max_row + 1) * FontHeight(screen) + j;
#else /* !KTERM */
    width = (screen->max_col + 1) * screen->fullVwin.f_width + i;
    height = (screen->max_row + 1) * screen->fullVwin.f_height + j;
#endif /* !KTERM */
    screen->fullVwin.fullwidth = width;
    screen->fullVwin.fullheight = height;
    screen->fullVwin.width = width - i;
    screen->fullVwin.height = height - j;

    if (doresize) {
	if (VWindow(screen)) {
	    XClearWindow (screen->display, VWindow(screen));
	}
	DoResizeScreen (term);		/* set to the new natural size */
	if (screen->scrollWidget)
	  ResizeScrollBar (screen->scrollWidget, -1, -1,
#ifdef STATUSLINE
			   screen->statusheight +
#endif /* STATUSLINE */
			   Height(screen) + screen->border * 2);
	Redraw ();
    }
    set_vt_box (screen);
#ifdef KTERM
    set_vt_graphics (screen);
#endif /* KTERM */
}

set_vt_box (screen)
	TScreen *screen;
{
	XPoint	*vp;

#ifdef KTERM
	vp = &VTbox[1];
	(vp++)->x = FontWidth(screen) - 1;
	(vp++)->y = screen->fullVwin.f_height - 1;
	(vp++)->x = -(FontWidth(screen) - 1);
	vp->y = -(screen->fullVwin.f_height - 1);
# ifdef KTERM_MBCS
	vp = &VTwbox[1];
	(vp++)->x = FontWidth(screen) * 2 - 1;
	(vp++)->y = screen->fullVwin.f_height - 1;
	(vp++)->x = -(FontWidth(screen) * 2 - 1);
	vp->y = -(screen->fullVwin.f_height - 1);
# endif /* KTERM_MBCS */
	set_vt_box_per_gset(screen);
#else /* !KTERM */
	vp = &VTbox[1];
	(vp++)->x = FontWidth(screen) - 1;
	(vp++)->y = FontHeight(screen) - 1;
	(vp++)->x = -(FontWidth(screen) - 1);
	vp->y = -(FontHeight(screen) - 1);
	screen->box = VTbox;
#endif /* !KTERM */
}


#ifdef KTERM
set_vt_graphics (screen)
	TScreen *screen;
{
	static GC bmgc;
	static Pixmap gray;
	static Pixmap vtgraphics[256]; /* Bitmaps */
	XPoint pts[4];
	Display *dpy = screen->display;
	Window win = RootWindowOfScreen(XtScreen(term));
	int W = FontWidth(screen), H = FontHeight(screen);
	int w = W - 1, h = H - 1;
	int w2 = w/2, h2 = h/2;
	int i;

	if (!gray) {
		/*
		static char gray_bits[] = { 0x08, 0x02, 0x04, 0x01 };
		gray = XCreateBitmapFromData(dpy, win, gray_bits, 4, 4);
		*/
		static char gray_bits[] = { 0x11, 0x44 };
		gray = XCreateBitmapFromData(dpy, win, gray_bits, 8, 2);
	}
	if (!bmgc) {
		bmgc = XCreateGC(dpy, gray, 0, NULL);
	}

	for (i = 0; i < 256; i ++) {
		if (vtgraphics[i]) {
			XFreePixmap(dpy, vtgraphics[i]);
			vtgraphics[i] = 0;
		}
	}

	vtgraphics[' '] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['`'] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['a'] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['j'] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['k'] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['l'] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['m'] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['n'] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['o'] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['p'] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['q'] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['r'] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['s'] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['t'] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['u'] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['v'] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['w'] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['x'] = XCreatePixmap(dpy, win, W, H, 1);
	vtgraphics['~'] = XCreatePixmap(dpy, win, W, H, 1);

	XSetForeground(dpy, bmgc, 0);
	XSetFillStyle(dpy, bmgc, FillSolid);
	for (i = 0; i < 256; i ++) {
		if (vtgraphics[i]) {
			XFillRectangle(dpy, vtgraphics[i], bmgc, 0, 0, W, H);
		}
	}

	XSetForeground(dpy, bmgc, 1);

	pts[0].x = w2;   pts[0].y = 0;
	pts[1].x = 0;    pts[1].y = h2;
	pts[2].x = w2;   pts[2].y = h2*2;
	pts[3].x = w2*2; pts[3].y = h2;
	XFillPolygon(dpy, vtgraphics['`'], bmgc, pts, 4, Nonconvex, CoordModeOrigin);

	XSetFillStyle(dpy, bmgc, FillStippled);
	XSetStipple(dpy, bmgc, gray);
	XFillRectangle(dpy, vtgraphics['a'], bmgc, 0, 0, W, H);

	XSetFillStyle(dpy, bmgc, FillSolid);
	XDrawLine(dpy, vtgraphics['j'], bmgc, 0, h2, w2, h2);
	XDrawLine(dpy, vtgraphics['j'], bmgc, w2, 0, w2, h2);

	XDrawLine(dpy, vtgraphics['k'], bmgc, 0, h2, w2, h2);
	XDrawLine(dpy, vtgraphics['k'], bmgc, w2, h2, w2, h);

	XDrawLine(dpy, vtgraphics['l'], bmgc, w2, h2, w, h2);
	XDrawLine(dpy, vtgraphics['l'], bmgc, w2, h2, w2, h);

	XDrawLine(dpy, vtgraphics['m'], bmgc, w2, h2, w, h2);
	XDrawLine(dpy, vtgraphics['m'], bmgc, w2, 0, w2, h2);

	XDrawLine(dpy, vtgraphics['n'], bmgc, 0, h2, w, h2);
	XDrawLine(dpy, vtgraphics['n'], bmgc, w2, 0, w2, h);

	XDrawLine(dpy, vtgraphics['o'], bmgc, 0, 0, w, 0);

	XDrawLine(dpy, vtgraphics['p'], bmgc, 0, h/4, w, h/4);

	XDrawLine(dpy, vtgraphics['q'], bmgc, 0, h2, w, h2);

	XDrawLine(dpy, vtgraphics['r'], bmgc, 0, h*3/4, w, h*3/4);

	XDrawLine(dpy, vtgraphics['s'], bmgc, 0, h, w, h);

	XDrawLine(dpy, vtgraphics['t'], bmgc, w2, h2, w, h2);
	XDrawLine(dpy, vtgraphics['t'], bmgc, w2, 0, w2, h);

	XDrawLine(dpy, vtgraphics['u'], bmgc, 0, h2, w2, h2);
	XDrawLine(dpy, vtgraphics['u'], bmgc, w2, 0, w2, h);

	XDrawLine(dpy, vtgraphics['v'], bmgc, 0, h2, w, h2);
	XDrawLine(dpy, vtgraphics['v'], bmgc, w2, 0, w2, h2);

	XDrawLine(dpy, vtgraphics['w'], bmgc, 0, h2, w, h2);
	XDrawLine(dpy, vtgraphics['w'], bmgc, w2, h2, w2, h);

	XDrawLine(dpy, vtgraphics['x'], bmgc, w2, 0, w2, h);

	XDrawLine(dpy, vtgraphics['~'], bmgc, w2-1, h2, w2+1, h2);
	XDrawLine(dpy, vtgraphics['~'], bmgc, w2, h2-1, w2, h2+1);

	screen->graphics = vtgraphics;
}
#endif /* KTERM */


#ifdef KTERM
set_cursor_gcs (screen, fnum)
    int fnum;
#else /* !KTERM */
set_cursor_gcs (screen)
#endif /* !KTERM */
    TScreen *screen;
{
    XGCValues xgcv;
    unsigned long mask;
    unsigned long cc = screen->cursorcolor;
    unsigned long fg = screen->foreground;
    unsigned long bg = term->core.background_pixel;
    GC new_cursorGC = NULL, new_reversecursorGC = NULL;
    GC new_cursoroutlineGC = NULL;

    /*
     * Let's see, there are three things that have "color":
     *
     *     background
     *     text
     *     cursorblock
     *
     * And, there are four situation when drawing a cursor, if we decide
     * that we like have a solid block of cursor color with the letter
     * that it is highlighting shown in the background color to make it
     * stand out:
     *
     *     selected window, normal video - background on cursor
     *     selected window, reverse video - foreground on cursor
     *     unselected window, normal video - foreground on background
     *     unselected window, reverse video - background on foreground
     *
     * Since the last two are really just normalGC and reverseGC, we only
     * need two new GC's.  Under monochrome, we get the same effect as
     * above by setting cursor color to foreground.
     */

    xgcv.font = screen->fnt_norm->fid;
    mask = (GCForeground | GCBackground | GCFont);
    if (cc != fg && cc != bg) {
	/* we have a colored cursor */
	xgcv.foreground = fg;
	xgcv.background = cc;
	new_cursorGC = XtGetGC ((Widget) term, mask, &xgcv);

	if (screen->always_highlight) {
	    new_reversecursorGC = (GC) 0;
	    new_cursoroutlineGC = (GC) 0;
	} else {
	    xgcv.foreground = bg;
	    xgcv.background = cc;
	    new_reversecursorGC = XtGetGC ((Widget) term, mask, &xgcv);
	    xgcv.foreground = cc;
	    xgcv.background = bg;
	    new_cursoroutlineGC = XtGetGC ((Widget) term, mask, &xgcv);
		}
    } else {
	new_cursorGC = (GC) 0;
	new_reversecursorGC = (GC) 0;
	new_cursoroutlineGC = (GC) 0;
    }
    if (screen->cursorGC) XtReleaseGC ((Widget)term, screen->cursorGC);
    if (screen->reversecursorGC)
	XtReleaseGC ((Widget)term, screen->reversecursorGC);
    if (screen->cursoroutlineGC)
	XtReleaseGC ((Widget)term, screen->cursoroutlineGC);
    screen->cursorGC = new_cursorGC;
    screen->reversecursorGC = new_reversecursorGC;
    screen->cursoroutlineGC = new_cursoroutlineGC;
}

#ifdef KTERM
char **
ParseList(list)
char	*list;
{
	static char **params;
	static int nparams;
	static char *listbuf;
	static int listbufsize;
	char *p, *q, *s, *e;
	int n;
	int len;
	
	len = strlen(list);
	if (listbuf && listbufsize < len) {
		XtFree(listbuf);
		listbuf = NULL;
	}
	if (!listbuf) {
		listbuf = XtMalloc(len + 1);
		listbufsize = len;
	}

	strcpy(listbuf, list);

	n = 0;
	for (p = listbuf; q = index(p, ','); p = q + 1) {
		n++;
	}
	n++;
	if (params && nparams < n) {
		XtFree((char *)params);
		params = NULL;
	}
	if (!params) {
		params = (char **)XtMalloc(sizeof(char *) * (n + 1));
		nparams = n;
	}

	n = 0;
	for (p = listbuf; q = index(p, ','); p = q + 1) {
		for (e = q-1; p<=e && *e && isspace(*e); e--) /* empty */;
		for (s = p; s<=e && *s && isspace(*s); s++) /* empty */;
		params[n++] = s;
		*(e+1) = '\0';
	}
	for (e = p+strlen(p)-1; p<=e && *e && isspace(*e); e--) /* empty */;
	for (s = p; s<=e && *s && isspace(*s); s++) /* empty */;
	params[n++] = s;
	*(e+1) = '\0';
	params[n] = NULL;

	return params;
}
#endif /* KTERM */
