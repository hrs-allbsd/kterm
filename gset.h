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
   * No character set uses designating characters less than '@'
   * except for ESC ( 0.
   */
#define GSET(c)		((c) - ('@' - 1))
#define GSETFC(i)	(((i) & ~(MBCS|CS96)) + ('@' - 1))
		/* final character of a designation sequense for a gset */
/* code of Ichr,Bchr */

#define GSET_GRAPH	(CS96|0)
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
#define GSET_LATIN6R	(CS96|GSET('V'))
#define GSET_THAI	(CS96|GSET('T'))
#define GSET_LATIN7R	(CS96|GSET('Y'))
#define GSET_LATIN8R	(CS96|GSET('_'))
#define GSET_LATIN9R	(CS96|GSET('b'))
#define GSET_LATIN10R	(CS96|GSET('f'))

#ifdef KTERM_MBCS
# define GSET_OLDKANJI  (MBCS|GSET('@'))
# define GSET_HANZI     (MBCS|GSET('A'))
# define GSET_KANJI     (MBCS|GSET('B'))
# define GSET_HANJA     (MBCS|GSET('C'))
# define GSET_HOJOKANJI (MBCS|GSET('D'))
# define GSET_CNS1      (MBCS|GSET('G'))
# define GSET_CNS2      (MBCS|GSET('H'))
# define GSET_CNS3      (MBCS|GSET('I'))
# define GSET_CNS4      (MBCS|GSET('J'))
# define GSET_CNS5      (MBCS|GSET('K'))
# define GSET_CNS6      (MBCS|GSET('L'))
# define GSET_CNS7      (MBCS|GSET('M'))
# define GSET_EXTKANJI1 (MBCS|GSET('O'))
# define GSET_EXTKANJI2 (MBCS|GSET('P'))
# define GSET_EXTKANJI2004_1 (MBCS|GSET('Q'))
# define GSET_90KANJI   (MBCS|GSET('_'))
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
#define F_ISO8859_10	9
#define F_ISO8859_11	10
#define F_ISO8859_13	11
#define F_ISO8859_14	12
#define F_ISO8859_15	13
#define F_ISO8859_16	14
#define F_JISX0201_0	15

#ifdef KTERM_MBCS
#  define F_JISX0208_1990_0	16
#  define F_JISX0208_0		17
#  define F_JISX0212_0		18
#  define F_GB2312_0		19
#  define F_KSC5601_0		20
#  define F_JISC6226_0		21
#  define F_CNS11643_1		22
#  define F_CNS11643_2		23
#  define F_CNS11643_3		24
#  define F_CNS11643_4		25
#  define F_CNS11643_5		26
#  define F_CNS11643_6		27
#  define F_CNS11643_7		28
#  define F_JISX0213_1		29
#  define F_JISX0213_2		30
#  define F_JISX0213_2004_1	31
#  define FCNT                  32
#else
#  define FCNT		16
#endif

extern int gsettofnum[];
extern Boolean gsetontheright[];
extern Char *gsetmaponfont[];
extern int fnumtogset[];
