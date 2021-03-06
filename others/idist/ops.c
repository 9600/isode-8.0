/* idist.c - remote distribution - initiator */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/others/idist/RCS/ops.c,v 9.0 1992/06/16 12:42:00 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/others/idist/RCS/ops.c,v 9.0 1992/06/16 12:42:00 isode Rel $
 *
 * This file is entirely new, and handles the dispatching of the
 * remote operations to the server. It tries hard to remove all hint
 * of the ISODE parts and to hide the posy generated structures.
 *
 * Julian Onions <jpo@cs.nott.ac.uk>
 * Nottingham University Computer Science.
 *
 *
 * $Log: ops.c,v $
 * Revision 9.0  1992/06/16  12:42:00  isode
 * Release 8.0
 *
 */

#include <stdio.h>
#include "Idist-types.h"	/* type definitions */
#include "Idist-ops.h"		/* operation definitions */
#include "defs.h"

extern struct type_Idist_FileSpec *makefs ();
extern	char target[];
static void print_ia5list ();

int	basic_error ();
int	null_result (), ia5_result (), query_result ();

int	result_value;

int
initdir (int flag, char *dest) {
	struct qbuf *qb;
	struct type_Idist_InitDir *idp;

	qb = str2qb (dest, strlen(dest), 1);
	idp = (struct type_Idist_InitDir *) malloc (sizeof *idp);
	if (idp == (struct type_Idist_InitDir *)0)
		adios ("memory", "out of");
	if (flag) {
		idp -> offset = type_Idist_InitDir_destdir;
		idp -> un.destdir = qb;
	} else {
		idp -> offset = type_Idist_InitDir_nodestdir;
		idp -> un.nodestdir = qb;
	}
	return invoke (operation_Idist_init, (caddr_t)idp,
				   &_ZIdist_mod, _ZInitDirIdist,
				   null_result,
				   basic_error);
}

int	transfer (type, opts, mode, size, mtime, uname, group, name, lname)
unsigned short type, mode;
time_t	mtime;
off_t	size;
int	opts;
char	*uname, *group, *name, *lname;
{
	struct type_Idist_FileSpec *fs;

	fs = makefs (type, opts, mode, size, mtime,
				 uname, group, name, lname);

	return invoke (operation_Idist_transfer, (caddr_t)fs,
				   &_ZIdist_mod, _ZFileSpecIdist,
				   ia5_result,
				   basic_error);
}


int
terminate (int type, int status) {
	struct type_Idist_TermStatus *ts;
	struct type_Idist_FileType	*makeftype ();

	if ((ts = (struct type_Idist_TermStatus *) malloc ( sizeof *ts)) == NULL)
		adios ("memory", "out of");
	ts -> filetype = makeftype ((unsigned short)type);
	ts -> status = (status == OK) ? 1 : 0;
	return invoke (operation_Idist_terminate, (caddr_t)ts,
				   &_ZIdist_mod, _ZTermStatusIdist,
				   null_result,
				   basic_error);
}

int
tran_data (char *data, int len) {
	struct type_Idist_Data *dat;

	dat = str2qb (data, len, 1);

	return invoke (operation_Idist_data, (caddr_t)dat,
				   &_ZIdist_mod,_ZDataIdist,
				   null_result,
				   basic_error);
}

int
runspecial (char *cmd) {
	struct type_UNIV_IA5String *ia5;

	ia5 = str2qb (cmd, strlen(cmd), 1);

	return invoke (operation_Idist_special, (caddr_t)ia5,
				   &_ZUNIV_mod, _ZIA5StringUNIV,
				   ia5_result,
				   basic_error);
}

static time_t	cmtime;
static off_t	csize;
static unsigned short cmode;

int	rquery (file, mtime, size, mode)
char	*file;
time_t	*mtime;
off_t	*size;
unsigned short *mode;
{
	struct type_UNIV_IA5String *ia5;
	int	retval;

	ia5 = str2qb (file, strlen(file), 1);

	retval = invoke (operation_Idist_query, (caddr_t)ia5,
					 &_ZUNIV_mod, _ZIA5StringUNIV,
					 query_result,
					 basic_error);
	if (retval == NOTOK)
		return NOTOK;
	if (result_value != OK)
		return result_value;
	else {
		*mtime = cmtime;
		*mode = cmode;
		*size = csize;
		return OK;
	}
}

