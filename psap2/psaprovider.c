/* psaprovider.c - implement the presentation protocol */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/psap2/RCS/psaprovider.c,v 9.0 1992/06/16 12:29:42 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/psap2/RCS/psaprovider.c,v 9.0 1992/06/16 12:29:42 isode Rel $
 *
 *
 * $Log: psaprovider.c,v $
 * Revision 9.0  1992/06/16  12:29:42  isode
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
#include "PS-types.h"
#include "ppkt.h"
#include "tailor.h"

/*    DATA */

static int  once_only = 0;
static struct psapblk psapque;
static struct psapblk *PHead = &psapque;


struct pair preq_pairs[] = {
	PR_MANAGEMENT, bit_PS_Presentation__requirements_context__management,
	PR_RESTORATION, bit_PS_Presentation__requirements_restoration,
	0, 0
};


struct pair sreq_pairs[] = {
	SR_HALFDUPLEX, bit_PS_User__session__requirements_half__duplex,
	SR_DUPLEX, bit_PS_User__session__requirements_duplex,
	SR_EXPEDITED, bit_PS_User__session__requirements_expedited__data,
	SR_MINORSYNC, bit_PS_User__session__requirements_minor__synchronize,
	SR_MAJORSYNC, bit_PS_User__session__requirements_major__synchronize,
	SR_RESYNC, bit_PS_User__session__requirements_resynchronize,
	SR_ACTIVITY, bit_PS_User__session__requirements_activity__management,
	SR_NEGOTIATED, bit_PS_User__session__requirements_negotiated__release,
	SR_CAPABILITY, bit_PS_User__session__requirements_capability__data,
	SR_EXCEPTIONS, bit_PS_User__session__requirements_exceptions,
	SR_TYPEDATA, bit_PS_User__session__requirements_typed__data,
	0, 0
};

/*  */

#define	doABORT		ss2psabort


static int	DATAser (), TOKENser (), SYNCser (), ACTIVITYser (), REPORTser (),
		FINISHser (), ABORTser ();


static int  PReadRequestAux ();
static int  doDATA ();
static int  doTOKEN ();
static int  doSYNC ();
static int  doACTIVITY ();
static int  doREPORT ();
static int  doFINISH ();

/*    P-[*-]DATA.REQUEST */

int
PDataRequest (int sd, PE *data, int ndata, struct PSAPindication *pi) {
	return PDataRequestAux (sd, data, ndata, pi, "user", SDataRequest,
							"SDataRequest", "P-DATA user-data", PPDU_TD);
}

/*  */

int
PDataRequestAux (int sd, PE *data, int ndata, struct PSAPindication *pi, char *dtype, IFP sfunc, char *stype, char *text, int ppdu) {
	SBV	    smask;
	int     i,
			len,
			result;
	char   *base,
		   *realbase;
	struct psapblk *pb;
	struct SSAPindication   sis;
	struct SSAPabort  *sa = &sis.si_abort;
	PE	   *d,
	 p;

	missingP (data);
	toomuchP (data, ndata, NPDATA, dtype);
	if (ndata <= 0)
		return psaplose (pi, PC_PARAMETER, NULLCP,
						 "illegal number of PDVs (%d)", ndata);
	missingP (pi);
	missingP (sfunc);
	missingP (stype);
	missingP (text);

	smask = sigioblock ();

	psapPsig (pb, sd);

	if (ppdu == PPDU_TE) {
		for (d = data, i = 0; i < ndata; i++)
			if ((p = *d++) && p -> pe_context != PE_DFLT_CTX) {
				sigiomask (smask);
				return psaplose (pi, PC_OPERATION, NULLCP,
								 "defined context not permited with expedited service");
			}
	}

	if (ppdu == PPDU_TTD && !(pb -> pb_urequirements & SR_TYPEDATA)) {
		sigiomask (smask);
		return psaplose (pi, PC_OPERATION, NULLCP,
						 "typed data service unavailable");
	}

	if ((result = info2ssdu (pb, pi, data, ndata, &realbase, &base, &len, text,
							 ppdu)) != OK)
		goto out2;

	if ((result = (*sfunc) (sd, base, len, &sis)) == NOTOK)
		if (SC_FATAL (sa -> sa_reason))
			ss2pslose (pb, pi, stype, sa);
		else {
			ss2pslose (NULLPB, pi, stype, sa);
			goto out1;
		}

out2:
	;
	if (result == NOTOK)
		freepblk (pb);
	else if (result == DONE)
		result = NOTOK;
out1:
	;
	if (realbase)
		free (realbase);
	else if (base)
		free (base);

	sigiomask (smask);

	return result;
}

/*    P-READ.REQUEST (pseudo) */

int
PReadRequest (int sd, struct PSAPdata *px, int secs, struct PSAPindication *pi) {
	SBV	    smask;
	int     result;
	struct psapblk *pb;

	missingP (px);
	missingP (pi);

	smask = sigioblock ();

	psapPsig (pb, sd);

	result = PReadRequestAux (pb, px, secs, pi);

	sigiomask (smask);

	return result;
}

/*  */

static int
PReadRequestAux (struct psapblk *pb, struct PSAPdata *px, int secs, struct PSAPindication *pi) {
	int	    result;
	struct SSAPdata sxs;
	struct SSAPdata   *sx = &sxs;
	struct SSAPindication   sis;
	struct SSAPindication *si = &sis;

	bzero ((char *) px, sizeof *px);
	bzero ((char *) pi, sizeof *pi);

	switch (result = SReadRequest (pb -> pb_fd, sx, secs, si)) {
	case NOTOK:
		return doABORT (pb, &si -> si_abort, pi);

	case OK:
		return doDATA (pb, sx, px, pi);

	case DONE:
		switch (si -> si_type) {
		case SI_TOKEN:
			return doTOKEN (pb, &si -> si_token, pi);

		case SI_SYNC:
			return doSYNC (pb, &si -> si_sync, pi);

		case SI_ACTIVITY:
			return doACTIVITY (pb, &si -> si_activity, pi);

		case SI_REPORT:
			return doREPORT (pb, &si -> si_report, pi);

		case SI_FINISH:
			return doFINISH (pb, &si -> si_finish, pi);

		default:
			ppktlose (pb, pi, PC_PROTOCOL, PPDU_NONE,
					  NULLCP,
					  "unknown indication (0x%x) from session",
					  si -> si_type);
			break;
		}
		break;

	default:
		ppktlose (pb, pi, PC_PROTOCOL, PPDU_NONE, NULLCP,
				  "unexpected return from SReadRequest=%d", result);
		break;
	}

	freepblk (pb);
	return NOTOK;
}

