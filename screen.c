/*
 *	$XConsortium: screen.c,v 1.33 94/04/02 17:34:36 gildea Exp $
 *	$Id: screen.c,v 6.3 1996/06/23 08:00:09 kagotani Rel $
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

/* screen.c */

#include "ptyx.h"
#include "error.h"
#include "data.h"

#include <stdio.h>
#include <signal.h>
#if defined(SVR4) || defined(hpux)
#include <termios.h>
#else
#include <sys/ioctl.h>
#endif

#ifdef att
#include <sys/termio.h>
#include <sys/stream.h>			/* get typedef used in ptem.h */
#include <sys/ptem.h>
#endif

extern Char *calloc(), *malloc(), *realloc();
extern void free();

ScrnBuf Allocate (nrow, ncol, addr)
/*
   allocates memory for a 2-dimensional array of chars and returns a pointer
   thereto
   each line is formed from a pair of char arrays.  The first (even) one is
   the actual character array and the second (odd) one is the attributes.
 */
register int nrow, ncol;
#ifdef KTERM
Bchr **addr;
#else /* !KTERM */
Char **addr;
#endif /* !KTERM */
{
	register ScrnBuf base;
#ifdef KTERM
	register Bchr *tmp;
#else /* !KTERM */
	register Char *tmp;
#endif /* !KTERM */
	register int i;

#ifdef STATUSLINE
	nrow++;
#endif /* STATUSLINE */
#ifdef KTERM
	if ((base = (ScrnBuf) calloc ((unsigned)nrow, sizeof (Bchr *))) == 0)
#else /* !KTERM */
	if ((base = (ScrnBuf) calloc ((unsigned)(nrow *= 2), sizeof (char *))) == 0)
#endif /* !KTERM */
		SysError (ERROR_SCALLOC);

#ifdef KTERM
	if ((tmp = (Bchr *)calloc ((unsigned) (nrow * ncol), sizeof(Bchr))) == 0)
#else /* !KTERM */
	if ((tmp = calloc ((unsigned) (nrow * ncol), sizeof(char))) == 0)
#endif /* !KTERM */
		SysError (ERROR_SCALLOC2);

	*addr = tmp;
	for (i = 0; i < nrow; i++, tmp += ncol)
		base[i] = tmp;

	return (base);
}

/*
 *  This is called when the screen is resized.
 *  Returns the number of lines the text was moved down (neg for up).
 *  (Return value only necessary with SouthWestGravity.)
 */
static
Reallocate(sbuf, sbufaddr, nrow, ncol, oldrow, oldcol)
    ScrnBuf *sbuf;
#ifdef KTERM
    Bchr **sbufaddr;
#else /* !KTERM */
    Char **sbufaddr;
#endif /* !KTERM */
    int nrow, ncol, oldrow, oldcol;
{
	register ScrnBuf base;
#ifdef KTERM
	register Bchr *tmp;
	register int i, minrows, mincols;
	Bchr *oldbuf;
# ifdef STATUSLINE
	Bchr *oldstat;
# endif /* STATUSLINE */
#else /* !KTERM */
	register Char *tmp;
	register int i, minrows, mincols;
	Char *oldbuf;
# ifdef STATUSLINE
	Char *oldstat;
# endif /* STATUSLINE */
#endif /* !KTERM */
	int move_down = 0, move_up = 0;
	
	if (sbuf == NULL || *sbuf == NULL)
		return 0;

#ifdef STATUSLINE
	nrow++;
	oldrow++;
	/* save pointers for the statusline (the last row) */
	oldstat = (*sbuf)[oldrow-1];
#endif /* STATUSLINE */
#ifndef KTERM
	oldrow *= 2;
#endif /* !KTERM */
	oldbuf = *sbufaddr;

	/*
	 * Special case if oldcol == ncol - straight forward realloc and
	 * update of the additional lines in sbuf
	 */

	/* this is a good idea, but doesn't seem to be implemented.  -gildea */

	/* 
	 * realloc sbuf, the pointers to all the lines.
	 * If the screen shrinks, remove lines off the top of the buffer
	 * if resizeGravity resource says to do so.
	 */
#ifdef KTERM
	if (nrow < oldrow  &&  term->misc.resizeGravity == SouthWestGravity) {
	    /* Remove lines off the top of the buffer if necessary. */
	    move_up = oldrow-nrow 
		        - (term->screen.max_row - term->screen.cur_row);
	    if (move_up < 0)
		move_up = 0;
	    /* Overlapping memmove here! */
	    memmove( *sbuf, *sbuf+move_up, 
		  (oldrow-move_up)*sizeof((*sbuf)[0]) );
	}
	*sbuf = (ScrnBuf) realloc((char *) (*sbuf),
				  (unsigned) (nrow * sizeof(Bchr *)));
#else /* !KTERM */
	nrow *= 2;
	if (nrow < oldrow  &&  term->misc.resizeGravity == SouthWestGravity) {
	    /* Remove lines off the top of the buffer if necessary. */
	    move_up = oldrow-nrow 
		        - 2*(term->screen.max_row - term->screen.cur_row);
	    if (move_up < 0)
		move_up = 0;
	    /* Overlapping memmove here! */
	    memmove( *sbuf, *sbuf+move_up, 
		  (oldrow-move_up)*sizeof((*sbuf)[0]) );
	}
	*sbuf = (ScrnBuf) realloc((char *) (*sbuf),
				  (unsigned) (nrow * sizeof(char *)));
#endif /* !KTERM */
	if (*sbuf == 0)
	    SysError(ERROR_RESIZE);
	base = *sbuf;

	/* 
	 *  create the new buffer space and copy old buffer contents there
	 *  line by line.
	 */
#ifdef KTERM
	if ((tmp = (Bchr *)calloc((unsigned) (nrow * ncol), sizeof(Bchr))) == 0)
#else /* !KTERM */
	if ((tmp = calloc((unsigned) (nrow * ncol), sizeof(char))) == 0)
#endif /* !KTERM */
		SysError(ERROR_SREALLOC);
#ifdef STATUSLINE
	nrow--;
	oldrow--;
#endif /* STATUSLINE */
	*sbufaddr = tmp;
	minrows = (oldrow < nrow) ? oldrow : nrow;
	mincols = (oldcol < ncol) ? oldcol : ncol;
#ifdef KTERM
	mincols *= sizeof(Bchr);
#endif /* KTERM */
	if (nrow > oldrow  &&  term->misc.resizeGravity == SouthWestGravity) {
	    /* move data down to bottom of expanded screen */
#ifdef KTERM
	    move_down = Min(nrow-oldrow, term->screen.savedlines);
#else /* !KTERM */
	    move_down = Min(nrow-oldrow, 2*term->screen.savedlines);
#endif /* !KTERM */
	    tmp += ncol*move_down;
	}
	for (i = 0; i < minrows; i++, tmp += ncol) {
		memmove( tmp, base[i], mincols);
	}
	/*
	 * update the pointers in sbuf
	 */
	for (i = 0, tmp = *sbufaddr; i < nrow; i++, tmp += ncol)
	    base[i] = tmp;

#ifdef STATUSLINE
	memmove( tmp, oldstat, mincols);
	base[nrow] = tmp;
# ifndef KTERM
	tmp += ncol;
	oldstat += oldcol;
	memmove( tmp, oldstat, mincols);
	base[nrow+1] = tmp;
# endif /* !KTERM */
#endif /* STATUSLINE */

        /* Now free the old buffer */
	free(oldbuf);

#ifdef KTERM
	return move_down ? move_down : -move_up;
#else /* !KTERM */
	return move_down ? move_down/2 : -move_up/2; /* convert to rows */
#endif /* !KTERM */
}

