/* $XConsortium: menu.h,v 1.25 94/04/17 20:23:31 gildea Exp $ */
/* $Id: menu.h,v 6.2 1996/07/02 05:01:31 kagotani Rel $ */
/*

Copyright (c) 1989  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/

typedef struct _MenuEntry {
    char *name;
    void (*function)();
    Widget widget;
} MenuEntry;

#ifdef KTERM_NOTEK
extern MenuEntry mainMenuEntries[], vtMenuEntries[];
#else /* !KTERM_NOTEK */
extern MenuEntry mainMenuEntries[], vtMenuEntries[], tekMenuEntries[];
#endif /* !KTERM_NOTEK */
extern MenuEntry fontMenuEntries[];
extern Arg menuArgs[];

extern void HandleAllowSends();
extern void HandleSetVisualBell();
#ifdef ALLOWLOGGING
extern void HandleLogging();
#endif
extern void HandleRedraw();
extern void HandleSendSignal();
extern void HandleQuit();
extern void HandleScrollbar();
extern void HandleJumpscroll();
extern void HandleReverseVideo();
extern void HandleAutoWrap();
extern void HandleReverseWrap();
extern void HandleAutoLineFeed();
extern void HandleAppCursor();
extern void HandleAppKeypad();
extern void HandleScrollKey();
extern void HandleScrollTtyOutput();
extern void HandleAllow132();
extern void HandleCursesEmul();
extern void HandleMarginBell();
extern void HandleAltScreen();
extern void HandleSoftReset();
extern void HandleHardReset();
extern void HandleClearSavedLines();
extern void HandleSetTerminalType();
extern void HandleVisibility();
extern void HandleSetTekText();
extern void HandleTekPage();
extern void HandleTekReset();
extern void HandleTekCopy();
extern void DoSecureKeyboard();
#ifdef STATUSLINE
extern void HandleStatusLine();
extern void HandleStatusReverse();
#endif /* STATUSLINE */
#ifdef KTERM_KANJIMODE
extern void HandleSetKanjiMode();
#endif /* KTERM_KANJIMODE */

/*
 * The following definitions MUST match the order of entries given in 
 * the mainMenuEntries, vtMenuEntries, and tekMenuEntries arrays in menu.c.
 */

/*
 * items in primary menu
 */
#define mainMenu_securekbd 0
#define mainMenu_allowsends 1
#ifdef ALLOWLOGGING
#define mainMenu_logging 2
#endif
#define mainMenu_redraw 3
#define mainMenu_line1 4
#define mainMenu_suspend 5
#define mainMenu_continue 6
#define mainMenu_interrupt 7
#define mainMenu_hangup 8
#define mainMenu_terminate 9
#define mainMenu_kill 10
#define mainMenu_line2 11
#define mainMenu_quit 12


/*
 * items in vt100 mode menu
 */
#define vtMenu_scrollbar 0
#define vtMenu_jumpscroll 1
#define vtMenu_reversevideo 2
#define vtMenu_autowrap 3
#define vtMenu_reversewrap 4
#define vtMenu_autolinefeed 5
#define vtMenu_appcursor 6
#define vtMenu_appkeypad 7
#define vtMenu_scrollkey 8
#define vtMenu_scrollttyoutput 9
#define vtMenu_allow132 10
#define vtMenu_cursesemul 11
#define vtMenu_visualbell 12
#define vtMenu_marginbell 13
#define vtMenu_altscreen 14
#define vtMenu_line1 15
#define vtMenu_softreset 16
#define vtMenu_hardreset 17
#define vtMenu_clearsavedlines 18
#ifdef KTERM_NOTEK /* implies KTERM */
# define vtMenu_line3 19
#else /* !KTERM_NOTEK */
#define vtMenu_line2 19
#define vtMenu_tekshow 20
#define vtMenu_tekmode 21
#define vtMenu_vthide 22
# if defined(STATUSLINE) || defined(KTERM)
#  define vtMenu_line3 23
# endif /* STATUSLINE || KTERM */
#endif /* !KTERM_NOTEK */
#ifdef STATUSLINE
#  define vtMenu_statusline (vtMenu_line3+1)
#  define vtMenu_reversestatus (vtMenu_statusline+1)
#endif /* STATUSLINE */
#ifdef KTERM_KANJIMODE
#  ifdef vtMenu_reversestatus
#    define vtMenu_eucmode (vtMenu_reversestatus+1)
#  else
#    define vtMenu_eucmode (vtMenu_line3+1)
#  endif
#  define vtMenu_sjismode (vtMenu_eucmode+1)
#  define vtMenu_utf8mode (vtMenu_sjismode+1)
#endif /* KTERM_KANJIMODE */
#ifdef KTERM_XIM
#  ifdef vtMenu_utf8mode
#    define vtMenu_openim (vtMenu_utf8mode+1)
#  else
#    ifdef vtMenu_sjismode
#      define vtMenu_openim (vtMenu_sjismode+1)
#    else
#      ifdef vtMenu_reversestatus
#        define vtMenu_openim (vtMenu_reversestatus+1)
#      else
#        define vtMenu_openim (vtMenu_line3+1)
#      endif
#    endif
#  endif
#endif /* KTERM_XIM */

