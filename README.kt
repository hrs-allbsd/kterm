---------------------------
README file for kterm-6.2.0
---------------------------

Kterm is an X11 terminal emulator that can handle multi-lingual text.
This release is based on xterm of X11R6.1.


Changes in this release
-----------------------
 o Kterm now handles multi-lingual text in addition to Japanese.
 o Kterm now supports X Input Method protocol in addition to kinput2 protocol.
 o Fonts are loaded dynamically by default.
 o Tektronix emulation is disabled by default configuration.
 o Many bug fixes.


Configuration
-------------
You can disable any of the features added to xterm by editing Imakefile
and kterm.h.

Imakefile:
 o KTERM:           enables kterm features. see kterm.h part.
 o STATUSLINE:      enables statusline support.
 o KEEPALIVE:       enables keepalive socket option for the server connection.

kterm.h:
 o KTERM_MBCS:      enables support for MBCS text.
 o KTERM_MBCC:      enables MB character class support. It is used for word
		    selection.  KTERM_MBCS must be defined.
 o KTERM_KANJIMODE: enables support for Kanji specific encodings, Japanese EUC
		    and Shift-JIS.  KTERM_MBCS must be defined.
 o KTERM_XIM:       enables support for Kanji text input using X Input Method
		    protocol.  KTERM_MBCS must be defined.
 o KTERM_KINPUT2:   enables support for Kanji text input using kinput2
		    protocol.  KTERM_MBCS must be defined.
 o KTERM_COLOR:	    enables colored text support.
 o KTERM_NOTEK:	    disables Tektronix emulation.
 o KTERM_XAW3D:	    enables kterm to work with the Xaw3d library compiled
		    with -DARROW_SCROLLBAR option (disables by default).


Compilation
-----------
This kterm basically needs X11R6.1 libraries and include files.
In X11R6.1 environment, just type:
 % xmkmf -a; make

In X11R6 environment, if you have some luck (your system supports fd_set
type and FD_* macros), you can compile kterm without modification.


Installation
------------
Install kterm and KTerm.ad:
 % make install

Install kterm.man manual page:
 % make install.man

If your system supports Japanese manual pages and you want to install
kterm.jman, copy it to an appropriate directory/filename in an
appropriate encoding by hand.  Note that kterm.jman is encoded in JIS
code.

If your system does not have kterm entry in the termcap or terminfo,
you may want to install termcap.kt or terminfo.kt by hand.


Supported Systems
-----------------
kterm-6.2.0 should be successfully built on the following systems:

    sparc
	SunOS 4.1.x (*1)
	SunOS 4.1.x-JLE*
	Solaris 2.x
	Solaris 2.xJ
	NetBSD-1.1B/sparc
	NetBSD/sparc-1.2_ALPHA
	UXP/DS V20L10 Y96021 [Fujitsu DS/90 7000]

    i386
	Solaris 2.5 x86
	NetBSD1.1
	NetBSD/{i386,pc98} 1.2_ALPHA
	FreeBSD(98)2.1R Alpha (XFree86 3.1.2Eb) (*2)
	FreeBSD 2.2-96{0323,0612}-SNAP (XFree86 3.1.2E) (*2) (*3)
	BSD/OS 2.1
	Linux-2.0.0 ELF libc 5.3.12

    sony
	NEWS-OS 4.2.1a+RD
	NEWS-OS 6.0.2

    sgi
	IRIX 5.3 {,IP22}
	IRIX 6.1

    hp
	HP-UX 9.0[57] (with -DX_LOCALE, cc)
	HP-UX 10.01

    dec
	OSF/1 3.0 [DEC 7000 AXP system 620]
	OSF/1 3.2A [DEC 3000/500]
	Digital UNIX 3.2C [DEC AlphaStation 200 4/100]
	Digital UNIX 3.2D [DEC AlphaServer 2100 4/275]
	*** does not work on DEC 3000 running DEC OSF/1 2.0 ***

    *1	Kterm and X libraries must be compiled with -DX_LOCALE to use XIM.
    *2	Kterm must be compiled with -DUSE_POSIX_WAIT to eliminate a warning.
    *3	Kterm must be linked with -lxpg4 to use XIM.


CAUTION
-------
Killing an IM server while it is establishing connection with a kterm,
or on a kterm which is connected with the server may hang the kterm
(because of imperfection around IM handling in Xlib).


To Do
-----
 o Use of fontSet resources.
 o More ISO6429 attributes.


Bug report
----------
If you find any bug, please make sure that it is not an xterm's bug, then
report it to kagotani@in.it.okayama-u.ac.jp.  Don't forget to include
the version of kterm (see kterm.h or do "kterm -version") and information
about your system.


Enhancement
-----------
If you would like to distribute your local enhancement, please change
the version number to something like "6.2.0-yourname1".


						July 12, 1996
						Hiroto Kagotani
						kagotani@in.it.okayama-u.ac.jp