#ifdef KTERM
ScreenWrite (screen, str, flags, gset, length)
#else /* !KTERM */
ScreenWrite (screen, str, flags, length)
#endif /* !KTERM */
/*
   Writes str into buf at row row and column col.  Characters are set to match
   flags.
 */
TScreen *screen;
#ifdef KTERM
Char *str;
register unsigned flags;
register Char gset;
#else /* !KTERM */
char *str;
register unsigned flags;
#endif /* !KTERM */
register int length;		/* length of string */
{
#ifdef KTERM
	register int avail  = screen->max_col - screen->cur_col + 1;
	register Bchr *col, *col0;
	Boolean ontheright = gsetontheright[gset];
#else /* !KTERM */
	register Char *attrs, *attrs0;
	register int avail  = screen->max_col - screen->cur_col + 1;
	register Char *col;
#endif /* !KTERM */
	register int wrappedbit;

	if (length > avail)
	    length = avail;
	if (length <= 0)
		return;

#ifdef KTERM
	col = col0 = screen->buf[avail = screen->cur_row] + screen->cur_col;
	wrappedbit = col0->attr&LINEWRAPPED;
#else /* !KTERM */
	col = screen->buf[avail = 2 * screen->cur_row] + screen->cur_col;
	attrs = attrs0 = screen->buf[avail + 1] + screen->cur_col;
	wrappedbit = *attrs0&LINEWRAPPED;
#endif /* !KTERM */
	flags &= ATTRIBUTES;
	flags |= CHARDRAWN;
#ifdef KTERM
# ifdef KTERM_MBCS
	if (gset & MBCS) {
		while(length > 0) {
			col->code = ontheright ? *str++|0x80 : *str++&0x7f;
			col->gset = gset;
			col->attr = flags;
			col++;
			col->code = ontheright ? *str++|0x80 : *str++&0x7f;
			col->gset = MBC2;
			col->attr = flags;
			col++;
			length -= 2;
		}
	} else
# endif /* KTERM_MBCS */
	{
		while(length-- > 0) {
			col->code = ontheright ? *str++|0x80 : *str++&0x7f;
			col->gset = gset;
			col->attr = flags;
			col++;
		}
	}
	if (wrappedbit)
	    col0->attr |= LINEWRAPPED;
#else /* !KTERM */
	memmove( col, str, length);
	while(length-- > 0)
		*attrs++ = flags;
	if (wrappedbit)
	    *attrs0 |= LINEWRAPPED;
#endif /* !KTERM */
}

ScrnInsertLine (sb, last, where, n, size)
/*
   Inserts n blank lines at sb + where, treating last as a bottom margin.
   Size is the size of each entry in sb.
   Requires: 0 <= where < where + n <= last
   	     n <= MAX_ROWS
 */