/* ARGSUSED */
int
query_result (int sd, int id, int error, struct type_Idist_QueryResult *qr, struct RoSAPindication *roi) {
	long	convtime ();

	result_value = OK;
	if (qr -> offset == type_Idist_QueryResult_doesntExist) {
		result_value = DONE;
		return OK;
	} else {
		cmode = 0;
		switch (qr -> un.doesExist -> filetype -> parm) {
		case int_Idist_FileType_directory:
			cmode = S_IFDIR;
		case int_Idist_FileType_symlink:
			if (cmode == 0)
				cmode = S_IFLNK;
		case int_Idist_FileType_regular:
			if (cmode == 0)
				cmode = S_IFREG;
			cmtime = convtime (qr -> un.doesExist -> filemtime);
			csize = qr -> un.doesExist -> filesize;
			break;

		default:
			result_value = NOTOK;
			return OK;
		}
		result_value = OK;
		return OK;
	}
}

static int copts;
int
rmchk (int opts) {
	int	listcdir_result ();

	copts = opts;
	return invoke (operation_Idist_listcdir, (caddr_t)NULL,
				   (modtyp *) 0, -1,
				   listcdir_result,
				   basic_error);
}

/* ARGSUSED */
int
listcdir_result (int sd, int id, int error, struct type_Idist_FileList *files, struct RoSAPindication *roi) {
	struct type_Idist_FileList *fl;
	char	buf[BUFSIZ];
	char	*name;
	struct stat stb;
	extern char basename[];

	for (fl = files; fl; fl = fl -> next) {
		name = qb2str (fl -> FileSpec -> filename);
		sprintf (buf, "%s/%s", target, name);
		if (lstat (buf, &stb) < 0) {
			if (copts & VERIFY)
				printf ("need to remove: %s/%s\n",
						target, name);
			else
				deletefile (name,
							(int) fl -> FileSpec -> filetype -> parm);
		}
		free (name);
	}
	return OK;
}

int
deletefile (char *str, int mode) {
	struct type_UNIV_IA5String *ia5;
	char	buffer[BUFSIZ];

	switch (mode) {
	case int_Idist_FileType_regular:
	case int_Idist_FileType_hardlink:
		mode = S_IFREG;
		break;
	case int_Idist_FileType_directory:
		mode = S_IFDIR;
		break;
	case int_Idist_FileType_symlink:
		mode = S_IFLNK;
		break;
	default:
		mode = 0;
		break;
	}

	sprintf (buffer, "%s/%s", target, str);
	if ((copts & QUERYM) && !query ("Delete", mode, buffer))
		return OK;

	ia5 = str2qb (str, strlen(str), 1);

	return invoke (operation_Idist_deletefile, (caddr_t) ia5,
				   &_ZUNIV_mod, _ZIA5StringUNIV,
				   ia5_result,
				   basic_error);
}


/* ARGSUSED */
static int
basic_error (int sd, int id, int error, struct type_Idist_IA5List *parameter, struct RoSAPindication *roi) {
	struct RyError *rye;

	if (error == RY_REJECT) {
		advise (NULLCP, "%s", RoErrString ((int) parameter));
		result_value = NOTOK;
		return OK;
	}

	if (rye = finderrbyerr (table_Idist_Errors, error))
		advise (NULLCP, "%s", rye -> rye_name);
	else
		advise (NULLCP, "Error %d", error);

	if (parameter)
		print_ia5list (parameter);

	result_value = NOTOK;
	return OK;
}

/* ARGSUSED */

static null_result (sd, id, dummy, result, roi)
int     sd,
		id,
		dummy;
caddr_t result;
struct RoSAPindication *roi;
{
	result_value = OK;
	return OK;
}

/* ARGSUSED */
static int ia5_result (sd, id, parameter, result, roi)
int     sd,
		id,
		parameter;
caddr_t result;
struct RoSAPindication *roi;
{
	result_value = OK;
	print_ia5list ((struct type_Idist_IA5List *)result);
	return OK;
}

static void
print_ia5list (struct type_Idist_IA5List *ia5) {
	struct qbuf *p,
			   *q;

	for (; ia5; ia5 = ia5 -> next) {
		p = ia5 -> IA5String;
		for (q = p -> qb_forw; q != p ; q = q -> qb_forw)
			printf ("%*.*s", q -> qb_len, q -> qb_len, q -> qb_data);
		printf ("\n");
	}
}
