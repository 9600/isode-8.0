/* country.c - search for a country */

#ifndef	lint
static char *rcsid = "$Header: /xtel/isode/isode/others/quipu/uips/de/RCS/country.c,v 9.1 1992/08/25 15:50:26 isode Exp $";
#endif

/*
 * $Header: /xtel/isode/isode/others/quipu/uips/de/RCS/country.c,v 9.1 1992/08/25 15:50:26 isode Exp $
 *
 *
 * $Log: country.c,v $
 * Revision 9.1  1992/08/25  15:50:26  isode
 * PARADISE release
 *
 * Revision 8.1  1991/09/13  14:36:41  isode
 * PARADISE Upgrade release
 *
 * Revision 8.0  91/07/17  13:18:38  isode
 * Release 7.0
 *
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


#include <signal.h>
#include "quipu/util.h"
#include "quipu/read.h"
#include "quipu/list.h"
#include "quipu/ds_search.h"
#include "quipu/dua.h"
#include "demanifest.h"
#include "namelist.h"
#include "filter.h"
#include "destrings.h"

struct ds_read_arg rarg;
struct ds_read_result rresult;

struct ds_list_arg larg;
struct ds_list_result lresult;

struct ds_search_arg sarg;
struct ds_search_result sresult;
struct DSError serror;

extern int abandoned;
extern int highNumber;
extern int exactMatch;
extern int fuzzyMatching;
extern char exactString[];

extern struct namelist * coatts;

int listAtRoot = TRUE;

struct ds_read_arg *fillMostCountryReadArgs();
struct ds_list_arg *fillMostCountryListArgs();
struct ds_search_arg *fillMostCountrySearchArgs();

void makeExplicitCoFilter();
void coFilter1(), coFilter2(), coFilter3(), coFilter4();

VFP explicitCo[] = {makeExplicitCoFilter, NULLVFP};
VFP normalCo[] = {coFilter1, coFilter2, coFilter3, coFilter4, NULLVFP};

int
makeRootCountry(clistp)
struct namelist ** clistp;
{
	*clistp = list_alloc();
	(*clistp)->name = copy_string("root");
	(*clistp)->ats = NULLATTR;
	(*clistp)->next = NULLLIST;
	return OK;
}


int
listCos(cstr, clistp)
char * cstr;
struct namelist ** clistp;
{
	clearProblemFlags();
	initAlarm();
	if (exactMatch == COUNTRY)
		return(listExactCos(exactString, clistp));
	else if (strcmp(cstr, "*") == 0)
		return(listAllCos(clistp));
	else if (strcmp(cstr, "-") == 0)
		return(makeRootCountry(clistp)); /* make the root a "country" to
      	                       enable searching under the root of the tree */
	else
		return(listMatchingCos(cstr, clistp));
}

void
printListCos(cstr, clistp)
char * cstr;
struct namelist * clistp;
{
	struct namelist * x;
	int i;

	if (clistp == NULLLIST)
		pageprint("No countries match entered string\n");
	else {
		for (i =1, x = clistp; x != NULLLIST; i++, x = x->next)
			printLastComponent(INDENTON, x->name, COUNTRY, i);
		showAnyProblems(cstr);
	}
}

void
freeCos(listpp)
struct namelist ** listpp;
{
	struct namelist * x, * y;

	x = *listpp;
	while (x != NULLLIST) {
		if (x->name != NULLCP)
			free(x->name);
		as_free(x->ats);
		y = x->next;
		free((char *)x);
		x = y;
	}
	*listpp = NULLLIST;
}

void
freeCoListArgs() {
	dn_free(larg.lsa_object);
}

void
freeCoSearchArgs() {
	dn_free(sarg.sra_baseobject);
}

listAllCos(clistp)
struct namelist ** clistp;
{
	int ret;

	if (listAtRoot == TRUE) {
		larg = * fillMostCountryListArgs();
		ret = reallyMakeListCountries(clistp);
	} else {
		sarg = * fillMostCountrySearchArgs(NULLCP, SRA_ONELEVEL);
		makeAllCoFilter(&sarg.sra_filter);
		ret = makeListCountries(clistp);
	}
	if (ret != OK)
		logListSuccess(LIST_ERROR, "co", 0);
	else
		logListSuccess(LIST_OK, "co", listlen(*clistp));

	if (listAtRoot == TRUE)
		freeCoListArgs();
	else
		freeCoSearchArgs();
	alarmCleanUp();
	return ret;
}

listMatchingCos(cstr, clistp)
char * cstr;
struct namelist ** clistp;
{
	VFP * filtarray;
	VFP filterfunc;
	int filtnumber;
	int ret;
	char str[5];