register ScrnBuf sb;
int last;
register int where, n, size;
{
	register int i;
#ifdef KTERM
	Bchr *savebuf [MAX_ROWS];
	Bchr **save;
	save = n > MAX_ROWS ? (Bchr **)XtMalloc(sizeof(Bchr *) * n) : savebuf;
#else /* !KTERM */
	char *save [2 * MAX_ROWS];
#endif /* !KTERM */


	/* save n lines at bottom */
#ifdef KTERM
	memmove( (char *) save, (char *) &sb [last -= n - 1], 
		sizeof (Bchr *) * n);
#else /* !KTERM */
	memmove( (char *) save, (char *) &sb [2 * (last -= n - 1)], 
		2 * sizeof (char *) * n);
#endif /* !KTERM */
	
	/* clear contents of old rows */
#ifdef KTERM
	for (i = n - 1; i >= 0; i--)
		bzero ((char *) save [i], size *sizeof(Bchr));
#else /* !KTERM */
	for (i = 2 * n - 1; i >= 0; i--)
		bzero ((char *) save [i], size);
#endif /* !KTERM */

	/*
	 * WARNING, overlapping copy operation.  Move down lines (pointers).
	 *
	 *   +----|---------|--------+
	 *
	 * is copied in the array to:
	 *
	 *   +--------|---------|----+
	 */
#ifdef KTERM
	memmove( (char *) &sb [where + n], (char *) &sb [where], 
		sizeof (Bchr *) * (last - where));
#else /* !KTERM */
	memmove( (char *) &sb [2 * (where + n)], (char *) &sb [2 * where], 
		2 * sizeof (char *) * (last - where));
#endif /* !KTERM */

	/* reuse storage for new lines at where */
#ifdef KTERM
	memmove( (char *) &sb[where], (char *)save, sizeof(Bchr *) * n);

	if (save != savebuf) XtFree((char *)save);
#else /* !KTERM */
	memmove( (char *) &sb[2 * where], (char *)save, 2 * sizeof(char *) * n);
#endif /* !KTERM */
}


ScrnDeleteLine (sb, last, where, n, size)
/*
   Deletes n lines at sb + where, treating last as a bottom margin.
   Size is the size of each entry in sb.
   Requires 0 <= where < where + n < = last
   	    n <= MAX_ROWS
 */
register ScrnBuf sb;
register int n, last, size;
int where;
{
	register int i;
#ifdef KTERM
	Bchr *savebuf [MAX_ROWS];
	Bchr **save;
	save = n > MAX_ROWS ? (Bchr **)XtMalloc(sizeof(Bchr *) * n) : savebuf;
#else /* !KTERM */
	char *save [2 * MAX_ROWS];
#endif /* !KTERM */

	/* save n lines at where */
#ifdef KTERM
	memmove( (char *)save, (char *) &sb[where], sizeof(Bchr *) * n);
#else /* !KTERM */
	memmove( (char *)save, (char *) &sb[2 * where], 2 * sizeof(char *) * n);
#endif /* !KTERM */

	/* clear contents of old rows */
#ifdef KTERM
	for (i = n - 1 ; i >= 0 ; i--)
		bzero ((char *) save [i], size * sizeof(Bchr));
#else /* !KTERM */
	for (i = 2 * n - 1 ; i >= 0 ; i--)
		bzero ((char *) save [i], size);
#endif /* !KTERM */

	/* move up lines */
#ifdef KTERM
	memmove( (char *) &sb[where], (char *) &sb[where + n], 
		sizeof (Bchr *) * ((last -= n - 1) - where));
#else /* !KTERM */
	memmove( (char *) &sb[2 * where], (char *) &sb[2 * (where + n)], 
		2 * sizeof (char *) * ((last -= n - 1) - where));
#endif /* !KTERM */

	/* reuse storage for new bottom lines */
#ifdef KTERM
	memmove( (char *) &sb[last], (char *)save, sizeof(Bchr *) * n);

	if (save != savebuf) XtFree((char *)save);
#else /* !KTERM */
	memmove( (char *) &sb[2 * last], (char *)save, 
		2 * sizeof(char *) * n);
#endif /* !KTERM */
}


ScrnInsertChar (sb, row, col, n, size)
    /*
      Inserts n blanks in sb at row, col.  Size is the size of each row.
      */
    ScrnBuf sb;
    int row, size;
    register int col, n;
{
	register int i, j;
#ifdef KTERM
	register Bchr *ptr = sb [row];
	int wrappedbit = ptr[0].attr&LINEWRAPPED;

	ptr[0].attr &= ~LINEWRAPPED; /* make sure the bit isn't moved */
#else /* !KTERM */
	register Char *ptr = sb [2 * row];
	register Char *attrs = sb [2 * row + 1];
	int wrappedbit = attrs[0]&LINEWRAPPED;

	attrs[0] &= ~LINEWRAPPED; /* make sure the bit isn't moved */
#endif /* !KTERM */

	for (i = size - 1; i >= col + n; i--) {
		ptr[i] = ptr[j = i - n];
#ifndef KTERM
		attrs[i] = attrs[j];
#endif /* !KTERM */
	}

#ifdef KTERM
	for (i=col; i<col+n; i++) {
	    ptr[i].gset = GSET_ASCII;
	    ptr[i].code = ' ';
	    ptr[i].attr = CHARDRAWN;
	}

	if (wrappedbit)
	    ptr[0].attr |= LINEWRAPPED;
#else /* !KTERM */
	for (i=col; i<col+n; i++)
	    ptr[i] = ' ';
	for (i=col; i<col+n; i++)
	    attrs[i] = CHARDRAWN;

	if (wrappedbit)
	    attrs[0] |= LINEWRAPPED;
#endif /* !KTERM */
}


