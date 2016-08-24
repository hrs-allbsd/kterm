/*
 *	$Id: gset.h,v 1.3 1996/06/23 08:00:09 kagotani Rel $
 */

/*
 * Copyright (c) 1996
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

#define CS96		0x80 /* character sets which have 96 characters */
#define MBCS		0x40 /* multi-byte character sets */
#define MBC2		0x7f /* second byte of a mbcs character */
  /*
   * No character set uses designating characters less than '/'.
   * Final characters more than 'n' can not be used in current kterm.
   */
#define GSET(c)		((c) - '/')
#define GSETFC(i)	(((i) & ~(MBCS|CS96)) + '/')
		/* final character of a designation sequense for a gset */
/* code of Ichr,Bchr */

#define GSET_GRAPH	GSET('0')
#define GSET_IRV	GSET('@')
#define GSET_UK		GSET('A')
#define GSET_ASCII	GSET('B')
#define GSET_SWEDISH	GSET('C')
#define GSET_NORWEGIAN	GSET('E')
#define GSET_KANA	GSET('I')
#define GSET_JISROMAN	GSET('J')
#define GSET_GERMAN	GSET('K')
#define GSET_FRENCH	GSET('R')
#define GSET_ITALIAN	GSET('Y')
#define GSET_SPANISH	GSET('Z')
#define GSET_LATIN1R	(CS96|GSET('A'))
#define GSET_LATIN2R	(CS96|GSET('B'))
#define GSET_LATIN3R	(CS96|GSET('C'))
#define GSET_LATIN4R	(CS96|GSET('D'))
#define GSET_CYRILLIC	(CS96|GSET('L'))
#define GSET_ARABIC	(CS96|GSET('G'))
#define GSET_GREEK	(CS96|GSET('F'))
#define GSET_HEBREW	(CS96|GSET('H'))
#define GSET_LATIN5R	(CS96|GSET('M'))

#ifdef KTERM_MBCS
# define GSET_OLDKANJI	(MBCS|GSET('@'))
# define GSET_HANZI	(MBCS|GSET('A'))
# define GSET_KANJI	(MBCS|GSET('B'))
# define GSET_HANJA	(MBCS|GSET('C'))
# define GSET_HOJOKANJI	(MBCS|GSET('D'))
#endif /* KTERM_MBCS */

#define F_ISO8859_1	0
#define F_ISO8859_2	1
#define F_ISO8859_3	2
#define F_ISO8859_4	3
#define F_ISO8859_5	4
#define F_ISO8859_6	5
#define F_ISO8859_7	6
#define F_ISO8859_8	7
#define F_ISO8859_9	8
#define F_JISX0201_0	9
#ifdef KTERM_MBCS
#  define F_JISX0208_0	10
#  define F_JISX0212_0	11
#  define F_GB2312_0	12
#  define F_KSC5601_0	13
#  define F_JISC6226_0	14
#  define FCNT		15
#else
#  define FCNT		10
#endif

extern int gsettofnum[];
extern Boolean gsetontheright[];
extern Char *gsetmaponfont[];
