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
#include "unicode_map.h"

#define NUL	0x00

#define IsGsetAscii(gset)	((gset)==GSET_ASCII || (gset)==GSET_JISROMAN)
#define IsGsetKanji(gset)      (  (gset)==GSET_OLDKANJI   \
                               || (gset)==GSET_KANJI      \
                               || (gset)==GSET_90KANJI    \
                               || (gset)==GSET_EXTKANJI1  \
                               || (gset)==GSET_EXTKANJI2004_1 )

#define JIStoSJIS(c1, c2, s1_p, s2_p)                                 \
	*(s1_p) = ((c1) - 0x21) / 2 + (((c1) <= 0x5e) ? 0x81 : 0xc1); \
	if ((c1) & 1)	/* odd */                                     \
	    *(s2_p) = (c2) + (((c2) <= 0x5f) ? 0x1f : 0x20);          \
	else                                                          \
	    *(s2_p) = (c2) + 0x7e;

#define JIStoSJIS2(c1, c2, s1_p, s2_p)                                \
	if ((c1) < 0x30)                                              \
            *(s1_p) = ((c1) + 0x1bf) / 2 - (((c1) - 0x20) / 8) * 3;   \
        else                                                          \
            *(s1_p) = ((c1) + 0x17b) / 2;                             \
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

