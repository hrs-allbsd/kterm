/*
 *	$Id: xim.c,v 6.0 1996/07/12 05:01:39 kagotani Rel $
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
 */ 

#include <stdio.h>
#include <X11/Xos.h>
#include <X11/Xlocale.h>
#include "ptyx.h"
#include "menu.h"
#include "data.h"

extern char ** ParseList();

static void
setColor(screen, fg_p, bg_p)
TScreen *screen;
unsigned long *fg_p;
unsigned long *bg_p;
{
	extern XtermWidget term;
	*fg_p = screen->foreground;
	*bg_p = term->core.background_pixel;
}

void
IMSendColor(screen)
TScreen *screen;
{
	unsigned long	fg, bg;
	XVaNestedList	preedit_attr;

	if (!screen->xic || !(screen->xicstyle & XIMPreeditPosition)) return;

	setColor(screen, &fg, &bg);

	preedit_attr = XVaCreateNestedList(0,
					XNForeground, fg,
					XNBackground, bg,
					NULL);
	XSetICValues(screen->xic, XNPreeditAttributes, preedit_attr, NULL);
	XFree(preedit_attr);
}

static void
setSpot(screen, spot_p)
TScreen *screen;
XPoint *spot_p;
{
	spot_p->x = CursorX(screen, screen->cur_col);
	spot_p->y = CursorY(screen, screen->cur_row) + screen->max_ascent;
}

void
IMSendSpot(screen)
TScreen *screen;
{
	XPoint		spot;
	XVaNestedList	preedit_attr;

	if (!screen->xic || !(screen->xicstyle & XIMPreeditPosition)) return;

	setSpot(screen, &spot);

	preedit_attr = XVaCreateNestedList(0, XNSpotLocation, &spot, NULL);
	XSetICValues(screen->xic, XNPreeditAttributes, preedit_attr, NULL);
	XFree(preedit_attr);
}

static void
setFonts(screen, fontset_p)
TScreen *screen;
XFontSet *fontset_p;
{
	char		**missing_list, *def_string;
	int		missing_count;
	int		i, fnum, len;
	int		fontnum = screen->menu_font_number;
	char		*fn;
	static char	*fontnamelist;
	static int	fontnamelistlen;

	len = 0;
	for (fnum = F_ISO8859_1; fnum < FCNT; fnum++)
		if (fn = screen->_menu_font_names[fnum][fontnum])
			len += strlen(fn) + 1;
	if (fn = screen->menu_font_list[fontnum])
		len += strlen(fn) + 1;
	if (len > fontnamelistlen) {
		if (fontnamelist) XtFree(fontnamelist);
		fontnamelist = XtMalloc(fontnamelistlen = len);
	}
	fontnamelist[0] = '\0';

	for (fnum = F_ISO8859_1; fnum < FCNT; fnum++)
		if (fn = screen->_menu_font_names[fnum][fontnum]) {
			strcat(fontnamelist, fn);
			strcat(fontnamelist, ",");
		}
	if (fn = screen->menu_font_list[fontnum]) {
		strcat(fontnamelist, fn);
	}

	*fontset_p = XCreateFontSet(screen->display, fontnamelist,
				&missing_list, &missing_count, &def_string);
	for(i = 0; i < missing_count; i++)
		fprintf(stderr, "Warning: missing font: %s\n", missing_list[i]);
	XFreeStringList(missing_list);
}

void
IMSendFonts(screen)
TScreen *screen;
{
	XFontSet	fontset;
	XVaNestedList	preedit_attr;

	if (!screen->xic || !(screen->xicstyle & XIMPreeditPosition)) return;

	setFonts(screen, &fontset);

	if (fontset) {
		preedit_attr = XVaCreateNestedList(0, XNFontSet, fontset, NULL);
		XSetICValues(screen->xic, XNPreeditAttributes, preedit_attr, NULL);
		XFree(preedit_attr);
		if (screen->xicfontset)
			XFreeFontSet(screen->display, screen->xicfontset);
		screen->xicfontset = fontset;
	}
}

static void
setSize(screen, rect_p)
TScreen *screen;
XRectangle *rect_p;
{
	rect_p->x = screen->scrollbar;
	rect_p->y = 0;
	rect_p->width  = FullWidth(screen)  - screen->scrollbar;
	rect_p->height = FullHeight(screen);
}

