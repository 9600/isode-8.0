/* gtime.c - inverse gmtime */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/psap/RCS/gtime.c,v 9.0 1992/06/16 12:25:44 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/psap/RCS/gtime.c,v 9.0 1992/06/16 12:25:44 isode Rel $
 *
 *
 * $Log: gtime.c,v $
 * Revision 9.0  1992/06/16  12:25:44  isode
 * Release 8.0
 *
 */

/*
 *				  NOTICE
 *
 *    Acquisition, use, and distribution of this module and related
 *    materials are subject to the restrictions of a license agreement.
 *    Consult the Preface in the User's Manual for the full terms of
 *    this agreement.
 *
 */


/* LINTLIBRARY */

#include <stdio.h>
#include <time.h>
#include "psap.h"
#ifdef	OSX
#include <sys/time.h>
#endif
#ifdef	notdef
#include <sys/timeb.h>
#endif

/*    DATA */

/* gtime(): the inverse of localtime().
	This routine was supplied by Mike Accetta at CMU many years ago.
 */

#ifdef masscomp
extern int	dmsize[];
#else
int	dmsize[] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
#endif

#define	dysize(y)	\
	(((y) % 4) ? 365 : (((y) % 100) ? 366 : (((y) % 400) ? 365 : 366)))

#define	YEAR(y)		((y) >= 100 ? (y) : (y) + 1900)

/*  */

long
gtime (struct tm *tm) {
	int    i,
		   sec,
		   mins,
		   hour,
		   mday,
		   mon,
		   year;
	long   result;
#ifdef	notdef
	long    local;
	struct timeb    tb;
#endif

	if ((sec = tm -> tm_sec) < 0 || sec > 59
			|| (mins = tm -> tm_min) < 0 || mins > 59
			|| (hour = tm -> tm_hour) < 0 || hour > 24
			|| (mday = tm -> tm_mday) < 1 || mday > 31
			|| (mon = tm -> tm_mon + 1) < 1 || mon > 12)
		return ((long) NOTOK);
	if (hour == 24) {
		hour = 0;
		mday++;
	}
	year = YEAR (tm -> tm_year);

	result = 0L;
	for (i = 1970; i < year; i++)
		result += dysize (i);
	if (dysize (year) == 366 && mon >= 3)
		result++;
	while (--mon)
		result += dmsize[mon - 1];
	result += mday - 1;
	result = 24 * result + hour;
	result = 60 * result + mins;
	result = 60 * result + sec;

#ifdef	notdef
	ftime (&tb);
	result += 60 * tb.timezone;
	local = result;
	if ((tm = localtime (&local)) && tm -> tm_isdst)
		result -= 60 * 60;
#endif

	return result;
}
