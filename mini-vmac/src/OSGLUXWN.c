/*
	OSGLUXWN.c

	Copyright (C) 2009 Michael Hanni, Christian Bauer,
	Stephan Kochen, Paul C. Pratt, and others

	You can redistribute this file and/or modify it under the terms
	of version 2 of the GNU General Public License as published by
	the Free Software Foundation.  You should have received a copy
	of the license along with this file; see the file COPYING.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	license for more details.
*/

/*
	Operating System GLUe for X WiNdow system

	All operating system dependent code for the
	X Window System should go here.

	This code is descended from Michael Hanni's X
	port of vMac, by Philip Cummins.
	I learned more about how X programs work by
	looking at other programs such as Basilisk II,
	the UAE Amiga Emulator, Bochs, QuakeForge,
	DooM Legacy, and the FLTK. A few snippets
	from them are used here.

	Drag and Drop support is based on the specification
	"XDND: Drag-and-Drop Protocol for the X Window System"
	developed by John Lindal at New Planet Software, and
	looking at included examples, one by Paul Sheer.
*/

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "AppVersion.h"

#include "OSGCOMUI.h"
#include "OSGCOMUD.h"

#ifdef WantOSGLUXWN

/* --- some simple utilities --- */

GLOBALOSGLUPROC MyMoveBytes(anyp srcPtr, anyp destPtr, si5b byteCount)
{
	(void) memcpy((char *)destPtr, (char *)srcPtr, byteCount);
}

/* --- control mode and internationalization --- */

#define NeedCell2PlainAsciiMap 1

#include "INTLCHAR.h"


LOCALVAR char *d_arg = NULL;
LOCALVAR char *n_arg = NULL;

#if CanGetAppPath
LOCALVAR char *app_parent = NULL;
LOCALVAR char *app_name = NULL;
LOCALVAR char *pref_dir = NULL;
LOCALVAR blnr runningAsAppImage = 0;
#endif

LOCALFUNC tMacErr ChildPath(char *x, char *y, char **r)
{
	tMacErr err = mnvm_miscErr;
	int nx = strlen(x);
	int ny = strlen(y);
	{
		if ((nx > 0) && ('/' == x[nx - 1])) {
			--nx;
		}
		{
			int nr = nx + 1 + ny;
			char *p = malloc(nr + 1);
			if (p != NULL) {
				char *p2 = p;
				(void) memcpy(p2, x, nx);
				p2 += nx;
				*p2++ = '/';
				(void) memcpy(p2, y, ny);
				p2 += ny;
				*p2 = 0;
				*r = p;
				err = mnvm_noErr;
			}
		}
	}

	return err;
}

#if UseActvFile || IncludeSonyNew
LOCALFUNC tMacErr FindOrMakeChild(char *x, char *y, char **r)
{
	tMacErr err;
	struct stat folder_info;
	char *r0;

	if (mnvm_noErr == (err = ChildPath(x, y, &r0))) {
		if (0 != stat(r0, &folder_info)) {
			if (0 != mkdir(r0, S_IRWXU)) {
				err = mnvm_miscErr;
			} else {
				*r = r0;
				err = mnvm_noErr;
			}
		} else {
			if (! S_ISDIR(folder_info.st_mode)) {
				err = mnvm_miscErr;
			} else {
				*r = r0;
				err = mnvm_noErr;
			}
		}
	}

	return err;
}
#endif

LOCALPROC MyMayFree(char *p)
{
	if (NULL != p) {
		free(p);
	}
}

/* --- sending debugging info to file --- */

#if dbglog_HAVE

#ifndef dbglog_ToStdErr
#define dbglog_ToStdErr 0
#endif

#if ! dbglog_ToStdErr
LOCALVAR FILE *dbglog_File = NULL;
#endif

LOCALFUNC blnr dbglog_open0(void)
{
#if dbglog_ToStdErr
	return trueblnr;
#else
	dbglog_File = fopen("dbglog.txt", "w");
	return (NULL != dbglog_File);
#endif
}

LOCALPROC dbglog_write0(char *s, uimr L)
{
#if dbglog_ToStdErr
	(void) fwrite(s, 1, L, stderr);
#else
	if (dbglog_File != NULL) {
		(void) fwrite(s, 1, L, dbglog_File);
	}
#endif
}

LOCALPROC dbglog_close0(void)
{
#if ! dbglog_ToStdErr
	if (dbglog_File != NULL) {
		fclose(dbglog_File);
		dbglog_File = NULL;
	}
#endif
}

#endif

/* --- debug settings and utilities --- */

#if ! dbglog_HAVE
#define WriteExtraErr(s)
#else
LOCALPROC WriteExtraErr(char *s)
{
	dbglog_writeCStr("*** error: ");
	dbglog_writeCStr(s);
	dbglog_writeReturn();
}
#endif

LOCALVAR Display *x_display = NULL;

#define MyDbgEvents (dbglog_HAVE && 0)

#if MyDbgEvents
LOCALPROC WriteDbgAtom(char *s, Atom x)
{
	char *name = XGetAtomName(x_display, x);
	if (name != NULL) {
		dbglog_writeCStr("Atom ");
		dbglog_writeCStr(s);
		dbglog_writeCStr(": ");
		dbglog_writeCStr(name);
		dbglog_writeReturn();
		XFree(name);
	}
}
#endif

/* --- information about the environment --- */

LOCALVAR Atom MyXA_DeleteW = (Atom)0;
#if EnableDragDrop
LOCALVAR Atom MyXA_UriList = (Atom)0;
LOCALVAR Atom MyXA_DndAware = (Atom)0;
LOCALVAR Atom MyXA_DndEnter = (Atom)0;
LOCALVAR Atom MyXA_DndLeave = (Atom)0;
LOCALVAR Atom MyXA_DndDrop = (Atom)0;
LOCALVAR Atom MyXA_DndPosition = (Atom)0;
LOCALVAR Atom MyXA_DndStatus = (Atom)0;
LOCALVAR Atom MyXA_DndActionCopy = (Atom)0;
LOCALVAR Atom MyXA_DndActionPrivate = (Atom)0;
LOCALVAR Atom MyXA_DndSelection = (Atom)0;
LOCALVAR Atom MyXA_DndFinished = (Atom)0;
LOCALVAR Atom MyXA_MinivMac_DndXchng = (Atom)0;
LOCALVAR Atom MyXA_NetActiveWindow = (Atom)0;
LOCALVAR Atom MyXA_NetSupported = (Atom)0;
#endif
#if IncludeHostTextClipExchange
LOCALVAR Atom MyXA_CLIPBOARD = (Atom)0;
LOCALVAR Atom MyXA_TARGETS = (Atom)0;
LOCALVAR Atom MyXA_MinivMac_Clip = (Atom)0;
#endif

LOCALPROC LoadMyXA(void)
{
	MyXA_DeleteW = XInternAtom(x_display, "WM_DELETE_WINDOW", False);
#if EnableDragDrop
	MyXA_UriList = XInternAtom (x_display, "text/uri-list", False);
	MyXA_DndAware = XInternAtom (x_display, "XdndAware", False);
	MyXA_DndEnter = XInternAtom(x_display, "XdndEnter", False);
	MyXA_DndLeave = XInternAtom(x_display, "XdndLeave", False);
	MyXA_DndDrop = XInternAtom(x_display, "XdndDrop", False);
	MyXA_DndPosition = XInternAtom(x_display, "XdndPosition", False);
	MyXA_DndStatus = XInternAtom(x_display, "XdndStatus", False);
	MyXA_DndActionCopy = XInternAtom(x_display,
		"XdndActionCopy", False);
	MyXA_DndActionPrivate = XInternAtom(x_display,
		"XdndActionPrivate", False);
	MyXA_DndSelection = XInternAtom(x_display, "XdndSelection", False);
	MyXA_DndFinished = XInternAtom(x_display, "XdndFinished", False);
	MyXA_MinivMac_DndXchng = XInternAtom(x_display,
		"_MinivMac_DndXchng", False);
	MyXA_NetActiveWindow = XInternAtom(x_display,
		"_NET_ACTIVE_WINDOW", False);
	MyXA_NetSupported = XInternAtom(x_display,
		"_NET_SUPPORTED", False);
#endif
#if IncludeHostTextClipExchange
	MyXA_CLIPBOARD = XInternAtom(x_display, "CLIPBOARD", False);
	MyXA_TARGETS = XInternAtom(x_display, "TARGETS", False);
	MyXA_MinivMac_Clip = XInternAtom(x_display,
		"_MinivMac_Clip", False);
#endif
}

#if EnableDragDrop
LOCALFUNC blnr NetSupportedContains(Atom x)
{
	/*
		Note that the window manager could be replaced at
		any time, so don't cache results of this function.
	*/
	Atom ret_type;
	int ret_format;
	unsigned long ret_item;
	unsigned long remain_byte;
	unsigned long i;
	unsigned char *s = 0;
	blnr foundit = falseblnr;
	Window rootwin = XRootWindow(x_display,
		DefaultScreen(x_display));

	if (Success != XGetWindowProperty(x_display, rootwin,
		MyXA_NetSupported,
		0, 65535, False, XA_ATOM, &ret_type, &ret_format,
		&ret_item, &remain_byte, &s))
	{
		WriteExtraErr("XGetWindowProperty failed");
	} else if (! s) {
		WriteExtraErr("XGetWindowProperty failed");
	} else if (ret_type != XA_ATOM) {
		WriteExtraErr("XGetWindowProperty returns wrong type");
	} else {
		Atom *v = (Atom *)s;

		for (i = 0; i < ret_item; ++i) {
			if (v[i] == x) {
				foundit = trueblnr;
				/* fprintf(stderr, "found the hint\n"); */
			}
		}
	}
	if (s) {
		XFree(s);
	}
	return foundit;
}
#endif

#define WantColorTransValid 1

#include "COMOSGLU.h"

#include "PBUFSTDC.h"

#include "CONTROLM.h"

/* --- text translation --- */

#if IncludePbufs
/* this is table for Windows, any changes needed for X? */
LOCALVAR const ui3b Native2MacRomanTab[] = {
	0xAD, 0xB0, 0xE2, 0xC4, 0xE3, 0xC9, 0xA0, 0xE0,
	0xF6, 0xE4, 0xB6, 0xDC, 0xCE, 0xB2, 0xB3, 0xB7,
	0xB8, 0xD4, 0xD5, 0xD2, 0xD3, 0xA5, 0xD0, 0xD1,
	0xF7, 0xAA, 0xC5, 0xDD, 0xCF, 0xB9, 0xC3, 0xD9,
	0xCA, 0xC1, 0xA2, 0xA3, 0xDB, 0xB4, 0xBA, 0xA4,
	0xAC, 0xA9, 0xBB, 0xC7, 0xC2, 0xBD, 0xA8, 0xF8,
	0xA1, 0xB1, 0xC6, 0xD7, 0xAB, 0xB5, 0xA6, 0xE1,
	0xFC, 0xDA, 0xBC, 0xC8, 0xDE, 0xDF, 0xF0, 0xC0,
	0xCB, 0xE7, 0xE5, 0xCC, 0x80, 0x81, 0xAE, 0x82,
	0xE9, 0x83, 0xE6, 0xE8, 0xED, 0xEA, 0xEB, 0xEC,
	0xF5, 0x84, 0xF1, 0xEE, 0xEF, 0xCD, 0x85, 0xF9,
	0xAF, 0xF4, 0xF2, 0xF3, 0x86, 0xFA, 0xFB, 0xA7,
	0x88, 0x87, 0x89, 0x8B, 0x8A, 0x8C, 0xBE, 0x8D,
	0x8F, 0x8E, 0x90, 0x91, 0x93, 0x92, 0x94, 0x95,
	0xFD, 0x96, 0x98, 0x97, 0x99, 0x9B, 0x9A, 0xD6,
	0xBF, 0x9D, 0x9C, 0x9E, 0x9F, 0xFE, 0xFF, 0xD8
};
#endif

#if IncludePbufs
LOCALFUNC tMacErr NativeTextToMacRomanPbuf(char *x, tPbuf *r)
{
	if (NULL == x) {
		return mnvm_miscErr;
	} else {
		ui3p p;
		ui5b L = strlen(x);

		p = (ui3p)malloc(L);
		if (NULL == p) {
			return mnvm_miscErr;
		} else {
			ui3b *p0 = (ui3b *)x;
			ui3b *p1 = (ui3b *)p;
			int i;

			for (i = L; --i >= 0; ) {
				ui3b v = *p0++;
				if (v >= 128) {
					v = Native2MacRomanTab[v - 128];
				} else if (10 == v) {
					v = 13;
				}
				*p1++ = v;
			}

			return PbufNewFromPtr(p, L, r);
		}
	}
}
#endif

#if IncludePbufs
/* this is table for Windows, any changes needed for X? */
LOCALVAR const ui3b MacRoman2NativeTab[] = {
	0xC4, 0xC5, 0xC7, 0xC9, 0xD1, 0xD6, 0xDC, 0xE1,
	0xE0, 0xE2, 0xE4, 0xE3, 0xE5, 0xE7, 0xE9, 0xE8,
	0xEA, 0xEB, 0xED, 0xEC, 0xEE, 0xEF, 0xF1, 0xF3,
	0xF2, 0xF4, 0xF6, 0xF5, 0xFA, 0xF9, 0xFB, 0xFC,
	0x86, 0xB0, 0xA2, 0xA3, 0xA7, 0x95, 0xB6, 0xDF,
	0xAE, 0xA9, 0x99, 0xB4, 0xA8, 0x80, 0xC6, 0xD8,
	0x81, 0xB1, 0x8D, 0x8E, 0xA5, 0xB5, 0x8A, 0x8F,
	0x90, 0x9D, 0xA6, 0xAA, 0xBA, 0xAD, 0xE6, 0xF8,
	0xBF, 0xA1, 0xAC, 0x9E, 0x83, 0x9A, 0xB2, 0xAB,
	0xBB, 0x85, 0xA0, 0xC0, 0xC3, 0xD5, 0x8C, 0x9C,
	0x96, 0x97, 0x93, 0x94, 0x91, 0x92, 0xF7, 0xB3,
	0xFF, 0x9F, 0xB9, 0xA4, 0x8B, 0x9B, 0xBC, 0xBD,
	0x87, 0xB7, 0x82, 0x84, 0x89, 0xC2, 0xCA, 0xC1,
	0xCB, 0xC8, 0xCD, 0xCE, 0xCF, 0xCC, 0xD3, 0xD4,
	0xBE, 0xD2, 0xDA, 0xDB, 0xD9, 0xD0, 0x88, 0x98,
	0xAF, 0xD7, 0xDD, 0xDE, 0xB8, 0xF0, 0xFD, 0xFE
};
#endif

#if IncludePbufs
LOCALFUNC blnr MacRomanTextToNativePtr(tPbuf i, blnr IsFileName,
	ui3p *r)
{
	ui3p p;
	void *Buffer = PbufDat[i];
	ui5b L = PbufSize[i];

	p = (ui3p)malloc(L + 1);
	if (p != NULL) {
		ui3b *p0 = (ui3b *)Buffer;
		ui3b *p1 = (ui3b *)p;
		int j;

		if (IsFileName) {
			for (j = L; --j >= 0; ) {
				ui3b x = *p0++;
				if (x < 32) {
					x = '-';
				} else if (x >= 128) {
					x = MacRoman2NativeTab[x - 128];
				} else {
					switch (x) {
						case '/':
						case '<':
						case '>':
						case '|':
						case ':':
							x = '-';
						default:
							break;
					}
				}
				*p1++ = x;
			}
			if ('.' == p[0]) {
				p[0] = '-';
			}
		} else {
			for (j = L; --j >= 0; ) {
				ui3b x = *p0++;
				if (x >= 128) {
					x = MacRoman2NativeTab[x - 128];
				} else if (13 == x) {
					x = '\n';
				}
				*p1++ = x;
			}
		}
		*p1 = 0;

		*r = p;
		return trueblnr;
	}
	return falseblnr;
}
#endif

LOCALPROC NativeStrFromCStr(char *r, char *s)
{
	ui3b ps[ClStrMaxLength];
	int i;
	int L;

	ClStrFromSubstCStr(&L, ps, s);

	for (i = 0; i < L; ++i) {
		r[i] = Cell2PlainAsciiMap[ps[i]];
	}

	r[L] = 0;
}

/* --- drives --- */

#define NotAfileRef NULL

LOCALVAR FILE *Drives[NumDrives]; /* open disk image files */
#if IncludeSonyGetName || IncludeSonyNew
LOCALVAR char *DriveNames[NumDrives];
#endif

LOCALPROC InitDrives(void)
{
	/*
		This isn't really needed, Drives[i] and DriveNames[i]
		need not have valid values when not vSonyIsInserted[i].
	*/
	tDrive i;

	for (i = 0; i < NumDrives; ++i) {
		Drives[i] = NotAfileRef;
#if IncludeSonyGetName || IncludeSonyNew
		DriveNames[i] = NULL;
#endif
	}
}

GLOBALOSGLUFUNC tMacErr vSonyTransfer(blnr IsWrite, ui3p Buffer,
	tDrive Drive_No, ui5r Sony_Start, ui5r Sony_Count,
	ui5r *Sony_ActCount)
{
	tMacErr err = mnvm_miscErr;
	FILE *refnum = Drives[Drive_No];
	ui5r NewSony_Count = 0;

	if (0 == fseek(refnum, Sony_Start, SEEK_SET)) {
		if (IsWrite) {
			NewSony_Count = fwrite(Buffer, 1, Sony_Count, refnum);
		} else {
			NewSony_Count = fread(Buffer, 1, Sony_Count, refnum);
		}

		if (NewSony_Count == Sony_Count) {
			err = mnvm_noErr;
		}
	}

	if (nullpr != Sony_ActCount) {
		*Sony_ActCount = NewSony_Count;
	}

	return err; /*& figure out what really to return &*/
}

GLOBALOSGLUFUNC tMacErr vSonyGetSize(tDrive Drive_No, ui5r *Sony_Count)
{
	tMacErr err = mnvm_miscErr;
	FILE *refnum = Drives[Drive_No];
	long v;

	if (0 == fseek(refnum, 0, SEEK_END)) {
		v = ftell(refnum);
		if (v >= 0) {
			*Sony_Count = v;
			err = mnvm_noErr;
		}
	}

	return err; /*& figure out what really to return &*/
}

#ifndef HaveAdvisoryLocks
#define HaveAdvisoryLocks 1
#endif

/*
	What is the difference between fcntl(fd, F_SETLK ...
	and flock(fd ... ?
*/

#if HaveAdvisoryLocks
LOCALFUNC blnr MyLockFile(FILE *refnum)
{
	blnr IsOk = falseblnr;

#if 1
	struct flock fl;
	int fd = fileno(refnum);

	fl.l_start = 0; /* starting offset */
	fl.l_len = 0; /* len = 0 means until end of file */
	/* fl.pid_t l_pid; */ /* lock owner, don't need to set */
	fl.l_type = F_WRLCK; /* lock type: read/write, etc. */
	fl.l_whence = SEEK_SET; /* type of l_start */
	if (-1 == fcntl(fd, F_SETLK, &fl)) {
		MacMsg(kStrImageInUseTitle, kStrImageInUseMessage,
			falseblnr);
	} else {
		IsOk = trueblnr;
	}
#else
	int fd = fileno(refnum);

	if (-1 == flock(fd, LOCK_EX | LOCK_NB)) {
		MacMsg(kStrImageInUseTitle, kStrImageInUseMessage,
			falseblnr);
	} else {
		IsOk = trueblnr;
	}
#endif

	return IsOk;
}
#endif

#if HaveAdvisoryLocks
LOCALPROC MyUnlockFile(FILE *refnum)
{
#if 1
	struct flock fl;
	int fd = fileno(refnum);

	fl.l_start = 0; /* starting offset */
	fl.l_len = 0; /* len = 0 means until end of file */
	/* fl.pid_t l_pid; */ /* lock owner, don't need to set */
	fl.l_type = F_UNLCK;     /* lock type: read/write, etc. */
	fl.l_whence = SEEK_SET;   /* type of l_start */
	if (-1 == fcntl(fd, F_SETLK, &fl)) {
		/* an error occurred */
	}
#else
	int fd = fileno(refnum);

	if (-1 == flock(fd, LOCK_UN)) {
	}
#endif
}
#endif

LOCALFUNC tMacErr vSonyEject0(tDrive Drive_No, blnr deleteit)
{
	FILE *refnum = Drives[Drive_No];

	DiskEjectedNotify(Drive_No);

#if HaveAdvisoryLocks
	MyUnlockFile(refnum);
#endif

	fclose(refnum);
	Drives[Drive_No] = NotAfileRef; /* not really needed */

#if IncludeSonyGetName || IncludeSonyNew
	{
		char *s = DriveNames[Drive_No];
		if (NULL != s) {
			if (deleteit) {
				remove(s);
			}
			free(s);
			DriveNames[Drive_No] = NULL; /* not really needed */
		}
	}
#endif

	return mnvm_noErr;
}

GLOBALOSGLUFUNC tMacErr vSonyEject(tDrive Drive_No)
{
	return vSonyEject0(Drive_No, falseblnr);
}

#if IncludeSonyNew
GLOBALOSGLUFUNC tMacErr vSonyEjectDelete(tDrive Drive_No)
{
	return vSonyEject0(Drive_No, trueblnr);
}
#endif

LOCALPROC UnInitDrives(void)
{
	tDrive i;

	for (i = 0; i < NumDrives; ++i) {
		if (vSonyIsInserted(i)) {
			(void) vSonyEject(i);
		}
	}
}

#if IncludeSonyGetName
GLOBALOSGLUFUNC tMacErr vSonyGetName(tDrive Drive_No, tPbuf *r)
{
	char *drivepath = DriveNames[Drive_No];
	if (NULL == drivepath) {
		return mnvm_miscErr;
	} else {
		char *s = strrchr(drivepath, '/');
		if (NULL == s) {
			s = drivepath;
		} else {
			++s;
		}
		return NativeTextToMacRomanPbuf(s, r);
	}
}
#endif

LOCALFUNC blnr Sony_Insert0(FILE *refnum, blnr locked,
	char *drivepath)
{
	tDrive Drive_No;
	blnr IsOk = falseblnr;

	if (! FirstFreeDisk(&Drive_No)) {
		MacMsg(kStrTooManyImagesTitle, kStrTooManyImagesMessage,
			falseblnr);
	} else {
		/* printf("Sony_Insert0 %d\n", (int)Drive_No); */

#if HaveAdvisoryLocks
		if (locked || MyLockFile(refnum))
#endif
		{
			Drives[Drive_No] = refnum;
			DiskInsertNotify(Drive_No, locked);

#if IncludeSonyGetName || IncludeSonyNew
			{
				ui5b L = strlen(drivepath);
				char *p = malloc(L + 1);
				if (p != NULL) {
					(void) memcpy(p, drivepath, L + 1);
				}
				DriveNames[Drive_No] = p;
			}
#endif

			IsOk = trueblnr;
		}
	}

	if (! IsOk) {
		fclose(refnum);
	}

	return IsOk;
}

LOCALFUNC blnr Sony_Insert1(char *drivepath, blnr silentfail)
{
	blnr locked = falseblnr;
	/* printf("Sony_Insert1 %s\n", drivepath); */
	FILE *refnum = fopen(drivepath, "rb+");
	if (NULL == refnum) {
		locked = trueblnr;
		refnum = fopen(drivepath, "rb");
	}
	if (NULL == refnum) {
		if (! silentfail) {
			MacMsg(kStrOpenFailTitle, kStrOpenFailMessage, falseblnr);
		}
	} else {
		return Sony_Insert0(refnum, locked, drivepath);
	}
	return falseblnr;
}