/*  */

static int
doDATA (struct psapblk *pb, struct SSAPdata *sx, struct PSAPdata *px, struct PSAPindication *pi) {
	int     ppdu,
			result;
	char   *text;

	switch (px -> px_type = sx -> sx_type) {
	case SX_NORMAL:
		ppdu = PPDU_TD;
		text = "P-DATA user-data";
		break;

	case SX_EXPEDITED:
		ppdu = PPDU_TE;
		text = "P-EXPEDITED-DATA user-data";
		break;

	case SX_CAPDIND:
		ppdu = PPDU_TC;
		goto capd;
	case SX_CAPDCNF:
		ppdu = PPDU_TCC;
capd:
		;
		text = "P-CAPABILITY-DATA user-data";
		break;

	case SX_TYPED:
		ppdu = PPDU_TTD;
		text = "P-TYPED-DATA user-data";
		break;

	default:
		result = ppktlose (pb, pi, PC_PROTOCOL, PPDU_NONE, NULLCP,
						   "unknown data indication type=0x%x, %d bytes",
						   sx -> sx_type, sx -> sx_cc);
		freepblk (pb);
		goto out;
	}

	result = qbuf2info (pb, pi, &sx -> sx_qbuf, sx -> sx_cc,
						px -> px_info, &px -> px_ninfo, text, ppdu);

out:
	;
	if (result == NOTOK)
		SXFREE (sx);

	return result;
}

/*  */

static int
doTOKEN (struct psapblk *pb, struct SSAPtoken *st, struct PSAPindication *pi) {
	int	    result;
	struct PSAPtoken  *pt = &pi -> pi_token;

	pi -> pi_type = PI_TOKEN;

	pt -> pt_type = st -> st_type;
	pt -> pt_tokens = st -> st_tokens;
	pt -> pt_owned = pb -> pb_owned = st -> st_owned;

	result = ssdu2info (pb, pi, st -> st_data, st -> st_cc, pt -> pt_info,
						&pt -> pt_ninfo, "P-PLEASE-TOKEN user-data", PPDU_NONE);

	STFREE (st);

	return (result != NOTOK ? DONE : NOTOK);
}

/*  */

static int
doSYNC (struct psapblk *pb, struct SSAPsync *sn, struct PSAPindication *pi) {
	int	    result;
	struct PSAPsync   *pn = &pi -> pi_sync;

	pi -> pi_type = PI_SYNC;

	pn -> pn_type = sn -> sn_type;
	pn -> pn_options = sn -> sn_options;
	pn -> pn_ssn = sn -> sn_ssn;
	pn -> pn_settings = sn -> sn_settings;

	result = ssdu2info (pb, pi, sn -> sn_data, sn -> sn_cc, pn -> pn_info,
						&pn -> pn_ninfo, sn -> sn_type <= SN_MAJORCNF
						? "P-MAJOR-SYNC user-data"
						: sn -> sn_type <= SN_MINORCNF
						? "P-MINOR-SYNC user-data"
						: "P-RESYNCHRONIZE user-data",
						sn -> sn_type == SN_RESETIND
						? PPDU_RS
						: sn -> sn_type == SN_RESETCNF
						? PPDU_RSA
						: PPDU_NONE);

	SNFREE (sn);

	return (result != NOTOK ? DONE : NOTOK);
}

/*  */

static int
doACTIVITY (struct psapblk *pb, struct SSAPactivity *sv, struct PSAPindication *pi) {
	int	    result;
	struct PSAPactivity   *pv = &pi -> pi_activity;

	pi -> pi_type = PI_ACTIVITY;

	pv -> pv_type = sv -> sv_type;
	pv -> pv_id = sv -> sv_id;			/* struct copy */
	pv -> pv_oid = sv -> sv_oid;		/* struct copy */
	pv -> pv_connect = sv -> sv_connect;	/* struct copy */
	pv -> pv_ssn = sv -> sv_ssn;
	pv -> pv_reason = sv -> sv_reason;

	result = ssdu2info (pb, pi, sv -> sv_data, sv -> sv_cc, pv -> pv_info,
						&pv -> pv_ninfo, sv -> sv_type <= SV_START
						? "P-ACTIVITY-START user-data"
						: sv -> sv_type <= SV_RESUME
						? "P-ACTIVITY-RESUME user-data"
						: "P-ACTIVITY-END user-data", PPDU_NONE);

	SVFREE (sv);

	return (result != NOTOK ? DONE : NOTOK);
}

/*  */

static int
doREPORT (struct psapblk *pb, struct SSAPreport *sp, struct PSAPindication *pi) {
	int	    result;
	struct PSAPreport *pp = &pi -> pi_report;

	pi -> pi_type = PI_REPORT;

	pp -> pp_peer = sp -> sp_peer;
	pp -> pp_reason = sp -> sp_reason;

	result = ssdu2info (pb, pi, sp -> sp_data, sp -> sp_cc, pp -> pp_info,
						&pp -> pp_ninfo, "P-U-EXCEPTION-REPORT user-data", PPDU_NONE);

	SPFREE (sp);

	return (result != NOTOK ? DONE : NOTOK);
}

/*  */

static int
doFINISH (struct psapblk *pb, struct SSAPfinish *sf, struct PSAPindication *pi) {
	int	    result;
	struct PSAPfinish *pf = &pi -> pi_finish;

	pi -> pi_type = PI_FINISH;

	result = ssdu2info (pb, pi, sf -> sf_data, sf -> sf_cc, pf -> pf_info,
						&pf -> pf_ninfo, "P-RELEASE user-data", PPDU_NONE);

	SFFREE (sf);

	if (result == NOTOK)
		return NOTOK;

	pb -> pb_flags |= PB_FINN;

	return DONE;
}

/*  */

