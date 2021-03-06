/* pe2text.c - write a PE thru a debug filter */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/psap/RCS/pe2text.c,v 9.0 1992/06/16 12:25:44 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/psap/RCS/pe2text.c,v 9.0 1992/06/16 12:25:44 isode Rel $
 *
 *
 * $Log: pe2text.c,v $
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
#include "psap.h"
#include "logger.h"

/*  */

/* logfile-backed abstract for PStreams */

/* ARGSUSED */

static int
ll_pswrite (PS ps, PElementData data, PElementLen n, int in_line) {
	LLog    *lp = (LLog *) ps -> ps_addr;

	if (lp -> ll_stat & LLOGTTY) {
		fflush (stdout);

		fwrite ((char *) data, sizeof *data, (int) n, stderr);
		fflush (stderr);
	}

	if (lp -> ll_fd == NOTOK) {
		if ((lp -> ll_stat & (LLOGERR | LLOGTTY)) == (LLOGERR | LLOGTTY))
			return ((int) n);
		if (ll_open (lp) == NOTOK)
			return NOTOK;
	} else if (ll_check (lp) == NOTOK)
		return NOTOK;

	return write (lp -> ll_fd, (char *) data, (int) n);
}

/*  */

static int
ll_psopen (PS ps) {
	ps -> ps_writeP = ll_pswrite;

	return OK;
}

#define	ll_psetup(ps, lp)	((ps) -> ps_addr = (caddr_t) (lp), OK)

/*  */

void
pe2text (LLog *lp, PE pe, int rw, int cc) {
	char   *bp;
	char   buffer[BUFSIZ];
	PS ps;

	bp = buffer;
	sprintf (bp, "%s PE", rw ? "read" : "wrote");
	bp += strlen (bp);
	if (pe -> pe_context != PE_DFLT_CTX) {
		sprintf (bp, ", context %d", pe -> pe_context);
		bp += strlen (bp);
	}
	if (cc != NOTOK) {
		sprintf (bp, ", length %d", cc);
		bp += strlen (bp);
	}
	LLOG (lp, LLOG_ALL, ("%s", buffer));

	if ((ps = ps_alloc (ll_psopen)) != NULLPS) {
		if (ll_psetup (ps, lp) != NOTOK)
			pe2pl (ps, pe);

		ps_free (ps);
	}

	ll_printf (lp, "-------\n");

	ll_sync (lp);
}