	/* if 2 letter country entered - let's see if we can read the entry*/
	if (strlen(cstr) == 2) {
		sprintf(str, "%s=%s", SHORT_CO, cstr);
		ret = readCo(str, clistp);
		if (ret == OK) {
			alarmCleanUp();
			return OK;
		}
	}

	if (index(cstr, '*') != NULLCP) { /* contains at least one asterisk */
		filtarray = explicitCo;
		filtnumber = -1;
	} else {
		filtarray = normalCo;
		filtnumber = 0;
	}
	sarg = * fillMostCountrySearchArgs(NULLCP, SRA_ONELEVEL);
	while ((filterfunc = *filtarray++) != NULLVFP) {
		filtnumber++;
		filterfunc(cstr, &sarg.sra_filter);
		if (sarg.sra_filter == NULLFILTER)
			continue;
		if (makeListCountries(clistp) != OK) {
			freeCoSearchArgs();
			logSearchSuccess(SEARCH_ERROR, "co", cstr, filtnumber, 0);
			alarmCleanUp();
			return NOTOK;
		}
		if (*clistp != NULLLIST)
			break;
	}
	if (*clistp != NULLLIST)
		logSearchSuccess(SEARCH_OK, "co", cstr, filtnumber, listlen(*clistp));
	else
		logSearchSuccess(SEARCH_FAIL, "co", cstr, filtnumber, 0);
	freeCoSearchArgs();
	alarmCleanUp();
	return OK;
}

readCo(objectstr, clistp)
char * objectstr;
struct namelist ** clistp;
{
	int retval;

	rarg = * fillMostCountryReadArgs(objectstr);
	retval = ds_read(&rarg, &serror, &rresult);
	if ((retval == DSE_INTR_ABANDONED) &&
			(serror.dse_type == DSE_ABANDONED))
		abandoned = TRUE;
	if (retval != OK)
		return NOTOK;
	/* setProblemFlags(rresult); */

	*clistp = list_alloc();
	(*clistp)->name = dn2pstr(rresult.rdr_entry.ent_dn);
	(*clistp)->ats = as_cpy(rresult.rdr_entry.ent_attr);
	(*clistp)->next = NULLLIST;
	highNumber = 1;
	return OK;

}

listExactCos(objectstr, clistp)
char * objectstr;
struct namelist ** clistp;
{
	int ret;

	sarg = * fillMostCountrySearchArgs(objectstr, SRA_BASEOBJECT);
	makeAllCoFilter(&sarg.sra_filter);
	ret = makeListCountries(clistp);
	freeCoSearchArgs();
	alarmCleanUp();
	return ret;
}

reallyMakeListCountries(clistp)
struct namelist ** clistp;
{
	struct subordinate * x;
	int retval;
	char * cp;

	if (rebind() != OK)
		return NOTOK;
	retval = ds_list(&larg, &serror, &lresult);
	if ((retval == DSE_INTR_ABANDONED) &&
			(serror.dse_type == DSE_ABANDONED))
		abandoned = TRUE;

	/* setProblemFlags(sresult); */
	highNumber = 0;
	for (x = lresult.lsr_subordinates; x != (struct subordinate *) NULL;
			x = x->sub_next) {
		/* only add countries and localities to the list */
		cp = strdup(attr2name(x->sub_rdn->rdn_at, OIDPART));
		if ((strcmp(cp, DE_COUNTRY_NAME) == 0) ||
				(strcmp(cp, DE_LOCALITY_NAME) == 0)) {
			*clistp = list_alloc();
			(*clistp)->name = rdn2pstr(x->sub_rdn);
			(*clistp)->ats = NULLATTR;
			clistp = &(*clistp)->next;
			highNumber++;
		}
		free(cp);
	}
	*clistp = NULLLIST;
	/* free something */
	dn_free (lresult.lsr_object);
	/* free something else */
	return OK;
}

makeListCountries(clistp)
struct namelist ** clistp;
{
	entrystruct * x;
	int retval;

	if (rebind() != OK)
		return NOTOK;
	retval = ds_search(&sarg, &serror, &sresult);
	if ((retval == DSE_INTR_ABANDONED) &&
			(serror.dse_type == DSE_ABANDONED))
		abandoned = TRUE;
	correlate_search_results (&sresult);

	setProblemFlags(sresult);

	highNumber = 0;
	for (x = sresult.CSR_entries; x != NULLENTRYINFO; x = x->ent_next) {
		*clistp = list_alloc();
		(*clistp)->name = dn2pstr(x->ent_dn);
		(*clistp)->ats = as_cpy(x->ent_attr);
		clistp = &(*clistp)->next;
		highNumber++;
	}
	*clistp = NULLLIST;
	entryinfo_free(sresult.CSR_entries,0);
	dn_free (sresult.CSR_object);
	crefs_free (sresult.CSR_cr);
	filter_free(sarg.sra_filter);
	return OK;
}

