/* tsaprespond.c - TPM: responder */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/tsap/RCS/tsaprespond.c,v 9.0 1992/06/16 12:40:39 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/tsap/RCS/tsaprespond.c,v 9.0 1992/06/16 12:40:39 isode Rel $
 *
 *
 * $Log: tsaprespond.c,v $
 * Revision 9.0  1992/06/16  12:40:39  isode
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
#include "tpkt.h"
#include "tailor.h"

#ifdef X25
#include "x25.h"
#endif

/*    T-CONNECT.INDICATION */

int
TInit (int vecp, char **vec, struct TSAPstart *ts, struct TSAPdisconnect *td) {
	struct tsapblk *tb;

	isodetailor (NULLCP, 0);

	if (vecp < 3)
		return tsaplose (td, DR_PARAMETER, NULLCP,
						 "bad initialization vector");
	missingP (vec);
	missingP (ts);
	missingP (td);

	if ((tb = newtblk ()) == NULL)
		return tsaplose (td, DR_CONGEST, NULLCP, "out of memory");

	vec += vecp - 2;
	switch (*vec[0]) {
	case NT_TCP:
#ifdef	TCP
		if (tcprestore (tb, vec[0] + 1, td) == NOTOK)
			goto out;
		break;
#else
		goto not_supported;
#endif

	case NT_X25:
#ifdef	X25
		if (x25restore (tb, vec[0] + 1, td) == NOTOK)
			goto out;
		break;
#else
		goto not_supported;
#endif


	case NT_X2584:
#ifdef AEF_NSAP
		if (x25nsaprestore (tb, vec[0] + 1, td) == NOTOK)
			goto out;
		break;
#else
		goto not_supported;
#endif

	case NT_BSD:
#ifdef	BSD_TP4
		if (tp4restore (tb, vec[0] + 1, td) == NOTOK)
			goto out;
		break;
#else
		goto not_supported;
#endif

	case NT_SUN:
#ifdef	SUN_TP4
		if (tp4restore (tb, vec[0] + 1, td) == NOTOK)
			goto out;
		break;
#else
		goto not_supported;
#endif
	case NT_TLI:
#ifdef TLI_TP
		if (tp4restore (tb, vec[0] + 1, td) == NOTOK)
			goto out;
		break;
#else
		goto not_supported;
#endif
	case NT_XTI:
#ifdef XTI_TP
		if (tp4restore (tb, vec[0] + 1, td) == NOTOK)
			goto out;
		break;
#else
		goto not_supported;
#endif

	default:
		tsaplose (td, DR_PARAMETER, NULLCP,
				  "unknown network type: 0x%x (%c)", *vec[0], *vec[0]);
		goto out;
	}
	bzero (vec[0], strlen (vec[0]));

	if ((*tb -> tb_startPfnx) (tb, vec[1], ts, td) == NOTOK)
		goto out;
	bzero (vec[1], strlen (vec[1]));

	*vec = NULL;

	return OK;

not_supported:
	;
	tsaplose (td, DR_PARAMETER, NULLCP,
			  "not configured for network type: 0x%x (%c)",
			  *vec[0], *vec[0]);

out:
	;
	freetblk (tb);

	return NOTOK;
}

/*    T-CONNECT.RESPONSE */

int
TConnResponse (int sd, struct TSAPaddr *responding, int expedited, char *data, int cc, struct QOStype *qos, struct TSAPdisconnect *td) {
	int	    result;
	struct tsapblk *tb;
	struct tsapADDR tas;

	if ((tb = findtblk (sd)) == NULL || (tb -> tb_flags & TB_CONN))
		return tsaplose (td, DR_PARAMETER, NULLCP, "invalid transport descriptor");
#ifdef	notdef
	missingP (responding);
#endif
	if (responding) {
		copyTSAPaddrY (responding, &tas);
		if (bcmp ((char *) &tb -> tb_responding, (char *) &tas, sizeof tas))
			tb -> tb_responding = tas;	/* struct copy */
		else
			responding = NULLTA;
	}
	if (expedited && !(tb -> tb_flags & TB_EXPD))
		return tsaplose (td, DR_PARAMETER, NULLCP,
						 "expedited service not available");
	toomuchP (data, cc, TC_SIZE, "initial");
#ifdef	notdef
	missingP (qos);
#endif
	missingP (td);

	if (!expedited)
		tb -> tb_flags &= ~TB_EXPD;

	if ((result = (*tb -> tb_acceptPfnx) (tb, responding ? 1 : 0, data, cc,
										  qos, td)) == NOTOK)
		freetblk (tb);
#ifdef	X25
	else if (tb -> tb_flags & TB_X25)
		LLOG (x25_log, LLOG_NOTICE,
			  ("connection %d from %s",
			   sd, na2str (&tb -> tb_initiating.ta_addr)));
#endif

	return result;
}