LOCALFUNC tMacErr LoadMacRomFrom(char *path)
{
	tMacErr err;
	FILE *ROM_File;
	int File_Size;

	ROM_File = fopen(path, "rb");
	if (NULL == ROM_File) {
		err = mnvm_fnfErr;
	} else {
		File_Size = fread(ROM, 1, kROM_Size, ROM_File);
		if (kROM_Size != File_Size) {
			if (feof(ROM_File)) {
				MacMsgOverride(kStrShortROMTitle,
					kStrShortROMMessage);
				err = mnvm_eofErr;
			} else {
				MacMsgOverride(kStrNoReadROMTitle,
					kStrNoReadROMMessage);
				err = mnvm_miscErr;
			}
		} else {
			err = ROM_IsValid();
		}
		fclose(ROM_File);
	}

	return err;
}

LOCALFUNC blnr Sony_Insert1a(char *drivepath, blnr silentfail)
{
	blnr v;

	if (! ROM_loaded) {
		v = (mnvm_noErr == LoadMacRomFrom(drivepath));
	} else {
		v = Sony_Insert1(drivepath, silentfail);
	}

	return v;
}

LOCALFUNC blnr Sony_Insert2(char *s)
{
	char *d =
#if CanGetAppPath
		(NULL == d_arg) ? ((runningAsAppImage && pref_dir) ? pref_dir : app_parent) :
#endif
		d_arg;
	blnr IsOk = falseblnr;

	if (NULL == d) {
		IsOk = Sony_Insert1(s, trueblnr);
	} else {
		char *t;

		if (mnvm_noErr == ChildPath(d, s, &t)) {
			IsOk = Sony_Insert1(t, trueblnr);
			free(t);
		}
	}

	return IsOk;
}

LOCALFUNC blnr Sony_InsertIth(int i)
{
	blnr v;

	if ((i > 9) || ! FirstFreeDisk(nullpr)) {
		v = falseblnr;
	} else {
		char s[] = "disk?.dsk";

		s[4] = '0' + i;

		v = Sony_Insert2(s);
	}

	return v;
}

LOCALFUNC blnr LoadInitialImages(void)
{
	if (! AnyDiskInserted()) {
		int i, v;

		v = Sony_Insert2("BlahBlob.dsk");

		if (v != trueblnr) {
			fprintf(stderr, "Couldn't find BlahBlob.dsk\n");
			return falseblnr;
		}

		v = Sony_Insert2("BlahBlobData.dsk");

		if (v != trueblnr) {
			fprintf(stderr, "Couldn't find BlahBlobData.dsk\n");
			return falseblnr;
		}

		for (i = 1; Sony_InsertIth(i); ++i) {
			/* stop on first error (including file not found) */
		}
	}

	return trueblnr;
}

#if IncludeSonyNew
LOCALFUNC blnr WriteZero(FILE *refnum, ui5b L)
{
#define ZeroBufferSize 2048
	ui5b i;
	ui3b buffer[ZeroBufferSize];

	memset(&buffer, 0, ZeroBufferSize);

	while (L > 0) {
		i = (L > ZeroBufferSize) ? ZeroBufferSize : L;
		if (fwrite(buffer, 1, i, refnum) != i) {
			return falseblnr;
		}
		L -= i;
	}
	return trueblnr;
}
#endif

#if IncludeSonyNew
LOCALPROC MakeNewDisk0(ui5b L, char *drivepath)
{
	blnr IsOk = falseblnr;
	FILE *refnum = fopen(drivepath, "wb+");
	if (NULL == refnum) {
		MacMsg(kStrOpenFailTitle, kStrOpenFailMessage, falseblnr);
	} else {
		if (WriteZero(refnum, L)) {
			IsOk = Sony_Insert0(refnum, falseblnr, drivepath);
			refnum = NULL;
		}
		if (refnum != NULL) {
			fclose(refnum);
		}
		if (! IsOk) {
			(void) remove(drivepath);
		}
	}
}
#endif

#if IncludeSonyNew
LOCALPROC MakeNewDisk(ui5b L, char *drivename)
{
	char *d =
#if CanGetAppPath
		(NULL == d_arg) ? app_parent :
#endif
		d_arg;

	if (NULL == d) {
		MakeNewDisk0(L, drivename); /* in current directory */
	} else {
		tMacErr err;
		char *t = NULL;
		char *t2 = NULL;

		if (mnvm_noErr == (err = FindOrMakeChild(d, "out", &t)))
		if (mnvm_noErr == (err = ChildPath(t, drivename, &t2)))
		{
			MakeNewDisk0(L, t2);
		}

		MyMayFree(t2);
		MyMayFree(t);
	}
}
#endif

#if IncludeSonyNew
LOCALPROC MakeNewDiskAtDefault(ui5b L)
{
	char s[ClStrMaxLength + 1];

	NativeStrFromCStr(s, "untitled.dsk");
	MakeNewDisk(L, s);
}
#endif

/* --- ROM --- */

LOCALVAR char *rom_path = NULL;

#if 0
#include <pwd.h>
#include <unistd.h>
#endif

LOCALFUNC tMacErr FindUserHomeFolder(char **r)
{
	tMacErr err;
	char *s;
#if 0
	struct passwd *user;
#endif

	if (NULL != (s = getenv("HOME"))) {
		*r = s;
		err = mnvm_noErr;
	} else
#if 0
	if ((NULL != (user = getpwuid(getuid())))
		&& (NULL != (s = user->pw_dir)))
	{
		/*
			From getpwuid man page:
			"An application that wants to determine its user's
			home directory should inspect the value of HOME
			(rather than the value getpwuid(getuid())->pw_dir)
			since this allows the user to modify their notion of
			"the home directory" during a login session."

			But it is possible for HOME to not be set.
			Some sources say to use getpwuid in that case.
		*/
		*r = s;
		err = mnvm_noErr;
	} else
#endif
	{
		err = mnvm_fnfErr;
	}

	return err;
}

LOCALFUNC tMacErr LoadMacRomFromHome(void)
{
	tMacErr err;
	char *s;
	char *t = NULL;
	char *t2 = NULL;
	char *t3 = NULL;

	if (mnvm_noErr == (err = FindUserHomeFolder(&s)))
	if (mnvm_noErr == (err = ChildPath(s, ".gryphel", &t)))
	if (mnvm_noErr == (err = ChildPath(t, "mnvm_rom", &t2)))
	if (mnvm_noErr == (err = ChildPath(t2, RomFileName, &t3)))
	{
		err = LoadMacRomFrom(t3);
	}

	MyMayFree(t3);
	MyMayFree(t2);
	MyMayFree(t);

	return err;
}

#if CanGetAppPath
LOCALFUNC tMacErr LoadMacRomFromAppPar(void)
{
	tMacErr err;
	char *d =
#if CanGetAppPath
		(NULL == d_arg) ? app_parent :
#endif
		d_arg;
	char *t = NULL;

	if (NULL == d) {
		err = mnvm_fnfErr;
	} else {
		if (mnvm_noErr == (err = ChildPath(d, RomFileName,
			&t)))
		{
			err = LoadMacRomFrom(t);
		}
	}

	MyMayFree(t);

	return err;
}
#endif

LOCALFUNC blnr LoadMacRom(void)
{
	tMacErr err;

	if ((NULL == rom_path)
		|| (mnvm_fnfErr == (err = LoadMacRomFrom(rom_path))))
#if CanGetAppPath
	if (mnvm_fnfErr == (err = LoadMacRomFromAppPar()))
#endif
	if (mnvm_fnfErr == (err = LoadMacRomFromHome()))
	if (mnvm_fnfErr == (err = LoadMacRomFrom(RomFileName)))
	{
	}

	return trueblnr; /* keep launching Mini vMac, regardless */
}

#if UseActvFile

#define ActvCodeFileName "act_1"

LOCALFUNC tMacErr ActvCodeFileLoad(ui3p p)
{
	tMacErr err;
	char *s;
	char *t = NULL;
	char *t2 = NULL;
	char *t3 = NULL;

	if (mnvm_noErr == (err = FindUserHomeFolder(&s)))
	if (mnvm_noErr == (err = ChildPath(s, ".gryphel", &t)))
	if (mnvm_noErr == (err = ChildPath(t, "mnvm_act", &t2)))
	if (mnvm_noErr == (err = ChildPath(t2, ActvCodeFileName, &t3)))
	{
		FILE *Actv_File;
		int File_Size;

		Actv_File = fopen(t3, "rb");
		if (NULL == Actv_File) {
			err = mnvm_fnfErr;
		} else {
			File_Size = fread(p, 1, ActvCodeFileLen, Actv_File);
			if (File_Size != ActvCodeFileLen) {
				if (feof(Actv_File)) {
					err = mnvm_eofErr;
				} else {
					err = mnvm_miscErr;
				}
			} else {
				err = mnvm_noErr;
			}
			fclose(Actv_File);
		}
	}

	MyMayFree(t3);
	MyMayFree(t2);
	MyMayFree(t);

	return err;
}

LOCALFUNC tMacErr ActvCodeFileSave(ui3p p)
{
	tMacErr err;
	char *s;
	char *t = NULL;
	char *t2 = NULL;
	char *t3 = NULL;

	if (mnvm_noErr == (err = FindUserHomeFolder(&s)))
	if (mnvm_noErr == (err = FindOrMakeChild(s, ".gryphel", &t)))
	if (mnvm_noErr == (err = FindOrMakeChild(t, "mnvm_act", &t2)))
	if (mnvm_noErr == (err = ChildPath(t2, ActvCodeFileName, &t3)))
	{
		FILE *Actv_File;
		int File_Size;

		Actv_File = fopen(t3, "wb+");
		if (NULL == Actv_File) {
			err = mnvm_fnfErr;
		} else {
			File_Size = fwrite(p, 1, ActvCodeFileLen, Actv_File);
			if (File_Size != ActvCodeFileLen) {
				err = mnvm_miscErr;
			} else {
				err = mnvm_noErr;
			}
			fclose(Actv_File);
		}
	}

	MyMayFree(t3);
	MyMayFree(t2);
	MyMayFree(t);

	return err;
}

#endif /* UseActvFile */

/* --- video out --- */

LOCALVAR Window my_main_wind = 0;
LOCALVAR GC my_gc = NULL;
LOCALVAR blnr NeedFinishOpen1 = falseblnr;
LOCALVAR blnr NeedFinishOpen2 = falseblnr;

LOCALVAR XColor x_black;
LOCALVAR XColor x_white;

#if MayFullScreen
LOCALVAR short hOffset;
LOCALVAR short vOffset;
#endif

#if VarFullScreen
LOCALVAR blnr UseFullScreen = (WantInitFullScreen != 0);
#endif

#if EnableMagnify
LOCALVAR blnr UseMagnify = (WantInitMagnify != 0);
#endif

LOCALVAR blnr gBackgroundFlag = falseblnr;
LOCALVAR blnr gTrueBackgroundFlag = falseblnr;
LOCALVAR blnr CurSpeedStopped = trueblnr;

#ifndef UseColorImage
#define UseColorImage (0 != vMacScreenDepth)
#endif

LOCALVAR XImage *my_image = NULL;

#if EnableMagnify
LOCALVAR XImage *my_Scaled_image = NULL;
#endif

#if EnableMagnify
#define MaxScale MyWindowScale
#else
#define MaxScale 1
#endif

#define WantScalingTabl (EnableMagnify || UseColorImage)

#if WantScalingTabl

LOCALVAR ui3p ScalingTabl = nullpr;

#define ScalingTablsz1 (256 * MaxScale)

#if UseColorImage
#define ScalingTablsz (ScalingTablsz1 << 5)
#else
#define ScalingTablsz ScalingTablsz1
#endif

#endif /* WantScalingTabl */


#define WantScalingBuff (EnableMagnify || UseColorImage)

#if WantScalingBuff

LOCALVAR ui3p ScalingBuff = nullpr;


#if UseColorImage
#define ScalingBuffsz \
	(vMacScreenNumPixels * 4 * MaxScale * MaxScale)
#else
#define ScalingBuffsz ((long)vMacScreenMonoNumBytes \
	* MaxScale * MaxScale)
#endif

#endif /* WantScalingBuff */


#if EnableMagnify && ! UseColorImage
LOCALPROC SetUpScalingTabl(void)
{
	ui3b *p4;
	int i;
	int j;
	int k;
	ui3r bitsRemaining;
	ui3b t1;
	ui3b t2;

	p4 = ScalingTabl;
	for (i = 0; i < 256; ++i) {
		bitsRemaining = 8;
		t2 = 0;
		for (j = 8; --j >= 0; ) {
			t1 = (i >> j) & 1;
			for (k = MyWindowScale; --k >= 0; ) {
				t2 = (t2 << 1) | t1;
				if (--bitsRemaining == 0) {
					*p4++ = t2;
					bitsRemaining = 8;
					t2 = 0;
				}
			}
		}
	}
}
#endif

#if EnableMagnify && (0 != vMacScreenDepth) && (vMacScreenDepth < 4)
LOCALPROC SetUpColorScalingTabl(void)
{
	int i;
	int j;
	int k;
	int a;
	ui5r v;
	ui5p p4;

	p4 = (ui5p)ScalingTabl;
	for (i = 0; i < 256; ++i) {
		for (k = 1 << (3 - vMacScreenDepth); --k >= 0; ) {
			j = (i >> (k << vMacScreenDepth)) & (CLUT_size - 1);
			v = (((long)CLUT_reds[j] & 0xFF00) << 8)
				| ((long)CLUT_greens[j] & 0xFF00)
				| (((long)CLUT_blues[j] & 0xFF00) >> 8);
			for (a = MyWindowScale; --a >= 0; ) {
				*p4++ = v;
			}
		}
	}
}
#endif

#if (0 != vMacScreenDepth) && (vMacScreenDepth < 4)
LOCALPROC SetUpColorTabl(void)
{
	int i;
	int j;
	int k;
	ui5p p4;

	p4 = (ui5p)ScalingTabl;
	for (i = 0; i < 256; ++i) {
		for (k = 1 << (3 - vMacScreenDepth); --k >= 0; ) {
			j = (i >> (k << vMacScreenDepth)) & (CLUT_size - 1);
			*p4++ = (((long)CLUT_reds[j] & 0xFF00) << 8)
				| ((long)CLUT_greens[j] & 0xFF00)
				| (((long)CLUT_blues[j] & 0xFF00) >> 8);
		}
	}
}
#endif

#if EnableMagnify && UseColorImage
LOCALPROC SetUpBW2ColorScalingTabl(void)
{
	int i;
	int k;
	int a;
	ui5r v;
	ui5p p4;

	p4 = (ui5p)ScalingTabl;
	for (i = 0; i < 256; ++i) {
		for (k = 8; --k >= 0; ) {
			if (0 != ((i >> k) & 1)) {
				v = 0;
			} else {
				v = 0xFFFFFF;
			}

			for (a = MyWindowScale; --a >= 0; ) {
				*p4++ = v;
			}
		}
	}
}
#endif

#if UseColorImage
LOCALPROC SetUpBW2ColorTabl(void)
{
	int i;
	int k;
	ui5r v;
	ui5p p4;

	p4 = (ui5p)ScalingTabl;
	for (i = 0; i < 256; ++i) {
		for (k = 8; --k >= 0; ) {
			if (0 != ((i >> k) & 1)) {
				v = 0;
			} else {
				v = 0xFFFFFF;
			}
			*p4++ = v;
		}
	}
}
#endif


#if EnableMagnify && ! UseColorImage

#define ScrnMapr_DoMap UpdateScaledBWCopy
#define ScrnMapr_Src GetCurDrawBuff()
#define ScrnMapr_Dst ScalingBuff
#define ScrnMapr_SrcDepth 0
#define ScrnMapr_DstDepth 0
#define ScrnMapr_Map ScalingTabl
#define ScrnMapr_Scale MyWindowScale

#include "SCRNMAPR.h"

#endif


#if (0 != vMacScreenDepth) && (vMacScreenDepth < 4)

#define ScrnMapr_DoMap UpdateMappedColorCopy
#define ScrnMapr_Src GetCurDrawBuff()
#define ScrnMapr_Dst ScalingBuff
#define ScrnMapr_SrcDepth vMacScreenDepth
#define ScrnMapr_DstDepth 5
#define ScrnMapr_Map ScalingTabl

#include "SCRNMAPR.h"

#endif


#if EnableMagnify && (0 != vMacScreenDepth) && (vMacScreenDepth < 4)

#define ScrnMapr_DoMap UpdateMappedScaledColorCopy
#define ScrnMapr_Src GetCurDrawBuff()
#define ScrnMapr_Dst ScalingBuff
#define ScrnMapr_SrcDepth vMacScreenDepth
#define ScrnMapr_DstDepth 5
#define ScrnMapr_Map ScalingTabl
#define ScrnMapr_Scale MyWindowScale

#include "SCRNMAPR.h"

#endif


#if vMacScreenDepth >= 4

#define ScrnTrns_DoTrans UpdateTransColorCopy
#define ScrnTrns_Src GetCurDrawBuff()
#define ScrnTrns_Dst ScalingBuff
#define ScrnTrns_SrcDepth vMacScreenDepth
#define ScrnTrns_DstDepth 5

#include "SCRNTRNS.h"

#endif

#if EnableMagnify && (vMacScreenDepth >= 4)

#define ScrnTrns_DoTrans UpdateTransScaledColorCopy
#define ScrnTrns_Src GetCurDrawBuff()
#define ScrnTrns_Dst ScalingBuff
#define ScrnTrns_SrcDepth vMacScreenDepth
#define ScrnTrns_DstDepth 5
#define ScrnTrns_Scale MyWindowScale

#include "SCRNTRNS.h"

#endif


#if EnableMagnify && UseColorImage

#define ScrnMapr_DoMap UpdateMappedScaledBW2ColorCopy
#define ScrnMapr_Src GetCurDrawBuff()
#define ScrnMapr_Dst ScalingBuff
#define ScrnMapr_SrcDepth 0
#define ScrnMapr_DstDepth 5
#define ScrnMapr_Map ScalingTabl
#define ScrnMapr_Scale MyWindowScale

#include "SCRNMAPR.h"

#endif


#if UseColorImage

#define ScrnMapr_DoMap UpdateMappedBW2ColorCopy
#define ScrnMapr_Src GetCurDrawBuff()
#define ScrnMapr_Dst ScalingBuff
#define ScrnMapr_SrcDepth 0
#define ScrnMapr_DstDepth 5
#define ScrnMapr_Map ScalingTabl

#include "SCRNMAPR.h"

#endif


LOCALPROC HaveChangedScreenBuff(ui4r top, ui4r left,
	ui4r bottom, ui4r right)
{
	int XDest;
	int YDest;
	char *the_data;

#if VarFullScreen
	if (UseFullScreen)
#endif
#if MayFullScreen
	{
		if (top < ViewVStart) {
			top = ViewVStart;
		}
		if (left < ViewHStart) {
			left = ViewHStart;
		}
		if (bottom > ViewVStart + ViewVSize) {
			bottom = ViewVStart + ViewVSize;
		}
		if (right > ViewHStart + ViewHSize) {
			right = ViewHStart + ViewHSize;
		}

		if ((top >= bottom) || (left >= right)) {
			goto label_exit;
		}
	}
#endif

	XDest = left;
	YDest = top;

#if VarFullScreen
	if (UseFullScreen)
#endif
#if MayFullScreen
	{
		XDest -= ViewHStart;
		YDest -= ViewVStart;
	}
#endif

#if EnableMagnify
	if (UseMagnify) {
		XDest *= MyWindowScale;
		YDest *= MyWindowScale;
	}
#endif

#if VarFullScreen
	if (UseFullScreen)
#endif
#if MayFullScreen
	{
		XDest += hOffset;
		YDest += vOffset;
	}
#endif

#if EnableMagnify
	if (UseMagnify) {
#if UseColorImage
#if 0 != vMacScreenDepth
		if (UseColorMode) {
#if vMacScreenDepth < 4
			if (! ColorTransValid) {
				SetUpColorScalingTabl();
				ColorTransValid = trueblnr;
			}

			UpdateMappedScaledColorCopy(top, left, bottom, right);
#else
			UpdateTransScaledColorCopy(top, left, bottom, right);
#endif
		} else
#endif /* 0 != vMacScreenDepth */
		{
			if (! ColorTransValid) {
				SetUpBW2ColorScalingTabl();
				ColorTransValid = trueblnr;
			}

			UpdateMappedScaledBW2ColorCopy(top, left, bottom, right);
		}
#else /* ! UseColorImage */
		/* assume 0 == vMacScreenDepth */
		{
			if (! ColorTransValid) {
				SetUpScalingTabl();
				ColorTransValid = trueblnr;
			}

			UpdateScaledBWCopy(top, left, bottom, right);
		}
#endif /* UseColorImage */

		{
			char *saveData = my_Scaled_image->data;
			my_Scaled_image->data = (char *)ScalingBuff;

			XPutImage(x_display, my_main_wind, my_gc, my_Scaled_image,
				left * MyWindowScale, top * MyWindowScale,
				XDest, YDest,
				(right - left) * MyWindowScale,
				(bottom - top) * MyWindowScale);

			my_Scaled_image->data = saveData;
		}
	} else
#endif /* EnableMagnify */
	{
#if UseColorImage
#if 0 != vMacScreenDepth
		if (UseColorMode) {
#if vMacScreenDepth < 4

			if (! ColorTransValid) {
				SetUpColorTabl();
				ColorTransValid = trueblnr;
			}

			UpdateMappedColorCopy(top, left, bottom, right);

			the_data = (char *)ScalingBuff;
#else
			/*
				if vMacScreenDepth == 5 and MSBFirst, could
				copy directly with the_data = (char *)GetCurDrawBuff();
			*/
			UpdateTransColorCopy(top, left, bottom, right);

			the_data = (char *)ScalingBuff;
#endif
		} else
#endif /* 0 != vMacScreenDepth */
		{
			if (! ColorTransValid) {
				SetUpBW2ColorTabl();
				ColorTransValid = trueblnr;
			}

			UpdateMappedBW2ColorCopy(top, left, bottom, right);

			the_data = (char *)ScalingBuff;
		}
#else /* ! UseColorImage */
		{
			the_data = (char *)GetCurDrawBuff();
		}
#endif /* UseColorImage */

		{
			char *saveData = my_image->data;
			my_image->data = the_data;

			XPutImage(x_display, my_main_wind, my_gc, my_image,
				left, top, XDest, YDest,
				right - left, bottom - top);

			my_image->data = saveData;
		}
	}

#if MayFullScreen
label_exit:
	;
#endif
}

LOCALPROC MyDrawChangesAndClear(void)
{
	if (ScreenChangedBottom > ScreenChangedTop) {
		HaveChangedScreenBuff(ScreenChangedTop, ScreenChangedLeft,
			ScreenChangedBottom, ScreenChangedRight);
		ScreenClearChanges();
	}
}

/* --- mouse --- */

/* cursor hiding */

LOCALVAR blnr HaveCursorHidden = falseblnr;
LOCALVAR blnr WantCursorHidden = falseblnr;

LOCALPROC ForceShowCursor(void)
{
	if (HaveCursorHidden) {
		HaveCursorHidden = falseblnr;
		if (my_main_wind) {
			XUndefineCursor(x_display, my_main_wind);
		}
	}
}

LOCALVAR Cursor blankCursor = None;

