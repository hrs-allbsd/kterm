/*
 *	$Id: gset.c,v 1.3 1996/07/02 05:01:31 kagotani Rel $
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

#include "ptyx.h"
#include "data.h"

int gsettofnum[256];
Boolean gsetontheright[256];
Char *gsetmaponfont[256];
int fnumtogset[FCNT];

static Char irv[128];
static Char uk[128];
static Char swedish[128];
static Char norwegian[128];
static Char german[128];
static Char french[128];
static Char italian[128];
static Char spanish[128];

void
setupgset()
{
	int i;

	for (i = 0; i < 128; i ++) {
		irv[i] = uk[i] = swedish[i] = norwegian[i] =
		 german[i] = french[i] = italian[i] = spanish[i] = i;
	}

	gsettofnum[GSET_GRAPH] = F_ISO8859_1;
	gsetontheright[GSET_GRAPH] = False;

	gsettofnum[GSET_IRV] = F_ISO8859_1;
	gsetontheright[GSET_IRV] = False;
	gsetmaponfont[GSET_IRV] = irv;
	irv['$'] = '\244';	/* currency sign */
	irv['~'] = '\257';	/* macron */

	gsettofnum[GSET_UK] = F_ISO8859_1;
	gsetontheright[GSET_UK] = False;
	gsetmaponfont[GSET_UK] = uk;
	uk['#'] = '\243';	/* pound sign */
	uk['~'] = '\257';	/* macron */

	gsettofnum[GSET_ASCII] = F_ISO8859_1;
	gsetontheright[GSET_ASCII] = False;

	gsettofnum[GSET_SWEDISH] = F_ISO8859_1;
	gsetontheright[GSET_SWEDISH] = False;
	gsetmaponfont[GSET_SWEDISH] = swedish;
	swedish['@'] = '\311';	/* capital letter E with acute accent */
	swedish['['] = '\304';	/* capital letter A with diaeresis */
	swedish['\\']= '\326';	/* capital letter O with diaeresis */
	swedish[']'] = '\305';	/* capital letter A with ring above */
	swedish['^'] = '\334';	/* capital letter U with diaeresis */
	swedish['`'] = '\351';	/* small letter e with acute accent */
	swedish['{'] = '\344';	/* small letter a with diaeresis */
	swedish['|'] = '\366';	/* small letter o with diaeresis */
	swedish['}'] = '\345';	/* small letter a with ring above */
	swedish['~'] = '\374';	/* small letter u with diaeresis */

	gsettofnum[GSET_NORWEGIAN] = F_ISO8859_1;
	gsetontheright[GSET_NORWEGIAN] = False;
	gsetmaponfont[GSET_NORWEGIAN] = norwegian;
	norwegian['['] = '\306';/* capital diphthong A with E */
	norwegian['\\']= '\330';/* capital letter O with oblique stroke */
	norwegian[']'] = '\305';/* capital letter A with ring above */
	norwegian['^'] = '\334';/* capital letter U with diaeresis */
	norwegian['{'] = '\346';/* small diphthong e with e */
	norwegian['|'] = '\370';/* small letter o with oblique stroke */
	norwegian['}'] = '\345';/* small letter a with ring above */
	norwegian['~'] = '\374';/* small letter u with diaeresis */

	gsettofnum[GSET_GERMAN] = F_ISO8859_1;
	gsetontheright[GSET_GERMAN] = False;
	gsetmaponfont[GSET_GERMAN] = german;
	german['@'] = '\247';	/* paragraph sign, section sign */
	german['['] = '\304';	/* capital letter A with diaeresis */
	german['\\']= '\326';	/* capital letter O with diaeresis */
	german[']'] = '\334';	/* capital letter U with diaeresis */
	german['{'] = '\344';	/* small letter a with diaeresis */
	german['|'] = '\366';	/* small letter o with diaeresis */
	german['}'] = '\374';	/* small letter u with diaeresis */
	german['~'] = '\337';	/* small german letter sharp s */

	gsettofnum[GSET_FRENCH] = F_ISO8859_1;
	gsetontheright[GSET_FRENCH] = False;
	gsetmaponfont[GSET_FRENCH] = french;
	french['#'] = '\243';	/* pound sign */
	french['@'] = '\340';	/* small letter a with grave accent */
	french['['] = '\260';	/* ring above, degree sign */
	french['\\']= '\347';	/* small letter c with cedilla */
	french[']'] = '\247';	/* paragraph sign, section sign */
	french['{'] = '\351';	/* small letter e with acute accent */
	french['|'] = '\371';	/* small letter u with grave accent */
	french['}'] = '\350';	/* small letter e with grave accent */
	french['~'] = '\250';	/* diaeresis */

	gsettofnum[GSET_ITALIAN] = F_ISO8859_1;
	gsetontheright[GSET_ITALIAN] = False;
	gsetmaponfont[GSET_ITALIAN] = italian;
	italian['#'] = '\243';	/* pound sign */
	italian['@'] = '\247';	/* paragraph sign, section sign */
	italian['['] = '\260';	/* ring above, degree sign */
	italian['\\']= '\347';	/* small letter c with cedilla */
	italian[']'] = '\351';	/* small letter e with acute accent */
	italian['`'] = '\371';	/* small letter u with grave accent */
	italian['{'] = '\340';	/* small letter a with grave accent */
	italian['|'] = '\362';	/* small letter o with grave accent */
	italian['}'] = '\350';	/* small letter e with grave accent */
	italian['~'] = '\354';	/* small letter i with grave accent */

	gsettofnum[GSET_SPANISH] = F_ISO8859_1;
	gsetontheright[GSET_SPANISH] = False;
	gsetmaponfont[GSET_SPANISH] = spanish;
	spanish['#'] = '\243';	/* pound sign */
	spanish['@'] = '\247';	/* paragraph sign, section sign */
	spanish['['] = '\241';	/* inverted exclamation mark */
	spanish['\\']= '\321';	/* capital letter N with tilde */
	spanish[']'] = '\277';	/* inverted question mark */
	spanish['{'] = '\260';	/* ring above, degree sign */
	spanish['|'] = '\361';	/* small letter n with tilde */
	spanish['}'] = '\347';	/* small letter c with cedilla */

	gsettofnum[GSET_LATIN1R] = F_ISO8859_1;
	gsetontheright[GSET_LATIN1R] = True;
	fnumtogset[F_ISO8859_1] = GSET_LATIN1R;

	gsettofnum[GSET_LATIN2R] = F_ISO8859_2;
	gsetontheright[GSET_LATIN2R] = True;
	fnumtogset[F_ISO8859_2] = GSET_LATIN2R;

	gsettofnum[GSET_LATIN3R] = F_ISO8859_3;
	gsetontheright[GSET_LATIN3R] = True;
	fnumtogset[F_ISO8859_3] = GSET_LATIN3R;

	gsettofnum[GSET_LATIN4R] = F_ISO8859_4;
	gsetontheright[GSET_LATIN4R] = True;
	fnumtogset[F_ISO8859_4] = GSET_LATIN4R;

	gsettofnum[GSET_CYRILLIC] = F_ISO8859_5;
	gsetontheright[GSET_CYRILLIC] = True;
	fnumtogset[F_ISO8859_5] = GSET_CYRILLIC;

	gsettofnum[GSET_ARABIC] = F_ISO8859_6;
	gsetontheright[GSET_ARABIC] = True;
	fnumtogset[F_ISO8859_6] = GSET_ARABIC;

	gsettofnum[GSET_GREEK] = F_ISO8859_7;
	gsetontheright[GSET_GREEK] = True;
	fnumtogset[F_ISO8859_7] = GSET_GREEK;

	gsettofnum[GSET_HEBREW] = F_ISO8859_8;
	gsetontheright[GSET_HEBREW] = True;
	fnumtogset[F_ISO8859_8] = GSET_HEBREW;

	gsettofnum[GSET_LATIN5R] = F_ISO8859_9;
	gsetontheright[GSET_LATIN5R] = True;
	fnumtogset[F_ISO8859_9] = GSET_LATIN5R;

	gsettofnum[GSET_LATIN6R] = F_ISO8859_10;
	gsetontheright[GSET_LATIN6R] = True;
	fnumtogset[F_ISO8859_10] = GSET_LATIN6R;

	gsettofnum[GSET_THAI] = F_ISO8859_11;
	gsetontheright[GSET_THAI] = True;
	fnumtogset[F_ISO8859_11] = GSET_THAI;

	gsettofnum[GSET_LATIN7R] = F_ISO8859_13;
	gsetontheright[GSET_LATIN7R] = True;
	fnumtogset[F_ISO8859_13] = GSET_LATIN7R;

	gsettofnum[GSET_LATIN8R] = F_ISO8859_14;
	gsetontheright[GSET_LATIN8R] = True;
	fnumtogset[F_ISO8859_14] = GSET_LATIN8R;

	gsettofnum[GSET_LATIN9R] = F_ISO8859_15;
	gsetontheright[GSET_LATIN9R] = True;
	fnumtogset[F_ISO8859_15] = GSET_LATIN9R;

	gsettofnum[GSET_LATIN10R] = F_ISO8859_16;
	gsetontheright[GSET_LATIN10R] = True;
	fnumtogset[F_ISO8859_16] = GSET_LATIN10R;

	gsettofnum[GSET_JISROMAN] = F_JISX0201_0;
	gsetontheright[GSET_JISROMAN] = False;

	gsettofnum[GSET_KANA] = F_JISX0201_0;
	gsetontheright[GSET_KANA] = True;
	fnumtogset[F_JISX0201_0] = GSET_KANA;

