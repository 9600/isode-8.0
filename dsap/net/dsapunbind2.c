/* dsapunbind2.c - DSAP: maps D-UNBIND mapping onto RO-UNBIND */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/dsap/net/RCS/dsapunbind2.c,v 9.0 1992/06/16 12:14:05 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/dsap/net/RCS/dsapunbind2.c,v 9.0 1992/06/16 12:14:05 isode Rel $
 *
 *
 * $Log: dsapunbind2.c,v $
 * Revision 9.0  1992/06/16  12:14:05  isode
 * Release 8.0
 *
 */

/*
 *                                NOTICE
 *
 *    Acquisition, use, and distribution of this module and related
 *    materials are subject to the restrictions of a license agreement.
 *    Consult the Preface in the User's Manual for the full terms of
 *    this agreement.
 *
 */


/* LINTLIBRARY */

#include "quipu/dsap.h"

/*    D-UNBIND.ACCEPT */

/* ARGSUSED */

int
DUnBindAccept (int sd, struct DSAPindication *di) {
	int			  result;
	struct RoNOTindication	  rni_s;
	struct RoNOTindication	* rni = &(rni_s);

	watch_dog ("RoUnBindResult");
	result = RoUnBindResult (sd, NULLPE, rni);
	watch_dog_reset();

	if (result == NOTOK) {
		ronot2dsaplose (di, "D-UNBIND.ACCEPT", rni);
		return (NOTOK);
	}

	return (result);
}

/*    D-UNBIND.REJECT */

/* ARGSUSED */

int
DUnBindReject (int sd, int status, int reason, struct DSAPindication *di) {
	int			  result;
	struct RoNOTindication	  rni_s;
	struct RoNOTindication	* rni = &(rni_s);

	watch_dog ("DUnBindReject");
	result = RoUnBindReject (sd, status, reason, rni);
	watch_dog_reset ();

	if (result == NOTOK) {
		ronot2dsaplose (di, "D-UNBIND.REJECT", rni);
		return (NOTOK);
	}

	return (result);
}