int
ss2psabort (struct psapblk *pb, struct SSAPabort *sa, struct PSAPindication *pi) {
	int	    result,
			ppdu;
	PE	    pe;
	struct PSAPabort *pa = &pi -> pi_abort;
	struct type_PS_Abort__type *pdu;
	struct element_PS_3 *aru;
	struct type_PS_ARP__PPDU *arp;
	struct type_PS_User__data *info;

	pdu = NULL, pe = NULLPE;
	if (!sa -> sa_peer) {
		if (sa -> sa_reason == SC_TIMER)
			return psaplose (pi, PC_TIMER, NULLCP, NULLCP);

		ss2pslose (pb, pi, NULLCP, sa);
		goto out;
	}

	if (sa -> sa_cc == 0) {
		psaplose (pi, PC_ABORTED, NULLCP, NULLCP);
		goto out;
	}

	bzero ((char *) pi, sizeof *pi);
	pi -> pi_type = PI_ABORT;

	if ((pe = ssdu2pe (sa -> sa_info, sa -> sa_cc, NULLCP, &result))
			== NULLPE) {
		psaplose (pi, result == PS_ERR_NMEM ? PC_CONGEST : PC_PROTOCOL,
				  NULLCP, "%s", ps_error (result));
		goto out;
	}

	if (decode_PS_Abort__type (pe, 1, NULLIP, NULLVP, &pdu) == NOTOK) {
		psaplose (pi, PC_UNRECOGNIZED, NULLCP, "%s", PY_pepy);
		goto out;
	}

	PLOGP (psap2_log,PS_Abort__type, pe, "Abort-type", 1);

	switch (pdu -> offset) {
	default:
		pa -> pa_peer = 1;
		pa -> pa_reason = PC_ABORTED;
		info = NULL, ppdu = PPDU_NONE;
		break;

	case type_PS_Abort__type_normal__mode:
		aru = pdu -> un.normal__mode;
		pa -> pa_peer = 1;
		pa -> pa_reason = PC_ABORTED;
		info = aru -> user__data, ppdu = PPDU_ARU;
		break;

	case type_PS_Abort__type_provider__abort:
		if ((arp = pdu -> un.provider__abort) -> provider__reason) {
			if ((result = arp -> provider__reason -> parm) == 0)
				result = PC_NOTSPECIFIED;
			else
				result += PC_ABORT_BASE;
		} else
			result = PC_NOTSPECIFIED;

		psaplose (pi, result, NULLCP, NULLCP);
		info = NULL, ppdu = PPDU_ARP;
		break;
	}

	ppdu2info (pb, pi, info, pa -> pa_info, &pa -> pa_ninfo, ppdu);

out:
	;
	SAFREE (sa);
	if (pe)
		pe_free (pe);
	if (pdu)
		free_PS_Abort__type (pdu);
	pb -> pb_fd = NOTOK;
	freepblk (pb);

	return NOTOK;
}

/*    define vectors for INDICATION events */

#define	e(i)	(data ? (i) : NULLIFP)


int
PSetIndications (int sd, IFP data, IFP tokens, IFP sync, IFP activity, IFP report, IFP finish, IFP abort, struct PSAPindication *pi) {
	SBV     smask;
	struct psapblk *pb;
	struct SSAPindication   sis;
	struct SSAPabort  *sa = &sis.si_abort;

	if (data || tokens || sync || activity || report || finish || abort) {
		missingP (data);
		missingP (tokens);
		missingP (sync);
		missingP (activity);
		missingP (report);
		missingP (finish);
		missingP (abort);
	}
	_iosignals_set = 1;

	smask = sigioblock ();

	psapPsig (pb, sd);

	if (pb -> pb_DataIndication = data)
		pb -> pb_flags |= PB_ASYN;
	else
		pb -> pb_flags &= ~PB_ASYN;
	pb -> pb_TokenIndication = tokens;
	pb -> pb_SyncIndication = sync;
	pb -> pb_ActivityIndication = activity;
	pb -> pb_ReportIndication = report;
	pb -> pb_ReleaseIndication = finish;
	pb -> pb_AbortIndication = abort;

	if (SSetIndications (pb -> pb_fd, e (DATAser), e (TOKENser),
						 e (SYNCser), e (ACTIVITYser), e (REPORTser), e (FINISHser),
						 e (ABORTser), &sis) == NOTOK) {
		pb -> pb_flags &= ~PB_ASYN;
		switch (sa -> sa_reason) {
		case SC_WAITING:
			sigiomask (smask);
			return psaplose (pi, PC_WAITING, NULLCP, NULLCP);

		default:
			ss2pslose (pb, pi, "SSetIndications", sa);
			freepblk (pb);
			sigiomask (smask);
			return NOTOK;
		}
	}
	sigiomask (smask);

	return OK;
}

#undef	e

/*    SSAP interface */

int
ss2pslose (struct psapblk *pb, struct PSAPindication *pi, char *event, struct SSAPabort *sa) {
	int     reason;
	char   *cp,
		   buffer[BUFSIZ];

	if (event && SC_FATAL (sa -> sa_reason))
		SLOG (psap2_log, LLOG_EXCEPTIONS, NULLCP,
			  (sa -> sa_cc > 0 ? "%s: %s [%*.*s]": "%s: %s", event,
			   SErrString (sa -> sa_reason), sa -> sa_cc, sa -> sa_cc,
			   sa -> sa_prdata));

	cp = "";
	switch (sa -> sa_reason) {
	case SC_SSAPID:
	case SC_SSUSER:
	case SC_ADDRESS:
		reason = PC_ADDRESS;
		break;

	case SC_REFUSED:
		reason = PC_REFUSED;
		break;

	case SC_CONGEST:
		reason = PC_CONGEST;
		break;

	case SC_TRANSPORT:
	case SC_ABORT:
		reason = PC_SESSION;
		break;

	default:
		reason = PC_SESSION;
		if (pb == NULLPB)
			switch (sa -> sa_reason) {
			case SC_PARAMETER:
				reason = PC_PARAMETER;
				break;

			case SC_OPERATION:
				reason = PC_OPERATION;
				break;

			case SC_TIMER:
				reason = PC_TIMER;
				break;

			case SC_WAITING:
				reason = PC_WAITING;
				break;
			}
		sprintf (cp = buffer, " (%s at session)",
				 SErrString (sa -> sa_reason));
		break;
	}

	if (pb) {
		if (sa -> sa_cc > 0)
			return ppktlose (pb, pi, reason, PPDU_NONE, NULLCP, "%*.*s%s",
							 sa -> sa_cc, sa -> sa_cc, sa -> sa_prdata, cp);
		else
			return ppktlose (pb, pi, reason, PPDU_NONE, NULLCP, "%s",
							 *cp ? cp + 1 : cp);
	} else {
		if (sa -> sa_cc > 0)
			return psaplose (pi, reason, NULLCP, "%*.*s%s",
							 sa -> sa_cc, sa -> sa_cc, sa -> sa_prdata, cp);
		else
			return psaplose (pi, reason, NULLCP, "%s",
							 *cp ? cp + 1 : cp);
	}
}

