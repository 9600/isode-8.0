/* acsapabort2.c - ACPM: interpret abort */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/acsap/RCS/acsapabort2.c,v 9.0 1992/06/16 12:05:59 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/acsap/RCS/acsapabort2.c,v 9.0 1992/06/16 12:05:59 isode Rel $
 *
 *
 * $Log: acsapabort2.c,v $
 * Revision 9.0  1992/06/16  12:05:59  isode
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
#include "ACS-types.h"
#define	ACSE
#include "acpkt.h"
#ifdef	DEBUG
#include "tailor.h"
#endif

/*    handle P-{U,P}-ABORT.INDICATION */

int
AcABORTser (int sd, struct PSAPabort *pa, struct AcSAPindication *aci) {
	SBV	    smask;
	int	    result;
	struct assocblk *acb;

	missingP (pa);
	missingP (aci);

	smask = sigioblock ();

	if ((acb = findacblk (sd)) == NULL) {
		sigiomask (smask);
		return acsaplose (aci, ACS_PARAMETER, NULLCP,
						  "invalid association descriptor");
	}

	result = ps2acsabort (acb, pa, aci);

	sigiomask (smask);

	return result;
}

/*  */

int
ps2acsabort (struct assocblk *acb, struct PSAPabort *pa, struct AcSAPindication *aci) {
	int	    result;
	PE	    pe;
	struct AcSAPabort *aca = &aci -> aci_abort;
	struct type_ACS_ACSE__apdu *pdu;
	struct type_ACS_ABRT__apdu *abrt;

	result = OK;
	pdu = NULL;

	if (!pa -> pa_peer) {
		if (PC_FATAL (pa -> pa_reason))
			acb -> acb_fd = NOTOK;

		ps2acslose (acb, aci, NULLCP, pa);
		goto out;
	}

	if (pa -> pa_ninfo == 0) {
		acsaplose (aci, ACS_ABORTED, NULLCP, NULLCP);
		if (acb -> acb_sversion == 1)
			aca -> aca_source = ACA_PROVIDER;
		goto out;
	}

	bzero ((char *) aci, sizeof *aci);
	aci -> aci_type = ACI_ABORT;

	if (acb -> acb_sversion == 1) {
		int	i;

		aca -> aca_reason = ACS_ABORTED;
		aca -> aca_source = ACA_USER;
		if ((i = pa -> pa_ninfo) > NACDATA)
			i = NACDATA;
		aca -> aca_ninfo = i;
		while (i-- > 0) {
			aca -> aca_info[i] = pa -> pa_info[i];
			pa -> pa_info[i] = NULLPE;
		}
		goto out;
	}

	result = decode_ACS_ACSE__apdu (pe = pa -> pa_info[0], 1, NULLIP, NULLVP,
									&pdu);

#ifdef	DEBUG
	if (result == OK && (acsap_log -> ll_events & LLOG_PDUS))
		pvpdu (acsap_log, print_ACS_ACSE__apdu_P, pe, "ACSE-apdu", 1);
#endif

	pe_free (pe);
	pe = pa -> pa_info[0] = NULLPE;

	if (result == NOTOK) {
		acsaplose (aci, ACS_PROTOCOL, NULLCP, "%s", PY_pepy);
		goto out;
	}

	if (pdu -> offset != type_ACS_ACSE__apdu_abrt) {
		result = acsaplose (aci, ACS_PROTOCOL, NULLCP,
							"unexpected PDU %d on P-U-ABORT", pdu -> offset);
		goto out;
	}

	abrt = pdu -> un.abrt;
	aca -> aca_reason = ACS_ABORTED;
	aca -> aca_source = abrt -> abort__source;
	apdu2info (acb, aci, abrt -> user__information, aca -> aca_info,
			   &aca -> aca_ninfo);

out:
	;
	acb -> acb_fd = NOTOK;
	PAFREE (pa);
	if (pdu)
		free_ACS_ACSE__apdu (pdu);

	return result;
}