ScrnDeleteChar (sb, row, col, n, size)
    /*
      Deletes n characters in sb at row, col. Size is the size of each row.
      */
    ScrnBuf sb;
    register int row, size;
    register int n, col;
{
#ifdef KTERM
	register Bchr *ptr = sb[row];
	register nbytes = (size - n - col);
	int wrappedbit = ptr[0].attr&LINEWRAPPED;
#else /* !KTERM */
	register Char *ptr = sb[2 * row];
	register Char *attrs = sb[2 * row + 1];
	register nbytes = (size - n - col);
	int wrappedbit = attrs[0]&LINEWRAPPED;
#endif /* !KTERM */

#ifdef KTERM
	memmove( ptr + col, ptr + col + n, nbytes * sizeof(Bchr));
	bzero (ptr + size - n, n * sizeof(Bchr));
	if (wrappedbit)
	    ptr[0].attr |= LINEWRAPPED;
#else /* !KTERM */
	memmove( ptr + col, ptr + col + n, nbytes);
	memmove( attrs + col, attrs + col + n, nbytes);
	bzero (ptr + size - n, n);
	bzero (attrs + size - n, n);
	if (wrappedbit)
	    attrs[0] |= LINEWRAPPED;
#endif /* !KTERM */
}


ScrnRefresh (screen, toprow, leftcol, nrows, ncols, force)
/*
   Repaints the area enclosed by the parameters.
   Requires: (toprow, leftcol), (toprow + nrows, leftcol + ncols) are
   	     coordinates of characters in screen;
	     nrows and ncols positive.
 */
