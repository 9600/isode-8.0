/* task_select.c - tidy connection mesh and select next DSA activity */

#ifndef lint
static char *rcsid = "$Header: /xtel/isode/isode/quipu/RCS/task_select.c,v 9.0 1992/06/16 12:34:01 isode Rel $";
#endif

/*
 * $Header: /xtel/isode/isode/quipu/RCS/task_select.c,v 9.0 1992/06/16 12:34:01 isode Rel $
 *
 *
 * $Log: task_select.c,v $
 * Revision 9.0  1992/06/16  12:34:01  isode
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


#include "quipu/util.h"
#include "quipu/connection.h"

extern LLog * log_dsap;
extern time_t	conn_timeout;
extern time_t	nsap_timeout;
extern time_t	slave_timeout;
extern time_t	timenow;
time_t lastedb_update;
struct oper_act * pending_ops = NULLOPER;
extern char quipu_shutdown;

#ifndef NO_STATS

extern LLog * log_stat;
static time_t last_log_close = (time_t)0;
#define LOGOPENTIME 5*60	/* Close once every 5 minutes */

#endif


struct task_act *
task_select (int *secs_p) {
	struct connection	* cn;
	struct connection	* cn_tmp;
	struct connection	**next_cn;
	struct task_act	* tk;
	struct task_act	**next_tk;
	struct oper_act	* on;
	int			  timeout_tmp;
	char		  process_edbs = TRUE;
	char		  do_timeout;
	int			  suspended = FALSE;
	int 		  xi = 0;
	struct task_act	* ret_tk = NULLTASK;
	extern char	  startup_update;
	struct oper_act	* newop = NULLOPER;

	time (&timenow);
	(*secs_p) = NOTOK;
	conns_used = 0;

	/*
	    DLOG(log_dsap, LLOG_DEBUG, ("task_select connections:"));
	    conn_list_log(connlist);
	*/

	for(cn=connlist; cn!=NULLCONN; cn=cn_tmp) {
		cn_tmp = cn->cn_next;	/* Nasty but necessary in conn_extract()
				   manages to get itself called somehow */

		do_timeout = FALSE;

#ifdef DEBUG
		conn_log(cn,LLOG_DEBUG);
#endif

		next_tk = &(cn->cn_tasklist);
		for(tk=cn->cn_tasklist; tk!=NULLTASK; tk=(*next_tk)) {
			if(tk->tk_timed) {
				if(tk->tk_timeout <= timenow) {
#ifdef DEBUG
					struct UTCtime	  ut;
					struct UTCtime	  ut2;

					DLOG(log_dsap, LLOG_TRACE, ("task has timelimit of %ld", tk->tk_timeout));
					tm2ut(gmtime(&(tk->tk_timeout)), &ut);
					DLOG(log_dsap, LLOG_DEBUG, ("converted timelimit = %s", utct2str(&(ut))));
					tm2ut(gmtime(&(timenow)), &ut2);
					DLOG(log_dsap, LLOG_DEBUG, ("time now = %s", utct2str(&(ut2))));
#endif
					(*next_tk) = tk->tk_next;
					timeout_task(tk);
					continue;
				} else {
					timeout_tmp = (int) tk->tk_timeout - timenow;
					if(((*secs_p) == NOTOK) || ((*secs_p) > timeout_tmp)) {
						(*secs_p) = timeout_tmp;
					}
				}
			}

			next_tk = &(tk->tk_next);
		}

		if(cn->cn_state == CN_OPEN) {
			next_tk = &(cn->cn_tasklist);
			for(tk=cn->cn_tasklist; tk!=NULLTASK; tk=(*next_tk)) {
				next_tk = &(tk->tk_next);

				if(tk->tk_state == TK_ACTIVE) {
					if(   (ret_tk == NULLTASK)
							|| (tk->tk_prio > ret_tk->tk_prio)
							|| (   (tk->tk_prio == ret_tk->tk_prio)
								   && (   (!ret_tk->tk_timed)
										  || (   (tk->tk_timed)
												 && (tk->tk_timeout < ret_tk->tk_timeout)
											 )
									  )
							   )
					  ) {
						ret_tk = tk;
					}
				}

				if(tk->tk_state == TK_SUSPEND) {
					/*
					*  A task suspended to allow the network to be polled.
					*  Set suspended to force polling.
					*/
					tk->tk_state = TK_ACTIVE;
					suspended = TRUE;
				}
			}

			if(cn->cn_tasklist == NULLTASK) {
				if(cn->cn_initiator) {
					if(cn->cn_operlist == NULLOPER) {
						if((cn->cn_last_used + conn_timeout) <= timenow) {
							do_timeout = TRUE;
						} else {
							timeout_tmp = (int) (cn->cn_last_used + conn_timeout) - timenow;
							if(((*secs_p) == NOTOK) || ((*secs_p) > timeout_tmp)) {
								(*secs_p) = timeout_tmp;
							}
						}
					} else {
						timeout_tmp = conn_timeout;	/* safety catch */
						if ((tk = cn->cn_operlist->on_task) != NULLTASK) {
							if (tk->tk_timed) {
								timeout_tmp = (int) tk->tk_timeout - timenow;
								if (timeout_tmp < 0)
									timeout_tmp = 0;
							}
						}
						if(((*secs_p) == NOTOK) || ((*secs_p) > timeout_tmp)) {
							(*secs_p) = timeout_tmp;
						}
						cn->cn_last_used = timenow;
					}
				}
			} else {
				cn->cn_last_used = timenow;
				process_edbs = FALSE;
			}
		} else  {
			if((cn->cn_last_used + nsap_timeout) <= timenow) {
				if ((cn->cn_state == CN_CONNECTING1) || (cn->cn_state == CN_CONNECTING2))
					conn_retry(cn,1);
				else if (cn->cn_state == CN_CLOSING) {
					if (conn_release_retry(cn) == NOTOK) {
						/* had its chance - abort */
						conn_rel_abort (cn);
						do_ds_unbind(cn);
						conn_extract(cn);
					}
				} else if ( (cn->cn_state == CN_OPENING)
							|| (cn->cn_state == CN_PRE_OPENING) ) {
					/* something started to associate - then gave up !!! */
					conn_rel_abort (cn);
					conn_extract (cn);
				}
				(*secs_p) = nsap_timeout;
			} else {
				timeout_tmp = (int) (cn->cn_last_used + nsap_timeout) - timenow;
				if(((*secs_p) == NOTOK) || ((*secs_p) > timeout_tmp)) {
					(*secs_p) = timeout_tmp;
				}
			}
		}

		if(do_timeout) {
			LLOG(log_dsap, LLOG_TRACE, ("Timing out connection %d",cn->cn_ad));
			if (conn_release(cn) == NOTOK) {
				(*secs_p) = nsap_timeout;
				conns_used++;
			}
		} else {
			conns_used++;
		}
	}

	/*
	*  Open the connection with the highest priority operation
	*  waiting on it...
	*
	*  Get DSA Info operations are highest priority, followed by
	*  BIND_COMPARE, and X500, and finally GetEDB operations.
	*/

	next_cn = &(connwaitlist);
	for(cn=connwaitlist; cn!=NULLCONN; cn=(*next_cn)) {
		if(conns_used >= MAX_CONNS)
			break;

		for(on=cn->cn_operlist; on!=NULLOPER; on=on->on_next_conn) {
			if(on->on_type == ON_TYPE_GET_DSA_INFO) {
				(*next_cn) = cn->cn_next;
				if(conn_request(cn) == OK) {
					conns_used++;
					cn->cn_next = connlist;
					connlist = cn;
					cn->cn_last_used = timenow;
					/* Do something with the operations */
				} else {
					/* Do something with the operations */
				}
				break;
			}
		}
		if(on == NULLOPER)
			next_cn = &(cn->cn_next);
	}

	next_cn = &(connwaitlist);
	for(cn=connwaitlist; cn!=NULLCONN; cn=(*next_cn)) {
		if(conns_used >= (MAX_CONNS - CONNS_RESERVED_DI))
			break;

		for(on=cn->cn_operlist; on!=NULLOPER; on=on->on_next_conn) {
			if(on->on_type != ON_TYPE_GET_EDB) {
				(*next_cn) = cn->cn_next;
				if(conn_request(cn) == OK) {
					conns_used++;
					cn->cn_next = connlist;
					connlist = cn;
					cn->cn_last_used = timenow;
					/* Do something with the operations */
				} else {
					/* Do something with the operations */
				}
				break;
			}
		}
		if(on == NULLOPER)
			next_cn = &(cn->cn_next);
	}

	next_cn = &(connwaitlist);
	for(cn=connwaitlist; cn!=NULLCONN; cn=(*next_cn)) {
		if(conns_used >= (MAX_CONNS - CONNS_RESERVED_DI - CONNS_RESERVED_X500))
			break;

		(*next_cn) = cn->cn_next;
		if(conn_request(cn) == OK) {
			conns_used++;
			cn->cn_next = connlist;
			connlist = cn;
			cn->cn_last_used = timenow;
			/* Do something with the operations */
		} else {
			/* Do something with the operations */
		}
	}

	if(process_edbs && !quipu_shutdown) {
		/*
		*  Nothing is happening that would be disturbed by writing back
		*  a retrieved EDB so it is a good time to process them.
		*/

		if (!get_edb_ops && pending_ops) {

			get_edb_ops = pending_ops;
			pending_ops = NULLOPER;

			if(oper_chain(get_edb_ops) != OK) {
				LLOG(log_dsap, LLOG_TRACE, ("Could not chain a pending operation"));
				(*secs_p) = 0;  /* service network and then try next one */

				pending_ops = get_edb_ops -> on_next_task;
				get_edb_ops -> on_next_task = NULLOPER;
				oper_free(get_edb_ops);
				get_edb_ops = NULLOPER;
			}
		} else if (get_edb_ops) {
			if (get_edb_ops->on_state == ON_COMPLETE) {
				if (get_edb_ops->on_type == ON_TYPE_GET_EDB)
					process_edb(get_edb_ops,&newop);
				else { /* ON_TYPE_SHADOW */
					process_shadow(get_edb_ops);
					ds_res_free (&get_edb_ops->
								 on_resp.di_result.dr_res.dcr_dsres);
				}

				if (newop) {
					newop->on_next_task = get_edb_ops->on_next_task;
					get_edb_ops->on_next_task = NULLOPER;

					oper_conn_extract(get_edb_ops);
					oper_free(get_edb_ops);

					if (oper_send_invoke (newop) != OK) {
						LLOG(log_dsap, LLOG_EXCEPTIONS,
							 ("oper_send getedb next failed"));
						oper_free (newop);
						get_edb_ops = NULLOPER;
					}
					get_edb_ops = newop;

				} else  if (get_edb_ops) {

					pending_ops = get_edb_ops->on_next_task;
					get_edb_ops->on_next_task = NULLOPER;

					oper_conn_extract(get_edb_ops);
					oper_free(get_edb_ops);

					get_edb_ops = NULLOPER;
				}
				(*secs_p) = 0; /* Schedule next one ! */

			} else if (get_edb_ops->on_state == ON_ABANDONED) {
				LLOG (log_dsap,LLOG_TRACE,("Get edb has been abandoned"));

				pending_ops = get_edb_ops->on_next_task;
				get_edb_ops->on_next_task = NULLOPER;

				oper_free(get_edb_ops);

				get_edb_ops = NULLOPER;
				(*secs_p) = 0; /* Schedule next one ! */
			}

		} else if (startup_update) {
			/* see if cache timer has expired - if so resend edb ops... */
			if ( (timenow - lastedb_update) >= slave_timeout )
				slave_update();
		}
	}

	if ((get_edb_ops == NULLOPER) && startup_update ) {
		/* make sure we are awake for the next EDB update */
		if ((timeout_tmp = lastedb_update + slave_timeout - timenow) >= 0)
			if (((*secs_p) == NOTOK) || ((*secs_p) > timeout_tmp))
				(*secs_p) = timeout_tmp;
	}

	if(suspended) {
		/*
		*  A task suspended in order for the network to be checked.
		*  Force this to happen by setting the selected task to NULL
		*  and the polling time of the network to 0 secs.
		*/
		ret_tk = NULLTASK;
		(*secs_p) = 0;
	}

	for(cn=connwaitlist; cn!=NULLCONN; cn=cn->cn_next)
		xi++;

	/* If someting is waiting, see if we can shut a connection down */
	/* Make arbitary choice for now */
	for(cn=connlist; (xi!=0 || quipu_shutdown) && (cn!=NULLCONN); cn=cn_tmp) {
		cn_tmp = cn->cn_next;

		if ((cn->cn_state == CN_OPEN)
				&& (cn->cn_tasklist == NULLTASK)
				&& (cn->cn_initiator)
				&& (cn->cn_operlist == NULLOPER)) {

			LLOG(log_dsap, LLOG_TRACE, ("Releasing connection %d early (%d waiting)",cn->cn_ad,xi));

			if (conn_release(cn) == NOTOK)
				conns_used++;
			else
				xi--;

			(*secs_p) = 0;	/* let connection be re-used */
		}
	}

#ifndef NO_STATS
	if ( (timenow - last_log_close) >= LOGOPENTIME ) {
		ll_close (log_stat);
		last_log_close = timenow;
	} else {
		if ( (ret_tk == NULLTASK) && (*secs_p >= LOGOPENTIME))
			*secs_p = LOGOPENTIME;	/* Wake to close log! */
	}
#endif

	if(process_edbs && quipu_shutdown)
		*secs_p = NOTOK;

	return(ret_tk);
}