/* CS -> Japanese EUC */
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
	} else if (cs->gset == GSET_HOJOKANJI
		   || cs->gset == GSET_EXTKANJI2) {
		c1 = cs++->code;
		c2 = cs++->code;
		if (es) {
			*es++ = SS3;
			*es++ = c1 | 0x80;
			*es++ = c2 | 0x80;
		}
		*es_p = es;
		*cs_p = cs;
		return 3;
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
	} else if (cs->gset == GSET_EXTKANJI2) {
		c1 = cs++->code;
		c2 = cs++->code;
		if (ss) {
			JIStoSJIS2(c1, c2, ss, ss+1);
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

static int
utf8_len(int c)
{
	if (c < 0)
		return 0;
	else if (c < 0x80)
		return 1;
	else if (c < 0x800)
		return 2;
	else if (c < 0x10000)
		return 3;
	else if (c < 0x200000)
		return 4;
	else if (c < 0x4000000)
		return 5;
	else
		return 6;
}

static void
convUTF8(int c, Char *us)
{
	if (c < 0) {
		return;
	} else if (c < 0x80) {
		*us++ = c;
	} else if (c < 0x800) {
		*us++ = ((c >> 6) & 0x1f) | 0xc0;
		*us++ = (c & 0x3f) | 0x80;
	} else if (c < 0x10000) {
		*us++ = ((c >> 12) & 0x0f) | 0xe0;
		*us++ = ((c >> 6) & 0x3f) | 0x80;
		*us++ = (c & 0x3f) | 0x80;
	} else if (c < 0x200000) {
		*us++ = ((c >> 18) & 0x07) | 0xf0;
		*us++ = ((c >> 12) & 0x3f) | 0x80;
		*us++ = ((c >> 6) & 0x3f) | 0x80;
		*us++ = (c & 0x3f) | 0x80;
	} else if (c < 0x4000000) {
		*us++ = ((c >> 24) & 0x03) | 0xf8;
		*us++ = ((c >> 18) & 0x3f) | 0x80;
		*us++ = ((c >> 12) & 0x3f) | 0x80;
		*us++ = ((c >> 6) & 0x3f) | 0x80;
		*us++ = (c & 0x3f) | 0x80;
	} else {
		*us++ = ((c >> 30) & 0x01) | 0xfc;
		*us++ = ((c >> 24) & 0x3f) | 0x80;
		*us++ = ((c >> 18) & 0x3f) | 0x80;
		*us++ = ((c >> 12) & 0x3f) | 0x80;
		*us++ = ((c >> 6) & 0x3f) | 0x80;
		*us++ = (c & 0x3f) | 0x80;
	}
}

static int
combind_uchar(int c1, int c2, Char *us)
{
	static struct st_uc {
		int c1;
		int c2;
		int u1;
		int u2;
	} uchar_map[] = {
		{ 4, 87, 0x304b, 0x309a },
		{ 4, 88, 0x304d, 0x309a },
		{ 4, 89, 0x304f, 0x309a },
		{ 4, 90, 0x3051, 0x309a },
		{ 4, 91, 0x3053, 0x309a },
		{ 5, 87, 0x30ab, 0x309a },
		{ 5, 88, 0x30ad, 0x309a },
		{ 5, 89, 0x30af, 0x309a },
		{ 5, 90, 0x30b1, 0x309a },
		{ 5, 91, 0x30b3, 0x309a },
		{ 5, 92, 0x30bb, 0x309a },
		{ 5, 93, 0x30c4, 0x309a },
		{ 5, 94, 0x30c8, 0x309a },
		{ 6, 88, 0x31f7, 0x309a },
		{ 11, 36, 0x00e6, 0x0300 },
		{ 11, 40, 0x0254, 0x0300 },
		{ 11, 41, 0x0254, 0x0301 },
		{ 11, 42, 0x028c, 0x0300 },
		{ 11, 43, 0x028c, 0x0301 },
		{ 11, 44, 0x0259, 0x0300 },
		{ 11, 45, 0x0259, 0x0301 },
		{ 11, 46, 0x025a, 0x0300 },
		{ 11, 47, 0x025a, 0x0301 },
		{ 11, 69, 0x02e9, 0x02e5 },
		{ 11, 70, 0x02e5, 0x02e9 },
		{ 11, 65, 0x02e5, 0x200c }, /* Zero Width Non-Joiner */
		{ 11, 68, 0x02e9, 0x200c }, /* Zero Width Non-Joiner */
		{ 11, 65, 0x02e5, 0x200b }, /* Zero Width Space */
		{ 11, 68, 0x02e9, 0x200b }, /* Zero Width Space */
		{ 11, 65, 0x02e5, 0xfeff }, /* Zero Width Non-Breaking Space */
		{ 11, 68, 0x02e9, 0xfeff }, /* Zero Width Non-Breaking Space */
		{ 0, 0, 0, 0 },
	}, *p;

	for (p = uchar_map; p->c1; ++ p) {
		if (c1 == p->c1 + 0x20 && c2 == p->c2 + 0x20) {
			int len1 = utf8_len(p->u1);
			int len2 = utf8_len(p->u2);

			if (us) {
				convUTF8(p->u1, us);
				us += len1;
				convUTF8(p->u2, us);
				us += len2;
			}

			return len1 + len2;
		}
	}

	return 0;
}


/* CS -> UTF8 */
static int
CStoUTF8(cs_p, us_p)
Ichr	**cs_p;
Char	**us_p;
{
	Ichr	*cs = *cs_p;
	Char	*us = *us_p;
	int i;

	if (IsGsetKanji(cs->gset)) {
		int c1 = cs++->code & ~0x80;
		int c2 = cs++->code & ~0x80;
		int ucode = ucode_kanji1[(c1-33)*94 + (c2-33)];
		int len = utf8_len(ucode);

		if (ucode == U_error) {
			len = combind_uchar(c1, c2, us);
			if (us && len)
				us += len;
		} else if (us) {
			convUTF8(ucode, us);
			us += len;
		}
		*us_p = us;
		*cs_p = cs;
		return len;
        } else if (cs->gset == GSET_HOJOKANJI
		   || cs->gset == GSET_EXTKANJI2) {
		int c1 = cs++->code & ~0x80;
		int c2 = cs++->code & ~0x80;
		int ucode = ucode_kanji2[(c1-33)*94 + (c2-33)];
		int len = utf8_len(ucode);

		if (us) {
			convUTF8(ucode, us);
			us += len;
		}
		*us_p = us;
		*cs_p = cs;
		return len;
	} else if (cs->gset == GSET_JISROMAN) {
		int c = cs++->code;
		int ucode = ucode_latin[F_JISX0201_0][c & 0x7f];
		int len = utf8_len(ucode);
		if (ucode < 128)
			return 0;
		if (us) {
			convUTF8(ucode, us);
			us += len;
		}
		*us_p = us;
		*cs_p = cs;
		return len;
	} else if (IsGsetAscii(cs->gset) && cs->code < 128) {
		return 0;
	}

	for (i = F_ISO8859_1; i <= F_JISX0201_0; ++ i) {
		while (cs->gset == fnumtogset[i]) {
			int c = cs++->code;
			int ucode = ucode_latin[i][c | 0x80];
			int len = utf8_len(ucode);
			if (ucode < 128)
				return 0;
			if (us) {
				convUTF8(ucode, us);
				us += len;
			}
			*us_p = us;
			*cs_p = cs;
			return len;
		}
	}

	return 0;
}

int
convCStoUTF8(cs, ss)
Ichr	*cs;
Char	*ss;
{
	return convCStoANY(cs, ss, CStoUTF8);
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
				if (func == NULL || !IsGsetAscii(gset)) {
					if (as) {
						*as++ = ESC;
						*as++ = '(';
						*as++ = GSETFC(cs->gset);
					}
					n += 3;
				}
			} else if (cs->gset == GSET_OLDKANJI
				|| cs->gset == GSET_HANZI
				|| cs->gset == GSET_KANJI) {
				/* Use ESC-$-F instead of ESC-$-(-F (for @AB) */
				if (as) {
					*as++ = ESC;
					*as++ = '$';
					*as++ = GSETFC(cs->gset);
				}
				n += 3;
			} else if (cs->gset == GSET_90KANJI) {
				if (as) {
					*as++ = '\033';
					*as++ = '&';
					*as++ = '@';
					*as++ = '\033';
					*as++ = '$';
					*as++ = 'B';
				}
				n += 6;
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
			if (g1 == GSET_90KANJI) {
				if (as) {
					*as++ = ESC;
					*as++ = '&';
					*as++ = '@';
					*as++ = ESC;
                                        *as++ = '$';
                                        *as++ = ')';
					*as++ = 'B';
				}
				n += 7;
			} else {
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

/* chect_ctext_kterm -- check COMPOUND_TEXT created by KTerm or not */
int
check_ctext_kterm(xstr, len)
Char *xstr;
int len;
{
    if (len <= 0)
	len = strlen((char *)xstr);
    while (len-- > 0) {
	int c;
        Char *xstr1;

	switch (c = *xstr++) {
	case NUL:
	case '\n':      /* NEWLINE */
	case '\t':      /* TAB */
	case ' ':       /* SPACE (Note: GL is always 94 charset) */
	    break;
	case CSI:
	    xstr1 = getcsi(xstr, len);
	    if (xstr1 == NULL)
		return 0;
	    len -= xstr1 - xstr;
	    xstr = xstr1;
	    break;
	case ESC:
	    xstr1 = getesc(xstr, len);
	    if (xstr1 == NULL)
		return 0;
	    len -= xstr1 - xstr;
	    switch (xstr1 - xstr) {
	    case 2:
		if (*xstr == '%' || *xstr == '$')
		    return 0;
		break;
	    case 3:
		switch (*xstr++) {
		case '$':
		    switch (*xstr++) {
		    case '(':
		    case '-':
			return 0;
		    case ')':
			break;
		    default:
			break;
		    }
		    break;
		case '%':
		    return 0;
		}
		break;
	    default:
		break;
	    }
	    xstr = xstr1;
	    break;
	default:
	    if (!(c & 0x60))
		return 0;
	}
    }

    return 1;
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
	int     csversion = 0;

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
			 *   Character set version
			 *      ESC-&-F
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
				csversion = 0;
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
				case '&':       /* Character set version */
					csversion = *xstr;
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
						if (csversion == '@'
						    && *xstr == 'B')
							g0 = GSET_90KANJI;
						break;
					case ')':	/* 94chars MBCS -> G1 */
						g1 = GSET(*xstr) | MBCS;
						if (csversion == '@'
						    && *xstr == 'B')
							g1 = GSET_90KANJI;
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
				csversion = 0;
				break;
			default:
				csversion = 0;
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
			csversion = 0;
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
	Char e1, e2, e3;
	Char gset = GSET_ASCII;
	int n = 0;

	while (e1 = *es++) {
		if (e1 == SS2) {
			if (e2 = *es++) {
				if (gset != GSET_KANA) {
					if (js) {
						*js++ = ESC;
						*js++ = '(';
						*js++ = GSETFC(GSET_KANA);
					}
					n += 3;
					gset = GSET_KANA;
				}
				if (js) {
					*js++ = e2 & ~0x80;
				}
				n++;
			}
			/* else { ??? } */
		} else if (e1 == SS3) {
			if ((e2 = *es++) && (e3 = *es++)) {
				if (isJISX0213_2(e2, e3)) {
					if (gset != GSET_EXTKANJI2) {
						if (js) {
							*js++ = ESC;
							*js++ = '$';
							*js++ = '(';
							*js++ = GSETFC(GSET_EXTKANJI2);
						}
						n += 4;
						gset = GSET_EXTKANJI2;
					}
				} else {
					if (gset != GSET_HOJOKANJI) {
						if (js) {
							*js++ = ESC;
							*js++ = '$';
							*js++ = '(';
							*js++ = GSETFC(GSET_HOJOKANJI);
						}
						n += 4;
						gset = GSET_HOJOKANJI;
					}
				}
				if (js) {
					*js++ = e2 & ~0x80;
					*js++ = e3 & ~0x80;
				}
				n += 2;
			}
		} else if (e1 & 0x80) {
			if (e2 = *es++) {
				if (isJISX0213_2004_1(e1, e2)) {
					if (gset != GSET_EXTKANJI2004_1) {
						if (js) {
							*js++ = ESC;
							*js++ = '$';
							*js++ = '(';
							*js++ = GSETFC(GSET_EXTKANJI2004_1);
						}
						n += 4;
						gset = GSET_EXTKANJI2004_1;
					}
				} else if (isJISX0213_1(e1, e2)) {
					if (gset != GSET_EXTKANJI1) {
						if (js) {
							*js++ = ESC;
							*js++ = '$';
							*js++ = '(';
							*js++ = GSETFC(GSET_EXTKANJI1);
						}
						n += 4;
						gset = GSET_EXTKANJI1;
					}
				} else if (isJISX0208_1990(e1, e2)) {
					if (gset != GSET_90KANJI) {
						if (js) {
							*js++ = ESC;
							*js++ = '&';
							*js++ = '@';
							*js++ = ESC;
							*js++ = '$';
							*js++ = GSETFC(GSET_KANJI);
						}
						n += 6;
						gset = GSET_90KANJI;
					}
				} else {
					if (gset != GSET_KANJI) {
						if (js) {
							*js++ = ESC;
							*js++ = '$';
							*js++ = GSETFC(GSET_KANJI);
						}
						n += 3;
						gset = GSET_KANJI;
					}
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

/* EUC -> UTF8 */
int
convEUCtoUTF8(es, us)
Char *es;
Char *us;
{
	Char e1, e2, e3;
	int n = 0;

	while (e1 = *es++) {
		if (e1 == SS2) {
			if (e2 = *es++) {
				int c = e2 | 0x80;
				int ucode = ucode_latin[F_JISX0201_0][c];
				int len = utf8_len(ucode);

				if (us) {
					convUTF8(ucode, us);
					us += len;
				}
				n += len;
			}
		} else if (e1 == SS3) {
			if ((e2 = *es++) && (e3 = *es++)) {
				int c1 = e2 & ~0x80;
				int c2 = e3 & ~0x80;
				int ucode = ucode_kanji2[(c1-33)*94 + (c2-33)];
				int len = utf8_len(ucode);
				if (us) {
					convUTF8(ucode, us);
					us += len;
				}
				n += len;
			}
		} else if (e1 & 0x80) {
			if (e2 = *es++) {
				int c1 = e1 & ~0x80;
				int c2 = e2 & ~0x80;
				int ucode = ucode_kanji1[(c1-33)*94 + (e2-33)];
				int len = utf8_len(ucode);

				if (ucode == U_error) {
					len = combind_uchar(c1, c2, us);
					if (us && len)
						us += len;
				} else if (us) {
					convUTF8(ucode, us);
					us += len;
				}
				n += len;
			}
		} else {
			if (us)
				*us++ = e1;
			n++;
		}
	}
	if (us)
		*us = 0;
	return n;
}

/* EUC -> SJIS */
int
convEUCtoSJIS(es, ss)
Char *es;
Char *ss;
{
	Char e1, e2, e3;
	int n = 0;

	while (e1 = *es++) {
		if (e1 == SS2) {
			if (e2 = *es++) {
				if (ss)
					*ss++ = e2 | 0x80;
				n++;
			}
			/* else { ??? } */
		} else if (e1 == SS3) {
			if ((e2 = *es++) && (e3 = *es++)) {
				if (ss) {
					if (isJISX0213_2(e2, e3)) {
						JIStoSJIS2(e2, e3, ss, ss+1);
					} else {
						JIStoSJIS(0x21, 0x22, ss, ss+1);
					}
					ss += 2;
				}
				n += 2;
			}
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

int
check_combined(int ucode, Char *cp, int cnt, int *umap)
{
	static struct st_cmb {
		int code;
		int clen;
		char *cstr;
		int umap;
	} combine_map[] = {
		{ 0x304b, 3, "\xe3\x82\x9a", UMAP(GSET_EXTKANJI1, 4*94+87-95)},
		{ 0x304d, 3, "\xe3\x82\x9a", UMAP(GSET_EXTKANJI1, 4*94+88-95)},
		{ 0x304f, 3, "\xe3\x82\x9a", UMAP(GSET_EXTKANJI1, 4*94+89-95)},
		{ 0x3051, 3, "\xe3\x82\x9a", UMAP(GSET_EXTKANJI1, 4*94+90-95)},
		{ 0x3053, 3, "\xe3\x82\x9a", UMAP(GSET_EXTKANJI1, 4*94+91-95)},
		{ 0x30ab, 3, "\xe3\x82\x9a", UMAP(GSET_EXTKANJI1, 5*94+87-95)},
		{ 0x30ad, 3, "\xe3\x82\x9a", UMAP(GSET_EXTKANJI1, 5*94+88-95)},
		{ 0x30af, 3, "\xe3\x82\x9a", UMAP(GSET_EXTKANJI1, 5*94+89-95)},
		{ 0x30b1, 3, "\xe3\x82\x9a", UMAP(GSET_EXTKANJI1, 5*94+90-95)},
		{ 0x30b3, 3, "\xe3\x82\x9a", UMAP(GSET_EXTKANJI1, 5*94+91-95)},
		{ 0x30bb, 3, "\xe3\x82\x9a", UMAP(GSET_EXTKANJI1, 5*94+92-95)},
		{ 0x30c4, 3, "\xe3\x82\x9a", UMAP(GSET_EXTKANJI1, 5*94+93-95)},
		{ 0x30c8, 3, "\xe3\x82\x9a", UMAP(GSET_EXTKANJI1, 5*94+94-95)},
		{ 0x31f7, 3, "\xe3\x82\x9a", UMAP(GSET_EXTKANJI1, 6*94+88-95)},
		{ 0x00e6, 2, "\xcc\x80", UMAP(GSET_EXTKANJI1, 11*94+36-95)},
		{ 0x0254, 2, "\xcc\x80", UMAP(GSET_EXTKANJI1, 11*94+40-95)},
		{ 0x0254, 2, "\xcc\x81", UMAP(GSET_EXTKANJI1, 11*94+41-95)},
		{ 0x028c, 2, "\xcc\x80", UMAP(GSET_EXTKANJI1, 11*94+42-95)},
		{ 0x028c, 2, "\xcc\x81", UMAP(GSET_EXTKANJI1, 11*94+43-95)},
		{ 0x0259, 2, "\xcc\x80", UMAP(GSET_EXTKANJI1, 11*94+44-95)},
		{ 0x0259, 2, "\xcc\x81", UMAP(GSET_EXTKANJI1, 11*94+45-95)},
		{ 0x025a, 2, "\xcc\x80", UMAP(GSET_EXTKANJI1, 11*94+46-95)},
		{ 0x025a, 2, "\xcc\x81", UMAP(GSET_EXTKANJI1, 11*94+47-95)},
		{ 0x02e9, 2, "\xcb\xa5", UMAP(GSET_EXTKANJI1, 11*94+69-95)},
		{ 0x02e5, 2, "\xcb\xa9", UMAP(GSET_EXTKANJI1, 11*94+70-95)},
		{ 0x02e5, 3, "\xe2\x80\x8c", UMAP(GSET_EXTKANJI1,11*94+65-95)},
		{ 0x02e9, 3, "\xe2\x80\x8c", UMAP(GSET_EXTKANJI1,11*94+68-95)},
		{ 0x02e5, 3, "\xe2\x80\x8b", UMAP(GSET_EXTKANJI1,11*94+65-95)},
		{ 0x02e9, 3, "\xe2\x80\x8b", UMAP(GSET_EXTKANJI1,11*94+68-95)},
		{ 0x02e5, 3, "\xef\xbb\xbf", UMAP(GSET_EXTKANJI1,11*94+65-95)},
		{ 0x02e9, 3, "\xef\xbb\xbf", UMAP(GSET_EXTKANJI1,11*94+68-95)},
		{ 0, 0, NULL, U_error },
	}, *p;

	for (p = combine_map; p->code; ++ p) {
		if (ucode == p->code
		    && cnt > p->clen  
		    && !strncmp(cp, p->cstr, p->clen)) {
			*umap = p->umap;
			return p->clen;
		}
	}

	return 0;
}

/* UTF8 => CS */
int
convUTF8toCS(p, len, cs)
Char *p;
int len;
Ichr *cs;
{
	int uchar;
	int count = 0;

# define UTF8_Head1(c) ((c) < 0x80)
# define UTF8_Head2(c) ((c) >= 0xc0 && (c) <= 0xdf)
# define UTF8_Head3(c) ((c) >= 0xe0 && (c) <= 0xef)
# define UTF8_Head4(c) ((c) >= 0xf0 && (c) <= 0xf7)
# define UTF8_Head5(c) ((c) >= 0xf8 && (c) <= 0xfb)
# define UTF8_Head6(c) ((c) >= 0xfc && (c) <= 0xfd)
# define UTF8_Tail(c)  ((c) >= 0x80 && (c) <= 0xbf)

	while (len > 0) {
		int n;
		int umap;
		int plane;
		int code;
		int gset;
		int offset;

		if (UTF8_Head1(*p)) {
			uchar = *p ++;
			len --;
		} else if (len >= 2 && UTF8_Head2(*p)
			   && UTF8_Tail(p[1])) {
			uchar = (*p & 0x1f) << 6 | (p[1] & 0x3f);
			p += 2;
			len -= 2;
		} else if (len >= 3 && UTF8_Head3(*p)
			   && UTF8_Tail(p[1])
			   && UTF8_Tail(p[2])) {
			uchar = (*p & 0xf) << 12
				| (p[1] & 0x3f) << 6
				| (p[2] & 0x3f);
			p += 3;
			len -= 3;
		} else if (len >= 4 && UTF8_Head4(*p)
			   && UTF8_Tail(p[1])
			   && UTF8_Tail(p[2])
			   && UTF8_Tail(p[3])) {
			uchar = (*p & 0x7) << 18
				| (p[1] & 0x3f) << 12
				| (p[2] & 0x3f) << 6
				| (p[3] & 0x3f);
			p += 4;
			len -= 4;
		} else if (len >= 5 && UTF8_Head5(*p)
			   && UTF8_Tail(p[1])
			   && UTF8_Tail(p[2])
			   && UTF8_Tail(p[3])
			   && UTF8_Tail(p[4])) {
			uchar = (*p & 0x3) << 24
				| (p[1] & 0x3f) << 18
				| (p[2] & 0x3f) << 12
				| (p[3] & 0x3f) << 6
				| (p[4] & 0x3f);
			p += 5;
			len -= 5;
		} else if (len >= 6 && UTF8_Head6(*p)
			   && UTF8_Tail(p[1])
			   && UTF8_Tail(p[2])
			   && UTF8_Tail(p[3])
			   && UTF8_Tail(p[4])
			   && UTF8_Tail(p[5])) {
			uchar = (*p & 0x1) << 30
				| (p[1] & 0x3f) << 24
				| (p[2] & 0x3f) << 18
				| (p[3] & 0x3f) << 12
				| (p[4] & 0x3f) << 6
				| (p[5] & 0x3f);
			p += 6;
			len -= 6;
		} else {
			/* Invalid UTF-8 character: skip first byte */
			p ++;
			len --;
			continue;
		}

		plane = (uchar & 0x7fff0000) >> 16;
		code  = uchar & 0xffff;
		n = check_combined(uchar, p, len, &umap);
		if (n) {
			p += n;
			len -= n;
		} else if (plane == 0) {
			umap = unicode0_map[code];
		} else if (plane == 2) {
			umap = unicode2_map[code];
		} else {
			/* skip non-japanese character */
			continue;
		}
		if (umap == U_error) {
			/* skip non-japanese character */
			continue;
		}

		gset = UMAP_GSET(umap);
		offset = UMAP_CHAR(umap);
		if (gset & MBCS) {
			if  (cs) {
				cs->gset = gset;
				cs->code = offset / 94 + 0x21;
				cs ++;
				cs->gset = gset;
				cs->code = offset % 94 + 0x21;
				cs ++;
			}
			count += 2;
		} else {
			if (cs) {
				cs->gset = gset;
				cs->code = offset;
				cs ++;
			}
			count ++;
		}

	}
	if (cs) {
		cs->gset = 0;
		cs->code = 0;
	}

	return count;
}

int
pasteCStoUTF8(cs, us)
Ichr *cs;
Char *us;
{
	int n = 0;

	while (cs->code) {
		int m = CStoUTF8(&cs, &us);
		if (m > 0) {
			n += m;
		} else if (IsGsetAscii(cs->gset) && cs->code < 128) {
			if (us)
				*us ++ = cs->code;
			cs ++;
			n ++;
		} else {
			cs ++;
		}
	}
	if (us)
		*us = '\0';

	return n;
}

#endif