/*
 * items in vt100 font menu
 */
#define fontMenu_fontdefault 0
#define fontMenu_font1 1
#define fontMenu_font2 2
#define fontMenu_font3 3
#define fontMenu_font4 4
#define fontMenu_font5 5
#define fontMenu_font6 6
#define fontMenu_lastBuiltin fontMenu_font6
#define fontMenu_fontescape 7
#define fontMenu_fontsel 8
/* number of non-line items should match NMENUFONTS in ptyx.h */


#ifndef KTERM_NOTEK
/*
 * items in tek4014 mode menu
 */
#define tekMenu_tektextlarge 0
#define tekMenu_tektext2 1
#define tekMenu_tektext3 2
#define tekMenu_tektextsmall 3
#define tekMenu_line1 4
#define tekMenu_tekpage 5
#define tekMenu_tekreset 6
#define tekMenu_tekcopy 7
#define tekMenu_line2 8
#define tekMenu_vtshow 9
#define tekMenu_vtmode 10
#define tekMenu_tekhide 11
#endif /* !KTERM_NOTEK */


/*
 * macros for updating menus
 */

#define update_menu_item(w,mi,val) { if (mi) { \
    menuArgs[0].value = (XtArgVal) ((val) ? term->screen.menu_item_bitmap \
				          : None); \
    XtSetValues (mi, menuArgs, (Cardinal) 1); }}


#define set_sensitivity(w,mi,val) { if (mi) { \
    menuArgs[1].value = (XtArgVal) (val); \
    XtSetValues (mi, menuArgs+1, (Cardinal) 1);  }}



/*
 * there should be one of each of the following for each checkable item
 */


#define update_securekbd() \
  update_menu_item (term->screen.mainMenu, \
		    mainMenuEntries[mainMenu_securekbd].widget, \
		    term->screen.grabbedKbd)

#define update_allowsends() \
  update_menu_item (term->screen.mainMenu, \
		    mainMenuEntries[mainMenu_allowsends].widget, \
		    term->screen.allowSendEvents)

#ifdef ALLOWLOGGING
#define update_logging() \
  update_menu_item (term->screen.mainMenu, \
		    mainMenuEntries[mainMenu_logging].widget, \
		    term->screen.logging)
#endif

#define update_scrollbar() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_scrollbar].widget, \
		    term->screen.scrollbar)

#define update_jumpscroll() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_jumpscroll].widget, \
		    term->screen.jumpscroll)

#define update_reversevideo() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_reversevideo].widget, \
		    (term->flags & REVERSE_VIDEO))

#define update_autowrap() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_autowrap].widget, \
		    (term->flags & WRAPAROUND))

#define update_reversewrap() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_reversewrap].widget, \
		    (term->flags & REVERSEWRAP))

#define update_autolinefeed() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_autolinefeed].widget, \
		    (term->flags & LINEFEED))

