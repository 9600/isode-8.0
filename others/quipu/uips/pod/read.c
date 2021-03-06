#include "quipu/util.h"
#include "quipu/common.h"
#include "quipu/entry.h"
#include "quipu/name.h"

#include <sys/types.h>
#include <ctype.h>

#include "sequence.h"
#include "defs.h"
#include "util.h"

#ifndef NO_STATS
extern LLog    *log_stat;
#endif

int as_print();
void kill_photo();
void readEntryPrint();

extern char goto_path[], base_path[], friendly_base_path[];
extern int dn_number, two_passes;

mailtype mailformat = rfc822;
void rfc2greybook();

Attr_Sequence read_types = 0, read_types2 = 0, oclass = 0;
bool photo_on = TRUE;

dsEnqError do_read(), read_all(), read_config_types();

Attr_Sequence sort_attrs(entry_attrs)
Attr_Sequence entry_attrs;
{
	Attr_Sequence last, next, curr, first, firstn;

	first = curr = entry_attrs;
	firstn = last = next = 0;

	while (curr)
		if (!strcmp("2.5.4.3", curr->attr_type->oa_ot.ot_stroid) ||
				!strcmp("2.5.4.4", curr->attr_type->oa_ot.ot_stroid) ||
				!strcmp("0.9.2342.19200300.100.1.3",
						curr->attr_type->oa_ot.ot_stroid) ||
				!strcmp("0.9.2342.19200300.100.1.2",
						curr->attr_type->oa_ot.ot_stroid) ||
				!strcmp("2.5.4.20", curr->attr_type->oa_ot.ot_stroid)) {

			if (first == curr) first = curr->attr_link;

			if (next)
				next->attr_link = curr;
			else
				firstn = curr;

			next = curr;

			if (last)
				last->attr_link = curr->attr_link;

			curr = curr->attr_link;
			next->attr_link = 0;
		} else {
			last = curr;
			curr = curr->attr_link;
		}

	if (next) {
		next->attr_link = first;
		return firstn;
	} else
		return first;
}


Attr_Sequence get_sorted_attrs(entry_attrs, wanted_attrs)
Attr_Sequence entry_attrs, wanted_attrs;
{
	Attr_Sequence curr_ent_attr, curr_found,
				  curr_wanted, attrs_found, next;

	attrs_found = NULLATTR;
	curr_wanted = wanted_attrs;
	curr_ent_attr = entry_attrs;

	while (curr_wanted != NULLATTR) {
		while (curr_ent_attr != NULLATTR) {
			if (AttrT_cmp(curr_wanted->attr_type, curr_ent_attr->attr_type) == 0) {
				next = curr_ent_attr->attr_link;
				curr_ent_attr->attr_link = NULLATTR;

				if (attrs_found == NULLATTR) {
					curr_found = attrs_found = as_cpy(curr_ent_attr);
				} else {
					curr_found->attr_link = as_cpy(curr_ent_attr);
					curr_found = curr_found->attr_link;
				}

				curr_ent_attr->attr_link = next;
				curr_ent_attr = next;
			} else
				curr_ent_attr = curr_ent_attr->attr_link;
		}
		curr_wanted = curr_wanted->attr_link;
		curr_ent_attr = entry_attrs;
	}

	curr_wanted = attrs_found;

	attrs_found = sort_attrs(attrs_found);
	return attrs_found;
}

dsEnqError read_all() {
	return do_read(NULLATTR);
}

dsEnqError read_config_types() {
	return do_read(read_types);
}