LOCALFUNC blnr CreateMyBlankCursor(Window rootwin)
/*
	adapted from X11_CreateNullCursor in context.x11.c
	in quakeforge 0.5.5, copyright Id Software, Inc.
	Zephaniah E. Hull, and Jeff Teunissen.
*/
{
	Pixmap cursormask;
	XGCValues xgc;
	GC gc;
	blnr IsOk = falseblnr;

	cursormask = XCreatePixmap(x_display, rootwin, 1, 1, 1);
	if (None == cursormask) {
		WriteExtraErr("XCreatePixmap failed.");
	} else {
		xgc.function = GXclear;
		gc = XCreateGC(x_display, cursormask, GCFunction, &xgc);
		if (None == gc) {
			WriteExtraErr("XCreateGC failed.");
		} else {
			XFillRectangle(x_display, cursormask, gc, 0, 0, 1, 1);
			XFreeGC(x_display, gc);

			blankCursor = XCreatePixmapCursor(x_display, cursormask,
							cursormask, &x_black, &x_white, 0, 0);
			if (None == blankCursor) {
				WriteExtraErr("XCreatePixmapCursor failed.");
			} else {
				IsOk = trueblnr;
			}
		}

		XFreePixmap(x_display, cursormask);
		/*
			assuming that XCreatePixmapCursor doesn't think it
			owns the pixmaps passed to it. I've seen code that
			assumes this, and other code that seems to assume
			the opposite.
		*/
	}
	return IsOk;
}

/* cursor moving */

#if EnableMoveMouse
LOCALFUNC blnr MyMoveMouse(si4b h, si4b v)
{
	int NewMousePosh;
	int NewMousePosv;
	int root_x_return;
	int root_y_return;
	Window root_return;
	Window child_return;
	unsigned int mask_return;
	blnr IsOk;
	int attempts = 0;

#if VarFullScreen
	if (UseFullScreen)
#endif
#if MayFullScreen
	{
		h -= ViewHStart;
		v -= ViewVStart;
	}
#endif

#if EnableMagnify
	if (UseMagnify) {
		h *= MyWindowScale;
		v *= MyWindowScale;
	}
#endif

#if VarFullScreen
	if (UseFullScreen)
#endif
#if MayFullScreen
	{
		h += hOffset;
		v += vOffset;
	}
#endif

	do {
		XWarpPointer(x_display, None, my_main_wind, 0, 0, 0, 0, h, v);
		XQueryPointer(x_display, my_main_wind,
			&root_return, &child_return,
			&root_x_return, &root_y_return,
			&NewMousePosh, &NewMousePosv,
			&mask_return);
		IsOk = (h == NewMousePosh) && (v == NewMousePosv);
		++attempts;
	} while ((! IsOk) && (attempts < 10));
	return IsOk;
}
#endif

#if EnableFSMouseMotion
LOCALPROC StartSaveMouseMotion(void)
{
	if (! HaveMouseMotion) {
		if (MyMoveMouse(ViewHStart + (ViewHSize / 2),
			ViewVStart + (ViewVSize / 2)))
		{
			SavedMouseH = ViewHStart + (ViewHSize / 2);
			SavedMouseV = ViewVStart + (ViewVSize / 2);
			HaveMouseMotion = trueblnr;
		}
	}
}
#endif

#if EnableFSMouseMotion
LOCALPROC StopSaveMouseMotion(void)
{
	if (HaveMouseMotion) {
		(void) MyMoveMouse(CurMouseH, CurMouseV);
		HaveMouseMotion = falseblnr;
	}
}
#endif

/* cursor state */

#if EnableFSMouseMotion
LOCALPROC MyMouseConstrain(void)
{
	si4b shiftdh;
	si4b shiftdv;

	if (SavedMouseH < ViewHStart + (ViewHSize / 4)) {
		shiftdh = ViewHSize / 2;
	} else if (SavedMouseH > ViewHStart + ViewHSize - (ViewHSize / 4)) {
		shiftdh = - ViewHSize / 2;
	} else {
		shiftdh = 0;
	}
	if (SavedMouseV < ViewVStart + (ViewVSize / 4)) {
		shiftdv = ViewVSize / 2;
	} else if (SavedMouseV > ViewVStart + ViewVSize - (ViewVSize / 4)) {
		shiftdv = - ViewVSize / 2;
	} else {
		shiftdv = 0;
	}
	if ((shiftdh != 0) || (shiftdv != 0)) {
		SavedMouseH += shiftdh;
		SavedMouseV += shiftdv;
		if (! MyMoveMouse(SavedMouseH, SavedMouseV)) {
			HaveMouseMotion = falseblnr;
		}
	}
}
#endif

LOCALPROC MousePositionNotify(int NewMousePosh, int NewMousePosv)
{
	blnr ShouldHaveCursorHidden = trueblnr;

#if VarFullScreen
	if (UseFullScreen)
#endif
#if MayFullScreen
	{
		NewMousePosh -= hOffset;
		NewMousePosv -= vOffset;
	}
#endif

#if EnableMagnify
	if (UseMagnify) {
		NewMousePosh /= MyWindowScale;
		NewMousePosv /= MyWindowScale;
	}
#endif

#if VarFullScreen
	if (UseFullScreen)
#endif
#if MayFullScreen
	{
		NewMousePosh += ViewHStart;
		NewMousePosv += ViewVStart;
	}
#endif

#if EnableFSMouseMotion
	if (HaveMouseMotion) {
		MyMousePositionSetDelta(NewMousePosh - SavedMouseH,
			NewMousePosv - SavedMouseV);
		SavedMouseH = NewMousePosh;
		SavedMouseV = NewMousePosv;
	} else
#endif
	{
		if (NewMousePosh < 0) {
			NewMousePosh = 0;
			ShouldHaveCursorHidden = falseblnr;
		} else if (NewMousePosh >= vMacScreenWidth) {
			NewMousePosh = vMacScreenWidth - 1;
			ShouldHaveCursorHidden = falseblnr;
		}
		if (NewMousePosv < 0) {
			NewMousePosv = 0;
			ShouldHaveCursorHidden = falseblnr;
		} else if (NewMousePosv >= vMacScreenHeight) {
			NewMousePosv = vMacScreenHeight - 1;
			ShouldHaveCursorHidden = falseblnr;
		}

#if VarFullScreen
		if (UseFullScreen)
#endif
#if MayFullScreen
		{
			ShouldHaveCursorHidden = trueblnr;
		}
#endif

		/* if (ShouldHaveCursorHidden || CurMouseButton) */
		/*
			for a game like arkanoid, would like mouse to still
			move even when outside window in one direction
		*/
		MyMousePositionSet(NewMousePosh, NewMousePosv);
	}

	WantCursorHidden = ShouldHaveCursorHidden;
}

LOCALPROC CheckMouseState(void)
{
	int NewMousePosh;
	int NewMousePosv;
	int root_x_return;
	int root_y_return;
	Window root_return;
	Window child_return;
	unsigned int mask_return;

	XQueryPointer(x_display, my_main_wind,
		&root_return, &child_return,
		&root_x_return, &root_y_return,
		&NewMousePosh, &NewMousePosv,
		&mask_return);
	MousePositionNotify(NewMousePosh, NewMousePosv);
}

/* --- keyboard input --- */

/*
	translation table - X11 KeySym -> Mac key code

	Used to create KC2MKC table (X11 key code -> Mac key code)

	Includes effect of any key mapping set with the
	mini vmac '-km' compile time option.

	The real CapsLock key needs special treatment,
	so use MKC_real_CapsLock here,
	which is later remapped to MKC_formac_CapsLock.

	Ordered to match order of keycodes on Linux, the most
	common port using this X11 code, making the code using
	this table more efficient.
*/

/*
	The actual data is in the comments of this enum,
	from which MT2KeySym and MT2MKC are created by script.
*/
enum {
	kMT_Escape, /* XK_Escape MKC_formac_Escape */
	kMT_1, /* XK_1 MKC_1 */
	kMT_2, /* XK_2 MKC_2 */
	kMT_3, /* XK_3 MKC_3 */
	kMT_4, /* XK_4 MKC_4 */
	kMT_5, /* XK_5 MKC_5 */
	kMT_6, /* XK_6 MKC_6 */
	kMT_7, /* XK_7 MKC_7 */
	kMT_8, /* XK_8 MKC_8 */
	kMT_9, /* XK_9 MKC_9 */
	kMT_0, /* XK_0 MKC_0 */
	kMT_minus, /* XK_minus MKC_Minus */
	kMT_underscore, /* XK_underscore MKC_Minus */
	kMT_equal, /* XK_equal MKC_Equal */
	kMT_plus, /* XK_plus MKC_Equal */
	kMT_BackSpace, /* XK_BackSpace MKC_BackSpace */
	kMT_Tab, /* XK_Tab MKC_Tab */
	kMT_q, /* XK_q MKC_Q */
	kMT_Q, /* XK_Q MKC_Q */
	kMT_w, /* XK_w MKC_W */
	kMT_W, /* XK_W MKC_W */
	kMT_e, /* XK_e MKC_E */
	kMT_E, /* XK_E MKC_E */
	kMT_r, /* XK_r MKC_R */
	kMT_R, /* XK_R MKC_R */
	kMT_t, /* XK_t MKC_T */
	kMT_T, /* XK_T MKC_T */
	kMT_y, /* XK_y MKC_Y */
	kMT_Y, /* XK_Y MKC_Y */
	kMT_u, /* XK_u MKC_U */
	kMT_U, /* XK_U MKC_U */
	kMT_i, /* XK_i MKC_I */
	kMT_I, /* XK_I MKC_I */
	kMT_o, /* XK_o MKC_O */
	kMT_O, /* XK_O MKC_O */
	kMT_p, /* XK_p MKC_P */
	kMT_P, /* XK_P MKC_P */
	kMT_bracketleft, /* XK_bracketleft MKC_LeftBracket */
	kMT_braceleft, /* XK_braceleft MKC_LeftBracket */
	kMT_bracketright, /* XK_bracketright MKC_RightBracket */
	kMT_braceright, /* XK_braceright MKC_RightBracket */
	kMT_Return, /* XK_Return MKC_Return */
	kMT_Control_L, /* XK_Control_L MKC_formac_Control */
	kMT_a, /* XK_a MKC_A */
	kMT_A, /* XK_A MKC_A */
	kMT_s, /* XK_s MKC_S */
	kMT_S, /* XK_S MKC_S */
	kMT_d, /* XK_d MKC_D */
	kMT_D, /* XK_D MKC_D */
	kMT_f, /* XK_f MKC_F */
	kMT_F, /* XK_F MKC_F */
	kMT_g, /* XK_g MKC_G */
	kMT_G, /* XK_G MKC_G */
	kMT_h, /* XK_h MKC_H */
	kMT_H, /* XK_H MKC_H */
	kMT_j, /* XK_j MKC_J */
	kMT_J, /* XK_J MKC_J */
	kMT_k, /* XK_k MKC_K */
	kMT_K, /* XK_K MKC_K */
	kMT_l, /* XK_l MKC_L */
	kMT_L, /* XK_L MKC_L */
	kMT_semicolon, /* XK_semicolon MKC_SemiColon */
	kMT_colon, /* XK_colon MKC_SemiColon */
	kMT_apostrophe, /* XK_apostrophe MKC_SingleQuote */
	kMT_quotedbl, /* XK_quotedbl MKC_SingleQuote */
	kMT_grave, /* XK_grave MKC_formac_Grave */
	kMT_asciitilde, /* XK_asciitilde MKC_formac_Grave */
	kMT_Shift_L, /* XK_Shift_L MKC_formac_Shift */
	kMT_backslash, /* XK_backslash MKC_formac_BackSlash */
	kMT_bar, /* XK_bar MKC_formac_BackSlash */
	kMT_z, /* XK_z MKC_Z */
	kMT_Z, /* XK_Z MKC_Z */
	kMT_x, /* XK_x MKC_X */
	kMT_X, /* XK_X MKC_X */
	kMT_c, /* XK_c MKC_C */
	kMT_C, /* XK_C MKC_C */
	kMT_v, /* XK_v MKC_V */
	kMT_V, /* XK_V MKC_V */
	kMT_b, /* XK_b MKC_B */
	kMT_B, /* XK_B MKC_B */
	kMT_n, /* XK_n MKC_N */
	kMT_N, /* XK_N MKC_N */
	kMT_m, /* XK_m MKC_M */
	kMT_M, /* XK_M MKC_M */
	kMT_comma, /* XK_comma MKC_Comma */
	kMT_period, /* XK_period MKC_Period */
	kMT_greater, /* XK_greater MKC_Period */
	kMT_slash, /* XK_slash MKC_formac_Slash */
	kMT_question, /* XK_question MKC_formac_Slash */
	kMT_Shift_R, /* XK_Shift_R MKC_formac_RShift */
	kMT_KP_Multiply, /* XK_KP_Multiply MKC_KPMultiply */
	kMT_Alt_L, /* XK_Alt_L MKC_formac_Command */
	kMT_space, /* XK_space MKC_Space */
	kMT_Caps_Lock, /* XK_Caps_Lock MKC_real_CapsLock */
	kMT_F1, /* XK_F1 MKC_formac_F1 */
	kMT_F2, /* XK_F2 MKC_formac_F2 */
	kMT_F3, /* XK_F3 MKC_formac_F3 */
	kMT_F4, /* XK_F4 MKC_formac_F4 */
	kMT_F5, /* XK_F5 MKC_formac_F5 */
	kMT_F6, /* XK_F6 MKC_F6 */
	kMT_F7, /* XK_F7 MKC_F7 */
	kMT_F8, /* XK_F8 MKC_F8 */
	kMT_F9, /* XK_F9 MKC_F9 */
	kMT_F10, /* XK_F10 MKC_F10 */
	kMT_Num_Lock, /* XK_Num_Lock MKC_Clear */
#ifdef XK_Scroll_Lock
	kMT_Scroll_Lock, /* XK_Scroll_Lock MKC_ScrollLock */
#endif
#ifdef XK_F14
	kMT_F14, /* XK_F14 MKC_ScrollLock */
#endif

	kMT_KP_7, /* XK_KP_7 MKC_KP7 */
#ifdef XK_KP_Home
	kMT_KP_Home, /* XK_KP_Home MKC_KP7 */
#endif

	kMT_KP_8, /* XK_KP_8 MKC_KP8 */
#ifdef XK_KP_Up
	kMT_KP_Up, /* XK_KP_Up MKC_KP8 */
#endif

	kMT_KP_9, /* XK_KP_9 MKC_KP9 */
#ifdef XK_KP_Page_Up
	kMT_KP_Page_Up, /* XK_KP_Page_Up MKC_KP9 */
#else
#ifdef XK_KP_Prior
	kMT_KP_Prior, /* XK_KP_Prior MKC_KP9 */
#endif
#endif

	kMT_KP_Subtract, /* XK_KP_Subtract MKC_KPSubtract */

	kMT_KP_4, /* XK_KP_4 MKC_KP4 */
#ifdef XK_KP_Left
	kMT_KP_Left, /* XK_KP_Left MKC_KP4 */
#endif

	kMT_KP_5, /* XK_KP_5 MKC_KP5 */
#ifdef XK_KP_Begin
	kMT_KP_Begin, /* XK_KP_Begin MKC_KP5 */
#endif

	kMT_KP_6, /* XK_KP_6 MKC_KP6 */
#ifdef XK_KP_Right
	kMT_KP_Right, /* XK_KP_Right MKC_KP6 */
#endif

	kMT_KP_Add, /* XK_KP_Add MKC_KPAdd */

	kMT_KP_1, /* XK_KP_1 MKC_KP1 */
#ifdef XK_KP_End
	kMT_KP_End, /* XK_KP_End MKC_KP1 */
#endif

	kMT_KP_2, /* XK_KP_2 MKC_KP2 */
#ifdef XK_KP_Down
	kMT_KP_Down, /* XK_KP_Down MKC_KP2 */
#endif

	kMT_KP_3, /* XK_KP_3 MKC_KP3 */
#ifdef XK_Page_Down
	kMT_KP_Page_Down, /* XK_KP_Page_Down MKC_KP3 */
#else
#ifdef XK_KP_Next
	kMT_KP_Next, /* XK_KP_Next MKC_KP3 */
#endif
#endif

	kMT_KP_0, /* XK_KP_0 MKC_KP0 */
#ifdef XK_KP_Insert
	kMT_KP_Insert, /* XK_KP_Insert MKC_KP0 */
#endif
#ifdef XK_KP_Delete
	kMT_KP_Delete, /* XK_KP_Delete MKC_Decimal */
#endif

	/* XK_ISO_Level3_Shift */
	/* nothing */
#ifdef XK_less
	kMT_less, /* XK_less MKC_Comma */
#endif
	kMT_F11, /* XK_F11 MKC_F11 */
	kMT_F12, /* XK_F12 MKC_F12 */
	/* nothing */
	/* XK_Katakana */
	/* XK_Hiragana */
	/* XK_Henkan */
	/* XK_Hiragana_Katakana */
	/* XK_Muhenkan */
	/* nothing */
	kMT_KP_Enter, /* XK_KP_Enter MKC_formac_Enter */
	kMT_Control_R, /* XK_Control_R MKC_formac_RControl */
	kMT_KP_Divide, /* XK_KP_Divide MKC_KPDevide */
#ifdef XK_Print
	kMT_Print, /* XK_Print MKC_Print */
#endif
	kMT_Alt_R, /* XK_Alt_R MKC_formac_RCommand */
	/* XK_Linefeed */
#ifdef XK_Home
	kMT_Home, /* XK_Home MKC_formac_Home */
#endif
	kMT_Up, /* XK_Up MKC_Up */

#ifdef XK_Page_Up
	kMT_Page_Up, /* XK_Page_Up MKC_formac_PageUp */
#else
#ifdef XK_Prior
	kMT_Prior, /* XK_Prior MKC_formac_PageUp */
#endif
#endif

	kMT_Left, /* XK_Left MKC_Left */
	kMT_Right, /* XK_Right MKC_Right */
#ifdef XK_End
	kMT_End, /* XK_End MKC_formac_End */
#endif
	kMT_Down, /* XK_Down MKC_Down */

#ifdef XK_Page_Down
	kMT_Page_Down, /* XK_Page_Down MKC_formac_PageDown */
#else
#ifdef XK_Next
	kMT_Next, /* XK_Next MKC_formac_PageDown */
#endif
#endif

#ifdef XK_Insert
	kMT_Insert, /* XK_Insert MKC_formac_Help */
#endif
#ifdef XK_Delete
	kMT_Delete, /* XK_Delete MKC_formac_ForwardDel */
#endif
	/* nothing */
	/* ? */
	/* ? */
	/* ? */
	/* ? */
	kMT_KP_Equal, /* XK_KP_Equal MKC_KPEqual */
	/* XK_plusminus */
#ifdef XK_Pause
	kMT_Pause, /* XK_Pause MKC_Pause */
#endif
#ifdef XK_F15
	kMT_F15, /* XK_F15 MKC_Pause */
#endif
	/* ? */
	kMT_KP_Decimal, /* XK_KP_Decimal MKC_Decimal */
	/* XK_Hangul */
	/* XK_Hangul_Hanja */
	/* nothing */
	kMT_Super_L, /* XK_Super_L MKC_formac_Option */
	kMT_Super_R, /* XK_Super_R MKC_formac_ROption */
	kMT_Menu, /* XK_Menu MKC_formac_Option */
	/* XK_Cancel */
	/* XK_Redo */
	/* ? */
	/* XK_Undo */
	/* ? */
	/* ? */
	/* ? */
	/* ? */
	/* XK_Find */
	/* ? */
#ifdef XK_Help
	kMT_Help, /* XK_Help MKC_formac_Help */
#endif
	/* ? */
	/* ? */
	/* nothing */
	/* ? */
	/* ? */
	/* ? */
	/* ? */
	/* nothing */

	/* XK_parenleft */
	/* XK_parenright */

	/* XK_Mode_switch */



	kMT_Meta_L, /* XK_Meta_L MKC_formac_Command */
	kMT_Meta_R, /* XK_Meta_R MKC_formac_RCommand */

	kMT_Mode_switch, /* XK_Mode_switch MKC_formac_Option */
	kMT_Hyper_L, /* XK_Hyper_L MKC_formac_Option */
	kMT_Hyper_R, /* XK_Hyper_R MKC_formac_ROption */

	kMT_F13, /* XK_F13 MKC_formac_Option */
		/*
			seen being used in Mandrake Linux 9.2
			for windows key
		*/

	kNumMTs
};