# ifdef KTERM_MBCS
	gsettofnum[GSET_OLDKANJI] = F_JISC6226_0;
	gsetontheright[GSET_OLDKANJI] = False;
	fnumtogset[F_JISC6226_0] = GSET_OLDKANJI;

	gsettofnum[GSET_KANJI] = F_JISX0208_0;
	gsetontheright[GSET_KANJI] = False;
	fnumtogset[F_JISX0208_0] = GSET_KANJI;

	gsettofnum[GSET_90KANJI] = F_JISX0208_1990_0;
	gsetontheright[GSET_90KANJI] = False;
	fnumtogset[F_JISX0208_1990_0] = GSET_90KANJI;

	gsettofnum[GSET_HOJOKANJI] = F_JISX0212_0;
	gsetontheright[GSET_HOJOKANJI] = False;
	fnumtogset[F_JISX0212_0] = GSET_HOJOKANJI;

	gsettofnum[GSET_HANZI] = F_GB2312_0;
	gsetontheright[GSET_HANZI] = False;
	fnumtogset[F_GB2312_0] = GSET_HANZI;

	gsettofnum[GSET_HANJA] = F_KSC5601_0;
	gsetontheright[GSET_HANJA] = False;
	fnumtogset[F_KSC5601_0] = GSET_HANJA;

	gsettofnum[GSET_CNS1] = F_CNS11643_1;
	gsetontheright[GSET_CNS1] = False;
	fnumtogset[F_CNS11643_1] = GSET_CNS1;

	gsettofnum[GSET_CNS2] = F_CNS11643_2;
	gsetontheright[GSET_CNS2] = False;
	fnumtogset[F_CNS11643_2] = GSET_CNS2;

	gsettofnum[GSET_CNS3] = F_CNS11643_3;
	gsetontheright[GSET_CNS3] = False;
	fnumtogset[F_CNS11643_3] = GSET_CNS3;

	gsettofnum[GSET_CNS4] = F_CNS11643_4;
	gsetontheright[GSET_CNS4] = False;
	fnumtogset[F_CNS11643_4] = GSET_CNS4;

	gsettofnum[GSET_CNS5] = F_CNS11643_5;
	gsetontheright[GSET_CNS5] = False;
	fnumtogset[F_CNS11643_5] = GSET_CNS5;

	gsettofnum[GSET_CNS6] = F_CNS11643_6;
	gsetontheright[GSET_CNS6] = False;
	fnumtogset[F_CNS11643_6] = GSET_CNS6;

	gsettofnum[GSET_CNS7] = F_CNS11643_7;
	gsetontheright[GSET_CNS7] = False;
	fnumtogset[F_CNS11643_7] = GSET_CNS7;

	gsettofnum[GSET_EXTKANJI1] = F_JISX0213_1;
	gsetontheright[GSET_EXTKANJI1] = False;
	fnumtogset[F_JISX0213_1] = GSET_EXTKANJI1;

	gsettofnum[GSET_EXTKANJI2] = F_JISX0213_2;
	gsetontheright[GSET_EXTKANJI2] = False;
	fnumtogset[F_JISX0213_2] = GSET_EXTKANJI2;

	gsettofnum[GSET_EXTKANJI2004_1] = F_JISX0213_2004_1;
	gsetontheright[GSET_EXTKANJI2004_1] = False;
	fnumtogset[F_JISX0213_2004_1] = GSET_EXTKANJI2004_1;
