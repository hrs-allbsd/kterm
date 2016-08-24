/*
 *	convert.c -- code converters for kterm
 *	$Id: convert.c,v 6.2 1996/06/23 08:00:09 kagotani Rel $
 */

/*
 * Copyright (c) 1989  Software Research Associates, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Software Research Associates not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  Software Research
 * Associates makes no representations about the suitability of this software
 * for any purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * Author:  Makoto Ishisone, Software Research Associates, Inc., Japan
 */
#ifdef KTERM

#include "ptyx.h"

#define NUL	0x00

#define IsGsetKanji(gset)	((gset)==GSET_KANJI || (gset)==GSET_OLDKANJI)
#define IsGsetAscii(gset)	((gset)==GSET_ASCII || (gset)==GSET_JISROMAN)
#define JIStoSJIS(c1, c2, s1_p, s2_p)                                 \
	*(s1_p) = ((c1) - 0x21) / 2 + (((c1) <= 0x5e) ? 0x81 : 0xc1); \
	if ((c1) & 1)	/* odd */                                     \
	    *(s2_p) = (c2) + (((c2) <= 0x5f) ? 0x1f : 0x20);          \
	else                                                          \
	    *(s2_p) = (c2) + 0x7e;

/* CS -> JIS using ESC-$-B */
int
convCStoJIS(cs, js)
Ichr	*cs;
Char	*js;
{
	return convCStoANY(cs, js, NULL);
}

/* CS -> EUC */
static int
CStoEUC(cs_p, es_p)
Ichr	**cs_p;
Char	**es_p;
{
	int	c1, c2;
	Ichr	*cs = *cs_p;
	Char	*es = *es_p;
	if (cs->gset == GSET_KANA) {
		c1 = cs++->code;
		if (es) {
			*es++ = SS2;
			*es++ = c1 | 0x80;
		}
		*es_p = es;
		*cs_p = cs;
		return 2;
	} else if (IsGsetKanji(cs->gset)) {
		c1 = cs++->code;
		c2 = cs++->code;
		if (es) {
			*es++ = c1 | 0x80;
			*es++ = c2 | 0x80;
		}
		*es_p = es;
		*cs_p = cs;
		return 2;
	}
	return 0;
}

int
convCStoEUC(cs, es)
Ichr	*cs;
Char	*es;
{
	return convCStoANY(cs, es, CStoEUC);
}

/* CS -> SJIS */
static int
CStoSJIS(cs_p, ss_p)
Ichr	**cs_p;
Char	**ss_p;
{
	int	c1, c2;
	Ichr	*cs = *cs_p;
	Char	*ss = *ss_p;
	if (cs->gset == GSET_KANA) {
		c1 = cs++->code;
		if (ss) {
			*ss++ = c1 | 0x80;
		}
		*ss_p = ss;
		*cs_p = cs;
		return 1;
	} else if (IsGsetKanji(cs->gset)) {
		c1 = cs++->code;
		c2 = cs++->code;
		if (ss) {
			JIStoSJIS(c1, c2, ss, ss+1);
			ss += 2;
		}
		*ss_p = ss;
		*cs_p = cs;
		return 2;
	}
	return 0;
}

int
convCStoSJIS(cs, ss)
Ichr	*cs;
Char	*ss;
{
	return convCStoANY(cs, ss, CStoSJIS);
}