/*  */

static int
DATAser (int sd, struct SSAPdata *sx) {
	IFP	    abort;
	struct psapblk *pb;
	struct PSAPindication   pis;
	struct PSAPindication *pi = &pis;
	struct PSAPdata   *px = &pi -> pi_data;

	if ((pb = findpblk (sd)) == NULL)
		return;

	bzero ((char *) px, sizeof *px);
	bzero ((char *) pi, sizeof *pi);
	abort = pb -> pb_AbortIndication;

	if (doDATA (pb, sx, px, pi) == NOTOK)
		(*abort) (sd, &pi -> pi_abort);
	else
		(*pb -> pb_DataIndication) (sd, px);
}

/*  */

static int
TOKENser (int sd, struct SSAPtoken *st) {
	IFP	    abort;
	struct psapblk *pb;
	struct PSAPindication   pis;
	struct PSAPindication *pi = &pis;

	if ((pb = findpblk (sd)) == NULL)
		return;

	bzero ((char *) pi, sizeof *pi);
	abort = pb -> pb_AbortIndication;

	if  (doTOKEN (pb, st, pi) == NOTOK)
		(*abort) (sd, &pi -> pi_abort);
	else
		(*pb -> pb_TokenIndication) (sd, &pi -> pi_token);
}

/*  */

static int
SYNCser (int sd, struct SSAPsync *sn) {
	IFP	    abort;
	struct psapblk *pb;
	struct PSAPindication   pis;
	struct PSAPindication *pi = &pis;

	if ((pb = findpblk (sd)) == NULL)
		return;

	bzero ((char *) pi, sizeof *pi);
	abort = pb -> pb_AbortIndication;

	if  (doSYNC (pb, sn, pi) == NOTOK)
		(*abort) (sd, &pi -> pi_abort);
	else
		(*pb -> pb_SyncIndication) (sd, &pi -> pi_sync);
}

/*  */

static int
ACTIVITYser (int sd, struct SSAPactivity *sv) {
	IFP	    abort;
	struct psapblk *pb;
	struct PSAPindication   pis;
	struct PSAPindication *pi = &pis;

	if ((pb = findpblk (sd)) == NULL)
		return;

	bzero ((char *) pi, sizeof *pi);
	abort = pb -> pb_AbortIndication;

	if  (doACTIVITY (pb, sv, pi) == NOTOK)
		(*abort) (sd, &pi -> pi_abort);
	else
		(*pb -> pb_ActivityIndication) (sd, &pi -> pi_activity);
}

/*  */

static int
REPORTser (int sd, struct SSAPreport *sp) {
	IFP	    abort;
	struct psapblk *pb;
	struct PSAPindication   pis;
	struct PSAPindication *pi = &pis;

	if ((pb = findpblk (sd)) == NULL)
		return;

	bzero ((char *) pi, sizeof *pi);
	abort = pb -> pb_AbortIndication;

	if  (doREPORT (pb, sp, pi) == NOTOK)
		(*abort) (sd, &pi -> pi_abort);
	else
		(*pb -> pb_ReportIndication) (sd, &pi -> pi_report);
}

/*  */

static int
FINISHser (int sd, struct SSAPfinish *sf) {
	IFP	    abort;
	struct psapblk *pb;
	struct PSAPindication   pis;
	struct PSAPindication *pi = &pis;

	if ((pb = findpblk (sd)) == NULL)
		return;

	bzero ((char *) pi, sizeof *pi);
	abort = pb -> pb_AbortIndication;

	if  (doFINISH (pb, sf, pi) == NOTOK)
		(*abort) (sd, &pi -> pi_abort);
	else
		(*pb -> pb_ReleaseIndication) (sd, &pi -> pi_finish);
}

/*  */

static int
ABORTser (int sd, struct SSAPabort *sa) {
	IFP	    abort;
	struct psapblk *pb;
	struct PSAPindication   pis;
	struct PSAPindication *pi = &pis;

	if ((pb = findpblk (sd)) == NULL)
		return;

	bzero ((char *) pi, sizeof *pi);
	abort = pb -> pb_AbortIndication;

	doABORT (pb, sa, pi);
	(*abort) (sd, &pi -> pi_abort);
}

/*    INTERNAL */

struct psapblk *
newpblk()  {
	struct psapblk *pb;

	pb = (struct psapblk   *) calloc (1, sizeof *pb);
	if (pb == NULL)
		return NULL;

	pb -> pb_fd = NOTOK;

	if (once_only == 0) {
		PHead -> pb_forw = PHead -> pb_back = PHead;
		once_only++;
	}

	insque (pb, PHead -> pb_back);

	return pb;
}


int
freepblk (struct psapblk *pb) {
	int    i;
	struct PSAPcontext *qp;

	if (pb == NULL)
		return;

	if (pb -> pb_fd != NOTOK) {
		struct SSAPindication   sis;

		SUAbortRequest (pb -> pb_fd, NULLCP, 0, &sis);
	}

	if (pb -> pb_realbase)
		free (pb -> pb_realbase);
	else if (pb -> pb_retry)
		free (pb -> pb_retry);

	for (qp = pb -> pb_contexts, i = pb -> pb_ncontext - 1;
			i >= 0;
			qp++, i--) {
		if (qp -> pc_asn) {
			oid_free (qp -> pc_asn);
			qp -> pc_asn = NULLOID;
		}
		if (qp -> pc_atn) {
			oid_free (qp -> pc_atn);
			qp -> pc_atn = NULLOID;
		}
	}
	if (pb -> pb_asn)
		oid_free (pb -> pb_asn);
	if (pb -> pb_atn)
		oid_free (pb -> pb_atn);

	if (pb -> pb_ber)
		oid_free (pb -> pb_ber);

	remque (pb);

	free ((char *) pb);
}

/*  */

struct psapblk *
findpblk (int sd) {
	struct psapblk *pb;

	if (once_only == 0)
		return NULL;

	for (pb = PHead -> pb_forw; pb != PHead; pb = pb -> pb_forw)
		if (pb -> pb_fd == sd)
			return pb;

	return NULL;
}

