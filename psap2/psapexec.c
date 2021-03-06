/* psapexec.c - PPM: exec */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/psap2/RCS/psapexec.c,v 9.0 1992/06/16 12:29:42 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/psap2/RCS/psapexec.c,v 9.0 1992/06/16 12:29:42 isode Rel $
 *
 *
 * $Log: psapexec.c,v $
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
#include "PS-types.h"
#include "ppkt.h"
#include "isoservent.h"
#include "tailor.h"

/*    SERVER only */

int
PExec (struct SSAPstart *ss, struct PSAPindication *pi, char *arg1, char *arg2, IFP hook, IFP setperms) {
	int	    len,
			result,
			result2;
	char   *base;
	struct isoservent *is;
	struct psapblk *pb;
	PE	    pe;
	struct SSAPref ref;
	struct SSAPindication   sis;
	struct SSAPindication *si = &sis;
	struct type_PS_CP__type *cp;
	struct element_PS_0 *cp_normal;
	struct type_PS_CPR__type *cpr;

	missingP (ss);
	missingP (pi);
	missingP (arg1);
	missingP (arg2);

	cp = NULL, cpr = NULL;
	pe = NULLPE;

	if ((pb = newpblk ()) == NULL)
		goto congest;
	pb -> pb_fd = ss -> ss_sd;

	if ((pe = ssdu2pe (ss -> ss_data, ss -> ss_cc, NULLCP, &result))
			== NULLPE) {
		if (result == PS_ERR_NMEM)
			goto congest;

		ppktlose (pb, pi, PC_PROTOCOL, PPDU_CP, NULLCP, "%s",
				  ps_error (result));
		goto out1;
	}

	if (decode_PS_CP__type (pe, 1, NULLIP, NULLVP, &cp) == NOTOK) {
		ppktlose (pb, pi, PC_UNRECOGNIZED, PPDU_CP, NULLCP, "%s",
				  PY_pepy);
		goto out1;
	}

	PLOGP (psap2_log,PS_CP__type, pe, "CP-type", 1);

	if (cp -> mode -> parm != int_PS_Mode__selector_normal__mode) {
		ppktlose (pb, pi, PC_INVALID, PPDU_CP, NULLCP,
				  "X.410-mode not supported");
		goto out1;
	}
	cp_normal = cp -> normal__mode;

	pe_free (pe);
	pe = NULLPE;

	if (cp_normal -> called == NULL)
		base = NULL, len = 0;
	else {
		if ((base = qb2str (cp_normal -> called)) == NULLCP)
			goto congest;
		len = cp_normal -> called -> qb_len;
	}

	switch (len) {
	case 0:
		is = NULL;
		break;

	default:
		is = getisoserventbyselector ("psap", base, len);
		break;
	}
	if (base)
		free (base);
	if (is == NULL) {
		result = SC_REJECTED, result2 = PC_ADDRESS;
		goto out2;
	}

	*is -> is_tail++ = arg1;
	*is -> is_tail++ = arg2;
	*is -> is_tail = NULL;

	switch (hook ? (*hook) (is, pi) : OK) {
	case NOTOK:
		return NOTOK;

	case DONE:
		return OK;

	case OK:
	default:
		if (setperms)
			(*setperms) (is);
		execv (*is -> is_vec, is -> is_vec);
		SLOG (psap2_log, LLOG_FATAL, *is -> is_vec,
			  ("unable to exec"));
		break;
	}

congest:
	;
	result = SC_CONGESTION;
	psaplose (pi, result2 = PC_CONGEST, NULLCP, NULLCP);

out2:
	;
	if (pe) {
		pe_free (pe);
		pe = NULLPE;
	}

	if (cpr = (struct type_PS_CPR__type *) calloc (1, sizeof *cpr)) {
		if (cp
				&& cp -> mode
				&& cp -> mode -> parm ==
				int_PS_Mode__selector_x410__1984__mode) {
			cpr -> offset = type_PS_CPR__type_x410__mode;
			if (pe = pe_alloc (PE_CLASS_UNIV, PE_FORM_CONS, PE_CONS_SET)) {
				cpr -> un.x410__mode = pe;
				set_add (pe, num2prim ((integer) (result2 != PC_CONGEST ? 0 : 3),
									   PE_CLASS_CONT, 0));
			}
		} else {
			struct element_PS_2 *cpr_normal;

			cpr -> offset = type_PS_CPR__type_normal__mode;
			if (cpr_normal = (struct element_PS_2 *)
							 calloc (1, sizeof *cpr_normal)) {
				cpr -> un.normal__mode = cpr_normal;
				cpr_normal -> optionals |= opt_PS_element_PS_2_reason;
				cpr_normal -> reason = result2 - PC_PROV_BASE;
			}
		}
	}

	if (encode_PS_CPR__type (&pe, 1, 0, NULLCP, cpr) != NOTOK) {
		PLOGP (psap2_log,PS_CPR__type, pe, "CPR-type", 0);

		if (pe)
			pe2ssdu (pe, &base, &len);
	} else
		base = NULL, len = 0;

	bzero ((char *) &ref, sizeof ref);
	SConnResponse (ss -> ss_sd, &ref, NULLSA, result, 0, 0,
				   SERIAL_NONE, base, len, si);
	if (base)
		free (base);
	psaplose (pi, result2, NULLCP, NULLCP);

out1:
	;
	SSFREE (ss);
	if (pe)
		pe_free (pe);
	if (cp)
		free_PS_CP__type (cp);
	if (cpr)
		free_PS_CPR__type (cpr);

	freepblk (pb);

	return NOTOK;
}