/* CS -> any */
int
convCStoANY(cs, as, func)
Ichr	*cs;
Char	*as;
int		(*func)();
{
	register int	c1, c2;
	register int	gset = GSET_ASCII;
	register int	n = 0, m;

	while (c1 = cs->code) {
		if (func && (m = (*func)(&cs, &as))) {
			n += m;
			continue;
		}
		if (gset != cs->gset) {
			if (IsGsetAscii(cs->gset)) { /* JISROMAN HACK */
				if (!IsGsetAscii(gset)) {
					if (as) {
						*as++ = ESC;
						*as++ = '(';
						*as++ = GSETFC(cs->gset);
					}
					n += 3;
				}
			} else if (IsGsetKanji(cs->gset)
				|| cs->gset == GSET_HANZI) {
				/* Use ESC-$-F instead of ESC-$-(-F (for @AB) */
				if (as) {
					*as++ = ESC;
					*as++ = '$';
					*as++ = GSETFC(cs->gset);
				}
				n += 3;
			} else {
				if (as) {
					*as++ = ESC;
					if (cs->gset & MBCS) {
						*as++ = '$';
					}
					if (cs->gset & CS96) {
						*as++ = '-';
					} else {
						*as++ = '(';
					}
					*as++ = GSETFC(cs->gset);
				}
				if (cs->gset & MBCS)
					n += 4;
				else
					n += 3;
			}
			if (!(gset & CS96) && cs->gset & CS96) {
				if (as)
					*as++ = LS1; /* SO */
				n++;
			} else if (gset & CS96 && !(cs->gset & CS96)) {
				if (as)
					*as++ = LS0; /* SI */
				n++;
			}
			gset = cs->gset;
		}
		cs++;
		if (gset & MBCS) {
			c2 = cs++->code;
			if (as) {
				*as++ = c1 & ~0x80;
				*as++ = c2 & ~0x80;
			}
			n += 2;
		} else {
			if (as)
				*as++ = c1 & ~0x80;
			n++;
		}
	}
	if (!IsGsetAscii(gset)) { /* JISROMAN HACK */
		if (as) {
			*as++ = ESC;
			*as++ = '(';
			*as++ = GSETFC(GSET_ASCII);
		}
		n += 3;
	}
	if (gset & CS96) {
		if (as)
			*as++ = LS0; /* SI */
		n++;
	}
	if (as)
		*as = '\0';

	return n;
}

/* CS -> ISO Latin-1 */
int
convCStoLatin1(cs, ls)
Ichr *cs;
Char *ls;
{
	register int	c;
	register int	n = 0;

	if (ls) {
		while (c = cs->code) {
			if (cs++->gset == GSET_ASCII) {
				*ls++ = c & ~0x80;
				n++;
			}
		}
		*ls = '\0';
	} else {
		while (c = cs->code) {
			if (cs++->gset == GSET_ASCII) {
				n++;
			}
		}
	}
	return n;
}

/******************************************************************************
COMPOUND_TEXT Summary
  (based on Comopund Text Encoding Version 1 -- MIT X Consortium Standard)
(1) Only G0 and G1 are used. G2 and G3 are not.
(2) G0 is invoked into GL and G1 into GR. These invocation are not changed.
	(In other words, Locking Shift and Single Shift are not used)
(3) In initial state, ISO Latin-1 is designated into G0/G1.
(4) To designate MBCS into G0, ESC-$-F is not used but ESC-$-(-F.
(5) In C0, only HT, NL, and ESC are used.
(6) In C1, only CSI is used.
(7) Text direction can be indecated.
	begin left-to-right string
	begin right-to-left string
	end of string
******************************************************************************/

/* convCStoCT -- Japanese Wide Character String -> COMPOUND_TEXT */
int
convCStoCT(cs, as)
register Ichr *cs;
register Char *as;
/* Convert Wide Character String cs to COMPOUND_TEXT xstr, return
 * length of xstr in bytes (not including the terminating null character).
 * If xstr is NULL, no conversion is done, but return length of xstr.
 */
{
	register int	c1, c2;
	register int	g0 = GSET_ASCII;
	register int	g1 = GSET_LATIN1R;
	register int	n = 0;

	while (c1 = cs->code) {
	    if (cs->gset & CS96
	     || cs->gset & MBCS
	     || cs->gset == GSET_KANA) {
		if (g1 != cs->gset) {
			g1 = cs->gset;
			if (as) {
				*as++ = ESC;
				if (g1 & MBCS) {
					*as++ = '$';
				}
				if (g1 & CS96) {
					*as++ = '-';
				} else {
					*as++ = ')';
				}
				*as++ = GSETFC(g1);
			}
			n += 3;
			if (g1 & MBCS)
				n ++;
		}
		cs++;
		if (g1 & MBCS) {
			c2 = cs++->code;
			if (as) {
				*as++ = c1 | 0x80;
				*as++ = c2 | 0x80;
			}
			n += 2;
		} else {
			if (as)
				*as++ = c1 | 0x80;
			n++;
		}
	    } else {
		if (g0 != cs->gset) {
			g0 = cs->gset;
			if (as) {
				*as++ = ESC;
				*as++ = '(';
				*as++ = GSETFC(g0);
			}
			n += 3;
		}
		cs++;
		if (as)
			*as++ = c1 & ~0x80;
		n++;
	    }
	}
	if (g0 != GSET_ASCII) {
		if (as) {
			*as++ = ESC;
			*as++ = '(';
			*as++ = GSETFC(GSET_ASCII);
		}
		n += 3;
	}
	if (as)
		*as = '\0';

	return n;
}