/*  */

struct type_PS_User__data *
info2ppdu (struct psapblk *pb, struct PSAPindication *pi, PE *data, int ndata, int ppdu) {
	int    i,
		   j;
	PE	   *d,
	 pe;
	struct qbuf *qb;
	struct PSAPcontext *qp;
	OID	    atn;
	struct type_PS_User__data *pdu;
	struct type_PS_Simply__encoded__data *simple;
	struct type_PS_Fully__encoded__data **complex,
			   *full;

	if ((pdu = (struct type_PS_User__data *) calloc (1, sizeof *pdu))
			== NULL) {
no_mem:
		;
		psaplose (pi, PC_CONGEST, NULLCP, "out of memory");
		goto out;
	}

	pdu -> offset = type_PS_User__data_simple;
	for (d = data, i = 0; i < ndata; i++) {
		if ((pe = *d++) == NULLPE) {
			psaplose (pi, PC_PARAMETER, NULLCP,
					  "missing %d%s PDV in PSDU", i + 1,
					  i == 0 ? "st" : i == 1 ? "nd" : i == 2 ? "rd" : "th");
			goto out;
		}
		if (pb -> pb_ncontext > 0
				&& (pe -> pe_context == PE_DFLT_CTX ||
					((pb -> pb_flags & (PB_DFLT|PB_MAGIC)) == PB_DFLT &&
					 pe -> pe_context == pb -> pb_dctxid))) {
			if (ppdu != PPDU_TE) {
				psaplose (pi, PC_PARAMETER, NULLCP,
						  "default context not permitted");
				goto out;
			}
		} else if (ppdu == PPDU_CP
				   || (pb -> pb_ncontext > 1
					   && pe -> pe_context != PE_DFLT_CTX))
			pdu -> offset = type_PS_User__data_complex;
	}

	if (pdu -> offset == type_PS_User__data_simple) {
		if ((qb = (struct qbuf *) malloc (sizeof *qb)) == NULL)
			goto no_mem;
		simple = pdu -> un.simple = qb;
		qb -> qb_forw = qb -> qb_back = qb;
		qb -> qb_data = NULL, qb -> qb_len = 0;

		j = 0;
		for (d = data, i = 0; i < ndata; i++)
			j += ps_get_abs (*d++);
		qb -> qb_len = j;
		if ((qb = (struct qbuf *) malloc (sizeof *qb + ((unsigned)j))) == NULL)
			goto no_mem;
		qb -> qb_data = qb -> qb_base, qb -> qb_len = j;
		insque (qb, simple -> qb_back);
	} else
		complex = &pdu -> un.complex;

	for (d = data, i = 0; i < ndata; i++) {
		pe = *d++;
		switch (pe -> pe_context) {
		case PE_DFLT_CTX:
			atn = pb -> pb_atn;
			break;

		default:
			for (j = 0, qp = pb -> pb_contexts;
					j < pb -> pb_ncontext;
					j++, qp++)
				if (qp -> pc_id == pe -> pe_context)
					break;
			if (j >= pb -> pb_ncontext) {
				psaplose (pi, PC_PARAMETER, NULLCP,
						  "context %d is undefined", pe -> pe_context);
				goto out;
			}
			if (qp -> pc_result != PC_ACCEPT) {
				psaplose (pi, PC_PARAMETER, NULLCP,
						  "context %d is unsupported", pe -> pe_context);
				goto out;
			}
			atn = qp -> pc_atn;
			break;
		}

		if (!atn_is_ber (pb, atn)) {
			psaplose (pi, PC_PARAMETER, NULLCP,
					  "ATN not BER for context %d", pe -> pe_context);
			goto out;
		}

		if (pdu -> offset == type_PS_User__data_simple) {
			if (info2qb (pe, qb, pi) == NULL)
				goto out;
			qb -> qb_data = qb -> qb_base, qb -> qb_len = simple -> qb_len;
		} else {
			PE    *q;

			if ((full = (struct type_PS_Fully__encoded__data *)
						calloc (1, sizeof *full)) == NULL)
				goto no_mem;
			*complex = full;
			complex = &full -> next;
			if ((full -> PDV__list = (struct type_PS_PDV__list *)
									 calloc (1, sizeof *full -> PDV__list))
					== NULL)
				goto no_mem;
			full -> PDV__list -> identifier = pe -> pe_context;
			if ((full -> PDV__list -> presentation__data__values =
						(struct choice_PS_0 *)
						calloc (1, sizeof (struct choice_PS_0))) == NULL)
				goto no_mem;

			for (q = d, j = i + 1; j < ndata; q++, j++)
				if ((*q) -> pe_context != pe -> pe_context)
					break;
			q--, j--;

			if (i == j) {
				full -> PDV__list -> presentation__data__values ->
				offset = choice_PS_0_single__ASN1__type;
				(full -> PDV__list -> presentation__data__values ->
				 un.single__ASN1__type = pe) -> pe_refcnt++;
			} else {
				struct qbuf *qb2;

				full -> PDV__list -> presentation__data__values ->
				offset = choice_PS_0_octet__aligned;
				if ((qb2 = (struct qbuf *) malloc (sizeof *qb2)) == NULL)
					goto no_mem;
				full -> PDV__list -> presentation__data__values ->
				un.octet__aligned = qb2;
				qb2 -> qb_forw = qb2 -> qb_back = qb2;
				qb2 -> qb_data = NULL, qb2 -> qb_len = 0;
				for (d--, j++; i < j; i++) {
					if ((qb = info2qb (*d++, (struct qbuf *) NULL, pi))
							== NULL)
						goto out;
					qb2 -> qb_len += qb -> qb_len;
					insque (qb, qb2 -> qb_back);
				}
			}
		}
	}
	if (pdu -> offset == type_PS_User__data_simple) {
		qb -> qb_len = simple -> qb_len;
		qb -> qb_data = qb -> qb_base;
	}

	return pdu;

out:
	;
	if (pdu)
		free_PS_User__data (pdu);

	return NULL;
}

/*  */

