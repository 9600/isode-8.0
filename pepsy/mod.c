/* mod.c */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/pepsy/RCS/mod.c,v 9.0 1992/06/16 12:24:03 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/pepsy/RCS/mod.c,v 9.0 1992/06/16 12:24:03 isode Rel $
 *
 *
 * $Log: mod.c,v $
 * Revision 9.0  1992/06/16  12:24:03  isode
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


#include <stdio.h>
#include <ctype.h>
#include "pepsydefs.h"
#include "pass2.h"

extern char *sysin;
extern char *proc_name();
extern char *my_strcat();

char   *calc_arg();

#define MAXLENGTH 30

#ifdef notdef
/*
 * output each of the actions associated with yp
 */
do_sw_type(name, yp, fp, fpa)
char   *name;
YP      yp;
FILE   *fp;
FILE   *fpa;
{
	char   *s;
	char   *arg;

	s = proc_name(name, 0);
	if (yp->yp_action1) {
		fprintf(fp, "\t\tcase %s%s:\n", s, ACT1);
		arg = calc_arg(yp, 1);
		my_do_action(fp, yp->yp_action1, 0, arg, yp->yp_act1_lineno, yp->yp_param_type);
		fprintf(fp, "\t\t\tbreak;\n");
	}
	if (yp->yp_action2) {
		fprintf(fp, "\t\tcase %s%s:\n", s, ACT2);
		arg = calc_arg(yp, 2);
		my_do_action(fp, yp->yp_action2, 0, arg, yp->yp_act2_lineno, yp->yp_param_type);
		fprintf(fp, "\t\t\tbreak;\n");
	}
	if (yp->yp_action05) {
		fprintf(fp, "\t\tcase %s%s:\n", s, ACT05);
		arg = calc_arg(yp, 5);
		my_do_action(fp, yp->yp_action05, 0, arg, yp->yp_act05_lineno, yp->yp_param_type);
		fprintf(fp, "\t\t\tbreak;\n");
	}
}
#endif

my_do_action(fp, action, level, arg, lineno, new)
FILE   *fp;
char   *action;
int     level;
char   *arg;
int     lineno;
char   *new;
{
	int     i;
	char    t[MAXLENGTH];
	char    c, d;

	fprintf(fp, "%*s{\n", level * 4, "");
	Printf(4, ("\n"));

	if (*sysin)
		fprintf(fp, "# line %d \"%s\"\n", lineno, sysin);

	for (d = NULL; c = *action; d = c, action++) {
		Printf(4, ("open char is %c\n", *action));
		for (i = 0; i < MAXLENGTH - 1 && (isalpha(*action) || *action == '_'); action++, i++)
			t[i] = *action;
		t[i] = '\0';
		Printf(4, ("WORD IS %s, %c, %d\n", t, *action, i));
		if (strcmp(t, "")) {
			if (!strcmp(t, "parm"))
				fprintf(fp, "(%s)%s", new, "parm");
			else
				fprintf(fp, "%s", t);
			c = *(action - 1);
			action--;
			continue;
			/*
			 * OR d = *(action - 1); c = *action;
			 */
		}
		switch (d) {
		case '$':
			if (c == '$') {
				fprintf(fp, "%s", arg);
				c = NULL;
				break;
			}
			fputc('$', fp);	/* fall */

		default:
			if (c != '$')
				fputc(c, fp);
			break;
		}
	}

	switch (d) {
	case '\n':
		break;

	case '$':
		fputc('$', fp);		/* fall */
	default:
		fputc('\n', fp);
		break;
	}

	fprintf(fp, "%*s}\n", level * 4, "");
}

char   *
calc_arg(yp, actno)
YP      yp;
int     actno;
{

	switch (actno) {
	case 1:
		return "";

	case 2:
		if (yp->yp_direction & YP_ENCODER)
			return "(pe)";
		else
			return "(pe)";

	case 5:
		if (yp->yp_direction & YP_ENCODER)
			return "";
		else
			return "(pe)";
	default:
		return "";
	}
}
