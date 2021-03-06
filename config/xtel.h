/* sunos4.h - site configuration file for SunOS release 4 */

/*
 * $Header: /xtel/isode/isode/config/RCS/xtel.h,v 9.0 1992/06/16 12:08:13 isode Rel $
 *
 *
 * $Log: xtel.h,v $
 * Revision 9.0  1992/06/16  12:08:13  isode
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


#ifndef	_CONFIG_
#define	_CONFIG_

#define	BSD42			/* Berkeley UNIX */
#define	SUNOS4			/*   with Sun's enhancements */
#define	WRITEV			/*   (sort of) real Berkeley UNIX */
#define	BSD43			/*   4.3BSD or later */
#define SUNOS41			/*   4.1sun stuff */

#define	VSPRINTF		/* has vprintf(3s) routines */

#define	TCP			/* has TCP/IP (of course) */
#define	SOCKETS			/*   provided by sockets */

#define X25
#define SUN_X25

/*
#define USE_PP			/* Have PP so use it in QUIPU */

#ifdef notdef
#define TP4			/* has TP4 */
#define SUN_TP4			/*   provided by SunLink OSI */
#define	SUNLINK_5_2		/*     define if using SunLink OSI release 5.2
				       or greater */
#define	SUNLINK_6_0		/*     define if using SunLink OSI release 6.0
				       or greater */
#define	SUNLINK_7_0		/*     define if using SunLink OSI release 7.0
				       or greater */

#endif

#define NOGOSIP
#define DEBUG

#define	GETDENTS		/* has getdirent(2) call */
#define	NFS			/* network filesystem -- has getdirentries */

#endif
