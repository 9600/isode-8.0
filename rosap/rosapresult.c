/* rosapresult.c - ROPM: result */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/rosap/RCS/rosapresult.c,v 9.0 1992/06/16 12:37:02 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/rosap/RCS/rosapresult.c,v 9.0 1992/06/16 12:37:02 isode Rel $
 *
 * Based on an TCP-based implementation by George Michaelson of University
 * College London.
 *
 *
 * $Log: rosapresult.c,v $
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

static int  RoResultRequestAux ();

/*    RO-RESULT.REQUEST */

int
RoResultRequest (int sd, int invokeID, int op, PE result, int priority, struct RoSAPindication *roi) {
	SBV	    smask;
	int     status;
	struct assocblk   *acb;

	missingP (roi);

	smask = sigioblock ();

	rosapPsig (acb, sd);

	status = RoResultRequestAux (acb, invokeID, op, result, priority, roi);

	sigiomask (smask);

	return status;
}

/*  */

static int
RoResultRequestAux (struct assocblk *acb, int invokeID, int op, PE result, int priority, struct RoSAPindication *roi) {
	PE pe,
	p,
	q;

	if ((acb -> acb_flags & ACB_INIT) && (acb -> acb_flags & ACB_ROS))
		return rosaplose (roi, ROS_OPERATION, NULLCP, "not responder");
	if (!(acb -> acb_flags & ACB_ACS)) {
		missingP (result);
	}

	if (acb -> acb_ready
			&& !(acb -> acb_flags & ACB_TURN)
			&& (*acb -> acb_ready) (acb, priority, roi) == NOTOK)
		return NOTOK;

#ifdef notyet
	if (!(acb -> acb_flags & ACB_ACS)) { /* want OPDU */
		struct type_ROS_OPDU opdu;
		struct type_ROS_ReturnResult rrs;
		struct type_ROS_InvokeIDType idtyp;

		opdu.offset = type_ROS_OPDU_2;
		opdu.un.choice_ROS_9 = &rrs;
		rrs.invokeID = &idtyp;
		rrs.result = result;
		idtyp.parm = invokeID;

		if (encode_ROS_OPDU(&pe, 1, 0, NULL, &opdu) == NOTOK) {
			abort();
			goto fail;
		}
	} else {
		struct type_ROS_ROSEapdus apdu;
		struct type_ROS_RORSapdu ras;
		struct type_ROS_InvokeIDType idtyp;
		struct element_ROS_1 el1;
		struct type_ROS_Operation ops;

		apdu.offset = type_ROS_ROSEapdus_rors__apdu;
		apdu.un.rors__apdu = &ras;
		idtyp.parm = invokeID;
		ras.invokeID = &idtyp;
		if (result) {
			ras.element_ROS_0 = &el1;
			el1.operation__value = &ops;
			ops.parm = op;
			el1.result = result;
		} else
			ras.element_ROS_0 = (struct element_ROS_1 *)0;

		if (encode_ROS_ROSEapdus(&pe, 1, 0, NULL, &apdu) == NOTOK) {
fail:
			if (pe) {
				pe_extract (pe, result);
				pe_free (pe);
			}
			freeacblk (acb);
			return rosaplose (roi, ROS_CONGEST, NULLCP, "out of memory");
		}
	}

#endif

	/* begin Result APDU */
	if ((pe = pe_alloc (PE_CLASS_CONT, PE_FORM_CONS, APDU_RESULT)) == NULLPE
			|| ((acb -> acb_flags & ACB_ACS)
				? (p = pe, 0)
				: set_add (pe, p = pe_alloc (PE_CLASS_UNIV, PE_FORM_CONS,
								   PE_CONS_SEQ)) == NOTOK)
			|| seq_add (p, int2prim (invokeID), -1) == NOTOK
			|| ((acb -> acb_flags & ACB_ACS)
				? (result
				   && (seq_add (p, q = pe_alloc (PE_CLASS_UNIV,
									   PE_FORM_CONS,
									   PE_CONS_SEQ),
								-1) == NOTOK
					   || seq_add (q, int2prim (op), -1) == NOTOK
					   || seq_add (q, result, -1) == NOTOK))
				: seq_add (p, result, -1) == NOTOK)) {
		if (pe) {
			pe_extract (pe, result);
			pe_free (pe);
		}
		freeacblk (acb);
		return rosaplose (roi, ROS_CONGEST, NULLCP, "out of memory");
	}
	/* end Result APDU */

	return (*acb -> acb_putosdu) (acb, pe, result, priority, roi);
}