int
ppdu2info (struct psapblk *pb, struct PSAPindication *pi, struct type_PS_User__data *info, PE *data, int *ndata, int ppdu) {
	int    i,
		   j;
	int	    ctx,
			allctx,
			result;
	PE	    pe;
	struct type_PS_Fully__encoded__data *full;

	*ndata = 0;
	if (info == NULL)
		return OK;

	i = 0;
	switch (info -> offset) {
	case type_PS_User__data_simple:
		if (pb -> pb_ncontext < 1 || ppdu == PPDU_TE)
			ctx = PE_DFLT_CTX;
		else if (pb -> pb_ncontext == 1)
			ctx = pb -> pb_contexts[0].pc_id;
		else ctx = PE_DFLT_CTX;

		while ((result = qb2info (info -> un.simple, &pe)) == PS_ERR_NONE) {
			if (i++ >= NPDATA) {
				pe_free (pe);
				return ppktlose (pb, pi, PC_CONGEST, ppdu, NULLCP,
								 "too much user information");
			}
			(*data++ = pe) -> pe_context = ctx;
		}
		if (result != PS_ERR_EOF)
			return ppktlose (pb, pi, result != PS_ERR_NMEM ? PC_INVALID
							 : PC_CONGEST, ppdu, NULLCP, "%s",
							 ps_error (result));
		break;

	case type_PS_User__data_complex:
		allctx = OK;
		for (full = info -> un.complex; full; full = full -> next) {
			struct qbuf *qb;
			struct PSAPcontext *qp;
			struct type_PS_PDV__list *pdv = full -> PDV__list;

			ctx = pdv -> identifier;
			if (allctx == OK)
				allctx = ctx;
			else if (allctx != NOTOK && allctx != ctx)
				allctx = NOTOK;
			for (j = 0, qp = pb -> pb_contexts;
					j < pb -> pb_ncontext;
					j++, qp++)
				if (qp -> pc_id == ctx)
					break;
			if (j >= pb -> pb_ncontext)
				return ppktlose (pb, pi, PC_INVALID, ppdu, NULLCP,
								 "unexpected use of context %d", ctx);
			switch (pdv -> presentation__data__values -> offset) {
			case choice_PS_0_single__ASN1__type:
				if (i++ >= NPDATA)
					return ppktlose (pb, pi, PC_CONGEST, ppdu, NULLCP,
									 "too much user information");
				pe = pdv -> presentation__data__values ->
					 un.single__ASN1__type;
				pdv -> presentation__data__values ->
				un.single__ASN1__type = NULLPE;
				(*data++ = pe) -> pe_context = ctx;
				break;

			case choice_PS_0_octet__aligned:
				qb = pdv -> presentation__data__values ->
					 un.octet__aligned;
				while ((result = qb2info (qb, &pe)) == PS_ERR_NONE) {
					pe -> pe_context = ctx;
					if (i++ >= NPDATA) {
						pe_free (pe);
						return ppktlose (pb, pi, PC_CONGEST, ppdu,
										 NULLCP,
										 "too much user information");
					}
					(*data++ = pe) -> pe_context = ctx;
				}
				if (result != PS_ERR_EOF)
					return ppktlose (pb, pi, result != PS_ERR_NMEM
									 ? PC_INVALID : PC_CONGEST, ppdu,
									 NULLCP, "%s", ps_error (result));
				break;

			default:
				return ppktlose (pb, pi, PC_INVALID, ppdu, NULLCP,
								 "not expecting non-BER encoding");
			}
		}
		if (allctx != NOTOK && allctx != OK &&
				((pb -> pb_flags & (PB_DFLT|PB_MAGIC)) == PB_DFLT) &&
				pb -> pb_dctxid == allctx)
			pb -> pb_flags |= PB_MAGIC; /* isode v6 bug */

		break;
	}
	*ndata = i;

	return OK;
}

/*  */

#ifndef	DEBUG
/* ARGSUSED */
#endif

int
info2ssdu (struct psapblk *pb, struct PSAPindication *pi, PE *data, int ndata, char **realbase, char **base, int *len, char *text, int ppdu) {
	int	    result;
	PE	    pe;
	struct type_PS_User__data *info;

	*realbase = *base = NULLCP, *len = 0;
	if (data == NULLPEP || ndata <= 0)
		return OK;

	if ((info = info2ppdu (pb, pi, data, ndata, ppdu)) == NULL)
		return (PC_FATAL (pi -> pi_abort.pa_reason) ? NOTOK : DONE);

	if (ppdu == PPDU_TTD) {
		pe = NULLPE;
		if ((result = encode_PS_User__data (&pe, 1, 0, NULLCP, info))
				== NOTOK) {
losing:
			;
			free_PS_User__data (info);
			return psaplose (pi, PC_CONGEST, NULLCP, "error encoding PDU: %s",
							 PY_pepy);
		}

		PLOGP (psap2_log,PS_User__data, pe, text, 0);

		goto serialize;
	} else if (ppdu == PPDU_RS || ppdu == PPDU_RSA) {
		/* this works 'cause RS-PPDU == RSA-PPDU */
		struct type_PS_RS__PPDU rss;
		struct type_PS_RS__PPDU *rs = &rss;

		if ((rs -> context__list = silly_list (pb, pi)) == NULL)
			return (PC_FATAL (pi -> pi_abort.pa_reason) ? NOTOK : DONE);
		rs -> user__data = info;

		pe = NULLPE;
		if ((result = encode_PS_RS__PPDU (&pe, 1, 0, NULLCP, rs))
				== NOTOK) {
			free_PS_Identifier__list (rs -> context__list);
			goto losing;
		}

		PLOGP (psap2_log,PS_RS__PPDU, pe, text, 0);

		free_PS_Identifier__list (rs -> context__list);

		goto serialize;
	}

	if (info -> offset == type_PS_User__data_simple) {
		struct qbuf *qb;

		qb = info -> un.simple;
		*len = qb -> qb_len;

		qb = qb -> qb_forw;
		remque (qb);

		*realbase = (char *) qb, *base = qb -> qb_base;

#ifdef	DEBUG
		if (psap2_log -> ll_events & LLOG_PDUS)
			while (ndata-- > 0)
				pvpdu (psap2_log, vunknown_P, *data++, text, 0);
#endif
	} else {
		pe = NULLPE;
		if (encode_PS_Fully__encoded__data (&pe, 0, 0, NULLCP,
											info -> un.complex) == NOTOK)
			goto losing;
		pe -> pe_class = PE_CLASS_APPL, pe -> pe_id = 1;

		PLOGP (psap2_log,PS_User__data, pe, text, 0);

serialize:
		;
		result = pe2ssdu (pe, base, len);

		pe_free (pe);

		if (result == NOTOK) {
			free_PS_User__data (info);
			return psaplose (pi, PC_CONGEST, NULLCP, NULLCP);
		}
	}
	free_PS_User__data (info);

	return OK;
}

