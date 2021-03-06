/* sys_tai.c - System tailoring routines */

#ifndef lint
static char *rcsid = "$Header: /xtel/isode/isode/dsap/common/RCS/sys_tai.c,v 9.0 1992/06/16 12:12:39 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/dsap/common/RCS/sys_tai.c,v 9.0 1992/06/16 12:12:39 isode Rel $
 *
 *
 * $Log: sys_tai.c,v $
 * Revision 9.0  1992/06/16  12:12:39  isode
 * Release 8.0
 *
 */

/*                                  NOTICE
 *
 *    Acquisition, use, and distribution of this module and related
 *    materials are subject to the restrictions of a license agreement.
 *    Consult the Preface in the User's Manual for the full terms of
 *    this agreement.
 *
 */


/* LINTLIBRARY */

#include "quipu/util.h"
#include "cmd_srch.h"
#include "tailor.h"

extern char *oidtable,
	   *dsa_address,
	   *local_dit,
	   dishinit,
	   *myname;

extern LLog *log_dsap;
#ifndef NO_STATS
extern LLog * log_stat;
#endif

extern int  oidformat,sizelimit,timelimit;

extern int ch_set;

#define SYSLOG           2
#define OIDTAB           4
#define OIDFMT           5
#define SIZELIMIT        12
#define STATS		 13
#define TIMELIMIT        14
#define DSAADDR          18
#define PHOTO		 19
#define LOCAL_DIT	 20
#define DISH_INIT	 21
#define CH_SET		 22
#define JPEG		 23
#define ISODE_TAILOR	 24

static  CMD_TABLE  cmdtab[] = {
	"DSAPLOG",      SYSLOG,
	"OIDTABLE",     OIDTAB,
	"OIDFORMAT",    OIDFMT,
	"SIZELIMIT",    SIZELIMIT,
	"TIMELIMIT",    TIMELIMIT,
	"DSA_ADDRESS",  DSAADDR,
	"PHOTO",	PHOTO,
	"JPEG",		JPEG,
	"ISODE",	ISODE_TAILOR,
	"LOCAL_DIT",	LOCAL_DIT,
	"QUIPURC",	DISH_INIT,
	"CH_SET",	CH_SET,
#ifndef NO_STATS
	"STATS",	STATS,
#endif
	0,              -1,
};

static  CMD_TABLE  oidtab[] = {
	"SHORT",        1,
	"LONG",         2,
	"NUMERIC",      3,
	0,              1,      /* default short */
};

static CMD_TABLE chtab[] = {
	"ASCII", 0,
	"US-ASCII", 0,
	"ISO8859", 1,
	"ISO8859-1", 1,
	(char *) 0, 0
};

/*
 * do system wide initialisations
 */

int
dsap_tai (int argc, char **argv) {
	char    *arg, *term;
	extern char * getenv ();
	short str2syntax ();

	if(argc < 2)
		return(NOTOK);

	arg = argv[1];

	switch(cmd_srch(argv[0], cmdtab)) {
	case SYSLOG:
		DLOG (log_dsap,LLOG_DEBUG,( "Tailor SYSLOG %s", arg));
		log_tai(log_dsap, &argv[1], argc-1);
		break;
#ifndef NO_STATS
	case STATS:
		DLOG (log_dsap,LLOG_DEBUG,( "Tailor STATS %s", arg));
		log_tai(log_stat, &argv[1], argc-1);
		break;
#endif
	case OIDTAB:
		DLOG (log_dsap,LLOG_DEBUG,( "Tailor OIDTable=%s", arg));
		oidtable = strdup (arg);
		break;
	case LOCAL_DIT:
		DLOG (log_dsap,LLOG_DEBUG,( "Tailor local_DIT=%s", arg));
		local_dit = strdup (arg);
		break;
	case OIDFMT:
		DLOG (log_dsap,LLOG_DEBUG,( "Tailor OIDFMT=%s", arg));
		oidformat = cmd_srch (arg,oidtab);
		break;
	case SIZELIMIT:
		sizelimit = atoi (arg);
		break;
	case TIMELIMIT:
		timelimit = atoi (arg);
		break;
	case DISH_INIT:
		if (lexequ (arg,"on") == 0)
			dishinit = TRUE;
		break;
	case PHOTO:
		DLOG (log_dsap,LLOG_DEBUG,( "Tailor photo=%s", arg));
		if ((term = getenv ("TERM")) && strcmp (term, arg) == 0) {
			if (*argv[2] == '/')
				set_av_pe_print (str2syntax("photo"),strdup(argv[2]));
			else {
				char proc [LINESIZE];
				strcpy (proc,isodefile("g3fax/", 1));
				strcat (proc,argv[2]);
				set_av_pe_print (str2syntax("photo"),strdup(proc));
			}
		}
		break;
	case JPEG:
		DLOG (log_dsap,LLOG_DEBUG,( "Tailor jpeg=%s", arg));
		if ((term = getenv ("TERM")) && strcmp (term, arg) == 0) {
			if (*argv[2] == '/')
				set_av_pe_print (str2syntax("jpeg"),strdup(argv[2]));
			else {
				char proc [LINESIZE];
				strcpy (proc,isodefile("g3fax/", 1));
				strcat (proc,argv[2]);
				set_av_pe_print (str2syntax("jpeg"),strdup(proc));
			}
		}
		break;
	case DSAADDR:
		if (myname == NULLCP) {
			/* use first 'dsa_address' in tailor file */
			DLOG (log_dsap,LLOG_DEBUG,( "Tailor DSA_ADDRESS=%s", argv[2]));
			dsa_address = strdup (argv[2]);
			myname = strdup (argv[1]);
		} else if (dsa_address == NULLCP) {
			/* User has given a '-c flag' */
			/* look for entry in address list */
			if (lexequ (arg,myname) == 0) {
				DLOG (log_dsap,LLOG_DEBUG,( "Tailor DSA_ADDRESS (USER) =%s", argv[2]));
				myname = strdup (arg);
				dsa_address = strdup (argv[2]);
			}
		}
		break;
	case ISODE_TAILOR:
		DLOG (log_dsap,LLOG_TRACE,( "Tailor Isode %s%s",arg,argv[2]));
		if (argc != 3)
			LLOG (log_dsap,LLOG_EXCEPTIONS,(
					  "Invalid isode option in quiputailor"));
		else
			isodesetvar(arg,strdup(argv[2]),0);
		break;
	case CH_SET:
		DLOG (log_dsap,LLOG_DEBUG,( "ch_set =%s", arg));
		ch_set = cmd_srch (arg,chtab);
		break;
	default:
		LLOG (log_dsap,LLOG_EXCEPTIONS, ("Unknown tailor option %s",arg));
		return (NOTOK);
	}
	if ((arg = getenv("CH_SET")) != NULLCP)
		ch_set = cmd_srch(arg, chtab);
	return (OK);
}