void
IMSendSize(screen)
TScreen *screen;
{
	XRectangle	rect;
	XVaNestedList	preedit_attr;

	if (!screen->xic || !(screen->xicstyle & XIMPreeditPosition)) return;

	setSize(screen, &rect);

	preedit_attr = XVaCreateNestedList(0, XNArea, &rect, NULL);
	XSetICValues(screen->xic, XNPreeditAttributes, preedit_attr, NULL);
	XFree(preedit_attr);
}

void
SetLocale(localelist)
char	*localelist;
{
	char **locales;

	for (locales = ParseList(localelist); *locales; locales++) {
		if (setlocale(LC_CTYPE, *locales) && XSupportsLocale()) return;
	}

	fprintf(stderr, "Couldn't set locale: %s\n", localelist);
}

static XIMStyle
BestStyle(im_styles, style_name)
XIMStyles *im_styles;
char *style_name;
{
	static XIMStyle OverTheSpot_styles[] = {
		XIMPreeditPosition | XIMStatusNothing,
	/*
	 *	*** Where can I display the status ***
	 *	XIMPreeditPosition | XIMStatusArea,
	 */
		0
	};

	static XIMStyle Root_styles[] = {
		XIMPreeditNothing  | XIMStatusNothing,
		XIMPreeditNone     | XIMStatusNone,
		0
	};

	XIMStyle *styles;
	unsigned short i;

	if (!strcmp(style_name, "OverTheSpot")) {
		styles = OverTheSpot_styles;
	} else if (!strcmp(style_name, "Root")) {
		styles = Root_styles;
	} else {
		return 0;
	}

	for (; *styles; styles++)
		for (i = 0; i < im_styles->count_styles; i++)
			if (*styles == im_styles->supported_styles[i])
				return *styles;

	return 0;
}

/* ARGSUSED */
static void
IMDestroyCallback(im, client_data, call_data)
XIM	im;
XPointer	client_data;
XPointer	call_data; /* unused */
{
	extern XtermWidget term;
	TScreen	*screen = &term->screen;
#ifdef KTERM_DEBUG
	printf("IMDestroyCallback()\n");
#endif
	screen->xic = NULL;
}

/* ARGSUSED */
static void
IMInstantiateCallback(display, client_data, call_data)
Display	*display;
XPointer	client_data; /* unused */
XPointer	call_data; /* unused */
{
	extern XtermWidget term;
	TScreen	*screen = &term->screen;
	char **types;
	XIM im = NULL;
	XIMCallback ximcallback;
	XIMStyles *im_styles;
	XIMStyle best_style = 0;
	XPoint spot;
	XRectangle rect;
	XFontSet fontset;
	unsigned long fg, bg;
	XVaNestedList preedit_attr = NULL;
#ifdef KTERM_DEBUG
	printf("IMInstantiateCallback()\n");
#endif
	
	if (screen->xic) return;

	im = XOpenIM(display, NULL, NULL, NULL);

	if (!im) return;
	
	ximcallback.callback = IMDestroyCallback;
	ximcallback.client_data = NULL;
	XSetIMValues(im, XNDestroyCallback, &ximcallback, NULL);

	XGetIMValues(im, XNQueryInputStyle, &im_styles, NULL);
#ifdef KTERM_DEBUG
	{
	    int i;
	    printf("IM supported styles =\n");
	    for (i = 0; i < im_styles->count_styles; i++) {
		printf("    ");
		if(im_styles->supported_styles[i] & XIMPreeditArea)
			printf("XIMPreeditArea");
		else if(im_styles->supported_styles[i] & XIMPreeditCallbacks)
			printf("XIMPreeditCallbacks");
		else if(im_styles->supported_styles[i] & XIMPreeditPosition)
			printf("XIMPreeditPosition");
		else if(im_styles->supported_styles[i] & XIMPreeditNothing)
			printf("XIMPreeditNothing");
		else if(im_styles->supported_styles[i] & XIMPreeditNone)
			printf("XIMPreeditNone");
		printf(" | ");
		if(im_styles->supported_styles[i] & XIMStatusArea)
			printf("XIMStatusArea");
		else if(im_styles->supported_styles[i] & XIMStatusCallbacks)
			printf("XIMStatusCallbacks");
		else if(im_styles->supported_styles[i] & XIMStatusNothing)
			printf("XIMStatusNothing");
		else if(im_styles->supported_styles[i] & XIMStatusNone)
			printf("XIMStatusNone");
		printf("\n");
	    }
	}
#endif
	for (types = ParseList(term->misc.preedit_type); *types; types++) {
		best_style = BestStyle(im_styles, *types);
		if (best_style) break;
	}
	XFree(im_styles);

	if (!best_style) {
		fprintf(stderr, "input method doesn't support my preedit type: %s\n", term->misc.preedit_type);
		XCloseIM(im);
		return;
	}

	if (best_style & XIMPreeditPosition) {
		setSize(screen, &rect);
		setSpot(screen, &spot);
		setFonts(screen, &fontset);
		setColor(screen, &fg, &bg);

		preedit_attr = XVaCreateNestedList(0,
					XNArea, &rect,
					XNSpotLocation, &spot,
					XNFontSet, fontset,
					XNForeground, fg,
					XNBackground, bg,
					XNLineSpace, FontHeight(screen),
					NULL);
	}

	screen->xic = XCreateIC(im,
			XNInputStyle, best_style,
			XNClientWindow, XtWindow(term),
			XNFocusWindow, XtWindow(term),
			preedit_attr ? XNPreeditAttributes : NULL, preedit_attr,
			NULL);

	XFree(preedit_attr);

	if (!screen->xic) {
		fprintf(stderr, "Couldn't create input context.\n");
		XCloseIM(im);
	}
	screen->xicstyle = best_style;
	if (preedit_attr) screen->xicfontset = fontset;
}