/*  */

#ifndef	DEBUG
/* ARGSUSED */
#endif

int
ssdu2info (struct psapblk *pb, struct PSAPindication *pi, char *base, int len, PE *data, int *ndata, char *text, int ppdu) {
	int    result;
	PE	    pe;
	struct type_PS_User__data *info;

	*ndata = 0;
	if (base == NULLCP || len <= 0)
		return OK;

	if (ppdu == PPDU_RS || ppdu == PPDU_RSA) {
		struct type_PS_RS__PPDU *rs;/* this works 'cause RS-PPDU == RSA-PPDU */

		if ((pe = ssdu2pe (base, len, NULLCP, &result)) == NULLPE)
			return ppktlose (pb, pi, result == PS_ERR_NMEM ? PC_CONGEST
							 : PC_PROTOCOL, ppdu, NULLCP, "%s",
							 ps_error (result));

		rs = NULL, info = NULL;
		result = decode_PS_RS__PPDU (pe, 1, NULLIP, NULLVP, &rs);

#ifdef	DEBUG
		if (result == OK && (psap2_log -> ll_events & LLOG_PDUS))
			pvpdu (psap2_log, print_PS_RS__PPDU_P, pe, text, 1);
#endif

		info = rs -> user__data, rs -> user__data = NULL;
		free_PS_RS__PPDU (rs);

		goto punch_it;
	}

	if ((info = (struct type_PS_User__data *) calloc (1, sizeof *info))
			== NULL) {
no_mem:
		;
		psaplose (pi, PC_CONGEST, NULLCP, "out of memory");
out:
		;
		if (info)
			free_PS_User__data (info);
		return NOTOK;
	}

	/* sort of header prediction - is this SED ? */
	if (ppdu == PPDU_TE ||
			(*base & (PE_CLASS_MASK|PE_CODE_MASK)) ==
			((PE_CLASS_APPL << PE_CLASS_SHIFT) |
			 (PE_FORM_PRIM << PE_FORM_SHIFT) |
			 (0))) {
		struct qbuf *qb;

		info -> offset = type_PS_User__data_simple;

		if ((qb = (struct qbuf *) malloc (sizeof *qb)) == NULL)
			goto no_mem;
		info -> un.simple = qb;
		qb -> qb_forw = qb -> qb_back = qb;
		qb -> qb_data = NULL, qb -> qb_len = len;
		if ((qb = (struct qbuf *) malloc (sizeof *qb)) == NULL)
			goto no_mem;
		insque (qb, info -> un.simple);
		qb -> qb_data = base, qb -> qb_len = len;
	} else {
		info -> offset = type_PS_User__data_complex;

		if ((pe = ssdu2pe (base, len, NULLCP, &result)) == NULLPE) {
			ppktlose (pb, pi, result == PS_ERR_NMEM ? PC_CONGEST
					  : PC_PROTOCOL, ppdu, NULLCP, "%s",
					  ps_error (result));
			goto out;
		}

		if (pe -> pe_class != PE_CLASS_APPL
				|| pe -> pe_form != PE_FORM_CONS
				|| pe -> pe_id != 1) {
			PY_advise (NULLCP,
					   "Fully-encoded-data bad class/form/id: %s/%d/0x%x",
					   pe_classlist[pe -> pe_class], pe -> pe_form,
					   pe -> pe_id);
			result = NOTOK;
		} else
			result = decode_PS_Fully__encoded__data (pe, 0, NULLIP, NULLVP,
					 &info -> un.complex);

#ifdef	DEBUG
		if (result == OK && (psap2_log -> ll_events & LLOG_PDUS))
			pvpdu (psap2_log, print_PS_User__data_P, pe, text, 1);
#endif

punch_it:
		;
		pe_free (pe);

		if (result == NOTOK) {
			ppktlose (pb, pi, PC_UNRECOGNIZED, ppdu, NULLCP, "%s",
					  PY_pepy);
			goto out;
		}
	}

	if ((result = ppdu2info (pb, pi, info, data, ndata, ppdu)) == NOTOK)
		result = PC_FATAL (pi -> pi_abort.pa_reason) ? NOTOK : DONE;

#ifdef	DEBUG
	if (result == OK
			&& ppdu != PPDU_RS
			&& ppdu != PPDU_RSA
			&& info -> offset == type_PS_User__data_simple
			&& (psap2_log -> ll_events & LLOG_PDUS)) {
		int	i;

		for (i = *ndata; i > 0; i--)
			pvpdu (psap2_log, vunknown_P, *data++, text, 1);
	}
#endif

	free_PS_User__data (info);

	return result;
}

/*  */

#ifndef	DEBUG
/* ARGSUSED */
#endif