int
timeout_task (struct task_act *tk) {
	struct oper_act	* on;
	struct DSError	* err = &(tk->tk_resp.di_error.de_err);
	struct ds_search_task *tmp;

	DLOG(log_dsap, LLOG_TRACE, ("timeout_task"));
	for(on=tk->tk_operlist; on!=NULLOPER; on=on->on_next_task) {
		/* Time out operations started by task */
		on->on_state = ON_ABANDONED;
		on->on_task = NULLTASK;
		if (on->on_dsas) {
			di_desist (on->on_dsas);
			on -> on_dsas = NULL_DI_BLOCK;
		}

	}

	if(tk->tk_dx.dx_arg.dca_dsarg.arg_type != OP_SEARCH) {
		ds_error_free (err);
		err->dse_type = DSE_SERVICEERROR;
		if (tk->tk_timed == TRUE)
			err->ERR_SERVICE.DSE_sv_problem = DSE_SV_TIMELIMITEXCEEDED;
		else /* tk->tk_timed == 2 */
			err->ERR_SERVICE.DSE_sv_problem = DSE_SV_ADMINLIMITEXCEEDED;
		task_error(tk);
		task_extract(tk);
	} else {
		/* Do search collation */
		if ((tk->tk_state == TK_ACTIVE) && (tk->local_st == NULL_ST)) {
			ds_error_free (err);
			/* nothing happened yet... */
			err->dse_type = DSE_SERVICEERROR;
			if (tk->tk_timed == TRUE)
				err->ERR_SERVICE.DSE_sv_problem = DSE_SV_TIMELIMITEXCEEDED;
			else /* tk->tk_timed == 2 */
				err->ERR_SERVICE.DSE_sv_problem = DSE_SV_ADMINLIMITEXCEEDED;
			task_error(tk);
		} else {
			/* send the results we have got... */
			tk->tk_result = &(tk->tk_resp.di_result.dr_res);
			tk->tk_result->dcr_dsres.result_type = tk->tk_dx.dx_arg.dca_dsarg.arg_type;
			tk->tk_resp.di_type = DI_RESULT;
			if (tk->tk_timed == TRUE)
				tk->tk_resp.di_result.dr_res.dcr_dsres.res_sr.CSR_limitproblem = LSR_TIMELIMITEXCEEDED;
			else /* tk->tk_timed == 2 */
				tk->tk_resp.di_result.dr_res.dcr_dsres.res_sr.CSR_limitproblem = LSR_ADMINSIZEEXCEEDED;

			/* Go through sub-tasks and add a POQ for each */
			for(tmp=tk->referred_st; tmp!= NULL_ST; tmp=tmp->st_next)
				add_cref2poq (&tk->tk_result->dcr_dsres.res_sr,tmp->st_cr);

			task_result(tk);

			st_free_dis(&tk->referred_st,1);
		}

		task_extract(tk);
	}

}


int
schedule_operation (struct oper_act *x) {
	struct oper_act *y;
	struct oper_act * on;
	DN xdn;

	if (x->on_dsas)
		xdn = x->on_dsas->di_dn;

	if (on = pending_ops) {	/* assign */
		/* group ops for same DSA together */
		for ( ; on->on_next_task != NULLOPER; on = on->on_next_task)
			if (( on->on_dsas )
					&& (dn_cmp (on->on_dsas->di_dn,xdn) == 0))
				break;

		y = on->on_next_task;
		on->on_next_task = x;
		x->on_next_task = y;
	} else
		pending_ops = x;
}

