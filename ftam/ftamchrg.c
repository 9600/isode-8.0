/* ftamchrg.c - FPM: encode/decode charging */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/ftam/RCS/ftamchrg.c,v 9.0 1992/06/16 12:14:55 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/ftam/RCS/ftamchrg.c,v 9.0 1992/06/16 12:14:55 isode Rel $
 *
 *
 * $Log: ftamchrg.c,v $
 * Revision 9.0  1992/06/16  12:14:55  isode
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
#include "fpkt.h"

/*  */

struct type_FTAM_Charging *
chrg2fpm (struct ftamblk *fsb, struct FTAMcharging *charging, struct FTAMindication *fti) {
	int    i;
	struct fc_charge  *fc;
	struct type_FTAM_Charging *fpmp;
	struct type_FTAM_Charging  *fpm,
			   **fpc;
	struct charge_element *f1;

	fpmp = NULL, fpc = &fpmp;
	for (fc = charging -> fc_charges, i = charging -> fc_ncharge - 1;
			i >= 0;
			fc++, i--) {
		if (fc -> fc_resource == NULL || fc -> fc_unit == NULL) {
			ftamlose (fti, FS_GEN (fsb), 0, NULLCP,
					  "empty charge at slot %d",
					  charging -> fc_ncharge - i - 1);
			goto out;
		}

		if ((fpm = (struct type_FTAM_Charging *) calloc (1, sizeof *fpm))
				== NULL) {
no_mem:
			;
			ftamlose (fti, FS_GEN (fsb), 1, NULLCP, "out of memory");
out:
			;
			if (fpmp)
				free_FTAM_Charging (fpmp);
			return NULL;
		}
		*fpc = fpm;

		if ((f1 = (struct charge_element *) calloc (1, sizeof *f1)) == NULL)
			goto no_mem;
		fpm -> charge = f1;

		if ((f1 -> resource__identifier = str2qb (fc -> fc_resource,
										  strlen (fc -> fc_resource),
										  1))
				== NULL
				|| (f1 -> charging__unit = str2qb (fc -> fc_unit,
										   strlen (fc -> fc_unit), 1))
				== NULL)
			goto no_mem;
		f1 -> charging__value = fc -> fc_value;

		fpc = &fpm -> next;
	}

	return fpmp;
}

/*  */

int
fpm2chrg (struct ftamblk *fsb, struct type_FTAM_Charging *fpm, struct FTAMcharging *charging, struct FTAMindication *fti) {
	int    i;
	struct fc_charge *fc;
	struct charge_element *f1;

	bzero ((char *) charging, sizeof *charging);

	fc = charging -> fc_charges, i = 0;
	for (; fpm; fpm = fpm -> next) {
		if (i >= NFCHRG)
			return ftamlose (fti, FS_GEN (fsb), 1, NULLCP,
							 "too many charges");

		f1 = fpm -> charge;
		if ((fc -> fc_resource = qb2str (f1 -> resource__identifier)) == NULL
				|| (fc -> fc_unit = qb2str (f1 -> charging__unit)) == NULL) {
			if (fc -> fc_resource)
				free (fc -> fc_resource), fc -> fc_resource = NULL;
			while (i-- > 0) {
				fc--;
				free (fc -> fc_resource), fc -> fc_resource = NULL;
				free (fc -> fc_unit), fc -> fc_unit = NULL;
			}

			return ftamlose (fti, FS_GEN (fsb), 1, NULLCP, "out of memory");
		}
		fc -> fc_value = f1 -> charging__value;

		fc++, i++;
	}

	return OK;
}