int
qbuf2info (struct psapblk *pb, struct PSAPindication *pi, struct qbuf *qb, int len, PE *data, int *ndata, char *text, int ppdu) {
	int	    result;
	PE	    pe;
	struct qbuf *qp;
	struct type_PS_User__data *info;

	*ndata = 0;
	if (qb == NULL || len <= 0)
		return OK;

	if (ppdu == PPDU_TTD) {
		if ((pe = qbuf2pe (qb, len, &result)) == NULLPE)
			return ppktlose (pb, pi, result == PS_ERR_NMEM ? PC_CONGEST
							 : PC_PROTOCOL, ppdu, NULLCP, "%s",
							 ps_error (result));

		info = NULL;
		result = decode_PS_User__data (pe, 1, NULLIP, NULLVP, &info);

#ifdef	DEBUG
		if (result == OK && (psap2_log -> ll_events & LLOG_PDUS))
			pvpdu (psap2_log, print_PS_User__data_P, pe, text, 1);
#endif

		goto punch_it;
	}

	if ((info = (struct type_PS_User__data *) calloc (1, sizeof *info))
			== NULL) {
no_mem:
		;
		psaplose (pi, PC_CONGEST, NULLCP, "out of memory");
		goto out;
	}

	if (ppdu == PPDU_TE ||
			(*qb -> qb_forw -> qb_data & (PE_CLASS_MASK|PE_CODE_MASK)) ==
			((PE_CLASS_APPL << PE_CLASS_SHIFT) |
			 (PE_FORM_PRIM << PE_FORM_SHIFT) |
			 (0))) {
		struct qbuf *qbp,
				   *qpp;

		info -> offset = type_PS_User__data_simple;
		if ((qp = (struct qbuf *) malloc (sizeof *qp)) == NULL)
			goto no_mem;
		info -> un.simple = qpp = qp;
		qp -> qb_forw = qp -> qb_back = qp;
		qp -> qb_data = NULL, qp -> qb_len = len;
		for (qp = qb -> qb_forw; qp != qb; qp = qbp) {
			qbp = qp -> qb_forw;

			remque (qp);
			insque (qp, qpp -> qb_back);
		}
	} else {
		info -> offset = type_PS_User__data_complex;

		if ((pe = qbuf2pe (qb, len, &result)) == NULLPE) {
			ppktlose (pb, pi, result == PS_ERR_NMEM ? PC_CONGEST
					  : PC_PROTOCOL, ppdu, NULLCP, "%s",
					  ps_error (result));
			goto out;
		}
		if (pe -> pe_class != PE_CLASS_APPL
				|| pe -> pe_form != PE_FORM_CONS
				|| pe -> pe_id != 1) {
			PY_advise (NULLCP,
					   "Fully-encoded-data bad class/form/id: %s/%d/0x%x",
					   pe_classlist[pe -> pe_class], pe -> pe_form,
					   pe -> pe_id);
			result = NOTOK;
		} else
			result = decode_PS_Fully__encoded__data (pe, 0, NULLIP, NULLVP,
					 &info -> un.complex);
#ifdef	DEBUG
		if (result == OK && (psap2_log -> ll_events & LLOG_PDUS))
			pvpdu (psap2_log, print_PS_User__data_P, pe, text, 1);
#endif

punch_it:
		;
		pe_free (pe);

		if (result == NOTOK) {
			ppktlose (pb, pi, PC_UNRECOGNIZED, ppdu, NULLCP, "%s",
					  PY_pepy);
			goto out;
		}
	}

	if ((result = ppdu2info (pb, pi, info, data, ndata, ppdu)) == NOTOK)
		result = PC_FATAL (pi -> pi_abort.pa_reason) ? NOTOK : DONE;

#ifdef	DEBUG
	if (result == OK
			&& ppdu != PPDU_TTD
			&& info -> offset == type_PS_User__data_simple
			&& (psap2_log -> ll_events & LLOG_PDUS)) {
		int	i;

		for (i = *ndata; i > 0; i--)
			pvpdu (psap2_log, vunknown_P, *data++, text, 1);
	}
#endif

	free_PS_User__data (info);

	return result;

out:
	;
	if (info)
		free_PS_User__data (info);

	return NOTOK;
}

/*  */

struct qbuf *
info2qb (PE pe, struct qbuf *qp, struct PSAPindication *pi) {
	int           len, qlen;
	struct qbuf *qb;

	if ((qb = qp) == NULL) {
		if ((qb = (struct qbuf *) malloc ((unsigned) sizeof *qb
										  + (len = ps_get_abs (pe))))
				== NULL) {
			psaplose (pi, PC_CONGEST, NULLCP, NULLCP);
			goto out_f;
		}

		qb -> qb_data = qb -> qb_base, qb -> qb_len = len;
	} else {
		len = ps_get_abs (pe);
		if (len > qb->qb_len) {
			psaplose (pi, PC_CONGEST, NULLCP,
					  "too much simple data when encoding user-info");
			goto out_f;
		}
	}


	Qcp = qb->qb_data;
	Ecp = Qcp + len;
	Len = 0;
	if ((qlen = pe2qb_f(pe)) == NOTOK) {
		psaplose (pi, PC_CONGEST, NULLCP, "error encoding user-info");
		goto out_f;
	}

	if (qp)
		qp -> qb_data += qlen, qp -> qb_len -= qlen;
	else
		qb -> qb_len = qlen;

#ifdef	DEBUG
	if (psap_log -> ll_events & LLOG_PDUS)
		pe2text (psap_log, pe, 0, qlen);
#endif

	return qb;

out_f:
	;
	if (qb && qb != qp)
		free ((char *) qb);

	return NULL;
}

/*  */

int
qb2info (struct qbuf *qb, PE *pe) {
	int	    result;
#ifdef	DEBUG
	int	    len;
#endif
	PE	    p;
	PS	    ps;

	*pe = NULLPE;

	if ((ps = ps_alloc (qbuf_open)) == NULLPS)
		return PS_ERR_NMEM;
#ifdef	DEBUG
	len = ps -> ps_byteno;
#endif
	if (qbuf_setup (ps, qb) == NOTOK || (p = ps2pe (ps)) == NULLPE) {
		if ((result = ps -> ps_errno) == PS_ERR_NONE)
			result = PS_ERR_EOF;
	} else {
		result = PS_ERR_NONE;
		*pe = p;
	}

	ps -> ps_addr = NULL;	/* so ps_free doesn't free remainder of qbuf */
#ifdef	DEBUG
	len = ps -> ps_byteno - len;
#endif
	ps_free (ps);

#ifdef	DEBUG
	if (p && (psap_log -> ll_events & LLOG_PDUS))
		pe2text (psap_log, p, 1, len);
#endif

	return result;
}

/*  */

struct type_PS_Identifier__list *
silly_list (struct psapblk *pb, struct PSAPindication *pi) {
	int    j;
	struct PSAPcontext *qp;
	struct type_PS_Identifier__list *list;
	struct type_PS_Identifier__list *lp,
			   **mp;

	list = NULL;
	mp = &list;

	for (j = 0, qp = pb -> pb_contexts;
			j < pb -> pb_ncontext;
			j++, qp++) {
		if ((lp = (struct type_PS_Identifier__list *)
				  calloc (1, sizeof *lp)) == NULL) {
no_mem:
			;
			psaplose (pi, PC_CONGEST, NULLCP, "out of memory");
			free_PS_Identifier__list (list);
			return NULL;
		}
		*mp = lp;
		mp = &lp -> next;
		if ((lp -> element_PS_10 = (struct element_PS_11 *)
								   calloc (1, sizeof (struct element_PS_11))) == NULL
				|| (lp -> element_PS_10 -> transfer__syntax =
						oid_cpy (qp -> pc_atn)) == NULLOID)
			goto no_mem;
		lp -> element_PS_10 -> identifier = qp -> pc_id;
	}

	return list;
}