dsEnqError do_read(attrs_to_read)
Attr_Sequence attrs_to_read;
{
	struct ds_read_arg read_arg;
	struct ds_read_result   result;
	struct DSError          error;
	Entry read_entry;
	Attr_Sequence wanted_attrs;
	extern void setReadEntryName();

	if (*base_path == '\0') {
		kill_photo();
		setReadEntryName(base_path);
		readEntryPrint("description - Big and Round (but getting smaller).\n");
		return Okay;
	}

	if (get_default_service (&read_arg.rda_common) != 0) {
		return nothingfound;
	}

	read_arg.rda_common.ca_servicecontrol.svc_options = SVC_OPT_PREFERCHAIN;

	read_arg.rda_eis.eis_allattributes = TRUE;
	read_arg.rda_eis.eis_infotypes = EIS_ATTRIBUTESANDVALUES;
	read_arg.rda_eis.eis_select = NULLATTR;

	read_arg.rda_object = (strncmp(friendly_base_path, "The World", 9)?
						   str2dn(base_path):
						   NULLDN);

	if ((read_entry = local_find_entry(read_arg.rda_object, FALSE))
			!= NULLENTRY &&
			read_entry->e_data != E_TYPE_CONSTRUCTOR) {
		kill_photo();
		setReadEntryName(base_path);

		if (attrs_to_read != NULLATTR)
			wanted_attrs = get_sorted_attrs(read_entry->e_attributes,
											attrs_to_read);
		else wanted_attrs = read_entry->e_attributes;

		read_print(as_print, (caddr_t) wanted_attrs);
		dn_free(read_arg.rda_object);

		if (wanted_attrs != read_entry->e_attributes)
			as_free(wanted_attrs);

		return Okay;
	}

#ifndef NO_STATS
	LLOG (log_stat, LLOG_NOTICE, ("read +%s",base_path));
#endif

	if (ds_read (&read_arg, &error, &result) != DS_OK) {
		int errcode;

		/* deal with error */
		log_ds_error(&error);

		errcode = error.dse_type;

		dn_free(read_arg.rda_object);
		ds_error_free(&error);

		switch (errcode) {
		case DSE_LOCALERROR:
			return duaerror;

		case DSE_REMOTEERROR:
			return localdsaerror;

		case DSE_ATTRIBUTEERROR:
			return attributerror;

		case DSE_REFERRAL:
		case DSE_DSAREFERRAL:
			return remotedsaerror;

		case DSE_SECURITYERROR:
			return security;

		case DSE_NAMEERROR:
			return namerror;

		case DSE_SERVICEERROR:
			return serviceerror;

		default:
			return localdsaerror;
		}
	} else {
		/* use data */
		if (result.rdr_entry.ent_attr == NULLATTR) {
			dn_number = 0;
			return nothingfound;
		} else {
			kill_photo();
			cache_entry(&(result.rdr_entry), TRUE, TRUE);

			if (attrs_to_read != NULLATTR)
				wanted_attrs =
					get_sorted_attrs(result.rdr_entry.ent_attr, attrs_to_read);
			else
				wanted_attrs = result.rdr_entry.ent_attr;

			setReadEntryName(base_path);

			read_print(as_print, (caddr_t) wanted_attrs);

			if (wanted_attrs != result.rdr_entry.ent_attr)
				as_free(wanted_attrs);

			entryinfo_comp_free(&result.rdr_entry, 0);

			dn_free(read_arg.rda_object);
			return Okay;
		}
	}
}

int
read_print (int (*func), caddr_t ptr) {
	PS ps;
	char *str;

	if ((ps = ps_alloc(str_open)) == NULLPS) return;
	if (str_setup(ps, NULLCP, 0, 0) == NOTOK) return;

	(*func) (ps, ptr, READOUT);
	*--ps->ps_ptr = NULL, ps->ps_cnt++;

	str = ps->ps_base;

	ps->ps_base = NULL;
	ps->ps_cnt = 0;
	ps->ps_ptr = NULL;
	ps->ps_bufsiz = 0;

	ps_free(ps);

	readEntryPrint(str);
	free(str);
}

/*ARGSUSED*/
podphoto(ps, picture, format)
PS ps;
PE picture;
int format;
{
	PS sps;

	if (photo_on == TRUE) {
		if ((sps = ps_alloc(str_open)) == NULLPS)
			return;
		if (str_setup (sps, NULLCP, LINESIZE, 0) == NOTOK) {
			ps_free(sps);
			return;
		}

		two_passes = 0;

		pe2ps(sps, picture);
		if (decode_t4(sps->ps_base, "photo", sps->ps_ptr - sps->ps_base ) == -1)
			goto out;

		if (two_passes &&
				decode_t4 (sps->ps_base, "photo", sps->ps_ptr - sps->ps_base) == -1 )
			goto out;

		ps_print(ps,"(see below)");

out:
		;
		ps_free(sps);
	}
}

int
entry2str (caddr_t ptr, char *cptr, int size) {
	PS ps;

	if ((ps = ps_alloc (str_open)) == NULLPS) return ;
	if (str_setup (ps, cptr, size, 1) == NOTOK) return ;

	as_print(ps, (Attr_Sequence)ptr, READOUT);
	*ps->ps_ptr = 0;

	ps_free(ps);
}

void
rfc2greybook (char *string) {
	char reversed[STRINGLEN];
	char *part;

	reversed[0] = '\0';
	part = string + strlen(string);

	if (*part != '\0') return;

	while(1) {
		while (*part != '.' && *part != '@') --part;

		if (*part == '.') {
			if (reversed[0] != '\0')  strcat(reversed, ".");
			part++;
			strcat(reversed, part);
			*--part = '\0';
			--part;
		} else {
			part++;
			strcat(reversed, ".");
			strcat(reversed, part);
			*part = '\0';
			/*       while (!isspace(*part)) --part; */
			strcat(string, reversed);
			return;
		}
	}
}