struct ds_read_arg *
fillMostCountryReadArgs(objectstr)
char * objectstr;
{
	static struct ds_read_arg arg;
	static CommonArgs sca = default_common_args;
	Attr_Sequence * atl;
	AttributeType at;
	struct namelist * x;

	arg.rda_common =sca; /* struct copy */
	arg.rda_common.ca_servicecontrol.svc_timelimit = SVC_NOTIMELIMIT;
	arg.rda_common.ca_servicecontrol.svc_sizelimit= SVC_NOSIZELIMIT;

	arg.rda_object = str2dn(objectstr);
	/* specify attributes of interest */
	arg.rda_eis.eis_allattributes = FALSE;
	atl = &(arg.rda_eis.eis_select);
	for (x = coatts; x != NULLLIST; x = x->next) {
		if ((at = str2AttrT(x->name)) == NULLAttrT)
			continue;
		*atl = as_comp_alloc();
		(*atl)->attr_type = at;
		(*atl)->attr_value = NULLAV;
		atl = &(*atl)->attr_link;
	}
	*atl = NULLATTR;
	arg.rda_eis.eis_infotypes = EIS_ATTRIBUTESANDVALUES;
	return (&arg);
}

struct ds_list_arg *
fillMostCountryListArgs() {
	static struct ds_list_arg arg;
	static CommonArgs sca = default_common_args;

	arg.lsa_common =sca; /* struct copy */
	arg.lsa_common.ca_servicecontrol.svc_timelimit = SVC_NOTIMELIMIT;
	arg.lsa_common.ca_servicecontrol.svc_sizelimit= SVC_NOSIZELIMIT;

	arg.lsa_object = str2dn("");
	return (&arg);
}

struct ds_search_arg *
fillMostCountrySearchArgs(objectstr, searchdepth)
char * objectstr;
int searchdepth;
{
	static struct ds_search_arg arg;
	static CommonArgs sca = default_common_args;
	Attr_Sequence * atl;
	AttributeType at;
	struct namelist * x;

	arg.sra_common = sca; /* struct copy */
	arg.sra_common.ca_servicecontrol.svc_timelimit = SVC_NOTIMELIMIT;
	arg.sra_common.ca_servicecontrol.svc_sizelimit= SVC_NOSIZELIMIT;

	arg.sra_subset = searchdepth;
	arg.sra_baseobject = str2dn(objectstr);
	/* specify attributes of interest */
	arg.sra_eis.eis_allattributes = FALSE;
	atl = &(arg.sra_eis.eis_select);
	for (x = coatts; x != NULLLIST; x = x->next) {
		if ((at = str2AttrT(x->name)) == NULLAttrT)
			continue;
		*atl = as_comp_alloc();
		(*atl)->attr_type = at;
		(*atl)->attr_value = NULLAV;
		atl = &(*atl)->attr_link;
	}
	*atl = NULLATTR;
	arg.sra_eis.eis_infotypes = EIS_ATTRIBUTESANDVALUES;
	return (&arg);
}

makeAllCoFilter(fpp)
struct s_filter ** fpp;
{
	struct s_filter * fp;

	*fpp = orfilter();
	fp = (*fpp)->FUFILT = eqfilter(FILTERITEM_EQUALITY, DE_OBJECT_CLASS, DE_COUNTRY);
	fp->flt_next = eqfilter(FILTERITEM_EQUALITY, DE_OBJECT_CLASS, DE_LOCALITY);
}

void
makeExplicitCoFilter(cstr, fpp)
char * cstr;
struct s_filter ** fpp;
{
	struct s_filter * fp, *fp2;
	int wildcardtype;
	char * ostr1, * ostr2;

