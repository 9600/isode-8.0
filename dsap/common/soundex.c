/* soundex.c - compare a filter and attribute value */

#ifndef lint
static char *rcsid = "$Header: /xtel/isode/isode/dsap/common/RCS/soundex.c,v 9.0 1992/06/16 12:12:39 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/dsap/common/RCS/soundex.c,v 9.0 1992/06/16 12:12:39 isode Rel $
 *
 *
 * $Log: soundex.c,v $
 * Revision 9.0  1992/06/16  12:12:39  isode
 * Release 8.0
 *
 */

/*
 *                                NOTICE
 *
 *    Acquisition, use, and distribution of this module and related
 *    materials are subject to the restrictions of a license agreement.
 *    Consult the Preface in the User's Manual for the full terms of
 *    this agreement.
 *
 */

#include <ctype.h>
#include "quipu/util.h"
#include "quipu/attrvalue.h"
#include "quipu/ds_search.h"

extern char chrcnv[];
extern LLog *log_dsap;

#ifndef SOUNDEX_LEN
#define SOUNDEX_LEN	-1
#endif

#define iswordbreak(x)	(!isascii(x) || isspace((unsigned char) (x)) || \
	ispunct((unsigned char) (x)) || \
	isdigit((unsigned char) (x)) || x == '\0')

char *
first_word (char *ptr) {
	if ( ptr == NULLCP )
		return NULLCP;

	while (iswordbreak(*ptr))
		if (*ptr == 0)
			return NULLCP;
		else
			ptr++;

	return(ptr);
}

char *
next_word (char *ptr) {
	if ( ptr == NULLCP )
		return NULLCP;

	for (;;) {
		if (*ptr == 0)
			return NULLCP;
		else if (iswordbreak(*ptr)) {
			while (*ptr && iswordbreak(*ptr))
				ptr++;
			if (*ptr == 0)
				return NULLCP;
			else
				return(ptr);
		} else
			ptr++;
	}
	/* NOTREACHED */
}

int
soundex (char *s, char **c) {
	char code, adjacent, ch, *p;
	int i, cmax;

	p = s;
	if ( *p == '\0' ) {
		*c = (char *) malloc(sizeof(char));
		**c = '\0';
		return;
	}

	/* assume at least four...realloc if we need to */
	*c = (char *) malloc(5*sizeof(char));
	cmax = 4;

	adjacent = '0';
	if ( islower( *p ) )
		(*c)[0] = toupper(*p);
	else
		(*c)[0] = *p;

	(*c)[1]  = '\0';
	for ( i = 0; i < 99 && (! iswordbreak(*p)); p++ ) {
		if ( islower( *p ) )
			ch = toupper (*p);
		else
			ch = *p;

		code = '0';

		switch (ch) {
		case 'B':
		case 'F':
		case 'P':
		case 'V':
			code = (adjacent != '1') ? '1' : '0';
			break;
		case 'S':
		case 'C':
		case 'G':
		case 'J':
		case 'K':
		case 'Q':
		case 'X':
		case 'Z':
			code = (adjacent != '2') ? '2' : '0';
			break;
		case 'D':
		case 'T':
			code = (adjacent != '3') ? '3' : '0';
			break;
		case 'L':
			code = (adjacent != '4') ? '4' : '0';
			break;
		case 'M':
		case 'N':
			code = (adjacent != '5') ? '5' : '0';
			break;
		case 'R':
			code = (adjacent != '6') ? '6' : '0';
			break;
		default:
			adjacent = '0';
		}

		if ( i == 0 ) {
			adjacent = code;
			i++;
		} else if (code != '0' ) {
			if (i == SOUNDEX_LEN)
				break;
			if (i == cmax) {
				*c = (char *) realloc(*c, (unsigned) (2 * cmax *
													  sizeof(char) + 1));
				cmax *= 2;
			}
			adjacent = (*c)[i] = code;
			i++;
		}
	}

	if (i > 0)
		(*c)[i] = '\0';

	return;
}

static char	*g_bcode;
static int	g_bcodelen;

static
match_word (char *a) {
	char	*as;
	int	cmp;

	soundex(a, &as);

#ifdef SOUNDEX_PREFIX
	cmp = strncmp(as, g_bcode, g_bcodelen);
#else
	cmp = strcmp(as, g_bcode);
#endif
	free(as);
	return(cmp == 0);
}

int
soundex_cmp (char *a, char *b) {
	char result = FALSE;
	char * ptr;

	for( ; a && b ; b = next_word (b) ) {
		soundex(b, &g_bcode);
		g_bcodelen = strlen(g_bcode);
		for (ptr=first_word(a); ptr; ptr=next_word(ptr) )  {
			if (match_word (ptr)) {
				a = next_word (ptr);
				result = TRUE;
				break;
			}
		}
		free(g_bcode);
		if (ptr == NULLCP)
			return FALSE;
	}
	if ((a == NULL) && (b != NULL))
		return FALSE;

	return (result);
}

soundex_match (fitem,avs)
struct filter_item *fitem;
AV_Sequence avs;
{
	for (; avs != NULLAV; avs=avs->avseq_next)
		if (soundex_cmp ((char *)avs->avseq_av.av_struct, (char *)fitem->UNAVA.ava_value->av_struct))
			return (OK);

	return (NOTOK);
}