/*
	MT2KeySym was generated by a script from
	enum and comments above.
*/
LOCALVAR const KeySym MT2KeySym[kNumMTs + 1] = {
	XK_Escape, /* kMT_Escape */
	XK_1, /* kMT_1 */
	XK_2, /* kMT_2 */
	XK_3, /* kMT_3 */
	XK_4, /* kMT_4 */
	XK_5, /* kMT_5 */
	XK_6, /* kMT_6 */
	XK_7, /* kMT_7 */
	XK_8, /* kMT_8 */
	XK_9, /* kMT_9 */
	XK_0, /* kMT_0 */
	XK_minus, /* kMT_minus */
	XK_underscore, /* kMT_underscore */
	XK_equal, /* kMT_equal */
	XK_plus, /* kMT_plus */
	XK_BackSpace, /* kMT_BackSpace */
	XK_Tab, /* kMT_Tab */
	XK_q, /* kMT_q */
	XK_Q, /* kMT_Q */
	XK_w, /* kMT_w */
	XK_W, /* kMT_W */
	XK_e, /* kMT_e */
	XK_E, /* kMT_E */
	XK_r, /* kMT_r */
	XK_R, /* kMT_R */
	XK_t, /* kMT_t */
	XK_T, /* kMT_T */
	XK_y, /* kMT_y */
	XK_Y, /* kMT_Y */
	XK_u, /* kMT_u */
	XK_U, /* kMT_U */
	XK_i, /* kMT_i */
	XK_I, /* kMT_I */
	XK_o, /* kMT_o */
	XK_O, /* kMT_O */
	XK_p, /* kMT_p */
	XK_P, /* kMT_P */
	XK_bracketleft, /* kMT_bracketleft */
	XK_braceleft, /* kMT_braceleft */
	XK_bracketright, /* kMT_bracketright */
	XK_braceright, /* kMT_braceright */
	XK_Return, /* kMT_Return */
	XK_Control_L, /* kMT_Control_L */
	XK_a, /* kMT_a */
	XK_A, /* kMT_A */
	XK_s, /* kMT_s */
	XK_S, /* kMT_S */
	XK_d, /* kMT_d */
	XK_D, /* kMT_D */
	XK_f, /* kMT_f */
	XK_F, /* kMT_F */
	XK_g, /* kMT_g */
	XK_G, /* kMT_G */
	XK_h, /* kMT_h */
	XK_H, /* kMT_H */
	XK_j, /* kMT_j */
	XK_J, /* kMT_J */
	XK_k, /* kMT_k */
	XK_K, /* kMT_K */
	XK_l, /* kMT_l */
	XK_L, /* kMT_L */
	XK_semicolon, /* kMT_semicolon */
	XK_colon, /* kMT_colon */
	XK_apostrophe, /* kMT_apostrophe */
	XK_quotedbl, /* kMT_quotedbl */
	XK_grave, /* kMT_grave */
	XK_asciitilde, /* kMT_asciitilde */
	XK_Shift_L, /* kMT_Shift_L */
	XK_backslash, /* kMT_backslash */
	XK_bar, /* kMT_bar */
	XK_z, /* kMT_z */
	XK_Z, /* kMT_Z */
	XK_x, /* kMT_x */
	XK_X, /* kMT_X */
	XK_c, /* kMT_c */
	XK_C, /* kMT_C */
	XK_v, /* kMT_v */
	XK_V, /* kMT_V */
	XK_b, /* kMT_b */
	XK_B, /* kMT_B */
	XK_n, /* kMT_n */
	XK_N, /* kMT_N */
	XK_m, /* kMT_m */
	XK_M, /* kMT_M */
	XK_comma, /* kMT_comma */
	XK_period, /* kMT_period */
	XK_greater, /* kMT_greater */
	XK_slash, /* kMT_slash */
	XK_question, /* kMT_question */
	XK_Shift_R, /* kMT_Shift_R */
	XK_KP_Multiply, /* kMT_KP_Multiply */
	XK_Alt_L, /* kMT_Alt_L */
	XK_space, /* kMT_space */
	XK_Caps_Lock, /* kMT_Caps_Lock */
	XK_F1, /* kMT_F1 */
	XK_F2, /* kMT_F2 */
	XK_F3, /* kMT_F3 */
	XK_F4, /* kMT_F4 */
	XK_F5, /* kMT_F5 */
	XK_F6, /* kMT_F6 */
	XK_F7, /* kMT_F7 */
	XK_F8, /* kMT_F8 */
	XK_F9, /* kMT_F9 */
	XK_F10, /* kMT_F10 */
	XK_Num_Lock, /* kMT_Num_Lock */
#ifdef XK_Scroll_Lock
	XK_Scroll_Lock, /* kMT_Scroll_Lock */
#endif
#ifdef XK_F14
	XK_F14, /* kMT_F14 */
#endif

	XK_KP_7, /* kMT_KP_7 */
#ifdef XK_KP_Home
	XK_KP_Home, /* kMT_KP_Home */
#endif

	XK_KP_8, /* kMT_KP_8 */
#ifdef XK_KP_Up
	XK_KP_Up, /* kMT_KP_Up */
#endif

	XK_KP_9, /* kMT_KP_9 */
#ifdef XK_KP_Page_Up
	XK_KP_Page_Up, /* kMT_KP_Page_Up */
#else
#ifdef XK_KP_Prior
	XK_KP_Prior, /* kMT_KP_Prior */
#endif
#endif

	XK_KP_Subtract, /* kMT_KP_Subtract */

	XK_KP_4, /* kMT_KP_4 */
#ifdef XK_KP_Left
	XK_KP_Left, /* kMT_KP_Left */
#endif

	XK_KP_5, /* kMT_KP_5 */
#ifdef XK_KP_Begin
	XK_KP_Begin, /* kMT_KP_Begin */
#endif

	XK_KP_6, /* kMT_KP_6 */
#ifdef XK_KP_Right
	XK_KP_Right, /* kMT_KP_Right */
#endif

	XK_KP_Add, /* kMT_KP_Add */

	XK_KP_1, /* kMT_KP_1 */
#ifdef XK_KP_End
	XK_KP_End, /* kMT_KP_End */
#endif

	XK_KP_2, /* kMT_KP_2 */
#ifdef XK_KP_Down
	XK_KP_Down, /* kMT_KP_Down */
#endif

	XK_KP_3, /* kMT_KP_3 */
#ifdef XK_Page_Down
	XK_KP_Page_Down, /* kMT_KP_Page_Down */
#else
#ifdef XK_KP_Next
	XK_KP_Next, /* kMT_KP_Next */
#endif
#endif

	XK_KP_0, /* kMT_KP_0 */
#ifdef XK_KP_Insert
	XK_KP_Insert, /* kMT_KP_Insert */
#endif
#ifdef XK_KP_Delete
	XK_KP_Delete, /* kMT_KP_Delete */
#endif

	/* XK_ISO_Level3_Shift */
	/* nothing */
#ifdef XK_less
	XK_less, /* kMT_less */
#endif
	XK_F11, /* kMT_F11 */
	XK_F12, /* kMT_F12 */
	/* nothing */
	/* XK_Katakana */
	/* XK_Hiragana */
	/* XK_Henkan */
	/* XK_Hiragana_Katakana */
	/* XK_Muhenkan */
	/* nothing */
	XK_KP_Enter, /* kMT_KP_Enter */
	XK_Control_R, /* kMT_Control_R */
	XK_KP_Divide, /* kMT_KP_Divide */
#ifdef XK_Print
	XK_Print, /* kMT_Print */
#endif
	XK_Alt_R, /* kMT_Alt_R */
	/* XK_Linefeed */
#ifdef XK_Home
	XK_Home, /* kMT_Home */
#endif
	XK_Up, /* kMT_Up */

#ifdef XK_Page_Up
	XK_Page_Up, /* kMT_Page_Up */
#else
#ifdef XK_Prior
	XK_Prior, /* kMT_Prior */
#endif
#endif

	XK_Left, /* kMT_Left */
	XK_Right, /* kMT_Right */
#ifdef XK_End
	XK_End, /* kMT_End */
#endif
	XK_Down, /* kMT_Down */

#ifdef XK_Page_Down
	XK_Page_Down, /* kMT_Page_Down */
#else
#ifdef XK_Next
	XK_Next, /* kMT_Next */
#endif
#endif

#ifdef XK_Insert
	XK_Insert, /* kMT_Insert */
#endif
#ifdef XK_Delete
	XK_Delete, /* kMT_Delete */
#endif
	/* nothing */
	/* ? */
	/* ? */
	/* ? */
	/* ? */
	XK_KP_Equal, /* kMT_KP_Equal */
	/* XK_plusminus */
#ifdef XK_Pause
	XK_Pause, /* kMT_Pause */
#endif
#ifdef XK_F15
	XK_F15, /* kMT_F15 */
#endif
	/* ? */
	XK_KP_Decimal, /* kMT_KP_Decimal */
	/* XK_Hangul */
	/* XK_Hangul_Hanja */
	/* nothing */
	XK_Super_L, /* kMT_Super_L */
	XK_Super_R, /* kMT_Super_R */
	XK_Menu, /* kMT_Menu */
	/* XK_Cancel */
	/* XK_Redo */
	/* ? */
	/* XK_Undo */
	/* ? */
	/* ? */
	/* ? */
	/* ? */
	/* XK_Find */
	/* ? */
#ifdef XK_Help
	XK_Help, /* kMT_Help */
#endif
	/* ? */
	/* ? */
	/* nothing */
	/* ? */
	/* ? */
	/* ? */
	/* ? */
	/* nothing */

	/* XK_parenleft */
	/* XK_parenright */

	/* XK_Mode_switch */



	XK_Meta_L, /* kMT_Meta_L */
	XK_Meta_R, /* kMT_Meta_R */

	XK_Mode_switch, /* kMT_Mode_switch */
	XK_Hyper_L, /* kMT_Hyper_L */
	XK_Hyper_R, /* kMT_Hyper_R */

	XK_F13, /* kMT_F13 */
		/*
			seen being used in Mandrake Linux 9.2
			for windows key
		*/

	0 /* just so last above line can end in ',' */
};

/*
	MT2MKC was generated by a script from
	enum and comments above.
*/
LOCALVAR const ui3r MT2MKC[kNumMTs + 1] = {
	MKC_formac_Escape, /* kMT_Escape */
	MKC_1, /* kMT_1 */
	MKC_2, /* kMT_2 */
	MKC_3, /* kMT_3 */
	MKC_4, /* kMT_4 */
	MKC_5, /* kMT_5 */
	MKC_6, /* kMT_6 */
	MKC_7, /* kMT_7 */
	MKC_8, /* kMT_8 */
	MKC_9, /* kMT_9 */
	MKC_0, /* kMT_0 */
	MKC_Minus, /* kMT_minus */
	MKC_Minus, /* kMT_underscore */
	MKC_Equal, /* kMT_equal */
	MKC_Equal, /* kMT_plus */
	MKC_BackSpace, /* kMT_BackSpace */
	MKC_Tab, /* kMT_Tab */
	MKC_Q, /* kMT_q */
	MKC_Q, /* kMT_Q */
	MKC_W, /* kMT_w */
	MKC_W, /* kMT_W */
	MKC_E, /* kMT_e */
	MKC_E, /* kMT_E */
	MKC_R, /* kMT_r */
	MKC_R, /* kMT_R */
	MKC_T, /* kMT_t */
	MKC_T, /* kMT_T */
	MKC_Y, /* kMT_y */
	MKC_Y, /* kMT_Y */
	MKC_U, /* kMT_u */
	MKC_U, /* kMT_U */
	MKC_I, /* kMT_i */
	MKC_I, /* kMT_I */
	MKC_O, /* kMT_o */
	MKC_O, /* kMT_O */
	MKC_P, /* kMT_p */
	MKC_P, /* kMT_P */
	MKC_LeftBracket, /* kMT_bracketleft */
	MKC_LeftBracket, /* kMT_braceleft */
	MKC_RightBracket, /* kMT_bracketright */
	MKC_RightBracket, /* kMT_braceright */
	MKC_Return, /* kMT_Return */
	MKC_formac_Control, /* kMT_Control_L */
	MKC_A, /* kMT_a */
	MKC_A, /* kMT_A */
	MKC_S, /* kMT_s */
	MKC_S, /* kMT_S */
	MKC_D, /* kMT_d */
	MKC_D, /* kMT_D */
	MKC_F, /* kMT_f */
	MKC_F, /* kMT_F */
	MKC_G, /* kMT_g */
	MKC_G, /* kMT_G */
	MKC_H, /* kMT_h */
	MKC_H, /* kMT_H */
	MKC_J, /* kMT_j */
	MKC_J, /* kMT_J */
	MKC_K, /* kMT_k */
	MKC_K, /* kMT_K */
	MKC_L, /* kMT_l */
	MKC_L, /* kMT_L */
	MKC_SemiColon, /* kMT_semicolon */
	MKC_SemiColon, /* kMT_colon */
	MKC_SingleQuote, /* kMT_apostrophe */
	MKC_SingleQuote, /* kMT_quotedbl */
	MKC_formac_Grave, /* kMT_grave */
	MKC_formac_Grave, /* kMT_asciitilde */
	MKC_formac_Shift, /* kMT_Shift_L */
	MKC_formac_BackSlash, /* kMT_backslash */
	MKC_formac_BackSlash, /* kMT_bar */
	MKC_Z, /* kMT_z */
	MKC_Z, /* kMT_Z */
	MKC_X, /* kMT_x */
	MKC_X, /* kMT_X */
	MKC_C, /* kMT_c */
	MKC_C, /* kMT_C */
	MKC_V, /* kMT_v */
	MKC_V, /* kMT_V */
	MKC_B, /* kMT_b */
	MKC_B, /* kMT_B */
	MKC_N, /* kMT_n */
	MKC_N, /* kMT_N */
	MKC_M, /* kMT_m */
	MKC_M, /* kMT_M */
	MKC_Comma, /* kMT_comma */
	MKC_Period, /* kMT_period */
	MKC_Period, /* kMT_greater */
	MKC_formac_Slash, /* kMT_slash */
	MKC_formac_Slash, /* kMT_question */
	MKC_formac_RShift, /* kMT_Shift_R */
	MKC_KPMultiply, /* kMT_KP_Multiply */
	MKC_formac_Command, /* kMT_Alt_L */
	MKC_Space, /* kMT_space */
	MKC_real_CapsLock, /* kMT_Caps_Lock */
	MKC_formac_F1, /* kMT_F1 */
	MKC_formac_F2, /* kMT_F2 */
	MKC_formac_F3, /* kMT_F3 */
	MKC_formac_F4, /* kMT_F4 */
	MKC_formac_F5, /* kMT_F5 */
	MKC_F6, /* kMT_F6 */
	MKC_F7, /* kMT_F7 */
	MKC_F8, /* kMT_F8 */
	MKC_F9, /* kMT_F9 */
	MKC_F10, /* kMT_F10 */
	MKC_Clear, /* kMT_Num_Lock */
#ifdef XK_Scroll_Lock
	MKC_ScrollLock, /* kMT_Scroll_Lock */
#endif
#ifdef XK_F14
	MKC_ScrollLock, /* kMT_F14  */
#endif

	MKC_KP7, /* kMT_KP_7 */
#ifdef XK_KP_Home
	MKC_KP7, /* kMT_KP_Home */
#endif

	MKC_KP8, /* kMT_KP_8 */
#ifdef XK_KP_Up
	MKC_KP8, /* kMT_KP_Up */
#endif

	MKC_KP9, /* kMT_KP_9 */
#ifdef XK_KP_Page_Up
	MKC_KP9, /* kMT_KP_Page_Up */
#else
#ifdef XK_KP_Prior
	MKC_KP9, /* kMT_KP_Prior */
#endif
#endif

	MKC_KPSubtract, /* kMT_KP_Subtract */

	MKC_KP4, /* kMT_KP_4 */
#ifdef XK_KP_Left
	MKC_KP4, /* kMT_KP_Left */
#endif

	MKC_KP5, /* kMT_KP_5 */
#ifdef XK_KP_Begin
	MKC_KP5, /* kMT_KP_Begin */
#endif

	MKC_KP6, /* kMT_KP_6 */
#ifdef XK_KP_Right
	MKC_KP6, /* kMT_KP_Right */
#endif

	MKC_KPAdd, /* kMT_KP_Add */

	MKC_KP1, /* kMT_KP_1 */
#ifdef XK_KP_End
	MKC_KP1, /* kMT_KP_End */
#endif

	MKC_KP2, /* kMT_KP_2 */
#ifdef XK_KP_Down
	MKC_KP2, /* kMT_KP_Down */
#endif

	MKC_KP3, /* kMT_KP_3 */
#ifdef XK_Page_Down
	MKC_KP3, /* kMT_KP_Page_Down */
#else
#ifdef XK_KP_Next
	MKC_KP3, /* kMT_KP_Next */
#endif
#endif

	MKC_KP0, /* kMT_KP_0 */
#ifdef XK_KP_Insert
	MKC_KP0, /* kMT_KP_Insert */
#endif
#ifdef XK_KP_Delete
	MKC_Decimal, /* kMT_KP_Delete */
#endif

	/* XK_ISO_Level3_Shift */
	/* nothing */
#ifdef XK_less
	MKC_Comma, /* kMT_less */
#endif
	MKC_F11, /* kMT_F11 */
	MKC_F12, /* kMT_F12 */
	/* nothing */
	/* XK_Katakana */
	/* XK_Hiragana */
	/* XK_Henkan */
	/* XK_Hiragana_Katakana */
	/* XK_Muhenkan */
	/* nothing */
	MKC_formac_Enter, /* kMT_KP_Enter */
	MKC_formac_RControl, /* kMT_Control_R */
	MKC_KPDevide, /* kMT_KP_Divide */
#ifdef XK_Print
	MKC_Print, /* kMT_Print */
#endif
	MKC_formac_RCommand, /* kMT_Alt_R */
	/* XK_Linefeed */
#ifdef XK_Home
	MKC_formac_Home, /* kMT_Home */
#endif
	MKC_Up, /* kMT_Up */

#ifdef XK_Page_Up
	MKC_formac_PageUp, /* kMT_Page_Up */
#else
#ifdef XK_Prior
	MKC_formac_PageUp, /* kMT_Prior */
#endif
#endif

	MKC_Left, /* kMT_Left */
	MKC_Right, /* kMT_Right */
#ifdef XK_End
	MKC_formac_End, /* kMT_End */
#endif
	MKC_Down, /* kMT_Down */

#ifdef XK_Page_Down
	MKC_formac_PageDown, /* kMT_Page_Down */
#else
#ifdef XK_Next
	MKC_formac_PageDown, /* kMT_Next */
#endif
#endif

#ifdef XK_Insert
	MKC_formac_Help, /* kMT_Insert */
#endif
#ifdef XK_Delete
	MKC_formac_ForwardDel, /* kMT_Delete */
#endif
	/* nothing */
	/* ? */
	/* ? */
	/* ? */
	/* ? */
	MKC_KPEqual, /* kMT_KP_Equal */
	/* XK_plusminus */
#ifdef XK_Pause
	MKC_Pause, /* kMT_Pause */
#endif
#ifdef XK_F15
	MKC_Pause, /* kMT_F15  */
#endif
	/* ? */
	MKC_Decimal, /* kMT_KP_Decimal */
	/* XK_Hangul */
	/* XK_Hangul_Hanja */
	/* nothing */
	MKC_formac_Option, /* kMT_Super_L */
	MKC_formac_ROption, /* kMT_Super_R */
	MKC_formac_Option, /* kMT_Menu */
	/* XK_Cancel */
	/* XK_Redo */
	/* ? */
	/* XK_Undo */
	/* ? */
	/* ? */
	/* ? */
	/* ? */
	/* XK_Find */
	/* ? */
#ifdef XK_Help
	MKC_formac_Help, /* kMT_Help */
#endif
	/* ? */
	/* ? */
	/* nothing */
	/* ? */
	/* ? */
	/* ? */
	/* ? */
	/* nothing */

	/* XK_parenleft */
	/* XK_parenright */

	/* XK_Mode_switch */



	MKC_formac_Command, /* kMT_Meta_L */
	MKC_formac_RCommand, /* kMT_Meta_R */

	MKC_formac_Option, /* kMT_Mode_switch */
	MKC_formac_Option, /* kMT_Hyper_L */
	MKC_formac_ROption, /* kMT_Hyper_R */

	MKC_formac_Option, /* kMT_F13 */
		/*
			seen being used in Mandrake Linux 9.2
			for windows key
		*/

	0 /* just so last above line can end in ',' */
};

LOCALVAR ui3b KC2MKC[256];
	/*
		translate X11 key code to Macintosh key code
	*/

#define KMInit_dolog (dbglog_HAVE && 0)

LOCALFUNC blnr KC2MKCInit(void)
{
	int i;
	int j;
	int last_j;
	int first_keycode;
	int last_keycode;
	int keysyms_per_keycode;
	KeySym *KeyMap;
	KeySym MaxUsedKeySym;

	/*
		In Linux, observe that most KeySyms not
		found in our translation table are large.
		So saves time to find largest KeySym we
		are interested in.
	*/
	MaxUsedKeySym = 0;
	for (j = 0; j < kNumMTs; j++) {
		KeySym x = MT2KeySym[j];
		if (x > MaxUsedKeySym) {
			MaxUsedKeySym = x;
		}
	}

#if KMInit_dolog
	dbglog_writelnHex("MaxUsedKeySym", MaxUsedKeySym);
#endif

	for (i = 0; i < 256; ++i) {
		KC2MKC[i] = MKC_None;
	}

	XDisplayKeycodes(x_display, &first_keycode, &last_keycode);
	KeyMap = XGetKeyboardMapping(x_display,
		first_keycode,
		last_keycode - first_keycode + 1,
		&keysyms_per_keycode);

	last_j = kNumMTs - 1;

	for (i = first_keycode; i <= last_keycode; i++) {
		KeySym ks = KeyMap[(i - first_keycode) * keysyms_per_keycode];

#if KMInit_dolog
		dbglog_writeNum(i);
		dbglog_writeSpace();
		dbglog_writeHex(ks);
		dbglog_writeSpace();
#endif
		if (0 == ks) {
#if KMInit_dolog
			dbglog_writeCStr("zero");
#endif
		} else
		if (ks > MaxUsedKeySym) {
#if KMInit_dolog
			dbglog_writeCStr("too large");
#endif
		} else
		{
			/*
				look up in the translation table, and try to be more
				efficient if the order of this table is similar
				to the order of the X11 KeyboardMapping.
			*/
			j = last_j;
label_retry:
			++j;
			if (j >= kNumMTs) {
				j = 0;
			}

			if (j == last_j) {
				/* back where we started */
#if KMInit_dolog
				dbglog_writeCStr("not found");
#endif
			} else
			if (ks != MT2KeySym[j]) {
#if KMInit_dolog && 1
				dbglog_writeCStr("*");
#endif
				goto label_retry; /* try the next one */
			} else
			{
#if KMInit_dolog
				dbglog_writeCStr("match");
				dbglog_writeSpace();
				dbglog_writeHex(MT2MKC[j]);
#endif
				KC2MKC[i] = MT2MKC[j];
				last_j = j;
			}
		}
#if KMInit_dolog
		dbglog_writeReturn();
#endif
	}

	XFree(KeyMap);

	InitKeyCodes();

	return trueblnr;
}

LOCALPROC CheckTheCapsLock(void)
{
	int NewMousePosh;
	int NewMousePosv;
	int root_x_return;
	int root_y_return;
	Window root_return;
	Window child_return;
	unsigned int mask_return;

	XQueryPointer(x_display, my_main_wind,
		&root_return, &child_return,
		&root_x_return, &root_y_return,
		&NewMousePosh, &NewMousePosv,
		&mask_return);

	Keyboard_UpdateKeyMap2(MKC_formac_CapsLock,
		(mask_return & LockMask) != 0);
}

LOCALPROC DoKeyCode0(int i, blnr down)
{
	ui3r key = KC2MKC[i];

	if (MKC_None == key) {
		/* ignore */
	} else
	if (MKC_real_CapsLock == key) {
		/* also ignore */
	} else
	{
		Keyboard_UpdateKeyMap2(key, down);
	}
}

LOCALPROC DoKeyCode(int i, blnr down, unsigned int modifiers)
{
	if (i == 36 &&
		(modifiers & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask)) == Mod1Mask) {
		// Alt+Enter
		if (down) {
			ToggleWantFullScreen();
			NeedWholeScreenDraw = trueblnr;
		}
		return;
	}

	if ((i >= 0) && (i < 256)) {
		ui3r key = KC2MKC[i];

		if (MKC_None == key) {
			/* ignore */
		} else
		if (MKC_real_CapsLock == key) {
			CheckTheCapsLock();
		} else
		{
			Keyboard_UpdateKeyMap2(key, down);
		}
	}
}

#if MayFullScreen && GrabKeysFullScreen
LOCALVAR blnr KeyboardIsGrabbed = falseblnr;
#endif

#if MayFullScreen && GrabKeysFullScreen
LOCALPROC MyGrabKeyboard(void)
{
	if (! KeyboardIsGrabbed) {
		(void) XGrabKeyboard(x_display, my_main_wind,
			False, GrabModeAsync, GrabModeAsync,
			CurrentTime);
		KeyboardIsGrabbed = trueblnr;
	}
}
#endif

#if MayFullScreen && GrabKeysFullScreen
LOCALPROC MyUnGrabKeyboard(void)
{
	if (KeyboardIsGrabbed && my_main_wind) {
		XUngrabKeyboard(x_display, CurrentTime);
		KeyboardIsGrabbed = falseblnr;
	}
}
#endif

LOCALVAR blnr NoKeyRepeat = falseblnr;
LOCALVAR int SaveKeyRepeat;

LOCALPROC DisableKeyRepeat(void)
{
	XKeyboardState r;
	XKeyboardControl k;

	if ((! NoKeyRepeat) && (x_display != NULL)) {
		NoKeyRepeat = trueblnr;

		XGetKeyboardControl(x_display, &r);
		SaveKeyRepeat = r.global_auto_repeat;

		k.auto_repeat_mode = AutoRepeatModeOff;
		XChangeKeyboardControl(x_display, KBAutoRepeatMode, &k);
	}
}

LOCALPROC RestoreKeyRepeat(void)
{
	XKeyboardControl k;

	if (NoKeyRepeat && (x_display != NULL)) {
		NoKeyRepeat = falseblnr;

		k.auto_repeat_mode = SaveKeyRepeat;
		XChangeKeyboardControl(x_display, KBAutoRepeatMode, &k);
	}
}

LOCALVAR blnr WantCmdOptOnReconnect = falseblnr;

LOCALPROC GetTheDownKeys(void)
{
	char keys_return[32];
	int i;
	int v;
	int j;

	XQueryKeymap(x_display, keys_return);

	for (i = 0; i < 32; ++i) {
		v = keys_return[i];
		for (j = 0; j < 8; ++j) {
			if (0 != ((1 << j) & v)) {
				int k = i * 8 + j;

				DoKeyCode0(k, trueblnr);
			}
		}
	}
}

LOCALPROC ReconnectKeyCodes3(void)
{
	CheckTheCapsLock();

	if (WantCmdOptOnReconnect) {
		WantCmdOptOnReconnect = falseblnr;

		GetTheDownKeys();
	}
}

LOCALPROC DisconnectKeyCodes3(void)
{
	DisconnectKeyCodes2();
	MyMouseButtonSet(falseblnr);
}

/* --- time, date, location --- */