	wildcardtype = starstring(cstr, &ostr1, &ostr2);
	*fpp = orfilter();
	fp = fp2 = (*fpp)->FUFILT = andfilter();
	fp = fp->FUFILT = eqfilter(FILTERITEM_EQUALITY, DE_OBJECT_CLASS, DE_COUNTRY);
	switch (wildcardtype) {
	case LEADSUBSTR: /* fall through */
	case TRAILSUBSTR: /* fall through */
	case ANYSUBSTR:
		fp = fp->flt_next = subsfilter(wildcardtype,
									   DE_FRIENDLY_COUNTRY, ostr1);
		break;
	case LEADANDTRAIL:
		fp = fp->flt_next = subsfilter(LEADSUBSTR,
									   DE_FRIENDLY_COUNTRY, ostr1);
		fp = fp->flt_next = subsfilter(TRAILSUBSTR,
									   DE_FRIENDLY_COUNTRY, ostr2);
		break;
	}
	fp = fp2->flt_next = andfilter();
	fp = fp->FUFILT = eqfilter(FILTERITEM_EQUALITY, DE_OBJECT_CLASS, DE_LOCALITY);
	switch (wildcardtype) {
	case LEADSUBSTR: /* fall through */
	case TRAILSUBSTR: /* fall through */
	case ANYSUBSTR:
		fp = fp->flt_next = subsfilter(wildcardtype,
									   DE_LOCALITY_NAME, ostr1);
		break;
	case LEADANDTRAIL:
		fp = fp->flt_next = subsfilter(LEADSUBSTR,
									   DE_LOCALITY_NAME, ostr1);
		fp = fp->flt_next = subsfilter(TRAILSUBSTR,
									   DE_LOCALITY_NAME, ostr2);
		break;
	}

}

void
coFilter1(cstr, fpp)
char * cstr;
struct s_filter ** fpp;
{
	struct s_filter * fp, * fp2;

	*fpp = orfilter();
	fp = fp2 = (*fpp)->FUFILT = andfilter();
	fp = fp->FUFILT = eqfilter(FILTERITEM_EQUALITY, DE_OBJECT_CLASS, DE_COUNTRY);
	if ((strlen(cstr) == 2) && (check_3166(cstr)))
		fp = fp->flt_next = eqfilter(FILTERITEM_EQUALITY, DE_COUNTRY_NAME, cstr);
	else
		fp = fp->flt_next = eqfilter(FILTERITEM_EQUALITY, DE_FRIENDLY_COUNTRY, cstr);
	fp = fp2->flt_next = andfilter();
	fp = fp->FUFILT = eqfilter(FILTERITEM_EQUALITY, DE_OBJECT_CLASS, DE_LOCALITY);
	fp = fp->flt_next = eqfilter(FILTERITEM_EQUALITY, DE_LOCALITY_NAME, cstr);
}

void
coFilter2(cstr, fpp)
char * cstr;
struct s_filter ** fpp;
{
	struct s_filter * fp, * fp2;

	*fpp = orfilter();
	fp = fp2 = (*fpp)->FUFILT = andfilter();
	fp = fp->FUFILT = eqfilter(FILTERITEM_EQUALITY, DE_OBJECT_CLASS, DE_COUNTRY);
	fp = fp->flt_next = subsfilter(LEADSUBSTR, DE_FRIENDLY_COUNTRY, cstr);
	fp = fp2->flt_next = andfilter();
	fp = fp->FUFILT = eqfilter(FILTERITEM_EQUALITY, DE_OBJECT_CLASS, DE_LOCALITY);
	fp = fp->flt_next = subsfilter(LEADSUBSTR, DE_LOCALITY_NAME, cstr);
}

void
coFilter3(cstr, fpp)
char * cstr;
struct s_filter ** fpp;
{
	struct s_filter * fp, * fp2;

	*fpp = orfilter();
	fp = fp2 = (*fpp)->FUFILT = andfilter();
	fp = fp->FUFILT = eqfilter(FILTERITEM_EQUALITY, DE_OBJECT_CLASS, DE_COUNTRY);
	fp = fp->flt_next = subsfilter(ANYSUBSTR, DE_FRIENDLY_COUNTRY, cstr);
	fp = fp2->flt_next = andfilter();
	fp = fp->FUFILT = eqfilter(FILTERITEM_EQUALITY, DE_OBJECT_CLASS, DE_LOCALITY);
	fp = fp->flt_next = subsfilter(ANYSUBSTR, DE_LOCALITY_NAME, cstr);
}

void
coFilter4(cstr, fpp)
char * cstr;
struct s_filter ** fpp;
{
	struct s_filter * fp, * fp2;

	if (fuzzyMatching == FALSE) {
		*fpp = NULLFILTER;
		return;
	}
	*fpp = orfilter();
	fp = fp2 = (*fpp)->FUFILT = andfilter();
	fp = fp->FUFILT = eqfilter(FILTERITEM_EQUALITY, DE_OBJECT_CLASS, DE_COUNTRY);
	fp = fp->flt_next = eqfilter(FILTERITEM_APPROX, DE_FRIENDLY_COUNTRY, cstr);
	fp = fp2->flt_next = andfilter();
	fp = fp->FUFILT = eqfilter(FILTERITEM_EQUALITY, DE_OBJECT_CLASS, DE_LOCALITY);
	fp = fp->flt_next = eqfilter(FILTERITEM_APPROX, DE_LOCALITY_NAME, cstr);
}