void
CloseIM(screen)
TScreen	*screen;
{
	extern XtermWidget term; /* for update_openim() */
	char *p;
	char **mp;

	if (!screen->imregistered) return;

#ifdef KTERM_DEBUG
	{
	    Bool ret;
	    ret =
#endif
		XUnregisterIMInstantiateCallback(screen->display,
			NULL, NULL, NULL, IMInstantiateCallback, NULL);
#ifdef KTERM_DEBUG
	    if (ret)
		printf("Unregister succeeded.\n");
	    else
		printf("Unregister failed.\n");
	}
#endif
	screen->imregistered = False;

	if (screen->xic) {
		XCloseIM(XIMOfIC(screen->xic));
		screen->xic = NULL;
	}

	update_openim();
}

/* ARGSUSED */
void
HandleCloseIM(gw, event, params, nparams)
Widget	gw;
XEvent	*event;
String	*params;
Cardinal	*nparams;
{
	XtermWidget w = (XtermWidget) gw;
	TScreen	*screen = &w->screen;
	CloseIM(screen);
}

static char *
methodtomodifier(method)
char	*method;
{
	char *mod;

	if (!strcmp(method, "XMODIFIERS")) {
		mod = XtMalloc(1);
		*mod = '\0'; /* "" */
	} else {
		mod = XtMalloc(strlen(method) + 5);
		strcpy(mod, "@im=");
		strcat(mod, method);
	}

	return mod;
}

void
SetLocaleModifiers(params, nparams)
String	*params;		/* array of comma-separated strings */
Cardinal	nparams;	/* number of the strings */
{
	char *p;
	char *mod;

	if (nparams) {
		mod = methodtomodifier(params[0]);	/* "@im=..." */
	} else {
		mod = methodtomodifier("XMODIFIERS");	/* "" */
	}

	p = XSetLocaleModifiers(mod);
#ifdef KTERM_DEBUG
	printf("XSetLocaleModifiers(\"%s\") = \"%s\"\n", mod, p);
#endif

	XtFree(mod);

	if (!p) {
		fprintf(stderr, "Couldn't set locale modifiers: %s\n", mod);
		return;
	}
}

void
OpenIM(screen)
TScreen	*screen;
{
	extern XtermWidget term; /* for update_openim() */
#ifdef KTERM_KINPUT2
	extern Boolean Kinput2UnderConversion();

	if (Kinput2UnderConversion()) {
		Bell(XkbBI_MinorError,0);
		return;
	}
#endif /* KTERM_KINPUT2 */

	XRegisterIMInstantiateCallback(screen->display,
		NULL, NULL, NULL, IMInstantiateCallback, NULL);
	screen->imregistered = True;

	update_openim();
}

/* ARGSUSED */
void
HandleOpenIM(gw, event, params, nparams)
Widget	gw;
XEvent	*event;	/* unused */
String	*params;
Cardinal	*nparams;
{
	TScreen	*screen = &((XtermWidget)gw)->screen;

	CloseIM(screen);

	if (*nparams) {
		SetLocaleModifiers(params, *nparams);
	}

	OpenIM(screen);
}