register TScreen *screen;
int toprow, leftcol, nrows, ncols;
Boolean force;			/* ... leading/trailing spaces */
{
#ifndef KTERM
	int y = toprow * FontHeight(screen) + screen->border +
		screen->fnt_norm->ascent;
#endif /* !KTERM */
	register int row;
	register int topline = screen->topline;
	int maxrow = toprow + nrows - 1;
	int scrollamt = screen->scroll_amt;
	int max = screen->max_row;

#ifdef STATUSLINE
	if(screen->cursor_col >= leftcol
	&& screen->cursor_col <= (leftcol + ncols - 1)
	&& (screen->cursor_row >= toprow + topline
	 && screen->cursor_row <= maxrow - (maxrow > screen->max_row) + topline
	 || maxrow > screen->max_row && screen->cursor_row > screen->max_row))
		screen->cursor_state = OFF;
#else /* !STATUSLINE */
	if(screen->cursor_col >= leftcol && screen->cursor_col <=
	 (leftcol + ncols - 1) && screen->cursor_row >= toprow + topline &&
	 screen->cursor_row <= maxrow + topline)
		screen->cursor_state = OFF;
#endif /* !STATUSLINE */
#ifdef KTERM
	for (row = toprow; row <= maxrow; row++) {
	   register Bchr *chars;
	   Char gset;
#else /* !KTERM */
	for (row = toprow; row <= maxrow; y += FontHeight(screen), row++) {
	   register Char *chars;
	   register Char *attrs;
#endif /* !KTERM */
	   register int col = leftcol;
	   int maxcol = leftcol + ncols - 1;
	   int lastind;
	   int flags;
#ifndef KTERM
	   int x, n;
	   GC gc;
#endif /* !KTERM */
	   Boolean hilite;	

#ifdef STATUSLINE
	   if (row > screen->max_row) { /* implies (row > screen->bot_marg) */
		int dostatus = 0, left, width;
# ifdef KTERM
		int fnum = F_ISO8859_1; /* *GC */
# endif /* KTERM */

		topline = 0;
		max = screen->max_row + 1; /* statusline buffer */

		left = CursorX(screen, leftcol);
		width = ncols * FontWidth(screen);
		if (leftcol == 0) {
		    left = screen->scrollbar;
		    width += screen->border;
		}
		if (leftcol + ncols - 1 >= screen->max_col)
		    width += screen->border;
		XFillRectangle(screen->display, TextWindow(screen),
		    screen->reversestatus ? screen->normalGC : screen->reverseGC,
		    left, Height(screen) + screen->border * 2,
		    width, screen->statusheight);
		if (!screen->reversestatus)
		    StatusBox(screen);
	   }
#endif /* STATUSLINE */

	   if (row < screen->top_marg || row > screen->bot_marg)
		lastind = row;
	   else
		lastind = row - scrollamt;

	   if (lastind < 0 || lastind > max)
	   	continue;

#ifdef KTERM
	   chars = screen->buf [lastind + topline];
#else /* !KTERM */
	   chars = screen->buf [2 * (lastind + topline)];
	   attrs = screen->buf [2 * (lastind + topline) + 1];
#endif /* !KTERM */

	   if (row < screen->startHRow || row > screen->endHRow ||
	       (row == screen->startHRow && maxcol < screen->startHCol) ||
	       (row == screen->endHRow && col >= screen->endHCol))
	       {
	       /* row does not intersect selection; don't hilite */
	       if (!force) {
#ifdef KTERM
		   while (col <= maxcol && (chars[col].attr & ~BOLD) == 0 &&
		          !(chars[col].gset & CS96) &&
			  (chars[col].code & ~040) == 0)
#else /* !KTERM */
		   while (col <= maxcol && (attrs[col] & ~BOLD) == 0 &&
			  (chars[col] & ~040) == 0)
#endif /* !KTERM */
		       col++;

#ifdef KTERM
		   while (col <= maxcol && (chars[maxcol].attr & ~BOLD) == 0 &&
		          !(chars[maxcol].gset & CS96) &&
			  (chars[maxcol].code & ~040) == 0)
#else /* !KTERM */
		   while (col <= maxcol && (attrs[maxcol] & ~BOLD) == 0 &&
			  (chars[maxcol] & ~040) == 0)
#endif /* !KTERM */
		       maxcol--;
	       }
	       hilite = False;
	   }
	   else {
	       /* row intersects selection; split into pieces of single type */
	       if (row == screen->startHRow && col < screen->startHCol) {
		   ScrnRefresh(screen, row, col, 1, screen->startHCol - col,
			       force);
		   col = screen->startHCol;
	       }
	       if (row == screen->endHRow && maxcol >= screen->endHCol) {
		   ScrnRefresh(screen, row, screen->endHCol, 1,
			       maxcol - screen->endHCol + 1, force);
		   maxcol = screen->endHCol - 1;
	       }
	       /* remaining piece should be hilited */
	       hilite = True;
	   }
#ifdef KTERM_MBCS
	   if (chars[col].gset == MBC2 && col != 0)
		col--;
	   if (chars[maxcol].gset & MBCS && chars[maxcol].gset != MBC2)
		maxcol++;
#endif /* KTERM_MBCS */

	   if (col > maxcol) continue;

#ifdef KTERM
	   flags = chars[col].attr;
	   gset = chars[col].gset;
#else /* !KTERM */
	   flags = attrs[col];

# ifdef STATUSLINE
	   if (((!hilite && (flags & INVERSE) != 0) ||
	        (hilite && (flags & INVERSE) == 0))
		^ (row > screen->max_row && screen->reversestatus))
# else /* !STATUSLINE */
	   if ( (!hilite && (flags & INVERSE) != 0) ||
	        (hilite && (flags & INVERSE) == 0) )
# endif /* !STATUSLINE */
	       if (flags & BOLD) gc = screen->reverseboldGC;
	       else gc = screen->reverseGC;
	   else 
	       if (flags & BOLD) gc = screen->normalboldGC;
	       else gc = screen->normalGC;

	   x = CursorX(screen, col);
#endif /* !KTERM */
	   lastind = col;

	   for (; col <= maxcol; col++) {
#ifdef KTERM
		if (chars[col].attr != flags || chars[col].gset != gset) {
		   if (hilite)
			flags ^= INVERSE;
		   ScreenDraw(screen, row + topline, lastind, col, flags, False);
#else /* !KTERM */
		if (attrs[col] != flags) {
		   XDrawImageString(screen->display, TextWindow(screen), 
		        	gc, x, y, (char *) &chars[lastind], n = col - lastind);
		   if((flags & BOLD) && screen->enbolden)
		 	XDrawString(screen->display, TextWindow(screen), 
			 gc, x + 1, y, (char *) &chars[lastind], n);
		   if(flags & UNDERLINE) 
			XDrawLine(screen->display, TextWindow(screen), 
			 gc, x, y+1, x+n*FontWidth(screen), y+1);

		   x += (col - lastind) * FontWidth(screen);
#endif /* !KTERM */

		   lastind = col;

#ifdef KTERM
		   flags = chars[col].attr;
		   gset = chars[col].gset;
		}

		if (gset & MBCS) col++;
#else /* !KTERM */
		   flags = attrs[col];

# ifdef STATUSLINE
	   	   if (((!hilite && (flags & INVERSE) != 0) ||
		        (hilite && (flags & INVERSE) == 0))
			^ (row > screen->max_row && screen->reversestatus))
# else /* !STATUSLINE */
	   	   if ((!hilite && (flags & INVERSE) != 0) ||
		       (hilite && (flags & INVERSE) == 0) )
# endif /* !STATUSLINE */
	       		if (flags & BOLD) gc = screen->reverseboldGC;
	       		else gc = screen->reverseGC;
	  	    else 
	      		 if (flags & BOLD) gc = screen->normalboldGC;
	      		 else gc = screen->normalGC;
		}

		if(chars[col] == 0)
			chars[col] = ' ';
#endif /* !KTERM */
	   }


#ifdef KTERM
	   if (hilite)
		flags ^= INVERSE;
	   ScreenDraw(screen, row + topline, lastind, col, flags, False);
#else /* !KTERM */
# ifdef STATUSLINE
	   if (((!hilite && (flags & INVERSE) != 0) ||
	        (hilite && (flags & INVERSE) == 0))
		^ (row > screen->max_row && screen->reversestatus))
# else /* !STATUSLINE */
	   if ( (!hilite && (flags & INVERSE) != 0) ||
	        (hilite && (flags & INVERSE) == 0) )
# endif /* !STATUSLINE */
	       if (flags & BOLD) gc = screen->reverseboldGC;
	       else gc = screen->reverseGC;
	   else 
	       if (flags & BOLD) gc = screen->normalboldGC;
	       else gc = screen->normalGC;
	   XDrawImageString(screen->display, TextWindow(screen), gc, 
	         x, y, (char *) &chars[lastind], n = col - lastind);
	   if((flags & BOLD) && screen->enbolden)
		XDrawString(screen->display, TextWindow(screen), gc,
		x + 1, y, (char *) &chars[lastind], n);
	   if(flags & UNDERLINE) 
		XDrawLine(screen->display, TextWindow(screen), gc, 
		 x, y+1, x + n * FontWidth(screen), y+1);
#endif /* !KTERM */
	}
}

ClearBufRows (screen, first, last)
/*
   Sets the rows first though last of the buffer of screen to spaces.
   Requires first <= last; first, last are rows of screen->buf.
 */
register TScreen *screen;
register int first, last;
{
#ifdef KTERM
	while (first <= last)
		bzero (screen->buf [first++], (screen->max_col + 1) * sizeof(Bchr));
#else /* !KTERM */
	first *= 2;
	last = 2 * last + 1;
	while (first <= last)
		bzero (screen->buf [first++], (screen->max_col + 1));
#endif /* !KTERM */
}

/*
  Resizes screen:
  1. If new window would have fractional characters, sets window size so as to
  discard fractional characters and returns -1.
  Minimum screen size is 1 X 1.
  Note that this causes another ExposeWindow event.
  2. Enlarges screen->buf if necessary.  New space is appended to the bottom
  and to the right
  3. Reduces  screen->buf if necessary.  Old space is removed from the bottom
  and from the right
  4. Cursor is positioned as closely to its former position as possible
  5. Sets screen->max_row and screen->max_col to reflect new size
  6. Maintains the inner border (and clears the border on the screen).
  7. Clears origin mode and sets scrolling region to be entire screen.
  8. Returns 0
  */
ScreenResize (screen, width, height, flags)
    register TScreen *screen;
    int width, height;
    unsigned *flags;
{
	int rows, cols;
	int border = 2 * screen->border;
	int move_down_by;
#if defined(sun) && !defined(SVR4)
#ifdef TIOCSSIZE
	struct ttysize ts;
#endif	/* TIOCSSIZE */
#else	/* sun */
#ifdef TIOCSWINSZ
	struct winsize ws;
#endif	/* TIOCSWINSZ */
#endif	/* sun */
	Window tw = TextWindow (screen);

	/* clear the right and bottom internal border because of NorthWest
	   gravity might have left junk on the right and bottom edges */
	XClearArea (screen->display, tw,
		    width - screen->border, 0,                /* right edge */
#ifdef STATUSLINE
		    screen->border, height - screen->statusheight,
#else /* !STATUSLINE */
		    screen->border, height,           /* from top to bottom */
#endif /* !STATUSLINE */
		    False);
	XClearArea (screen->display, tw, 
#ifdef STATUSLINE
		    0, height - screen->border - screen->statusheight,
#else /* !STATUSLINE */
		    0, height - screen->border,	                  /* bottom */
#endif /* !STATUSLINE */
		    width, screen->border,         /* all across the bottom */
		    False);

	/* round so that it is unlikely the screen will change size on  */
	/* small mouse movements.					*/
#ifdef STATUSLINE
	rows = (height + FontHeight(screen) / 2 - border - screen->statusheight) /
#else /* !STATUSLINE */
	rows = (height + FontHeight(screen) / 2 - border) /
#endif /* !STATUSLINE */
	 FontHeight(screen);
	cols = (width + FontWidth(screen) / 2 - border - screen->scrollbar) /
	 FontWidth(screen);
	if (rows < 1) rows = 1;
	if (cols < 1) cols = 1;
#ifdef STATUSLINE
	if (screen->statusheight
	 && term->misc.resizeGravity == NorthWestGravity) {
		/* bit_gravity != ForgetGravity */
		if (rows > screen->max_row + 1) {
		    XClearArea (screen->display, tw, 
				screen->scrollbar,
				(screen->max_row + 1) * FontHeight(screen) +
				screen->border * 2,
				(screen->max_col + 1) * FontHeight(screen) +
				screen->border * 2,
				screen->statusheight,
				True);
		} else if (rows < screen->max_row + 1) {
		    XClearArea (screen->display, tw, 
				screen->scrollbar,
				rows * FontHeight(screen) +
				screen->border * 2,
				cols * FontWidth(screen) + screen->border * 2,
				screen->statusheight,
				True);
		}
		screen->cursor_state = OFF;
	}
#endif /* STATUSLINE */

	/* update buffers if the screen has changed size */
	if (screen->max_row != rows - 1 || screen->max_col != cols - 1) {
		register int savelines = screen->scrollWidget ?
		 screen->savelines : 0;
		int delta_rows = rows - (screen->max_row + 1);
		
		if(screen->cursor_state)
			HideCursor();
		if ( screen->alternate
		     && term->misc.resizeGravity == SouthWestGravity )
		    /* swap buffer pointers back to make all this hair work */
		    SwitchBufPtrs(screen);
		if (screen->altbuf) 
		    (void) Reallocate(&screen->altbuf, (Char **)&screen->abuf_address,
			 rows, cols, screen->max_row + 1, screen->max_col + 1);
		move_down_by = Reallocate(&screen->allbuf,
					  (Char **)&screen->sbuf_address,
					  rows + savelines, cols,
					  screen->max_row + 1 + savelines,
					  screen->max_col + 1);
#ifdef KTERM
		screen->buf = &screen->allbuf[savelines];
#else /* !KTERM */
		screen->buf = &screen->allbuf[2 * savelines];
#endif /* !KTERM */

		screen->max_row += delta_rows;
		screen->max_col = cols - 1;

		if (term->misc.resizeGravity == SouthWestGravity) {
		    screen->savedlines -= move_down_by;
		    if (screen->savedlines < 0)
			screen->savedlines = 0;
		    if (screen->savedlines > screen->savelines)
			screen->savedlines = screen->savelines;
		    if (screen->topline < -screen->savedlines)
			screen->topline = -screen->savedlines;
		    screen->cur_row += move_down_by;
		    screen->cursor_row += move_down_by;
		    ScrollSelection(screen, move_down_by);

		    if (screen->alternate)
			SwitchBufPtrs(screen); /* put the pointers back */
		}
	
		/* adjust scrolling region */
		screen->top_marg = 0;
		screen->bot_marg = screen->max_row;
		*flags &= ~ORIGIN;

#ifdef STATUSLINE
		if (screen->instatus)
			screen->cur_row = screen->max_row + 1;
		else
#endif /* STATUSLINE */
		if (screen->cur_row > screen->max_row)
			screen->cur_row = screen->max_row;
		if (screen->cur_col > screen->max_col)
			screen->cur_col = screen->max_col;
	
#ifdef STATUSLINE
		screen->fullVwin.height = height - border - screen->statusheight;
#else /* !STATUSLINE */
		screen->fullVwin.height = height - border;
#endif /* !STATUSLINE */
		screen->fullVwin.width = width - border - screen->scrollbar;

	} else if(FullHeight(screen) == height && FullWidth(screen) == width)
	 	return(0);	/* nothing has changed at all */

	if(screen->scrollWidget)
		ResizeScrollBar(screen->scrollWidget, -1, -1, height);
	
	screen->fullVwin.fullheight = height;
	screen->fullVwin.fullwidth = width;
	ResizeSelection (screen, rows, cols);
#if defined(sun) && !defined(SVR4)
#ifdef TIOCSSIZE
	/* Set tty's idea of window size */
	ts.ts_lines = rows;
	ts.ts_cols = cols;
	ioctl (screen->respond, TIOCSSIZE, &ts);
#ifdef SIGWINCH
	if(screen->pid > 1) {
		int	pgrp;
		
		if (ioctl (screen->respond, TIOCGPGRP, &pgrp) != -1)
			kill_process_group(pgrp, SIGWINCH);
	}
#endif	/* SIGWINCH */
#endif	/* TIOCSSIZE */
#else	/* sun */
#ifdef TIOCSWINSZ
	/* Set tty's idea of window size */
	ws.ws_row = rows;
	ws.ws_col = cols;
	ws.ws_xpixel = width;
	ws.ws_ypixel = height;
	ioctl (screen->respond, TIOCSWINSZ, (char *)&ws);
#ifdef notdef	/* change to SIGWINCH if this doesn't work for you */
	if(screen->pid > 1) {
		int	pgrp;
		
		if (ioctl (screen->respond, TIOCGPGRP, &pgrp) != -1)
		    kill_process_group(pgrp, SIGWINCH);
	}
#endif	/* SIGWINCH */
#endif	/* TIOCSWINSZ */
#endif	/* sun */
#ifdef KTERM_XIM
	IMSendSize(screen);
	IMSendSpot(screen);
#endif /* KTERM_XIM */
#ifdef KTERM_KINPUT2
	Kinput2SendSize();
	Kinput2SendSpot();
#endif /* KTERM_KINPUT2 */
	return (0);
}

/*
 * Sets the attributes from the row, col, to row, col + length according to
 * mask and value. The bits in the attribute byte specified by the mask are
 * set to the corresponding bits in the value byte. If length would carry us
 * over the end of the line, it stops at the end of the line.
 */
void
ScrnSetAttributes(screen, row, col, mask, value, length)
TScreen *screen;
int row, col;
unsigned mask, value;
register int length;		/* length of string */
{
#ifdef KTERM
	register Bchr *cp;
#else /* !KTERM */
	register Char *attrs;
#endif /* !KTERM */
	register int avail  = screen->max_col - col + 1;

	if (length > avail)
	    length = avail;
	if (length <= 0)
		return;
#ifdef KTERM
	cp = screen->buf[row] + col;
#else /* !KTERM */
	attrs = screen->buf[2 * row + 1] + col;
#endif /* !KTERM */
	value &= mask;	/* make sure we only change the bits allowed by mask*/
	while(length-- > 0) {
#ifdef KTERM
		cp->attr &= ~mask;	/* clear the bits */
		cp->attr |= value;	/* copy in the new values */
		cp++;
#else /* !KTERM */
		*attrs &= ~mask;	/* clear the bits */
		*attrs |= value;	/* copy in the new values */
		attrs++;
#endif /* !KTERM */
	}
}

/*
 * Gets the attributes from the row, col, to row, col + length into the
 * supplied array, which is assumed to be big enough.  If length would carry us
 * over the end of the line, it stops at the end of the line. Returns
 * the number of bytes of attributes (<= length)
 */
int
ScrnGetAttributes(screen, row, col, str, length)
TScreen *screen;
int row, col;
Char *str;
register int length;		/* length of string */
{
#ifdef KTERM
	register Bchr *cp;
#else /* !KTERM */
	register Char *attrs;
#endif /* !KTERM */
	register int avail  = screen->max_col - col + 1;
	int ret;

	if (length > avail)
	    length = avail;
	if (length <= 0)
		return 0;
	ret = length;
#ifdef KTERM
	cp = screen->buf[row] + col;
#else /* !KTERM */
	attrs = screen->buf[2 * row + 1] + col;
#endif /* !KTERM */
	while(length-- > 0) {
#ifdef KTERM
		*str++ = cp++->attr;
#else /* !KTERM */
		*str++ = *attrs++;
#endif /* !KTERM */
	}
	return ret;
}
Bool
non_blank_line(sb, row, col, len)
ScrnBuf sb;
register int row, col, len;
{
	register int	i;
#ifdef KTERM
	register Bchr *cp = sb [row];
#else /* !KTERM */
	register Char *ptr = sb [2 * row];
#endif /* !KTERM */

	for (i = col; i < len; i++)	{
#ifdef KTERM
		if (cp[i].code)
#else /* !KTERM */
		if (ptr[i])
#endif /* !KTERM */
			return True;
	}
	return False;
}

#ifdef KTERM
ScreenDraw(screen, row, col, endcol, flags, oncursor)
TScreen	*screen;
int	row; /* in screen->buf */
int	col;
int	endcol;
#ifdef KTERM_COLOR
unsigned short	flags;
#else /* !KTERM_COLOR */
Char	flags;
#endif /* !KTERM_COLOR */
Boolean	oncursor;
{
	GC	gc;
	Boolean	hilitecursor = oncursor && (screen->select || screen->always_highlight);
	Bchr	*str;
	Char	gset;
	int	fnum;	/* for *GC, and box */
	XChar2b	drawbuf2[256], *dbuf2 = drawbuf2;
	char	drawbuf[256], *dbuf = drawbuf;
	int	n, c;
	int	x, y;

	x = CursorX(screen, col);
	y = CursorY(screen, row);
	if (row >= screen->top_marg && row <= screen->bot_marg)
		row -= screen->scroll_amt;
	str = screen->buf[row];
# ifdef STATUSLINE
	if (row > screen->max_row && screen->reversestatus)
		flags ^= INVERSE;
# endif /* STATUSLINE */

	gset = str[col].gset;
	fnum = gsettofnum[gset];

# ifdef KTERM_MBCS
	if (gset & MBCS) {
		if ((endcol-col+1)/2 > 256)
			dbuf2 = (XChar2b *)XtMalloc((endcol-col+1)/2 * sizeof(XChar2b));
		for (c = col, n = 0; c < endcol; c++, n++) {
			dbuf2[n].byte1 = str[c++].code;
			dbuf2[n].byte2 = str[c].code;
		}
	} else
# endif /* KTERM_MBCS */
	{
		if (endcol-col > 256)
			dbuf = XtMalloc((endcol-col) * sizeof(char));
		if (gset) {
			for (c = col, n = 0; c < endcol; c++, n++) {
				dbuf[n] = gsetmaponfont[gset]
					    ? gsetmaponfont[gset][str[c].code]
					    : str[c].code;
			}
		} else {
			for (c = col, n = 0; c < endcol; c++, n++) {
				dbuf[n] = ' ';
			}
		}
	}

	if (!(flags & BOLD) && !screen->normalGC)
	    LoadOneFont(screen, True, screen->menu_font_number, fnum, False);
	else if (flags & BOLD && !screen->normalboldGC)
	    LoadOneFont(screen, True, screen->menu_font_number, fnum, True);
	
	if (flags & INVERSE) {
		if (hilitecursor && screen->reversecursorGC)
			gc = screen->reversecursorGC;
		else if (flags & BOLD)
			gc = screen->reverseboldGC;
		else
			gc = screen->reverseGC;
	} else {
		if (hilitecursor && screen->cursorGC)
			gc = screen->cursorGC;
		else if (flags & BOLD)
			gc = screen->normalboldGC;
		else
			gc = screen->normalGC;
	}

#ifdef KTERM_COLOR
	if (!hilitecursor && flags & (FORECOLORED|BACKCOLORED)) {
		XGCValues	xgcv;
		unsigned long	f, b;
		f = flags&FORECOLORED ? screen->textcolor[FORECOLORNUM(flags)]
				      : screen->foreground;
		b = flags&BACKCOLORED ? screen->textcolor[BACKCOLORNUM(flags)]
				      : term->core.background_pixel;
		if (flags & INVERSE) {
		    xgcv.foreground = b;
		    xgcv.background = f;
		} else {
		    xgcv.foreground = f;
		    xgcv.background = b;
		}
		XChangeGC(screen->display, gc, GCForeground|GCBackground, &xgcv);
	}
#endif /* KTERM_COLOR */

	if (gset == GSET_GRAPH) {
		int	col, X;
		Pixmap	pm;
		for (col = 0, X = x; col < n; col ++, X += FontWidth(screen)) {
		    if ((pm = screen->graphics[dbuf[col]])
		     || (pm = screen->graphics[' '])) {
			XCopyPlane(screen->display, pm, TextWindow(screen), gc,
			    0, 0, FontWidth(screen), FontHeight(screen),
			    X, y, 1);
		    }
		}
	} else {
		int	Y = y + screen->linespace / 2 + screen->max_ascent;

#ifdef KTERM_MBCS
		if (gset & MBCS) {
			XDrawImageString16(screen->display, TextWindow(screen),
				gc, x, Y, dbuf2, n);
			if (flags & BOLD && screen->normalboldGC == screen->normalGC)
			    XDrawString16(screen->display, TextWindow(screen),
				gc, x + 1, Y, dbuf2, n);
		} else
#endif /* KTERM_MBCS */
		{
			XDrawImageString(screen->display, TextWindow(screen),
				gc, x, Y, dbuf, n);
			if (flags & BOLD && screen->normalboldGC == screen->normalGC)
			    XDrawString(screen->display, TextWindow(screen),
				gc, x + 1, Y, dbuf, n);
		}
		if (flags & UNDERLINE) 
		    XDrawLine(screen->display, TextWindow(screen),
			gc, x, Y+1, x + (endcol-col) * FontWidth(screen), Y+1);
	}
	if (oncursor && !hilitecursor) {
		screen->box->x = x;
		screen->box->y = y + screen->linespace / 2;
		XDrawLines (screen->display, TextWindow(screen),
			    screen->cursoroutlineGC ? screen->cursoroutlineGC
						    : gc,
			    screen->box, NBOX, CoordModePrevious);
	}

#ifdef KTERM_COLOR
	if (!hilitecursor && flags & (FORECOLORED|BACKCOLORED)) {
		XGCValues	xgcv;
		if (flags & INVERSE) {
		    xgcv.foreground = term->core.background_pixel;
		    xgcv.background = screen->foreground;
		} else {
		    xgcv.foreground = screen->foreground;
		    xgcv.background = term->core.background_pixel;
		}
		XChangeGC(screen->display, gc, GCForeground|GCBackground, &xgcv);
	}
#endif /* KTERM_COLOR */
#ifdef KTERM_MBCS
	if (dbuf2 != drawbuf2) XtFree((char *)dbuf2);
#endif /* KTERM_MBCS */
	if (dbuf != drawbuf) XtFree(dbuf);
}
#endif /* KTERM */