#define update_appcursor() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_appcursor].widget, \
		    (term->keyboard.flags & CURSOR_APL))

#define update_appkeypad() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_appkeypad].widget, \
		    (term->keyboard.flags & KYPD_APL))

#define update_scrollkey() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_scrollkey].widget,  \
		    term->screen.scrollkey)

#define update_scrollttyoutput() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_scrollttyoutput].widget, \
		    term->screen.scrollttyoutput)

#define update_allow132() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_allow132].widget, \
		    term->screen.c132)
  
#define update_cursesemul() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_cursesemul].widget, \
		    term->screen.curses)

#define update_visualbell() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_visualbell].widget, \
		    term->screen.visualbell)

#define update_marginbell() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_marginbell].widget, \
		    term->screen.marginbell)

#define update_altscreen() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_altscreen].widget, \
		    term->screen.alternate)

#ifndef KTERM_NOTEK
#define update_tekshow() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_tekshow].widget, \
		    term->screen.Tshow)

#define update_vttekmode() { \
    update_menu_item (term->screen.vtMenu, \
		      vtMenuEntries[vtMenu_tekmode].widget, \
		      term->screen.TekEmu) \
    update_menu_item (term->screen.tekMenu, \
		      tekMenuEntries[tekMenu_vtmode].widget, \
		      !term->screen.TekEmu) }

#define update_vtshow() \
  update_menu_item (term->screen.tekMenu, \
		    tekMenuEntries[tekMenu_vtshow].widget, \
		    term->screen.Vshow)
#endif /* !KTERM_NOTEK */
#ifdef STATUSLINE
#define update_statusline() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_statusline].widget, \
		    term->screen.statusheight)
#define update_reversestatus() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_reversestatus].widget, \
		    term->screen.reversestatus)
#endif /* STATUSLINE */
#ifdef KTERM_KANJIMODE
#define update_eucmode() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_eucmode].widget, \
		    term->flags & EUC_KANJI)

#define update_sjismode() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_sjismode].widget, \
		    term->flags & SJIS_KANJI)

#define update_utf8mode() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_utf8mode].widget, \
		    term->flags & UTF8_KANJI)
#endif /* KTERM_KANJIMODE */
#ifdef KTERM_XIM
#define update_openim() \
  update_menu_item (term->screen.vtMenu, \
		    vtMenuEntries[vtMenu_openim].widget, \
		    term->screen.imregistered)
#endif /* KTERM_XIM */


#ifndef KTERM_NOTEK
#define set_vthide_sensitivity() \
  set_sensitivity (term->screen.vtMenu, \
		   vtMenuEntries[vtMenu_vthide].widget, \
		   term->screen.Tshow)

#define set_tekhide_sensitivity() \
  set_sensitivity (term->screen.tekMenu, \
		   tekMenuEntries[tekMenu_tekhide].widget, \
		   term->screen.Vshow)
#endif /* !KTERM_NOTEK */

#define set_altscreen_sensitivity(val) \
  set_sensitivity (term->screen.vtMenu,\
		   vtMenuEntries[vtMenu_altscreen].widget, (val))
#ifdef STATUSLINE
#define set_reversestatus_sensitivity() \
  set_sensitivity (term->screen.vtMenu, \
		   vtMenuEntries[vtMenu_reversestatus].widget, \
		   term->screen.statusheight)
#endif /* STATUSLINE */


/*
 * macros for mapping font size to tekMenu placement
 */
#define FS2MI(n) (n)			/* font_size_to_menu_item */
#define MI2FS(n) (n)			/* menu_item_to_font_size */

#ifndef KTERM_NOTEK
#define set_tekfont_menu_item(n,val) \
  update_menu_item (term->screen.tekMenu, \
		    tekMenuEntries[FS2MI(n)].widget, \
		    (val))
#endif /* !KTERM_NOTEK */

#define set_menu_font(val) \
  update_menu_item (term->screen.fontMenu, \
		    fontMenuEntries[term->screen.menu_font_number].widget, \
		    (val))