#define dbglog_TimeStuff (0 && dbglog_HAVE)

LOCALVAR ui5b TrueEmulatedTime = 0;

#include "DATE2SEC.h"

#define TicksPerSecond 1000000

LOCALVAR blnr HaveTimeDelta = falseblnr;
LOCALVAR ui5b TimeDelta;

LOCALVAR ui5b NewMacDateInSeconds;

LOCALVAR ui5b LastTimeSec;
LOCALVAR ui5b LastTimeUsec;

LOCALPROC GetCurrentTicks(void)
{
	struct timeval t;

	gettimeofday(&t, NULL);
	if (! HaveTimeDelta) {
		time_t Current_Time;
		struct tm *s;

		(void) time(&Current_Time);
		s = localtime(&Current_Time);
		TimeDelta = Date2MacSeconds(s->tm_sec, s->tm_min, s->tm_hour,
			s->tm_mday, 1 + s->tm_mon, 1900 + s->tm_year) - t.tv_sec;
#if 0 && AutoTimeZone /* how portable is this ? */
		CurMacDelta = ((ui5b)(s->tm_gmtoff) & 0x00FFFFFF)
			| ((s->tm_isdst ? 0x80 : 0) << 24);
#endif
		HaveTimeDelta = trueblnr;
	}

	NewMacDateInSeconds = t.tv_sec + TimeDelta;
	LastTimeSec = (ui5b)t.tv_sec;
	LastTimeUsec = (ui5b)t.tv_usec;
}

#define MyInvTimeStep 16626 /* TicksPerSecond / 60.14742 */

LOCALVAR ui5b NextTimeSec;
LOCALVAR ui5b NextTimeUsec;

LOCALPROC IncrNextTime(void)
{
	NextTimeUsec += MyInvTimeStep;
	if (NextTimeUsec >= TicksPerSecond) {
		NextTimeUsec -= TicksPerSecond;
		NextTimeSec += 1;
	}
}

LOCALPROC InitNextTime(void)
{
	NextTimeSec = LastTimeSec;
	NextTimeUsec = LastTimeUsec;
	IncrNextTime();
}

LOCALPROC StartUpTimeAdjust(void)
{
	GetCurrentTicks();
	InitNextTime();
}

LOCALFUNC si5b GetTimeDiff(void)
{
	return ((si5b)(LastTimeSec - NextTimeSec)) * TicksPerSecond
		+ ((si5b)(LastTimeUsec - NextTimeUsec));
}

LOCALPROC UpdateTrueEmulatedTime(void)
{
	si5b TimeDiff;

	GetCurrentTicks();

	TimeDiff = GetTimeDiff();
	if (TimeDiff >= 0) {
		if (TimeDiff > 16 * MyInvTimeStep) {
			/* emulation interrupted, forget it */
			++TrueEmulatedTime;
			InitNextTime();

#if dbglog_TimeStuff
			dbglog_writelnNum("emulation interrupted",
				TrueEmulatedTime);
#endif
		} else {
			do {
				++TrueEmulatedTime;
				IncrNextTime();
				TimeDiff -= TicksPerSecond;
			} while (TimeDiff >= 0);
		}
	} else if (TimeDiff < - 16 * MyInvTimeStep) {
		/* clock goofed if ever get here, reset */
#if dbglog_TimeStuff
		dbglog_writeln("clock set back");
#endif

		InitNextTime();
	}
}

LOCALFUNC blnr CheckDateTime(void)
{
	if (CurMacDateInSeconds != NewMacDateInSeconds) {
		CurMacDateInSeconds = NewMacDateInSeconds;
		return trueblnr;
	} else {
		return falseblnr;
	}
}

LOCALFUNC blnr InitLocationDat(void)
{
	GetCurrentTicks();
	CurMacDateInSeconds = NewMacDateInSeconds;

	return trueblnr;
}

/* --- sound --- */

#if MySoundEnabled

#define kLn2SoundBuffers 4 /* kSoundBuffers must be a power of two */
#define kSoundBuffers (1 << kLn2SoundBuffers)
#define kSoundBuffMask (kSoundBuffers - 1)

#define DesiredMinFilledSoundBuffs 3
	/*
		if too big then sound lags behind emulation.
		if too small then sound will have pauses.
	*/

#define kLnOneBuffLen 9
#define kLnAllBuffLen (kLn2SoundBuffers + kLnOneBuffLen)
#define kOneBuffLen (1UL << kLnOneBuffLen)
#define kAllBuffLen (1UL << kLnAllBuffLen)
#define kLnOneBuffSz (kLnOneBuffLen + kLn2SoundSampSz - 3)
#define kLnAllBuffSz (kLnAllBuffLen + kLn2SoundSampSz - 3)
#define kOneBuffSz (1UL << kLnOneBuffSz)
#define kAllBuffSz (1UL << kLnAllBuffSz)
#define kOneBuffMask (kOneBuffLen - 1)
#define kAllBuffMask (kAllBuffLen - 1)
#define dbhBufferSize (kAllBuffSz + kOneBuffSz)

#define dbglog_SoundStuff (0 && dbglog_HAVE)
#define dbglog_SoundBuffStats (0 && dbglog_HAVE)

LOCALVAR tpSoundSamp TheSoundBuffer = nullpr;
LOCALVAR ui4b ThePlayOffset;
LOCALVAR ui4b TheFillOffset;
LOCALVAR ui4b TheWriteOffset;
LOCALVAR ui4b MinFilledSoundBuffs;

LOCALPROC MySound_Start0(void)
{
	/* Reset variables */
	ThePlayOffset = 0;
	TheFillOffset = 0;
	TheWriteOffset = 0;
	MinFilledSoundBuffs = kSoundBuffers;
}

GLOBALOSGLUFUNC tpSoundSamp MySound_BeginWrite(ui4r n, ui4r *actL)
{
	ui4b ToFillLen = kAllBuffLen - (TheWriteOffset - ThePlayOffset);
	ui4b WriteBuffContig =
		kOneBuffLen - (TheWriteOffset & kOneBuffMask);

	if (WriteBuffContig < n) {
		n = WriteBuffContig;
	}
	if (ToFillLen < n) {
		/* overwrite previous buffer */
#if dbglog_SoundStuff
		dbglog_writeln("sound buffer over flow");
#endif
		TheWriteOffset -= kOneBuffLen;
	}

	*actL = n;
	return TheSoundBuffer + (TheWriteOffset & kAllBuffMask);
}

LOCALFUNC blnr MySound_EndWrite0(ui4r actL)
{
	blnr v;

	TheWriteOffset += actL;

	if (0 != (TheWriteOffset & kOneBuffMask)) {
		v = falseblnr;
	} else {
		/* just finished a block */

		TheFillOffset = TheWriteOffset;

		v = trueblnr;
	}

	return v;
}

LOCALPROC MySound_SecondNotify0(void)
{
	if (MinFilledSoundBuffs > DesiredMinFilledSoundBuffs) {
#if dbglog_SoundStuff
			dbglog_writeln("MinFilledSoundBuffs too high");
#endif
		IncrNextTime();
	} else if (MinFilledSoundBuffs < DesiredMinFilledSoundBuffs) {
#if dbglog_SoundStuff
			dbglog_writeln("MinFilledSoundBuffs too low");
#endif
		++TrueEmulatedTime;
	}
	MinFilledSoundBuffs = kSoundBuffers;
}

#define SOUND_SAMPLERATE 22255 /* = round(7833600 * 2 / 704) */

#include "SOUNDGLU.h"

#endif

/* --- basic dialogs --- */

LOCALPROC CheckSavedMacMsg(void)
{
	if (nullpr != SavedBriefMsg) {
		char briefMsg0[ClStrMaxLength + 1];
		char longMsg0[ClStrMaxLength + 1];

		NativeStrFromCStr(briefMsg0, SavedBriefMsg);
		NativeStrFromCStr(longMsg0, SavedLongMsg);

		fprintf(stderr, "%s\n", briefMsg0);
		fprintf(stderr, "%s\n", longMsg0);

		SavedBriefMsg = nullpr;
	}
}

/* --- clipboard --- */

#if IncludeHostTextClipExchange
LOCALVAR ui3p MyClipBuffer = NULL;
#endif

#if IncludeHostTextClipExchange
LOCALPROC FreeMyClipBuffer(void)
{
	if (MyClipBuffer != NULL) {
		free(MyClipBuffer);
		MyClipBuffer = NULL;
	}
}
#endif

#if IncludeHostTextClipExchange
GLOBALOSGLUFUNC tMacErr HTCEexport(tPbuf i)
{
	tMacErr err = mnvm_miscErr;

	FreeMyClipBuffer();
	if (MacRomanTextToNativePtr(i, falseblnr,
		&MyClipBuffer))
	{
		XSetSelectionOwner(x_display, MyXA_CLIPBOARD,
			my_main_wind, CurrentTime);
		err = mnvm_noErr;
	}

	PbufDispose(i);

	return err;
}
#endif

#if IncludeHostTextClipExchange
LOCALFUNC blnr WaitForClipboardSelection(XEvent *xevent)
{
	struct timespec rqt;
	struct timespec rmt;
	int i;

	for (i = 100; --i >= 0; ) {
		while (XCheckTypedWindowEvent(x_display, my_main_wind,
			SelectionNotify, xevent))
		{
			if (xevent->xselection.selection != MyXA_CLIPBOARD) {
				/*
					not what we were looking for. lose it.
					(and hope it wasn't too important).
				*/
				WriteExtraErr("Discarding unwanted SelectionNotify");
			} else {
				/* this is our event */
				return trueblnr;
			}
		}

		rqt.tv_sec = 0;
		rqt.tv_nsec = 10000000;
		(void) nanosleep(&rqt, &rmt);
	}
	return falseblnr;
}
#endif

#if IncludeHostTextClipExchange
LOCALPROC HTCEimport_do(void)
{
	Window w = XGetSelectionOwner(x_display, MyXA_CLIPBOARD);

	if (w == my_main_wind) {
		/* We own the clipboard, already have MyClipBuffer */
	} else {
		FreeMyClipBuffer();
		if (w != None) {
			XEvent xevent;

			XDeleteProperty(x_display, my_main_wind,
				MyXA_MinivMac_Clip);
			XConvertSelection(x_display, MyXA_CLIPBOARD, XA_STRING,
				MyXA_MinivMac_Clip, my_main_wind, CurrentTime);

			if (WaitForClipboardSelection(&xevent)) {
				if (None == xevent.xselection.property) {
					/* oops, target not supported */
				} else {
					if (xevent.xselection.property
						!= MyXA_MinivMac_Clip)
					{
						/* not where we expected it */
					} else {
						Atom ret_type;
						int ret_format;
						unsigned long ret_item;
						unsigned long remain_byte;
						unsigned char *s = NULL;

						if ((Success != XGetWindowProperty(
							x_display, my_main_wind, MyXA_MinivMac_Clip,
							0, 65535, False, AnyPropertyType, &ret_type,
							&ret_format, &ret_item, &remain_byte, &s))
							|| (ret_type != XA_STRING)
							|| (ret_format != 8)
							|| (NULL == s))
						{
							WriteExtraErr(
								"XGetWindowProperty failed"
								" in HTCEimport_do");
						} else {
							MyClipBuffer = (ui3p)malloc(ret_item + 1);
							if (NULL == MyClipBuffer) {
								MacMsg(kStrOutOfMemTitle,
									kStrOutOfMemMessage, falseblnr);
							} else {
								MyMoveBytes((anyp)s, (anyp)MyClipBuffer,
									ret_item);
								MyClipBuffer[ret_item] = 0;
							}
							XFree(s);
						}
					}
					XDeleteProperty(x_display, my_main_wind,
						MyXA_MinivMac_Clip);
				}
			}
		}
	}
}
#endif

#if IncludeHostTextClipExchange
GLOBALOSGLUFUNC tMacErr HTCEimport(tPbuf *r)
{
	HTCEimport_do();

	return NativeTextToMacRomanPbuf((char *)MyClipBuffer, r);
}
#endif

#if IncludeHostTextClipExchange
LOCALFUNC blnr HandleSelectionRequestClipboard(XEvent *theEvent)
{
	blnr RequestFilled = falseblnr;

#if MyDbgEvents
	dbglog_writeln("Requested MyXA_CLIPBOARD");
#endif

	if (NULL == MyClipBuffer) {
		/* our clipboard is empty */
	} else if (theEvent->xselectionrequest.target == MyXA_TARGETS) {
		Atom a[2];

		a[0] = MyXA_TARGETS;
		a[1] = XA_STRING;

		XChangeProperty(x_display,
			theEvent->xselectionrequest.requestor,
			theEvent->xselectionrequest.property,
			MyXA_TARGETS,
			32,
				/*
					most, but not all, other programs I've
					look at seem to use 8 here, but that
					can't be right. can it?
				*/
			PropModeReplace,
			(unsigned char *)a,
			sizeof(a) / sizeof(Atom));

		RequestFilled = trueblnr;
	} else if (theEvent->xselectionrequest.target == XA_STRING) {
		XChangeProperty(x_display,
			theEvent->xselectionrequest.requestor,
			theEvent->xselectionrequest.property,
			XA_STRING,
			8,
			PropModeReplace,
			(unsigned char *)MyClipBuffer,
			strlen((char *)MyClipBuffer));

		RequestFilled = trueblnr;
	}

	return RequestFilled;
}
#endif

/* --- drag and drop --- */

#if EnableDragDrop
LOCALPROC MyActivateWind(Time time)
{
	if (NetSupportedContains(MyXA_NetActiveWindow)) {
		XEvent xevent;
		Window rootwin = XRootWindow(x_display,
			DefaultScreen(x_display));

		memset(&xevent, 0, sizeof (xevent));

		xevent.xany.type = ClientMessage;
		xevent.xclient.send_event = True;
		xevent.xclient.window = my_main_wind;
		xevent.xclient.message_type = MyXA_NetActiveWindow;
		xevent.xclient.format = 32;
		xevent.xclient.data.l[0] = 1;
		xevent.xclient.data.l[1]= time;

		if (0 == XSendEvent(x_display, rootwin, 0,
			SubstructureRedirectMask | SubstructureNotifyMask,
			&xevent))
		{
			WriteExtraErr("XSendEvent failed in MyActivateWind");
		}
	}

	XRaiseWindow(x_display, my_main_wind);
		/*
			In RedHat 7.1, _NET_ACTIVE_WINDOW supported,
			but XSendEvent of _NET_ACTIVE_WINDOW
			doesn't raise the window. So just always
			call XRaiseWindow. Hopefully calling
			XRaiseWindow won't do any harm on window
			managers where it isn't needed.
			(Such as in Ubuntu 5.10)
		*/
	XSetInputFocus(x_display, my_main_wind,
		RevertToPointerRoot, time);
		/* And call this always too, just in case */
}
#endif

#if EnableDragDrop
LOCALPROC ParseOneUri(char *s)
{
	/* printf("ParseOneUri %s\n", s); */
	if (('f' == s[0]) && ('i' == s[1]) && ('l' == s[2])
		&& ('e' == s[3]) && (':' == s[4]))
	{
		s += 5;
		if (('/' == s[0]) && ('/' == s[1])) {
			/* skip hostname */
			char c;

			s += 2;
			while ((c = *s) != '/') {
				if (0 == c) {
					return;
				}
				++s;
			}
		}
		(void) Sony_Insert1a(s, falseblnr);
	}
}
#endif

#if EnableDragDrop
LOCALFUNC int HexChar2Nib(char x)
{
	if ((x >= '0') && (x <= '9')) {
		return x - '0';
	} else if ((x >= 'A') && (x <= 'F')) {
		return x - 'A' + 10;
	} else if ((x >= 'a') && (x <= 'f')) {
		return x - 'a' + 10;
	} else {
		return -1;
	}
}
#endif

#if EnableDragDrop
LOCALPROC ParseUriList(char *s)
{
	char *p1 = s;
	char *p0 = s;
	char *p = s;
	char c;

	/* printf("ParseUriList %s\n", s); */
	while ((c = *p++) != 0) {
		if ('%' == c) {
			int a;
			int b;

			if (((a = HexChar2Nib(p[0])) >= 0) &&
				((b = HexChar2Nib(p[1])) >= 0))
			{
				p += 2;
				*p1++ = (a << 4) + b;
			} else {
				*p1++ = c;
			}
		} else if (('\n' == c) || ('\r' == c)) {
			*p1++ = 0;
			ParseOneUri(p0);
			p0 = p1;
		} else {
			*p1++ = c;
		}
	}
	*p1++ = 0;
	ParseOneUri(p0);
}
#endif

#if EnableDragDrop
LOCALVAR Window PendingDragWindow = None;
#endif

#if EnableDragDrop
LOCALPROC HandleSelectionNotifyDnd(XEvent *theEvent)
{
	blnr DropOk = falseblnr;

#if MyDbgEvents
	dbglog_writeln("Got MyXA_DndSelection");
#endif

	if ((theEvent->xselection.property == MyXA_MinivMac_DndXchng)
		&& (theEvent->xselection.target == MyXA_UriList))
	{
		Atom ret_type;
		int ret_format;
		unsigned long ret_item;
		unsigned long remain_byte;
		unsigned char *s = NULL;

		if ((Success != XGetWindowProperty(x_display, my_main_wind,
			MyXA_MinivMac_DndXchng,
			0, 65535, False, MyXA_UriList, &ret_type, &ret_format,
			&ret_item, &remain_byte, &s))
			|| (NULL == s))
		{
			WriteExtraErr(
				"XGetWindowProperty failed in SelectionNotify");
		} else {
			ParseUriList((char *)s);
			DropOk = trueblnr;
			XFree(s);
		}
	} else {
		WriteExtraErr("Got Unknown SelectionNotify");
	}

	XDeleteProperty(x_display, my_main_wind,
		MyXA_MinivMac_DndXchng);

	if (PendingDragWindow != None) {
		XEvent xevent;

		memset(&xevent, 0, sizeof(xevent));

		xevent.xany.type = ClientMessage;
		xevent.xany.display = x_display;
		xevent.xclient.window = PendingDragWindow;
		xevent.xclient.message_type = MyXA_DndFinished;
		xevent.xclient.format = 32;

		xevent.xclient.data.l[0] = my_main_wind;
		if (DropOk) {
			xevent.xclient.data.l[1] = 1;
		}
		xevent.xclient.data.l[2] = MyXA_DndActionPrivate;

		if (0 == XSendEvent(x_display,
			PendingDragWindow, 0, 0, &xevent))
		{
			WriteExtraErr("XSendEvent failed in SelectionNotify");
		}
	}
	if (DropOk && gTrueBackgroundFlag) {
		MyActivateWind(theEvent->xselection.time);

		WantCmdOptOnReconnect = trueblnr;
	}
}
#endif

#if EnableDragDrop
LOCALPROC HandleClientMessageDndPosition(XEvent *theEvent)
{
	XEvent xevent;
	int xr;
	int yr;
	unsigned int dr;
	unsigned int wr;
	unsigned int hr;
	unsigned int bwr;
	Window rr;
	Window srcwin = theEvent->xclient.data.l[0];

#if MyDbgEvents
	dbglog_writeln("Got XdndPosition");
#endif

	XGetGeometry(x_display, my_main_wind,
		&rr, &xr, &yr, &wr, &hr, &bwr, &dr);
	memset (&xevent, 0, sizeof(xevent));
	xevent.xany.type = ClientMessage;
	xevent.xany.display = x_display;
	xevent.xclient.window = srcwin;
	xevent.xclient.message_type = MyXA_DndStatus;
	xevent.xclient.format = 32;

	xevent.xclient.data.l[0] = theEvent->xclient.window;
		/* Target Window */
	xevent.xclient.data.l[1] = 1; /* Accept */
	xevent.xclient.data.l[2] = ((xr) << 16) | ((yr) & 0xFFFFUL);
	xevent.xclient.data.l[3] = ((wr) << 16) | ((hr) & 0xFFFFUL);
	xevent.xclient.data.l[4] = MyXA_DndActionPrivate; /* Action */

	if (0 == XSendEvent(x_display, srcwin, 0, 0, &xevent)) {
		WriteExtraErr(
			"XSendEvent failed in HandleClientMessageDndPosition");
	}
}
#endif

#if EnableDragDrop
LOCALPROC HandleClientMessageDndDrop(XEvent *theEvent)
{
	Time timestamp = theEvent->xclient.data.l[2];
	PendingDragWindow = (Window) theEvent->xclient.data.l[0];

#if MyDbgEvents
	dbglog_writeln("Got XdndDrop");
#endif

	XConvertSelection(x_display, MyXA_DndSelection, MyXA_UriList,
		MyXA_MinivMac_DndXchng, my_main_wind, timestamp);
}
#endif


#if EmLocalTalk

struct xqpr {
		int NewMousePosh;
		int NewMousePosv;
		int root_x_return;
		int root_y_return;
		Window root_return;
		Window child_return;
		unsigned int mask_return;
};
typedef struct xqpr xqpr;


LOCALFUNC blnr EntropyGather(void)
{
	/*
		gather some entropy from several places, just in case
		/dev/urandom is not available.
	*/

	{
		struct timeval t;

		gettimeofday(&t, NULL);

		EntropyPoolAddPtr((ui3p)&t, sizeof(t) / sizeof(ui3b));
	}

	{
		xqpr t;

		XQueryPointer(x_display, my_main_wind,
			&t.root_return, &t.child_return,
			&t.root_x_return, &t.root_y_return,
			&t.NewMousePosh, &t.NewMousePosv,
			&t.mask_return);

		EntropyPoolAddPtr((ui3p)&t, sizeof(t) / sizeof(ui3b));
	}

#if 0
	/*
		Another possible source of entropy. But if available,
		almost certainly /dev/urandom is also available.
	*/
	/* #include <sys/sysinfo.h> */
	{
		struct sysinfo t;

		if (0 != sysinfo(&t)) {
#if dbglog_HAVE
			dbglog_writeln("sysinfo fails");
#endif
		}

		/*
			continue even if error, it doesn't hurt anything
				if t is garbage.
		*/
		EntropyPoolAddPtr((ui3p)&t, sizeof(t) / sizeof(ui3b));
	}
#endif

	{
		pid_t t = getpid();

		EntropyPoolAddPtr((ui3p)&t, sizeof(t) / sizeof(ui3b));
	}

	{
		ui5b dat[2];
		int fd;

		if (-1 == (fd = open("/dev/urandom", O_RDONLY))) {
#if dbglog_HAVE
			dbglog_writeCStr("open /dev/urandom fails");
			dbglog_writeNum(errno);
			dbglog_writeCStr(" (");
			dbglog_writeCStr(strerror(errno));
			dbglog_writeCStr(")");
			dbglog_writeReturn();
#endif
		} else {

			if (read(fd, &dat, sizeof(dat)) < 0) {
#if dbglog_HAVE
				dbglog_writeCStr("open /dev/urandom fails");
				dbglog_writeNum(errno);
				dbglog_writeCStr(" (");
				dbglog_writeCStr(strerror(errno));
				dbglog_writeCStr(")");
				dbglog_writeReturn();
#endif
			} else {

#if dbglog_HAVE
				dbglog_writeCStr("dat: ");
				dbglog_writeHex(dat[0]);
				dbglog_writeCStr(" ");
				dbglog_writeHex(dat[1]);
				dbglog_writeReturn();
#endif

				e_p[0] ^= dat[0];
				e_p[1] ^= dat[1];
					/*
						if "/dev/urandom" is working correctly,
						this should make the previous contents of e_p
						irrelevant. if it is completely broken, like
						returning 0, this will not make e_p any less
						random.
					*/

#if dbglog_HAVE
				dbglog_writeCStr("ep: ");
				dbglog_writeHex(e_p[0]);
				dbglog_writeCStr(" ");
				dbglog_writeHex(e_p[1]);
				dbglog_writeReturn();
#endif
			}

			close(fd);
		}
	}

	return trueblnr;
}
#endif

