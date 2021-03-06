/* ftamfdf.c - FDF support */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/ftam/RCS/ftamfdf.c,v 9.0 1992/06/16 12:14:55 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/ftam/RCS/ftamfdf.c,v 9.0 1992/06/16 12:14:55 isode Rel $
 *
 *
 * $Log: ftamfdf.c,v $
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

int
fdf_p2names (int fd, PE bits, int *names, struct FTAMindication *fti) {
	struct ftamblk *fsb;

	if ((fsb = findfsblk (fd)) == NULL)
		return ftamlose (fti, FS_GEN_NOREASON, 0, NULLCP,
						 "invalid ftam descriptor");

	return fpm2bits (fsb, fname_pairs, bits, names, fti);
}

/*  */

int
fdf_names2p (int fd, int names, PE *bits, struct FTAMindication *fti) {
	struct ftamblk *fsb;

	if ((fsb = findfsblk (fd)) == NULL)
		return ftamlose (fti, FS_GEN_NOREASON, 0, NULLCP,
						 "invalid ftam descriptor");

	if ((*bits) = bits2fpm (fsb, fname_pairs, names, fti))
		return OK;
	return NOTOK;
}

/*  */

int
fdf_attrs2d (int fd, struct FTAMattributes *fa, struct type_FTAM_Read__Attributes **attrs, struct FTAMindication *fti) {
	struct ftamblk *fsb;

	if ((fsb = findfsblk (fd)) == NULL)
		return ftamlose (fti, FS_GEN_NOREASON, 0, NULLCP,
						 "invalid ftam descriptor");

	if ((*attrs) = attr2fpm (fsb, fa, fti))
		return OK;
	return NOTOK;
}

/*  */

int
fdf_d2attrs (int fd, struct type_FTAM_Read__Attributes *attrs, struct FTAMattributes *fa, struct FTAMindication *fti) {
	struct ftamblk *fsb;

	if ((fsb = findfsblk (fd)) == NULL)
		return ftamlose (fti, FS_GEN_NOREASON, 0, NULLCP,
						 "invalid ftam descriptor");

	return fpm2attr (fsb, attrs, fa, fti);
}