static Char *
getesc(str, len)
Char *str;
int len;
{
	register int	c;

	/* Find intermediate characters and final character
	 * following the escape character in an escape sequence.
	 */
	/* The intermediate character is 02/00 to 02/15 */
	while (len > 0) {
		c = *str;
		if (c < 0x20 || 0x2f < c)
			break;
		len--, str++;
	}
	/* The final character is 03/00 to 07/14 */
	if (--len < 0 || (c = *str++) < 0x30 || 0x7e < c)
		return NULL;

	return str;
}

static Char *
getcsi(str, len)
Char *str;
int len;
{
	register int	c;

	/* Find parameter characters, intermediate characters
	 * and final character following the CSI character
	 * in a CSI sequence.
	 */
	/* The parameter characters is 03/00 to 03/15 */
	while (len > 0) {
		c = *str;
		if (c < 0x30 || 0x3f < c)
			break;
		len--, str++;
	}
	/* The intermediate character is 02/00 to 02/15 */
	while (len > 0) {
		c = *str;
		if (c < 0x20 || 0x2f < c)
			break;
		len--, str++;
	}
	/* The final character is 04/00 to 07/14 */
	if (--len < 0 || (c = *str++) < 0x40 || 0x7e < c)
		return NULL;

	return str;
}

/* convCTtoCS -- COMPOUND_TEXT -> Japanese Wide Character String */
int
convCTtoCS(xstr, len, cs)
register Char *xstr;
int len;
Ichr *cs;
/* Convert COMPOUND_TEXT xstr to Wide Character String cs, return
 * length of cs in characters (not including the terminating null character).
 * If cs is NULL, no conversion is done, but return length of cs.
 */
{
	register int	c;
	int	nskip;
	int	n = 0;
	int	g0, g1, gs;
	Char	*xstr1;

	/*
	 * Compound Text can include null octet. Therefore the length
	 * of xstr is able to be specified by parameter len.
	 * But if len is zero or negative, get length by strlen() assuming
	 * that no null octet exists.
	 */
	if (len <= 0) {
		len = strlen((char *)xstr);
	}

	/* In initial state, ISO 8859/1 is designated into G0/G1 */
	g0 = GSET_ASCII;	/* ASCII -> G0 */
	g1 = GSET_LATIN1R;	/* Latin/1 right hand part -> G1 */

	while (len-- > 0) {
		switch (c = *xstr++) {
		case NUL:
			break;
		case '\n':	/* NEWLINE */
		case '\t':	/* TAB */
		case ' ':	/* SPACE (Note: GL is always 94 charset) */
			if (cs) {
				cs->code = c;
				cs->gset = GSET_ASCII;
				cs++;
			}
			n++;
			break;
		case CSI:
			/*
			 * CSI sequence is generally in following form:
			 *	CSI {P} {I} F
			 *        P : 03/00 to 03/15
			 *        I : 02/00 to 02/15
			 *        F : 04/00 to 07/14
			 */
			/*
			 * Currently only directionality is definde
			 * as following:
			 *	CSI-1-]		begin left-to-right text
			 *	CSI-2-]		begin right-to-left text
			 *	CSI-]		end of string
			 * But this implementation ignores them.
			 */
			xstr1 = getcsi(xstr, len);
			if (xstr1 == NULL)
				return -1;
			len -= xstr1 - xstr;
			xstr = xstr1;
			break;
		case ESC:
			/*
			 * ESC sequence is generally in following form:
			 *	ESC {I} F
			 *        I : 02/00 to 02/15
			 *        F : 03/00 to 07/14
			 */
			/*
			 * Currently, following functions are defined:
			 *   Standard character set
			 *	ESC-(-F
			 *	ESC-$-(-F
			 *	ESC-)-F
			 *	ESC---F
			 *	ESC-$-)-F
			 *   Non standard character set
			 *	ESC-%-/-[0123]
			 * Standard character set must be accepted correctly.
			 * Non standard one is ignored but must be parsed
			 * for skipping data.
			 */
			xstr1 = getesc(xstr, len);
			if (xstr1 == NULL)
				return -1;
			len -= xstr1 - xstr;
			switch (xstr1 - xstr) {
			case 2:		/* ESC - I - F */
				switch (*xstr++) {
				case '(':	/* 94chars CS -> G0 */
					g0 = GSET(*xstr);
					break;
				case ')':	/* 94chars CS -> G1 */
					g1 = GSET(*xstr);
					break;
				case '-':	/* 96chars CS -> G1 */
					g1 = GSET(*xstr) | CS96;
					break;
				default:	/* ignore */
					break;
				}
				break;
			case 3:		/* ESC - I - I - F */
				switch (*xstr++) {
				case '$':
					switch (*xstr++) {
					case '(':	/* 94chars MBCS -> G0 */
						g0 = GSET(*xstr) | MBCS;
						break;
					case ')':	/* 94chars MBCS -> G1 */
						g1 = GSET(*xstr) | MBCS;
						break;
					case '-':	/* 96chars MBCS -> G1 */
						g1 = GSET(*xstr) | CS96 | MBCS;
						break;
					default:	/* ignore */
						break;
					}
					break;
				case '%':
					if (*xstr++ != '/') {
						/* unknown sequence */
						break;
					}
					/*
					 * Private encoding is ignored.
					 * But following data must be skipped.
					 *	ESC-%-/-F-M-L
					 */
					len -= 2;
					if (len < 0)
						return -1;
					nskip = (*xstr1 & ~0x80) * 128 +
					    (*(xstr1 + 1) & ~0x80);
					if ((len -= nskip) < 0)
						return -1;
					xstr1 += nskip + 2;
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
			xstr = xstr1;
			break;
		default:
			if (!(c & 0x60)) {
				/*
				 * Non NL/TAB/ESC/CSI character in C0 or C1
				 * is an obvious error.
				 */
				return -1;
			}
			gs = (c & 0x80) ? g1 : g0;
			if (cs) {
				cs->code = c & ~0x80;
				cs->gset = gs;
				cs++;
			}
			n++;
			break;
		}
	}
	if (cs) {
		cs->code = 0;
		cs->gset = 0;
	}
	return n;
}

/* EUC -> JIS */
int
convEUCtoJIS(es, js)
Char *es;
Char *js;
{
	Char e1, e2;
	Char gset = GSET_ASCII;
	int n = 0;

	while (e1 = *es++) {
		if (e1 == SS2) {
			if (e2 = *es++) {
				if (js)
					*js++ = e2 | 0x80;
				n++;
			}
			/* else { ??? } */
		} else if (e1 & 0x80) {
			if (e2 = *es++) {
				if (gset != GSET_KANJI) {
					if (js) {
						*js++ = ESC;
						*js++ = '$';
						*js++ = GSETFC(GSET_KANJI);
					}
					n += 3;
					gset = GSET_KANJI;
				}
				if (js) {
					*js++ = e1 & ~0x80;
					*js++ = e2 & ~0x80;
				}
				n += 2;
			}
			/* else { ??? } */
		} else {
			if (gset != GSET_ASCII) {
				if (js) {
					*js++ = ESC;
					*js++ = '(';
					*js++ = GSETFC(GSET_ASCII);
				}
				n += 3;
				gset = GSET_ASCII;
			}
			if (js) {
				*js++ = e1 & ~0x80;
			}
			n++;
		}
	}
	if (gset != GSET_ASCII) {
		if (js) {
			*js++ = ESC;
			*js++ = '(';
			*js++ = GSETFC(GSET_ASCII);
		}
		n += 3;
	}
	if (js) {
		*js = 0;
	}
	return n;
}

/* EUC -> SJIS */
int
convEUCtoSJIS(es, ss)
Char *es;
Char *ss;
{
	Char e1, e2;
	int n = 0;

	while (e1 = *es++) {
		if (e1 == SS2) {
			if (e2 = *es++) {
				if (ss)
					*ss++ = e2 | 0x80;
				n++;
			}
			/* else { ??? } */
		} else if (e1 & 0x80) {
			if (e2 = *es++) {
				if (ss) {
					e1 &= ~0x80;
					e2 &= ~0x80;
					JIStoSJIS(e1, e2, ss, ss+1);
					ss += 2;
				}
				n += 2;
			}
			/* else { ??? } */
		} else {
			if (ss)
				*ss++ = e1;
			n++;
		}
	}
	if (ss)
		*ss = 0;
	return n;
}
#endif