#if EmLocalTalk

#include "LOCALTLK.h"

#endif


#define UseMotionEvents 1

#if UseMotionEvents
LOCALVAR blnr CaughtMouse = falseblnr;
#endif

#if MayNotFullScreen
LOCALVAR int SavedTransH;
LOCALVAR int SavedTransV;
#endif

/* --- event handling for main window --- */

LOCALPROC HandleTheEvent(XEvent *theEvent)
{
	if (theEvent->xany.display != x_display) {
		WriteExtraErr("Got event for some other display");
	} else switch(theEvent->type) {
		case KeyPress:
			if (theEvent->xkey.window != my_main_wind) {
				WriteExtraErr("Got KeyPress for some other window");
			} else {
#if MyDbgEvents
				dbglog_writeln("- event - KeyPress");
#endif
				MousePositionNotify(theEvent->xkey.x, theEvent->xkey.y);
				DoKeyCode(theEvent->xkey.keycode, trueblnr, theEvent->xkey.state);
			}
			break;
		case KeyRelease:
			if (theEvent->xkey.window != my_main_wind) {
				WriteExtraErr("Got KeyRelease for some other window");
			} else {
#if MyDbgEvents
				dbglog_writeln("- event - KeyRelease");
#endif

				MousePositionNotify(theEvent->xkey.x, theEvent->xkey.y);
				DoKeyCode(theEvent->xkey.keycode, falseblnr, theEvent->xkey.state);
			}
			break;
		case ButtonPress:
			/* any mouse button, we don't care which */
			if (theEvent->xbutton.window != my_main_wind) {
				WriteExtraErr("Got ButtonPress for some other window");
			} else {
				/*
					could check some modifiers, but don't bother for now
					Keyboard_UpdateKeyMap2(MKC_formac_CapsLock,
						(theEvent->xbutton.state & LockMask) != 0);
				*/
				MousePositionNotify(
					theEvent->xbutton.x, theEvent->xbutton.y);
				MyMouseButtonSet(trueblnr);
			}
			break;
		case ButtonRelease:
			/* any mouse button, we don't care which */
			if (theEvent->xbutton.window != my_main_wind) {
				WriteExtraErr(
					"Got ButtonRelease for some other window");
			} else {
				MousePositionNotify(
					theEvent->xbutton.x, theEvent->xbutton.y);
				MyMouseButtonSet(falseblnr);
			}
			break;
#if UseMotionEvents
		case MotionNotify:
			if (theEvent->xmotion.window != my_main_wind) {
				WriteExtraErr("Got MotionNotify for some other window");
			} else {
				MousePositionNotify(
					theEvent->xmotion.x, theEvent->xmotion.y);
			}
			break;
		case EnterNotify:
			if (theEvent->xcrossing.window != my_main_wind) {
				WriteExtraErr("Got EnterNotify for some other window");
			} else {
#if MyDbgEvents
				dbglog_writeln("- event - EnterNotify");
#endif

				CaughtMouse = trueblnr;
				MousePositionNotify(
					theEvent->xcrossing.x, theEvent->xcrossing.y);
			}
			break;
		case LeaveNotify:
			if (theEvent->xcrossing.window != my_main_wind) {
				WriteExtraErr("Got LeaveNotify for some other window");
			} else {
#if MyDbgEvents
				dbglog_writeln("- event - LeaveNotify");
#endif

				MousePositionNotify(
					theEvent->xcrossing.x, theEvent->xcrossing.y);
				CaughtMouse = falseblnr;
			}
			break;
#endif
		case Expose:
			if (theEvent->xexpose.window != my_main_wind) {
				WriteExtraErr(
					"Got SelectionRequest for some other window");
			} else {
				int x0 = theEvent->xexpose.x;
				int y0 = theEvent->xexpose.y;
				int x1 = x0 + theEvent->xexpose.width;
				int y1 = y0 + theEvent->xexpose.height;

#if 0 && MyDbgEvents
				dbglog_writeln("- event - Expose");
#endif

#if VarFullScreen
				if (UseFullScreen)
#endif
#if MayFullScreen
				{
					x0 -= hOffset;
					y0 -= vOffset;
					x1 -= hOffset;
					y1 -= vOffset;
				}
#endif

#if EnableMagnify
				if (UseMagnify) {
					x0 /= MyWindowScale;
					y0 /= MyWindowScale;
					x1 = (x1 + (MyWindowScale - 1)) / MyWindowScale;
					y1 = (y1 + (MyWindowScale - 1)) / MyWindowScale;
				}
#endif

#if VarFullScreen
				if (UseFullScreen)
#endif
#if MayFullScreen
				{
					x0 += ViewHStart;
					y0 += ViewVStart;
					x1 += ViewHStart;
					y1 += ViewVStart;
				}
#endif

				if (x0 < 0) {
					x0 = 0;
				}
				if (x1 > vMacScreenWidth) {
					x1 = vMacScreenWidth;
				}
				if (y0 < 0) {
					y0 = 0;
				}
				if (y1 > vMacScreenHeight) {
					y1 = vMacScreenHeight;
				}
				if ((x0 < x1) && (y0 < y1)) {
					HaveChangedScreenBuff(y0, x0, y1, x1);
				}

				NeedFinishOpen1 = falseblnr;
			}
			break;
#if IncludeHostTextClipExchange
		case SelectionRequest:
			if (theEvent->xselectionrequest.owner != my_main_wind) {
				WriteExtraErr(
					"Got SelectionRequest for some other window");
			} else {
				XEvent xevent;
				blnr RequestFilled = falseblnr;

#if MyDbgEvents
				dbglog_writeln("- event - SelectionRequest");
				WriteDbgAtom("selection",
					theEvent->xselectionrequest.selection);
				WriteDbgAtom("target",
					theEvent->xselectionrequest.target);
				WriteDbgAtom("property",
					theEvent->xselectionrequest.property);
#endif

				if (theEvent->xselectionrequest.selection ==
					MyXA_CLIPBOARD)
				{
					RequestFilled =
						HandleSelectionRequestClipboard(theEvent);
				}


				memset(&xevent, 0, sizeof(xevent));
				xevent.xselection.type = SelectionNotify;
				xevent.xselection.display = x_display;
				xevent.xselection.requestor =
					theEvent->xselectionrequest.requestor;
				xevent.xselection.selection =
					theEvent->xselectionrequest.selection;
				xevent.xselection.target =
					theEvent->xselectionrequest.target;
				xevent.xselection.property = (! RequestFilled) ? None
					: theEvent->xselectionrequest.property ;
				xevent.xselection.time =
					theEvent->xselectionrequest.time;

				if (0 == XSendEvent(x_display,
					xevent.xselection.requestor, False, 0, &xevent))
				{
					WriteExtraErr(
						"XSendEvent failed in SelectionRequest");
				}
			}
			break;
		case SelectionClear:
			if (theEvent->xselectionclear.window != my_main_wind) {
				WriteExtraErr(
					"Got SelectionClear for some other window");
			} else {
#if MyDbgEvents
				dbglog_writeln("- event - SelectionClear");
				WriteDbgAtom("selection",
					theEvent->xselectionclear.selection);
#endif

				if (theEvent->xselectionclear.selection ==
					MyXA_CLIPBOARD)
				{
					FreeMyClipBuffer();
				}
			}
			break;
#endif
#if EnableDragDrop
		case SelectionNotify:
			if (theEvent->xselection.requestor != my_main_wind) {
				WriteExtraErr(
					"Got SelectionNotify for some other window");
			} else {
#if MyDbgEvents
				dbglog_writeln("- event - SelectionNotify");
				WriteDbgAtom("selection",
					theEvent->xselection.selection);
				WriteDbgAtom("target", theEvent->xselection.target);
				WriteDbgAtom("property", theEvent->xselection.property);
#endif

				if (theEvent->xselection.selection == MyXA_DndSelection)
				{
					HandleSelectionNotifyDnd(theEvent);
				} else {
					WriteExtraErr(
						"Got Unknown selection in SelectionNotify");
				}
			}
			break;
#endif
		case ClientMessage:
			if (theEvent->xclient.window != my_main_wind) {
				WriteExtraErr(
					"Got ClientMessage for some other window");
			} else {
#if MyDbgEvents
				dbglog_writeln("- event - ClientMessage");
				WriteDbgAtom("message_type",
					theEvent->xclient.message_type);
#endif

#if EnableDragDrop
				if (theEvent->xclient.message_type == MyXA_DndEnter) {
					/* printf("Got XdndEnter\n"); */
				} else if (theEvent->xclient.message_type ==
					MyXA_DndLeave)
				{
					/* printf("Got XdndLeave\n"); */
				} else if (theEvent->xclient.message_type ==
					MyXA_DndPosition)
				{
					HandleClientMessageDndPosition(theEvent);
				} else if (theEvent->xclient.message_type ==
					MyXA_DndDrop)
				{
					HandleClientMessageDndDrop(theEvent);
				} else
#endif
				{
					if ((32 == theEvent->xclient.format) &&
						(theEvent->xclient.data.l[0] == MyXA_DeleteW))
					{
						/*
							I would think that should test that
								WM_PROTOCOLS == message_type
							but none of the other programs I looked
							at did.
						*/
						RequestMacOff = trueblnr;
					}
				}
			}
			break;
		case FocusIn:
			if (theEvent->xfocus.window != my_main_wind) {
				WriteExtraErr("Got FocusIn for some other window");
			} else {
#if MyDbgEvents
				dbglog_writeln("- event - FocusIn");
#endif

				gTrueBackgroundFlag = falseblnr;
#if UseMotionEvents
				CheckMouseState();
					/*
						Doesn't help on x11 for OS X,
						can't get new mouse position
						in any fashion until mouse moves.
					*/
#endif
			}
			break;
		case FocusOut:
			if (theEvent->xfocus.window != my_main_wind) {
				WriteExtraErr("Got FocusOut for some other window");
			} else {
#if MyDbgEvents
				dbglog_writeln("- event - FocusOut");
#endif

				gTrueBackgroundFlag = trueblnr;
			}
			break;
		default:
			break;
	}
}

/* --- main window creation and disposal --- */

LOCALVAR int my_argc;
LOCALVAR char **my_argv;

LOCALVAR char *display_name = NULL;

LOCALFUNC blnr Screen_Init(void)
{
	Window rootwin;
	int screen;
	Colormap Xcmap;
	Visual *Xvisual;

	x_display = XOpenDisplay(display_name);
	if (NULL == x_display) {
		fprintf(stderr, "Cannot connect to X server.\n");
		return falseblnr;
	}

	screen = DefaultScreen(x_display);

	rootwin = XRootWindow(x_display, screen);

	Xcmap = DefaultColormap(x_display, screen);

	Xvisual = DefaultVisual(x_display, screen);

	LoadMyXA();

	XParseColor(x_display, Xcmap, "#000000", &x_black);
	if (! XAllocColor(x_display, Xcmap, &x_black)) {
		WriteExtraErr("XParseColor black fails");
	}
	XParseColor(x_display, Xcmap, "#ffffff", &x_white);
	if (! XAllocColor(x_display, Xcmap, &x_white)) {
		WriteExtraErr("XParseColor white fails");
	}

	if (! CreateMyBlankCursor(rootwin)) {
		return falseblnr;
	}

#if ! UseColorImage
	my_image = XCreateImage(x_display, Xvisual, 1, XYBitmap, 0,
		NULL /* (char *)image_Mem1 */,
		vMacScreenWidth, vMacScreenHeight, 32,
		vMacScreenMonoByteWidth);
	if (NULL == my_image) {
		fprintf(stderr, "XCreateImage failed.\n");
		return falseblnr;
	}

#if 0
	fprintf(stderr, "bitmap_bit_order = %d\n",
		(int)my_image->bitmap_bit_order);
	fprintf(stderr, "byte_order = %d\n", (int)my_image->byte_order);
#endif

	my_image->bitmap_bit_order = MSBFirst;
	my_image->byte_order = MSBFirst;
#endif

#if UseColorImage
	my_image = XCreateImage(x_display, Xvisual, 24, ZPixmap, 0,
		NULL /* (char *)image_Mem1 */,
		vMacScreenWidth, vMacScreenHeight, 32,
			4 * (ui5r)vMacScreenWidth);
	if (NULL == my_image) {
		fprintf(stderr, "XCreateImage Color failed.\n");
		return falseblnr;
	}

#if 0
	fprintf(stderr, "DefaultDepth = %d\n",
		(int)DefaultDepth(x_display, screen));

	fprintf(stderr, "MSBFirst = %d\n", (int)MSBFirst);
	fprintf(stderr, "LSBFirst = %d\n", (int)LSBFirst);

	fprintf(stderr, "bitmap_bit_order = %d\n",
		(int)my_image->bitmap_bit_order);
	fprintf(stderr, "byte_order = %d\n",
		(int)my_image->byte_order);
	fprintf(stderr, "bitmap_unit = %d\n",
		(int)my_image->bitmap_unit);
	fprintf(stderr, "bits_per_pixel = %d\n",
		(int)my_image->bits_per_pixel);
	fprintf(stderr, "red_mask = %d\n",
		(int)my_image->red_mask);
	fprintf(stderr, "green_mask = %d\n",
		(int)my_image->green_mask);
	fprintf(stderr, "blue_mask = %d\n",
		(int)my_image->blue_mask);
#endif

#endif /* UseColorImage */

#if EnableMagnify && (! UseColorImage)
	my_Scaled_image = XCreateImage(x_display, Xvisual,
		1, XYBitmap, 0,
		NULL /* (char *)image_Mem1 */,
		vMacScreenWidth * MyWindowScale,
		vMacScreenHeight * MyWindowScale,
		32, vMacScreenMonoByteWidth * MyWindowScale);
	if (NULL == my_Scaled_image) {
		fprintf(stderr, "XCreateImage failed.\n");
		return falseblnr;
	}

	my_Scaled_image->bitmap_bit_order = MSBFirst;
	my_Scaled_image->byte_order = MSBFirst;
#endif

#if EnableMagnify && UseColorImage
	my_Scaled_image = XCreateImage(x_display, Xvisual,
		24, ZPixmap, 0,
		NULL /* (char *)image_Mem1 */,
		vMacScreenWidth * MyWindowScale,
		vMacScreenHeight * MyWindowScale,
		32, 4 * (ui5r)vMacScreenWidth * MyWindowScale);
	if (NULL == my_Scaled_image) {
		fprintf(stderr, "XCreateImage Scaled failed.\n");
		return falseblnr;
	}
#endif

#if 0 != vMacScreenDepth
	ColorModeWorks = trueblnr;
#endif

	DisableKeyRepeat();

	return trueblnr;
}

LOCALPROC CloseMainWindow(void)
{
	if (my_gc != NULL) {
		XFreeGC(x_display, my_gc);
		my_gc = NULL;
	}
	if (my_main_wind) {
		XDestroyWindow(x_display, my_main_wind);
		my_main_wind = 0;
	}
}

enum {
	kMagStateNormal,
#if EnableMagnify
	kMagStateMagnifgy,
#endif
	kNumMagStates
};

#define kMagStateAuto kNumMagStates

#if MayNotFullScreen
LOCALVAR int CurWinIndx;
LOCALVAR blnr HavePositionWins[kNumMagStates];
LOCALVAR int WinPositionWinsH[kNumMagStates];
LOCALVAR int WinPositionWinsV[kNumMagStates];
#endif

#if EnableRecreateW
LOCALPROC ZapMyWState(void)
{
	my_main_wind = 0;
	my_gc = NULL;
}
#endif

LOCALFUNC blnr CreateMainWindow(void)
{
	Window rootwin;
	int screen;
	int xr;
	int yr;
	unsigned int dr;
	unsigned int wr;
	unsigned int hr;
	unsigned int bwr;
	Window rr;
	int leftPos;
	int topPos;
#if MayNotFullScreen
	int WinIndx;
#endif
#if EnableDragDrop
	long int xdnd_version = 5;
#endif
	int NewWindowHeight = vMacScreenHeight;
	int NewWindowWidth = vMacScreenWidth;

	/* Get connection to X Server */
	screen = DefaultScreen(x_display);

	rootwin = XRootWindow(x_display, screen);

	XGetGeometry(x_display, rootwin,
		&rr, &xr, &yr, &wr, &hr, &bwr, &dr);

#if EnableMagnify
	if (UseMagnify) {
		NewWindowHeight *= MyWindowScale;
		NewWindowWidth *= MyWindowScale;
	}
#endif

	if (wr > NewWindowWidth) {
		leftPos = (wr - NewWindowWidth) / 2;
	} else {
		leftPos = 0;
	}
	if (hr > NewWindowHeight) {
		topPos = (hr - NewWindowHeight) / 2;
	} else {
		topPos = 0;
	}

#if VarFullScreen
	if (UseFullScreen)
#endif
#if MayFullScreen
	{
		ViewHSize = wr;
		ViewVSize = hr;
#if EnableMagnify
		if (UseMagnify) {
			ViewHSize /= MyWindowScale;
			ViewVSize /= MyWindowScale;
		}
#endif
		if (ViewHSize >= vMacScreenWidth) {
			ViewHStart = 0;
			ViewHSize = vMacScreenWidth;
		} else {
			ViewHSize &= ~ 1;
		}
		if (ViewVSize >= vMacScreenHeight) {
			ViewVStart = 0;
			ViewVSize = vMacScreenHeight;
		} else {
			ViewVSize &= ~ 1;
		}
	}
#endif

#if VarFullScreen
	if (! UseFullScreen)
#endif
#if MayNotFullScreen
	{
#if EnableMagnify
		if (UseMagnify) {
			WinIndx = kMagStateMagnifgy;
		} else
#endif
		{
			WinIndx = kMagStateNormal;
		}

		if (! HavePositionWins[WinIndx]) {
			WinPositionWinsH[WinIndx] = leftPos;
			WinPositionWinsV[WinIndx] = topPos;
			HavePositionWins[WinIndx] = trueblnr;
		} else {
			leftPos = WinPositionWinsH[WinIndx];
			topPos = WinPositionWinsV[WinIndx];
		}
	}
#endif

#if VarFullScreen
	if (UseFullScreen)
#endif
#if MayFullScreen
	{
		XSetWindowAttributes xattr;
		xattr.override_redirect = True;
		xattr.background_pixel = x_black.pixel;
		xattr.border_pixel = x_white.pixel;

		my_main_wind = XCreateWindow(x_display, rr,
			0, 0, wr, hr, 0,
			CopyFromParent, /* depth */
			InputOutput, /* class */
			CopyFromParent, /* visual */
			CWOverrideRedirect | CWBackPixel | CWBorderPixel,
				/* valuemask */
			&xattr /* attributes */);
	}
#endif
#if VarFullScreen
	else
#endif
#if MayNotFullScreen
	{
		my_main_wind = XCreateSimpleWindow(x_display, rootwin,
			leftPos,
			topPos,
			NewWindowWidth, NewWindowHeight, 4,
			x_white.pixel,
			x_black.pixel);
	}
#endif

	if (! my_main_wind) {
		WriteExtraErr("XCreateSimpleWindow failed.");
		return falseblnr;
	} else {
		char *win_name =
			(NULL != n_arg) ? n_arg : (
#if CanGetAppPath
			(NULL != app_name) ? app_name :
#endif
			kStrAppName);
		XSelectInput(x_display, my_main_wind,
			ExposureMask | KeyPressMask | KeyReleaseMask
			| ButtonPressMask | ButtonReleaseMask
#if UseMotionEvents
			| PointerMotionMask | EnterWindowMask | LeaveWindowMask
#endif
			| FocusChangeMask);

		XStoreName(x_display, my_main_wind, win_name);
		XSetIconName(x_display, my_main_wind, win_name);

		{
			XClassHint *hints = XAllocClassHint();
			if (hints) {
				hints->res_name = "minivmac";
				hints->res_class = "minivmac";
				XSetClassHint(x_display, my_main_wind, hints);
				XFree(hints);
			}
		}

		{
			XWMHints *hints = XAllocWMHints();
			if (hints) {
				hints->input = True;
				hints->initial_state = NormalState;
				hints->flags = InputHint | StateHint;
				XSetWMHints(x_display, my_main_wind, hints);
				XFree(hints);
			}

		}

		XSetCommand(x_display, my_main_wind, my_argv, my_argc);

		/* let us handle a click on the close box */
		XSetWMProtocols(x_display, my_main_wind, &MyXA_DeleteW, 1);

#if EnableDragDrop
		XChangeProperty (x_display, my_main_wind, MyXA_DndAware,
			XA_ATOM, 32, PropModeReplace,
			(unsigned char *) &xdnd_version, 1);
#endif

		my_gc = XCreateGC(x_display, my_main_wind, 0, NULL);
		if (NULL == my_gc) {
			WriteExtraErr("XCreateGC failed.");
			return falseblnr;
		}
		XSetState(x_display, my_gc, x_black.pixel, x_white.pixel,
			GXcopy, AllPlanes);

#if VarFullScreen
		if (! UseFullScreen)
#endif
#if MayNotFullScreen
		{
			XSizeHints *hints = XAllocSizeHints();
			if (hints) {
				hints->min_width = NewWindowWidth;
				hints->max_width = NewWindowWidth;
				hints->min_height = NewWindowHeight;
				hints->max_height = NewWindowHeight;

				/*
					Try again to say where the window ought to go.
					I've seen this described as obsolete, but it
					seems to work on all x implementations tried
					so far, and nothing else does.
				*/
				hints->x = leftPos;
				hints->y = topPos;
				hints->width = NewWindowWidth;
				hints->height = NewWindowHeight;

				hints->flags = PMinSize | PMaxSize | PPosition | PSize;
				XSetWMNormalHints(x_display, my_main_wind, hints);
				XFree(hints);
			}
		}
#endif

#if VarFullScreen
		if (UseFullScreen)
#endif
#if MayFullScreen
		{
			hOffset = leftPos;
			vOffset = topPos;
		}
#endif

		DisconnectKeyCodes3();
			/* since will lose keystrokes to old window */

#if MayNotFullScreen
		CurWinIndx = WinIndx;
#endif

		XMapRaised(x_display, my_main_wind);

#if 0
		XSync(x_display, 0);
#endif

#if 0
		/*
			This helps in Red Hat 9 to get the new window
			activated, and I've seen other programs
			do similar things.
		*/
		/*
			In current scheme, haven't closed old window
			yet. If old window full screen, never receive
			expose event for new one.
		*/
		{
			XEvent event;

			do {
				XNextEvent(x_display, &event);
				HandleTheEvent(&event);
			} while (! ((Expose == event.type)
				&& (event.xexpose.window == my_main_wind)));
		}
#endif

		NeedFinishOpen1 = trueblnr;
		NeedFinishOpen2 = trueblnr;

		return trueblnr;
	}
}

#if MayFullScreen
LOCALVAR blnr GrabMachine = falseblnr;
#endif

#if MayFullScreen
LOCALPROC GrabTheMachine(void)
{
#if EnableFSMouseMotion
	StartSaveMouseMotion();
#endif
#if GrabKeysFullScreen
	MyGrabKeyboard();
#endif
}
#endif

#if MayFullScreen
LOCALPROC UngrabMachine(void)
{
#if EnableFSMouseMotion
	StopSaveMouseMotion();
#endif
#if GrabKeysFullScreen
	MyUnGrabKeyboard();
#endif
}
#endif

#if EnableRecreateW
struct MyWState {
	Window f_my_main_wind;
	GC f_my_gc;
#if MayFullScreen
	short f_hOffset;
	short f_vOffset;
	ui4r f_ViewHSize;
	ui4r f_ViewVSize;
	ui4r f_ViewHStart;
	ui4r f_ViewVStart;
#endif
#if VarFullScreen
	blnr f_UseFullScreen;
#endif
#if EnableMagnify
	blnr f_UseMagnify;
#endif
};
typedef struct MyWState MyWState;
#endif