# endif /* KTERM_MBCS */
}

void
set_vt_box_per_gset(screen)
TScreen *screen;
{
	screen->_box[F_ISO8859_1] = VTbox;
	screen->_box[F_ISO8859_2] = VTbox;
	screen->_box[F_ISO8859_3] = VTbox;
	screen->_box[F_ISO8859_4] = VTbox;
	screen->_box[F_ISO8859_5] = VTbox;
	screen->_box[F_ISO8859_6] = VTbox;
	screen->_box[F_ISO8859_7] = VTbox;
	screen->_box[F_ISO8859_8] = VTbox;
	screen->_box[F_ISO8859_9] = VTbox;
	screen->_box[F_ISO8859_10] = VTbox;
	screen->_box[F_ISO8859_11] = VTbox;
	screen->_box[F_ISO8859_13] = VTbox;
	screen->_box[F_ISO8859_14] = VTbox;
	screen->_box[F_ISO8859_15] = VTbox;
	screen->_box[F_ISO8859_16] = VTbox;
	screen->_box[F_JISX0201_0] = VTbox;
#ifdef KTERM_MBCS
	screen->_box[F_JISX0208_1990_0] = VTwbox;
	screen->_box[F_JISX0208_0] = VTwbox;
	screen->_box[F_JISX0212_0] = VTwbox;
	screen->_box[F_GB2312_0] = VTwbox;
	screen->_box[F_KSC5601_0] = VTwbox;
	screen->_box[F_JISC6226_0] = VTwbox;
	screen->_box[F_CNS11643_1] = VTwbox;
	screen->_box[F_CNS11643_2] = VTwbox;
	screen->_box[F_CNS11643_3] = VTwbox;
	screen->_box[F_CNS11643_4] = VTwbox;
	screen->_box[F_CNS11643_5] = VTwbox;
	screen->_box[F_CNS11643_6] = VTwbox;
	screen->_box[F_CNS11643_7] = VTwbox;
	screen->_box[F_JISX0213_1] = VTwbox;
	screen->_box[F_JISX0213_2] = VTwbox;
	screen->_box[F_JISX0213_2004_1] = VTwbox;
#endif /* KTERM_MBCS */
}

