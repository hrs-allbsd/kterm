/*
 *	$Id: kterm.h,v 6.5 1996/07/12 05:01:34 kagotani Rel $
 */

/*
 * Copyright (c) 1988, 1989, 1990, 1991, 1992, 1993, 1994, and 1996
 * XXI working group in Japan Unix Society (XXI).
 *
 * The X Consortium, and any party obtaining a copy of these files from
 * the X Consortium, directly or indirectly, is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so.  This license includes without
 * limitation a license to do the foregoing actions under any patents of
 * the party supplying this software to the X Consortium.
 * 
 * Author:
 * 	Hiroto Kagotani
 * 	Department of Information Technology
 *	Okayama University
 * 	3-1-1 Tsushima-Naka, Okayama-shi 700, Japan
 * 	kagotani@in.it.okayama-u.ac.jp
 */ 

#ifndef _KTERM_H_
#define _KTERM_H_

#define KTERM_VERSION	"6.2.0"
#define KTERM_MBCS	/* multi-byte character set */
#define KTERM_MBCC	/* multi-byte character class for word selection */
#define KTERM_KANJIMODE	/* euc/sjis Kanji modes */
#define KTERM_XIM	/* XIM protocol */
#define KTERM_KINPUT2	/* Kinput2 protocol */
#define KTERM_COLOR	/* color sequence */
#define KTERM_NOTEK	/* disables Tektronix emulation */
#undef  KTERM_XAW3D	/* Xaw3d -DARROW_SCROLLBAR support */

#endif /* !_KTERM_H_ */