#if EnableRecreateW
LOCALPROC GetMyWState(MyWState *r)
{
	r->f_my_main_wind = my_main_wind;
	r->f_my_gc = my_gc;
#if MayFullScreen
	r->f_hOffset = hOffset;
	r->f_vOffset = vOffset;
	r->f_ViewHSize = ViewHSize;
	r->f_ViewVSize = ViewVSize;
	r->f_ViewHStart = ViewHStart;
	r->f_ViewVStart = ViewVStart;
#endif
#if VarFullScreen
	r->f_UseFullScreen = UseFullScreen;
#endif
#if EnableMagnify
	r->f_UseMagnify = UseMagnify;
#endif
}
#endif

#if EnableRecreateW
LOCALPROC SetMyWState(MyWState *r)
{
	my_main_wind = r->f_my_main_wind;
	my_gc = r->f_my_gc;
#if MayFullScreen
	hOffset = r->f_hOffset;
	vOffset = r->f_vOffset;
	ViewHSize = r->f_ViewHSize;
	ViewVSize = r->f_ViewVSize;
	ViewHStart = r->f_ViewHStart;
	ViewVStart = r->f_ViewVStart;
#endif
#if VarFullScreen
	UseFullScreen = r->f_UseFullScreen;
#endif
#if EnableMagnify
	UseMagnify = r->f_UseMagnify;
#endif
}
#endif

#if EnableRecreateW
LOCALVAR blnr WantRestoreCursPos = falseblnr;
LOCALVAR ui4b RestoreMouseH;
LOCALVAR ui4b RestoreMouseV;
#endif

#if EnableRecreateW
LOCALFUNC blnr ReCreateMainWindow(void)
{
	MyWState old_state;
	MyWState new_state;
#if IncludeHostTextClipExchange
	blnr OwnClipboard = falseblnr;
#endif

	if (HaveCursorHidden) {
		WantRestoreCursPos = trueblnr;
		RestoreMouseH = CurMouseH;
		RestoreMouseV = CurMouseV;
	}

	ForceShowCursor(); /* hide/show cursor api is per window */

#if MayNotFullScreen
#if VarFullScreen
	if (! UseFullScreen)
#endif
	if (my_main_wind)
	if (! NeedFinishOpen2)
	{
		/* save old position */
		int xr;
		int yr;
		unsigned int dr;
		unsigned int wr;
		unsigned int hr;
		unsigned int bwr;
		Window rr;
		Window rr2;

		/* Get connection to X Server */
		int screen = DefaultScreen(x_display);

		Window rootwin = XRootWindow(x_display, screen);

		XGetGeometry(x_display, rootwin,
			&rr, &xr, &yr, &wr, &hr, &bwr, &dr);

		/*
			Couldn't reliably find out where window
			is now, due to what seem to be some
			broken X implementations, and so instead
			track how far window has moved.
		*/
		XSync(x_display, 0);
		if (XTranslateCoordinates(x_display, my_main_wind, rootwin,
			0, 0, &xr, &yr, &rr2))
		{
			int newposh =
				WinPositionWinsH[CurWinIndx] + (xr - SavedTransH);
			int newposv =
				WinPositionWinsV[CurWinIndx] + (yr - SavedTransV);
			if ((newposv > 0) && (newposv < hr) && (newposh < wr)) {
				WinPositionWinsH[CurWinIndx] = newposh;
				WinPositionWinsV[CurWinIndx] = newposv;
				SavedTransH = xr;
				SavedTransV = yr;
			}
		}
	}
#endif

#if MayFullScreen
	if (GrabMachine) {
		GrabMachine = falseblnr;
		UngrabMachine();
	}
#endif

	GetMyWState(&old_state);
	ZapMyWState();

#if EnableMagnify
	UseMagnify = WantMagnify;
#endif
#if VarFullScreen
	UseFullScreen = WantFullScreen;
#endif

	ColorTransValid = falseblnr;

	if (! CreateMainWindow()) {
		CloseMainWindow();
		SetMyWState(&old_state);

		/* avoid retry */
#if VarFullScreen
		WantFullScreen = UseFullScreen;
#endif
#if EnableMagnify
		WantMagnify = UseMagnify;
#endif

		return falseblnr;
	} else {
		GetMyWState(&new_state);
		SetMyWState(&old_state);

#if IncludeHostTextClipExchange
		if (my_main_wind) {
			if (XGetSelectionOwner(x_display, MyXA_CLIPBOARD) ==
				my_main_wind)
			{
				OwnClipboard = trueblnr;
			}
		}
#endif

		CloseMainWindow();

		SetMyWState(&new_state);

#if IncludeHostTextClipExchange
		if (OwnClipboard) {
			XSetSelectionOwner(x_display, MyXA_CLIPBOARD,
				my_main_wind, CurrentTime);
		}
#endif
	}

	return trueblnr;
}
#endif

#if VarFullScreen && EnableMagnify
enum {
	kWinStateWindowed,
#if EnableMagnify
	kWinStateFullScreen,
#endif
	kNumWinStates
};
#endif

#if VarFullScreen && EnableMagnify
LOCALVAR int WinMagStates[kNumWinStates];
#endif

LOCALPROC ZapWinStateVars(void)
{
#if MayNotFullScreen
	{
		int i;

		for (i = 0; i < kNumMagStates; ++i) {
			HavePositionWins[i] = falseblnr;
		}
	}
#endif
#if VarFullScreen && EnableMagnify
	{
		int i;

		for (i = 0; i < kNumWinStates; ++i) {
			WinMagStates[i] = kMagStateAuto;
		}
	}
#endif
}

#if VarFullScreen
LOCALPROC ToggleWantFullScreen(void)
{
	WantFullScreen = ! WantFullScreen;

#if EnableMagnify
	{
		int OldWinState =
			UseFullScreen ? kWinStateFullScreen : kWinStateWindowed;
		int OldMagState =
			UseMagnify ? kMagStateMagnifgy : kMagStateNormal;
		int NewWinState =
			WantFullScreen ? kWinStateFullScreen : kWinStateWindowed;
		int NewMagState = WinMagStates[NewWinState];

		WinMagStates[OldWinState] = OldMagState;
		if (kMagStateAuto != NewMagState) {
			WantMagnify = (kMagStateMagnifgy == NewMagState);
		} else {
			WantMagnify = falseblnr;
			if (WantFullScreen) {
				Window rootwin;
				int xr;
				int yr;
				unsigned int dr;
				unsigned int wr;
				unsigned int hr;
				unsigned int bwr;
				Window rr;

				rootwin =
					XRootWindow(x_display, DefaultScreen(x_display));
				XGetGeometry(x_display, rootwin,
					&rr, &xr, &yr, &wr, &hr, &bwr, &dr);
				if ((wr >= vMacScreenWidth * MyWindowScale)
					&& (hr >= vMacScreenHeight * MyWindowScale)
					)
				{
					WantMagnify = trueblnr;
				}
			}
		}
	}
#endif
}
#endif

/* --- SavedTasks --- */

LOCALPROC LeaveBackground(void)
{
	ReconnectKeyCodes3();
	DisableKeyRepeat();
}

LOCALPROC EnterBackground(void)
{
	RestoreKeyRepeat();
	DisconnectKeyCodes3();

	ForceShowCursor();
}

LOCALPROC LeaveSpeedStopped(void)
{
#if MySoundEnabled
	MySound_Start();
#endif

	StartUpTimeAdjust();
}

LOCALPROC EnterSpeedStopped(void)
{
#if MySoundEnabled
	MySound_Stop();
#endif
}

LOCALPROC CheckForSavedTasks(void)
{
	if (MyEvtQNeedRecover) {
		MyEvtQNeedRecover = falseblnr;

		/* attempt cleanup, MyEvtQNeedRecover may get set again */
		MyEvtQTryRecoverFromFull();
	}

	if (NeedFinishOpen2 && ! NeedFinishOpen1) {
		NeedFinishOpen2 = falseblnr;

#if VarFullScreen
		if (UseFullScreen)
#endif
#if MayFullScreen
		{
			XSetInputFocus(x_display, my_main_wind,
				RevertToPointerRoot, CurrentTime);
		}
#endif
#if VarFullScreen
		else
#endif
#if MayNotFullScreen
		{
			Window rr;
			int screen = DefaultScreen(x_display);
			Window rootwin = XRootWindow(x_display, screen);
#if 0
			/*
				This doesn't work right in Red Hat 6, and may not
				be needed anymore, now that using PPosition hint.
			*/
			XMoveWindow(x_display, my_main_wind,
				leftPos, topPos);
				/*
					Needed after XMapRaised, because some window
					managers will apparently ignore where the
					window was asked to be put.
				*/
#endif

			XSync(x_display, 0);
				/*
					apparently, XTranslateCoordinates can be inaccurate
					without this
				*/
			XTranslateCoordinates(x_display, my_main_wind, rootwin,
				0, 0, &SavedTransH, &SavedTransV, &rr);
		}
#endif

#if EnableRecreateW
		if (WantRestoreCursPos) {
#if EnableFSMouseMotion
			if (! HaveMouseMotion)
#endif
			{
				(void) MyMoveMouse(RestoreMouseH, RestoreMouseV);
				WantCursorHidden = trueblnr;
			}
			WantRestoreCursPos = falseblnr;
		}
#endif
	}

#if EnableFSMouseMotion
	if (HaveMouseMotion) {
		MyMouseConstrain();
	}
#endif

	if (RequestMacOff) {
		RequestMacOff = falseblnr;
		if (AnyDiskInserted()) {
			MacMsgOverride(kStrQuitWarningTitle,
				kStrQuitWarningMessage);
		} else {
			ForceMacOff = trueblnr;
		}
	}

	if (ForceMacOff) {
		return;
	}

	if (gTrueBackgroundFlag != gBackgroundFlag) {
		gBackgroundFlag = gTrueBackgroundFlag;
		if (gTrueBackgroundFlag) {
			EnterBackground();
		} else {
			LeaveBackground();
		}
	}

	if (CurSpeedStopped != (SpeedStopped ||
		(gBackgroundFlag && ! RunInBackground
#if EnableAutoSlow && 0
			&& (QuietSubTicks >= 4092)
#endif
		)))
	{
		CurSpeedStopped = ! CurSpeedStopped;
		if (CurSpeedStopped) {
			EnterSpeedStopped();
		} else {
			LeaveSpeedStopped();
		}
	}

#if MayFullScreen
	if (gTrueBackgroundFlag
#if VarFullScreen
		&& WantFullScreen
#endif
		)
	{
		/*
			Since often get here on Ubuntu Linux 5.10
			running on a slow machine (emulated) when
			attempt to enter full screen, don't abort
			full screen, but try to fix it.
		*/
#if 0
		ToggleWantFullScreen();
#else
		XRaiseWindow(x_display, my_main_wind);
		XSetInputFocus(x_display, my_main_wind,
			RevertToPointerRoot, CurrentTime);
#endif
	}
#endif

#if EnableRecreateW
	if (0
#if EnableMagnify
		|| (UseMagnify != WantMagnify)
#endif
#if VarFullScreen
		|| (UseFullScreen != WantFullScreen)
#endif
		)
	{
		(void) ReCreateMainWindow();
	}
#endif


#if MayFullScreen
	if (GrabMachine != (
#if VarFullScreen
		UseFullScreen &&
#endif
		! (gTrueBackgroundFlag || CurSpeedStopped)))
	{
		GrabMachine = ! GrabMachine;
		if (GrabMachine) {
			GrabTheMachine();
		} else {
			UngrabMachine();
		}
	}
#endif

#if IncludeSonyNew
	if (vSonyNewDiskWanted) {
#if IncludeSonyNameNew
		if (vSonyNewDiskName != NotAPbuf) {
			ui3p NewDiskNameDat;
			if (MacRomanTextToNativePtr(vSonyNewDiskName, trueblnr,
				&NewDiskNameDat))
			{
				MakeNewDisk(vSonyNewDiskSize, (char *)NewDiskNameDat);
				free(NewDiskNameDat);
			}
			PbufDispose(vSonyNewDiskName);
			vSonyNewDiskName = NotAPbuf;
		} else
#endif
		{
			MakeNewDiskAtDefault(vSonyNewDiskSize);
		}
		vSonyNewDiskWanted = falseblnr;
			/* must be done after may have gotten disk */
	}
#endif

	if ((nullpr != SavedBriefMsg) & ! MacMsgDisplayed) {
		MacMsgDisplayOn();
	}

	if (NeedWholeScreenDraw) {
		NeedWholeScreenDraw = falseblnr;
		ScreenChangedAll();
	}

#if NeedRequestIthDisk
	if (0 != RequestIthDisk) {
		Sony_InsertIth(RequestIthDisk);
		RequestIthDisk = 0;
	}
#endif

	if (HaveCursorHidden != (WantCursorHidden
		&& ! (gTrueBackgroundFlag || CurSpeedStopped)))
	{
		HaveCursorHidden = ! HaveCursorHidden;
		if (HaveCursorHidden) {
			XDefineCursor(x_display, my_main_wind, blankCursor);
		} else {
			XUndefineCursor(x_display, my_main_wind);
		}
	}
}

/* --- command line parsing --- */

LOCALFUNC blnr ScanCommandLine(void)
{
	char *pa;
	int i = 1;

label_retry:
	if (i < my_argc) {
		pa = my_argv[i++];
		if ('-' == pa[0]) {
			if ((0 == strcmp(pa, "--display"))
				|| (0 == strcmp(pa, "-display")))
			{
				if (i < my_argc) {
					display_name = my_argv[i++];
					goto label_retry;
				}
			} else
			if ((0 == strcmp(pa, "--rom"))
				|| (0 == strcmp(pa, "-r")))
			{
				if (i < my_argc) {
					rom_path = my_argv[i++];
					goto label_retry;
				}
			} else
			if (0 == strcmp(pa, "-n"))
			{
				if (i < my_argc) {
					n_arg = my_argv[i++];
					goto label_retry;
				}
			} else
			if (0 == strcmp(pa, "-d"))
			{
				if (i < my_argc) {
					d_arg = my_argv[i++];
					goto label_retry;
				}
			} else
#ifndef UsingAlsa
#define UsingAlsa 0
#endif

#if UsingAlsa
			if ((0 == strcmp(pa, "--alsadev"))
				|| (0 == strcmp(pa, "-alsadev")))
			{
				if (i < my_argc) {
					alsadev_name = my_argv[i++];
					goto label_retry;
				}
			} else
#endif
#if 0
			if (0 == strcmp(pa, "-l")) {
				SpeedValue = 0;
				goto label_retry;
			} else
#endif
			{
				MacMsg(kStrBadArgTitle, kStrBadArgMessage, falseblnr);
			}
		} else {
			(void) Sony_Insert1(pa, falseblnr);
			goto label_retry;
		}
	}

	return trueblnr;
}

/* --- main program flow --- */

GLOBALOSGLUPROC DoneWithDrawingForTick(void)
{
#if EnableFSMouseMotion
	if (HaveMouseMotion) {
		AutoScrollScreen();
	}
#endif
	MyDrawChangesAndClear();
	XFlush(x_display);
}

GLOBALOSGLUFUNC blnr ExtraTimeNotOver(void)
{
	UpdateTrueEmulatedTime();
	return TrueEmulatedTime == OnTrueTime;
}

LOCALPROC WaitForTheNextEvent(void)
{
	XEvent event;

	XNextEvent(x_display, &event);
	HandleTheEvent(&event);
}

LOCALPROC CheckForSystemEvents(void)
{
	int i = 10;

	while ((XEventsQueued(x_display, QueuedAfterReading) > 0)
		&& (--i >= 0))
	{
		WaitForTheNextEvent();
	}
}

GLOBALOSGLUPROC WaitForNextTick(void)
{
label_retry:
	CheckForSystemEvents();
	CheckForSavedTasks();
	if (ForceMacOff) {
		return;
	}

	if (CurSpeedStopped) {
		DoneWithDrawingForTick();
		WaitForTheNextEvent();
		goto label_retry;
	}

	if (ExtraTimeNotOver()) {
		struct timespec rqt;
		struct timespec rmt;

		si5b TimeDiff = GetTimeDiff();
		if (TimeDiff < 0) {
			rqt.tv_sec = 0;
			rqt.tv_nsec = (- TimeDiff) * 1000;
			(void) nanosleep(&rqt, &rmt);
		}
		goto label_retry;
	}

	if (CheckDateTime()) {
#if MySoundEnabled
		MySound_SecondNotify();
#endif
#if EnableDemoMsg
		DemoModeSecondNotify();
#endif
	}

	if ((! gBackgroundFlag)
#if UseMotionEvents
		&& (! CaughtMouse)
#endif
		)
	{
		CheckMouseState();
	}

	OnTrueTime = TrueEmulatedTime;

#if dbglog_TimeStuff
	dbglog_writelnNum("WaitForNextTick, OnTrueTime", OnTrueTime);
#endif
}

/* --- platform independent code can be thought of as going here --- */

#include "PROGMAIN.h"

LOCALPROC ZapOSGLUVars(void)
{
	InitDrives();
	ZapWinStateVars();
}

LOCALPROC ReserveAllocAll(void)
{
#if dbglog_HAVE
	dbglog_ReserveAlloc();
#endif
	ReserveAllocOneBlock(&ROM, kROM_Size, 5, falseblnr);

	ReserveAllocOneBlock(&screencomparebuff,
		vMacScreenNumBytes, 5, trueblnr);
#if UseControlKeys
	ReserveAllocOneBlock(&CntrlDisplayBuff,
		vMacScreenNumBytes, 5, falseblnr);
#endif
#if WantScalingBuff
	ReserveAllocOneBlock(&ScalingBuff,
		ScalingBuffsz, 5, falseblnr);
#endif
#if WantScalingTabl
	ReserveAllocOneBlock(&ScalingTabl,
		ScalingTablsz, 5, falseblnr);
#endif

#if MySoundEnabled
	ReserveAllocOneBlock((ui3p *)&TheSoundBuffer,
		dbhBufferSize, 5, falseblnr);
#endif

	EmulationReserveAlloc();
}

LOCALFUNC blnr AllocMyMemory(void)
{
	uimr n;
	blnr IsOk = falseblnr;

	ReserveAllocOffset = 0;
	ReserveAllocBigBlock = nullpr;
	ReserveAllocAll();
	n = ReserveAllocOffset;
	ReserveAllocBigBlock = (ui3p)calloc(1, n);
	if (NULL == ReserveAllocBigBlock) {
		MacMsg(kStrOutOfMemTitle, kStrOutOfMemMessage, trueblnr);
	} else {
		ReserveAllocOffset = 0;
		ReserveAllocAll();
		if (n != ReserveAllocOffset) {
			/* oops, program error */
		} else {
			IsOk = trueblnr;
		}
	}

	return IsOk;
}

LOCALPROC UnallocMyMemory(void)
{
	if (nullpr != ReserveAllocBigBlock) {
		free((char *)ReserveAllocBigBlock);
	}
}

#if HaveAppPathLink
LOCALFUNC blnr ReadLink_Alloc(char *path, char **r)
{
	/*
		This should work to find size:

		struct stat r;

		if (lstat(path, &r) != -1) {
			r = r.st_size;
			IsOk = trueblnr;
		}

		But observed to return 0 in Ubuntu 10.04 x86-64
	*/

	char *s;
	int sz;
	char *p;
	blnr IsOk = falseblnr;
	size_t s_alloc = 256;

label_retry:
	s = (char *)malloc(s_alloc);
	if (NULL == s) {
		fprintf(stderr, "malloc failed.\n");
	} else {
		sz = readlink(path, s, s_alloc);
		if ((sz < 0) || (sz >= s_alloc)) {
			free(s);
			if (sz == s_alloc) {
				s_alloc <<= 1;
				goto label_retry;
			} else {
				fprintf(stderr, "readlink failed.\n");
			}
		} else {
			/* ok */
			p = (char *)malloc(sz + 1);
			if (NULL == p) {
				fprintf(stderr, "malloc failed.\n");
			} else {
				(void) memcpy(p, s, sz);
				p[sz] = 0;
				*r = p;
				IsOk = trueblnr;
			}
			free(s);
		}
	}

	return IsOk;
}
#endif

#if HaveSysctlPath
LOCALFUNC blnr ReadKernProcPathname(char **r)
{
	size_t s_alloc;
	char *s;
	int mib[] = {
		CTL_KERN,
		KERN_PROC,
		KERN_PROC_PATHNAME,
		-1
	};
	blnr IsOk = falseblnr;

	if (0 != sysctl(mib, sizeof(mib) / sizeof(int),
		NULL, &s_alloc, NULL, 0))
	{
		fprintf(stderr, "sysctl failed.\n");
	} else {
		s = (char *)malloc(s_alloc);
		if (NULL == s) {
			fprintf(stderr, "malloc failed.\n");
		} else {
			if (0 != sysctl(mib, sizeof(mib) / sizeof(int),
				s, &s_alloc, NULL, 0))
			{
				fprintf(stderr, "sysctl 2 failed.\n");
			} else {
				*r = s;
				IsOk = trueblnr;
			}
			if (! IsOk) {
				free(s);
			}
		}
	}

	return IsOk;
}
#endif

#if CanGetAppPath
LOCALFUNC blnr Path2ParentAndName(char *path,
	char **parent, char **name)
{
	blnr IsOk = falseblnr;

	char *t = strrchr(path, '/');
	if (NULL == t) {
		fprintf(stderr, "no directory.\n");
	} else {
		int par_sz = t - path;
		char *par = (char *)malloc(par_sz + 1);
		if (NULL == par) {
			fprintf(stderr, "malloc failed.\n");
		} else {
			(void) memcpy(par, path, par_sz);
			par[par_sz] = 0;
			{
				int s_sz = strlen(path);
				int child_sz = s_sz - par_sz - 1;
				char *child = (char *)malloc(child_sz + 1);
				if (NULL == child) {
					fprintf(stderr, "malloc failed.\n");
				} else {
					(void) memcpy(child, t + 1, child_sz);
					child[child_sz] = 0;

					*name = child;
					IsOk = trueblnr;
					/* free(child); */
				}
			}
			if (! IsOk) {
				free(par);
			} else {
				*parent = par;
			}
		}
	}

	return IsOk;
}
#endif

/*
FastLZ - Byte-aligned LZ77 compression library
Copyright (C) 2005-2020 Ariya Hidayat <ariya.hidayat@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#define MAX_L2_DISTANCE 8191

LOCALFUNC void fastlz_memmove(uint8_t* dest, const uint8_t* src, uint32_t count) {
	do {
		*dest++ = *src++;
	} while (--count);
}

LOCALFUNC int fastlz2_decompress(const void* input, int length, void* output, int maxout) {
	const uint8_t* ip = (const uint8_t*)input;
	const uint8_t* ip_limit = ip + length;
	const uint8_t* ip_bound = ip_limit - 2;
	uint8_t* op = (uint8_t*)output;
	uint8_t* op_limit = op + maxout;
	uint32_t ctrl = (*ip++) & 31;

	while (1) {
		if (ctrl >= 32) {
			uint32_t len = (ctrl >> 5) - 1;
			uint32_t ofs = (ctrl & 31) << 8;
			const uint8_t* ref = op - ofs - 1;

			uint8_t code;
			if (len == 7 - 1) do {
				if (!(ip <= ip_bound)) return 0;
				code = *ip++;
				len += code;
			} while (code == 255);
			code = *ip++;
			ref -= code;
			len += 3;

			/* match from 16-bit distance */
			if (code == 255)
				if (ofs == (31 << 8)) {
					if (!(ip < ip_bound)) return 0;
					ofs = (*ip++) << 8;
					ofs += *ip++;
					ref = op - ofs - MAX_L2_DISTANCE - 1;
				}

			if (!(op + len <= op_limit)) return 0;
			if (!(ref >= (uint8_t*)output)) return 0;
			fastlz_memmove(op, ref, len);
			op += len;
		} else {
			ctrl++;
			if (!(op + ctrl <= op_limit)) return 0;
			if (!(ip + ctrl <= ip_limit)) return 0;
			memcpy(op, ip, ctrl);
			ip += ctrl;
			op += ctrl;
		}

		if (ip >= ip_limit) break;
		ctrl = *ip++;
	}

	return op - (uint8_t*)output;
}