char **
csnames(fnum)
int fnum;
{
	static char *csnameslist[FCNT][4] = {
		{"iso8859-1"},				/* F_ISO8859_1 */
		{"iso8859-2"},				/* F_ISO8859_2 */
		{"iso8859-3"},				/* F_ISO8859_3 */
		{"iso8859-4"},				/* F_ISO8859_4 */
		{"iso8859-5"},				/* F_ISO8859_5 */
		{"iso8859-6"},				/* F_ISO8859_6 */
		{"iso8859-7"},				/* F_ISO8859_7 */
		{"iso8859-8"},				/* F_ISO8859_8 */
		{"iso8859-9"},				/* F_ISO8859_9 */
		{"iso8859-10"},				/* F_ISO8859_10 */
		{"iso8859-11"},				/* F_ISO8859_11 */
		{"iso8859-13"},				/* F_ISO8859_13 */
		{"iso8859-14"},				/* F_ISO8859_14 */
		{"iso8859-15"},				/* F_ISO8859_15 */
		{"iso8859-16"},				/* F_ISO8859_16 */
		{"jisx0201.1976-0"},			/* F_JISX0201_0 */
#ifdef KTERM_MBCS
		{"jisx0208.1990-0", "jisx0208.1983-0"},	/* F_JISX0208_1990_0 */
		{"jisx0208.1983-0"},			/* F_JISX0208_0 */
		{"jisx0212.1990-0"},			/* F_JISX0212_0 */
		{"gb2312.1980-0"},			/* F_GB2312_0 */
		{"ksc5601.1987-0"},			/* F_KSC5601_0 */
		{"jisc6226.1978-0", "jisx0208.1983-0"},	/* F_JISC6226_0 */
		{"cns11643.1992-1"},                    /* F_CNS11643_1 */
		{"cns11643.1992-2"},                    /* F_CNS11643_2 */
		{"cns11643.1992-3"},                    /* F_CNS11643_3 */
		{"cns11643.1992-4"},                    /* F_CNS11643_4 */
		{"cns11643.1992-5"},                    /* F_CNS11643_5 */
		{"cns11643.1992-6"},                    /* F_CNS11643_6 */
		{"cns11643.1992-7"},                    /* F_CNS11643_7 */
		{"jisx0213.2000-1", "jisx0213.2004-1", 
		 "jisx0208.1990-0", "jisx0208.1983-0"}, /* F_JISX0213_1 */
		{"jisx0213.2000-2", "jisx0213.2004-2"}, /* F_JISX0213_2 */
		{"jisx0213.2004-1", "jisx0213.2000-1",
		 "jisx0208.1990-0", "jisx0208.1983-0"}, /* F_JISX0213_2004_1 */
#endif /* KTERM_MBCS */
	};

	return csnameslist[fnum];
}
