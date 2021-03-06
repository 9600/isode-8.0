/* rosapinvoke.c - ROPM: invoke */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/rosap/RCS/rosapinvoke.c,v 9.0 1992/06/16 12:37:02 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/rosap/RCS/rosapinvoke.c,v 9.0 1992/06/16 12:37:02 isode Rel $
 *
 * Based on an TCP-based implementation by George Michaelson of University
 * College London.
 *
 *
 * $Log: rosapinvoke.c,v $
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
#include "ROS-types.h"
#include "ropkt.h"

static int  RoInvokeRequestAux ();

/*    RO-INVOKE.REQUEST */

int
RoInvokeRequest (int sd, int op, int class, PE args, int invokeID, int *linkedID, int priority, struct RoSAPindication *roi) {
	SBV	    smask;
	int     result;
	struct assocblk   *acb;

	switch (class) {
	case ROS_SYNC:
	case ROS_ASYNC:
		break;

	default:
		return rosaplose (roi, ROS_PARAMETER, NULLCP,
						  "bad value for class parameter");
	}
	missingP (roi);

	smask = sigioblock ();

	rosapPsig (acb, sd);

	result = RoInvokeRequestAux (acb, op, class, args, invokeID, linkedID,
								 priority, roi);

	sigiomask (smask);

	return result;
}

/*  */

static int
RoInvokeRequestAux (struct assocblk *acb, int op, int class, PE args, int invokeID, int *linkedID, int priority, struct RoSAPindication *roi) {
	PE	pe;

	struct type_ROS_ROSEapdus	papdu;
	struct type_ROS_ROIVapdu	piv;
	struct type_ROS_InvokeIDType pidt;
	struct type_ROS_Operation	prop;

	if (!(acb -> acb_flags & ACB_INIT) && (acb -> acb_flags & ACB_ROS))
		return rosaplose (roi, ROS_OPERATION, NULLCP, "not initiator");
	if (!(acb -> acb_flags & ACB_ACS)) {
		missingP (args);
		if (linkedID)
			return rosaplose (roi, ROS_OPERATION, NULLCP,
							  "linked operations not permitted");
	}

	if (acb -> acb_ready
			&& !(acb -> acb_flags & ACB_TURN)
			&& (*acb -> acb_ready) (acb, priority, roi) == NOTOK)
		return NOTOK;

	if ((acb -> acb_flags & ACB_ACS) == 0) { /* want OPDU */
		struct type_ROS_OPDU	opdu;
		struct	type_ROS_Invoke	s_invoke;

		s_invoke.invokeID = invokeID;
		s_invoke.element_ROS_2 = &prop;
		prop.parm = op;
		s_invoke.argument = args;
		opdu.offset = type_ROS_OPDU_1; 	/* ROS-Invoke */
		opdu.un.choice_ROS_8 = &s_invoke;
		if (encode_ROS_OPDU (&pe, 1, 0, NULLCP, &opdu) == NOTOK)
			return NOTOK;

	} else {
		if (linkedID) {
			piv.optionals = opt_ROS_ROIVapdu_linked__ID;
			piv.linked__ID = *linkedID;
		} else
			piv.optionals = 0;

		piv.invokeID = &pidt;
		pidt.parm = invokeID;
		piv.operation__value = &prop;
		prop.parm = op;
		piv.argument = args;
		papdu.offset = type_ROS_ROSEapdus_roiv__apdu;
		papdu.un.roiv__apdu = &piv;

		if (encode_ROS_ROSEapdus (&pe, 1, 0, NULLCP, &papdu) == NOTOK)
			return NOTOK;
	}

	if ((*acb -> acb_putosdu) (acb, pe, args, priority, roi) == NOTOK)
		return NOTOK;

	return (class == ROS_SYNC
			? (*acb -> acb_rowaitrequest)  (acb, &invokeID, NOTOK, roi)
			: OK);
}