LOCALFUNC blnr createBlahBlobData(const char *path)
{
	const uint8_t data[] =
		"\x3f\x21\x00\x00\xe0\xff\xff\xff\xf8\x01\x0d\x42\x44\xc7\x4d\xb4\x9f\xe0\x84\xa7"
		"\xfc\x01\x00\x00\x03\x20\x01\x05\x1f\x04\xfa\x00\x00\x1f\x02\x24\x14\x03\x08\x00"
		"\x00\x04\x20\x06\x0f\x14\x04\xe2\x0c\x42\x6c\x61\x68\x42\x6c\x6f\x62\x44\x61\x74"
		"\x61\x20\x12\xe0\x0c\x00\x08\x00\x41\x20\x2a\x20\x18\x40\x03\x20\x40\x33\x15\x03"
		"\xe0\x20\x00\x60\x33\x00\x0a\xe0\x01\x2e\x20\x0f\x00\x0a\x20\x01\xe0\xff\x5d\x00"
		"\x04\x20\x5d\x1f\x03\x80\xe1\xff\x5d\x69\xe0\x95\x00\x63\xfd\xe0\x0a\x00\x24\x09"
		"\x00\x07\x20\x16\x43\x8d\x00\x09\x20\x07\xe0\xc0\x00\xe2\xf7\xf3\x0b\x07\x01\xf8"
		"\x00\xf8\x00\x78\x00\x0e\xe1\xc0\xd0\x40\x8e\xe0\x05\x00\x19\x47\x00\xb3\xff\x33"
		"\xee\x77\xbb\x00\x05\x20\x07\x40\x0b\x00\x01\x20\x04\x00\x25\x20\x0b\x53\xff\x00"
		"\x06\x60\x3f\x1c\x00\xf0\xe0\xc0\xc9\xe0\x2d\x00\xf3\x06\xff\x02\x02\xff\x01\x41"
		"\xf7\x39\x79\x20\x0b\x00\x0e\x59\xef\x00\x20\x59\xf0\x20\x03\x1a\xf1\x00\x42\x22"
		"\x03\x09\x00\x54\x45\x58\x54\x74\x74\x78\x74\x01\x38\x31\x01\xff\xff\x20\x10\x02"
		"\x00\x00\x12\x20\xb1\x1b\x02\x00\x01\x01\x4c\x20\x3a\x08\x00\xe0\x83\xfb\x19\xe0"
		"\x84\x1d\xf8\xe0\x02\x18\xe0\x0f\x00\x00\x1e\x42\x74\xe0\x21\x08\x1f\x0d\x40\x0a"
		"\x08\x02\x07\x44\x65\x73\x6b\x74\x6f\x70\x40\x73\x08\x46\x4e\x44\x52\x45\x52\x49"
		"\x4b\x40\x40\x19\x80\x00\x00\x11\x80\x01\x06\xc0\x20\x3f\x1f\x59\x20\x32\x04\x00"
		"\xc7\x4d\xb5\xad\x3a\xc1\xe1\x1b\xd2\x00\x14\xe0\x05\x73\x00\x0b\x40\x48\x06\x02"
		"\x05\x49\x63\x6f\x6e\x0d\x40\x1f\x71\xc0\x00\xe0\x02\x71\x00\x13\xc0\x13\x80\x00"
		"\x01\x02\x8e\x5b\x16\x20\x6d\x00\xf6\x40\x03\x80\x13\xe0\x14\x00\x00\x1f\x20\xa0"
		"\x1f\xe0\x0d\x00\xe0\x06\x8b\xe0\x63\x00\x07\x01\x8a\x01\x70\x00\xfe\x00\x8a\x75"
		"\xff\x23\xfa\x3a\x07\x00\x01\x40\xa7\x61\xd2\x00\x01\x10\xfb\x04\xef\x00\x80\x60"
		"\x1e\x58\x1b\x00\x02\xbc\x29\x00\xf7\x20\x2e\x20\xbb\x1f\x28\x20\x2f\x08\x88\x00"
		"\x62\x05\x00\x00\x70\x02\xe8\x60\x23\xa0\x00\xf7\x0d\x59\x58\x44\x20\x31\x60\x48"
		"\xe0\x00\x00\xe0\x05\x69\x17\xe0\x00\x16\xe0\x0b\x00\xe0\x23\x55\x08\x08\x55\x6e"
		"\x74\x69\x74\x6c\x65\x64\xe0\x0b\x48\xe0\xff\x21\xcb\x06\xf4\x00\x9e\x00\x68\x61"
		"\xff\x20\xbf\x0a\x02\x20\x01\x65\xea\xe1\x1c\x95\x00\x02\x46\x20\x89\x02\xe4\x07"
		"\x29\x21\xd3\x0f\x08\x00\xe2\x03\x1a\xe0\xff\x86\x00\x02\x62\x00\x38\xe1\x02\xff"
		"\xe1\x03\x8f\x1f\xfb\x00\x4d\xa0\x00\x01\x50\xe5\x40\x03\x00\x46\x30\xdd\xff\x05"
		"\xff\x0c\x00\x02\x11\x04\xe6\xef\x03\x4f\xf1\x19\x9f\xf1\x0a\xb5\x14\xe0\x04\x00"
		"\x51\xad\x80\x10\x51\xbd\x40\x00\x72\x75\x54\x61\xa0\x00\x20\x0f\xff\x63\x20\x24"
		"\x1f\x0b\x0b\x0a\x46\x69\x6e\x64\x65\x72\x20\x31\x2e\x30\x80\x81\xe1\x07\x12\x03"
		"\x7f\x30\x0a\xa8\x20\x19\x00\x1c\x21\x1e\x04\x01\x53\x0f\x54\x52\x20\x20\x0b\x03"
		"\x12\x41\x50\x50\x32\xf0\x32\xc4\x33\x10\x22\x6f\x07\x08\x20\x25\x00\x0c\xa0\x0b"
		"\x03\x21\x8a\x02\x83\x40\x23\xe0\x03\x84\xa0\x00\x1f\xaf\x00\x72\xec\x00\x1a\x32"
		"\xab\x40\x03\x07\x32\xa8\x9f\x65\x72\x00\x12\x09\x20\x14\x15\x01\x9c\x00\x03\x03"
		"\xef\x00\x00\x3c\x60\x17\x01\x01\xea\x6e\x0c\xac\x2d\x64\x0a\x6c\xad\x8c\xff\x17"
		"\xff\x06\x1c\x00\x00\xff\x0b\xff\x06\x2a\x20\xa3\x00\x03\x41\xeb\x1f\x80\x10\x1f"
		"\x01\x4c\xd5\xd1\xd1\xbd\xb8\x81\x89\x95\x99\xbd\xc9\x96\xa2\x0a\xd2\xdc\xce\x45"
		"\x95\x94\x81\x4d\xd2\xb8\xf3\x45\xce\x1f\x4a\xed\x0c\x1f\xad\xdb\xd0\xb5\x90\xe7"
		"\x2f\x45\x65\x64\x2c\x90\xed\x2c\xe6\x5e\x5b\xdd\x48\x18\x49\x63\x85\x25\x46\x1d"
		"\xda\x5b\x1f\x1b\x09\xfd\x68\x1f\x86\x4f\x99\x88\x42\xe4\x2c\x71\x59\x78\x72\xe3"
		"\xc5\x22\x9b\x49\xb3\x32\xca\xa8\xca\xf0\xe9\x96\x01\x67\x4c\x1e\xd4\x85\xca\x7f"
		"\x03\x0a\x15\xe3\x38\xed\x83\xf6\xa3\x18\x2e\x01\xca\x40\x70\x00\x18\x40\x04\x03"
		"\x00\xee\x01\x58\x60\xf2\x60\x22\x08\x1f\x59\x20\x06\x00\x16\xa0\x0f\x02\x10\x00"
		"\x0c\x7f\xff\x04\x9f\x33\xc6\x80\x00\x21\x04\xe1\x02\x19\x03\x00\x71\xc0\xbc\xb4"
		"\x06\x06\x1f\x32\x00\x00\x73\x74\x79\x6c\x20\x1e\x02\x0a\x00\x80\x40\xf3\x20\x00"
		"\x02\x72\x24\xac\x20\x05\xe0\xaa\x00\x40\xe5\x01\x02\x48\x20\x1f\x07\x20\x03\x1f"
		"\x00\x46\xee\xff\xf4\x20\xd9\x20\xd9\x29\x7d\xb3\xb3\x50\x01\x03\x5a\x76\xca\x2a"
		"\x90\x7f\xae\x71\x78\x56\x52\x51\x10\x58\x16\x22\x68\x01\x00\x78\xff\x21\xff\x07"
		"\x2c\xff\x02\xff\x07\x44\x20\xfa\x1f\xff\x01\xff\x07\x38\x1f\x02\x8e\x20\xd9\x3d"
		"\x6e\x2f\x2d\x04\x4a\x2d\xf6\xbd\x67\x7a\x42\xae\x26\xce\x01\xff\xc8\x76\x26\xd0"
		"\x6c\x1f\x12\xff\xca\x3d\x7c\x6c\x03\x01\xff\xcc\xd0\x20\x04\x0f\xce\x76\x30\x01"
		"\xcc\xcc\x12\xff\xd0\x3d\x7c\x02\xcc\xcc\xff\xd2\x40\x13\x1f\x0d\xd4\x76\x31\x11"
		"\x40\x00\x11\xff\xd6\xd0\x32\x01\xff\xd8\x40\x04\x1f\xda\x76\x33\x57\x01\xff\xd0"
		"\x12\xaa\x14\x48\x6e\x26\x12\x1f\xaa\x15\x48\x6e\xca\x12\xa8\xa2\x48\x6e\x01\xff"
		"\xca\x35\x33\x34\x2c\x36\x15\x26\x02\xaa\x14\x60\x18\x11\x47\xed\x01\xff\xba\x57"
		"\x13\xca\x52\x11\xa8\xa5\x37\x12\xff\xb2\x20\xf3\x40\xff\xe0\x06\x00\x0b\x7f\xfe"
		"\x00\x23\x8e\x0d\x80\x03\xc0\x03\xc0\x07\x20\xb0\x07\x06\x00\x00\x60\x0e\x22\xd3"
		"\x1f\x22\x18\x1c\x30\x0c\x7c\x3e\x30\x0c\xfe\x7f\x30\x0d\xf7\xe3\xb0\x0d\xf3\xc1"
		"\xf0\x0d\xe3\xc7\xb0\x0d\x83\xcf\xf0\x0d\xc7\xef\xb0\x1f\x20\x17\x00\x70\x20\x1f"
		"\x18\xb0\x0c\x00\x01\x70\x0e\x00\x02\xb0\x0f\x00\x05\x70\x06\xa8\x2a\xe0\x07\x55"
		"\x55\xe0\x03\xea\xab\xc0\x1f\x40\x53\x40\x5b\xe0\x18\x00\xc0\x7f\x06\xff\xff\xc0"
		"\x07\xff\xff\xe0\x40\x03\x03\x0f\xff\xff\xf0\xe0\x2b\x03\x40\x3b\x40\x03\x40\x0d"
		"\x4b\xe0\x12\x7f\x1f\x40\x1f\xf8\x7f\xfe\x60\x06\xde\x7b\x22\xd9\x1b\xcf\xf9\x87"
		"\xe1\x9f\xf3\xdf\xff\xff\xde\x7b\xc0\x07\xe8\x2b\x75\x56\x7f\xfe\x1f\x00\xf8\x40"
		"\x1f\x02\x7f\xfe\xff\x24\xb3\x18\x40\x17\x01\x1f\xf8\x81\x47\xe2\x01\x47\x04\x00"
		"\x7a\x33\x64\x0b\x21\x38\x97\x34\x03\x49\x43\x4e\x23\x42\xb7\x0d\x69\x63\x73\x40"
		"\x07\x02\x1e\xbf\xb9\xa3\x35\x20\x00\x80\x24\xa1\x06\x37\x43\x01\x83\x58\x20\x0e"
		"\xe2\x0a\xb5\xe0\xff\xff\xff\xff\xff\xff\xff\xff\xff\xab\x00\x02\x23\x00\x1a\x8f"
		"\x13\x4d\xd9\x2e\x0a\x02\x01\x00\x03\x2d\x3c\x4f\x13\x0b\x00\x00\x00\x08\x20\x0a"
		"\x20\x06\x0b\x12\x04\xdc\xed\x00\x5d\x29\xee\x02\x10\x00\x03\x2f\x4f\x02\x14\x20"
		"\x1c\x4f\x0f\x03\x20\x4c\xe0\x21\xaf\x0d\x00\x2f\xcf\x0d\x09\xff\xff\x5a\x00\x04"
		"\x00\x00\x00\x00\x00";
	long buffer1Size = 3960;
	long buffer2Size = 655360;
	uint8_t *buffer1 = (uint8_t *)malloc(buffer1Size);
	uint8_t *buffer2 = (uint8_t *)malloc(buffer2Size);
	FILE *fp;
	long size1, size2;

	size1 = fastlz2_decompress(data, sizeof(data)-1, buffer1, buffer1Size);

	if (size1 == 0) {
		fprintf(stderr, "Unexpected failure! Couldn't construct BlahBlobData.dsk\n");
		return falseblnr;
	}

	size2 = fastlz2_decompress(buffer1, size1, buffer2, buffer2Size);

	if (size2 == 0) {
		fprintf(stderr, "Unexpected failure! Couldn't construct BlahBlobData.dsk\n");
		return falseblnr;
	}

	fp = fopen(path, "wb");

	if (!fp) {
		fprintf(stderr, "Failed to create file: %s\n", path);
		return falseblnr;
	}

	fwrite(buffer2, sizeof(uint8_t), size2, fp);
	fclose(fp);
	free(buffer1);
	free(buffer2);
	
	return trueblnr;
}

LOCALFUNC blnr copyFile(const char *sourceFile, const char *destinationFile)
{
    int sourceFd = open(sourceFile, O_RDONLY);
    if (sourceFd == -1) {
        perror("Failed to open source file");
        return falseblnr;
    }

    int destinationFd = open(destinationFile, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    
    if (destinationFd == -1) {
        perror("Failed to create destination file");
        close(sourceFd);
        return falseblnr;
    }
    
    const int BUF_SIZE = 4096;
    char buffer[BUF_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = read(sourceFd, buffer, BUF_SIZE)) > 0) {
        ssize_t bytesWritten = write(destinationFd, buffer, bytesRead);
        if (bytesWritten == -1) {
            perror("Failed to write to destination file");
            close(sourceFd);
            close(destinationFd);
            return falseblnr;
        }
    }

    if (bytesRead == -1) {
        perror("Failed to read from source file");
        close(sourceFd);
        close(destinationFd);
        return falseblnr;
    }

    close(sourceFd);
    close(destinationFd);
    
    return trueblnr;
}    

LOCALFUNC blnr CreatePrefsDir(void)
{
	char *append;
	char *ptr = NULL;
	char *org = "BlahBlob";
	char *app = "BlahBlob";
	size_t len = 0;
    char *envr = getenv("HOME");

    if (envr == NULL) {
        /* we could take heroic measures with /etc/passwd, but oh well. */
        fprintf(stderr, "HOME environment is not set\n");
        return falseblnr;
    }

    append = "/.local/share/";

    len = strlen(envr);

    if (envr[len - 1] == '/') {
        append += 1;
    }

    len += strlen(append) + strlen(org) + strlen(app) + 3;
    pref_dir = (char *)malloc(len);

    if (pref_dir == NULL) {
        fprintf(stderr, "Out of memory??\n");
        return falseblnr;
    }

	snprintf(pref_dir, len, "%s%s%s/%s/", envr, append, org, app);

    for (ptr = pref_dir + 1; *ptr; ptr++) {
        if (*ptr == '/') {
            *ptr = '\0';
            if (mkdir(pref_dir, 0700) != 0 && errno != EEXIST) {
                goto error;
            }
            *ptr = '/';
        }
    }
    if (mkdir(pref_dir, 0700) != 0 && errno != EEXIST) {
    error:
        fprintf(stderr, "Couldn't create directory '%s': '%s'\n", pref_dir, strerror(errno));
        free(pref_dir);
        pref_dir = NULL;
        return falseblnr;
    }

    return trueblnr;
}

LOCALFUNC blnr fileExists(const char *path)
{
	return access(path, F_OK) == 0;
}

#if CanGetAppPath
LOCALFUNC blnr InitWhereAmI(void)
{
	char *s, *dataPath = NULL, *oldDataPath = NULL;
    char *appimagePath = getenv("APPIMAGE");
    runningAsAppImage = (appimagePath != NULL && strlen(appimagePath) > 0);

	if (!
#if HaveAppPathLink
		ReadLink_Alloc(TheAppPathLink, &s)
#endif
#if HaveSysctlPath
		ReadKernProcPathname(&s)
#endif
		)
	{
		fprintf(stderr, "InitWhereAmI fails.\n");
	} else {
		if (! Path2ParentAndName(s, &app_parent, &app_name)) {
			fprintf(stderr, "Path2ParentAndName fails.\n");
		} else {
			/* ok */
			/*
				fprintf(stderr, "parent = %s.\n", app_parent);
				fprintf(stderr, "name = %s.\n", app_name);
			*/
		}

		free(s);
	}

	if (runningAsAppImage) {
		if (!CreatePrefsDir()) {
			return falseblnr;
		}

        char *appDskSrc, *appDskDest, *appVersionPath;
        int installedVersion = 0;
        
        printf("Detected game is running as AppImage\n");
        
        if (mnvm_noErr != ChildPath(pref_dir, "version.dat", &appVersionPath)) {
            fprintf(stderr, "Unexpected failure! Couldn't create string for path to version.dat\n");
			return falseblnr;
        }
        
        if (fileExists(appVersionPath)) {
            FILE *fp = fopen(appVersionPath, "r");
            
            if (!fp || fscanf(fp, "%d", &installedVersion) != 1) {
                perror("Failed to read appVersion, continuing anyway");
            }
            
            fclose(fp);
        }
        
        if (mnvm_noErr != ChildPath(pref_dir, "BlahBlob.dsk", &appDskDest)) {
            fprintf(stderr, "Unexpected failure! Couldn't create string for path to BlahBlob.dsk\n");
            return falseblnr;
        }
        
        if (!fileExists(appDskDest) || installedVersion < AppVersion) {
            FILE *fp = fopen(appVersionPath, "w");
            if (!fp) {
                fprintf(stderr, "Error: Couldn't write AppVersion to pref directory!\n");
                return falseblnr;
            }
            fprintf(fp, "%d", AppVersion);
            fclose(fp);
            
            if (mnvm_noErr != ChildPath(app_parent, "BlahBlob.dsk", &appDskSrc)) {
                fprintf(stderr, "Unexpected failure! Couldn't create string for path to BlahBlob.dsk\n");
                return falseblnr;
            }
            
            if (copyFile(appDskSrc, appDskDest) != trueblnr) {
                fprintf(stderr, "Couldn't copy BlahBlob.dsk to pref directory!\n");
                return falseblnr;
            }
            
            printf("Copied BlahBlob.dsk to pref directory: %s\n", pref_dir);
            
            MyMayFree(appDskSrc);
        } else {
            printf("BlahBlob.dsk already exists and is late enough version, no need to copy\n");
            printf("Installed version: %d\n", installedVersion);
            printf("Current version: %d\n", AppVersion);
        }
        
        MyMayFree(appDskDest);
        MyMayFree(appVersionPath);
        
        if (mnvm_noErr != ChildPath(pref_dir, "BlahBlobData.dsk", &dataPath)) {
            fprintf(stderr, "Unexpected failure! Couldn't create string for path to BlahBlobData.dsk\n");
            return falseblnr;
        }

        if (mnvm_noErr != ChildPath(pref_dir, "BlahBlob/BlahBlobData.dsk", &oldDataPath)) {
            fprintf(stderr, "Unexpected failure! Couldn't create string for path to old BlahBlobData.dsk\n");
            return falseblnr;
        }

        if (fileExists(oldDataPath)) {
        	printf("Found data at old location at: %s\n", oldDataPath);
        	printf("Moving to: %s\n", dataPath);
        	rename(oldDataPath, dataPath);
        }

        MyMayFree(oldDataPath);

	} else {
        if (mnvm_noErr != ChildPath(app_parent, "BlahBlobData.dsk", &dataPath)) {
            fprintf(stderr, "Unexpected failure! Couldn't create string for path to BlahBlobData.dsk\n");
            return falseblnr;
        }
	}

    if (fileExists(dataPath)) {
    	MyMayFree(dataPath);
        return trueblnr;
    }
    
    if (!createBlahBlobData(dataPath)) {
    	MyMayFree(dataPath);
    	return falseblnr;
    }

    printf("Created empty BlahBlobData.dsk at: %s\n", dataPath);

    MyMayFree(dataPath);

	return trueblnr; /* keep going regardless */
}
#endif

#if CanGetAppPath
LOCALPROC UninitWhereAmI(void)
{
	MyMayFree(app_parent);
	MyMayFree(app_name);
	MyMayFree(pref_dir);
}
#endif

LOCALFUNC blnr InitOSGLU(void)
{
	if (AllocMyMemory())
#if CanGetAppPath
	if (InitWhereAmI())
#endif
#if dbglog_HAVE
	if (dbglog_open())
#endif
	if (ScanCommandLine())
	if (LoadMacRom())
	if (LoadInitialImages())
#if UseActvCode
	if (ActvCodeInit())
#endif
	if (InitLocationDat())
#if MySoundEnabled
	if (MySound_Init())
#endif
	if (Screen_Init())
	if (CreateMainWindow())
	if (KC2MKCInit())
#if EmLocalTalk
	if (EntropyGather())
	if (InitLocalTalk())
#endif
	if (WaitForRom())
	{
		return trueblnr;
	}
	return falseblnr;
}

LOCALPROC UnInitOSGLU(void)
{
	if (MacMsgDisplayed) {
		MacMsgDisplayOff();
	}

#if EmLocalTalk
	UnInitLocalTalk();
#endif

	RestoreKeyRepeat();
#if MayFullScreen
	UngrabMachine();
#endif
#if MySoundEnabled
	MySound_Stop();
#endif
#if MySoundEnabled
	MySound_UnInit();
#endif
#if IncludeHostTextClipExchange
	FreeMyClipBuffer();
#endif
#if IncludePbufs
	UnInitPbufs();
#endif
	UnInitDrives();

	ForceShowCursor();
	if (blankCursor != None) {
		XFreeCursor(x_display, blankCursor);
	}

	if (my_image != NULL) {
		XDestroyImage(my_image);
	}
#if EnableMagnify
	if (my_Scaled_image != NULL) {
		XDestroyImage(my_Scaled_image);
	}
#endif

	CloseMainWindow();
	if (x_display != NULL) {
		XCloseDisplay(x_display);
	}

#if dbglog_HAVE
	dbglog_close();
#endif

#if CanGetAppPath
	UninitWhereAmI();
#endif
	UnallocMyMemory();

	CheckSavedMacMsg();
}

int main(int argc, char **argv)
{
	my_argc = argc;
	my_argv = argv;

	ZapOSGLUVars();
	if (InitOSGLU()) {
		ProgramMain();
	}
	UnInitOSGLU();

	return 0;
}

#endif /* WantOSGLUXWN */
