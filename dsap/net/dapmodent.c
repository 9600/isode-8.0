/* dapmodent.c - */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/dsap/net/RCS/dapmodent.c,v 9.0 1992/06/16 12:14:05 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/dsap/net/RCS/dapmodent.c,v 9.0 1992/06/16 12:14:05 isode Rel $
 *
 *
 * $Log: dapmodent.c,v $
 * Revision 9.0  1992/06/16  12:14:05  isode
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

#include "quipu/util.h"
#include "quipu/dap2.h"
#include "../x500as/DAS-types.h"

int
dap_modifyentry (int ad, int *id, struct ds_modifyentry_arg *arg, struct DSError *error) {
	struct DAPindication	  di_s;
	struct DAPindication	* di = &(di_s);

	++(*id);

	DapModifyEntry (ad, (*id), arg, di, ROS_INTR);

	error->dse_type = DSE_NOERROR;

	switch (di->di_type) {
	case DI_RESULT: {
		struct DAPresult	* dr = &(di->di_result);

		DRFREE (dr);
		return (DS_OK);
	}

	case DI_ERROR: {
		struct DAPerror	* de = &(di->di_error);

		(*error) = de->de_err;	/* struct copy */
		return (DS_ERROR_REMOTE);
	}

	case DI_PREJECT:
		error->dse_type = DSE_REMOTEERROR;
		return (DS_ERROR_PROVIDER);

	case DI_ABORT:
		error->dse_type = DSE_REMOTEERROR;
		return (DS_ERROR_CONNECT);

	default:
		error->dse_type = DSE_REMOTEERROR;
		return (DS_ERROR_PROVIDER);
	}
}

int
DapModifyEntry (int ad, int id, struct ds_modifyentry_arg *arg, struct DAPindication *di, int asyn) {
	PE                  arg_pe;

	if(encode_DAS_ModifyEntryArgument(&arg_pe,1,0,NULLCP,arg) != OK) {
		return(dapreject (di, DP_INVOKE, id, NULLCP, "ModifyEntry argument encoding failed"));
	}

	return (DapInvokeReqAux (ad, id, OP_MODIFYENTRY, arg_pe, di, asyn));

}

