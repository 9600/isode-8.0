/* rosapselect.c - ROPM: map descriptors */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/rosap/RCS/rosapselect.c,v 9.0 1992/06/16 12:37:02 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/rosap/RCS/rosapselect.c,v 9.0 1992/06/16 12:37:02 isode Rel $
 *
 * Based on an TCP-based implementation by George Michaelson of University
 * College London.
 *
 *
 * $Log: rosapselect.c,v $
 * Revision 9.0  1992/06/16  12:37:02  isode
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
#include <signal.h>
#include "ropkt.h"

/*    map association descriptors for select() */

/* ARGSUSED */

int
RoSelectMask (int sd, fd_set *mask, int *nfds, struct RoSAPindication *roi) {
	SBV	    smask;
	int     result;
	struct assocblk   *acb;

	missingP (mask);
	missingP (nfds);
	missingP (roi);

	smask = sigioblock ();

	rosapPsig (acb, sd);

	if (acb -> acb_apdu || (acb -> acb_flags & ACB_CLOSING)) {
		sigiomask (smask);
		return rosaplose (roi, ROS_WAITING, NULLCP, NULLCP);
	}

	result = (*acb -> acb_roselectmask) (acb, mask, nfds, roi);

	sigiomask (smask);

	return result;
}
