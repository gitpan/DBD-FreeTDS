/*
 * Copyright 1998 CDS Networks, Inc., Medford Oregon
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by CDS Networks, Inc.
 * 4. The name of CDS Networks, Inc.  may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CDS NETWORKS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL CDS NETWORKS, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */ 


#ifndef FREETDS_h
#define FREETDS_h



/*
 * DBIXS.h doesn't prevent multiple inclusions of itself, so we have to 
 * do it by hand.  (Some people's kids)
 */
#ifndef DBIXS_VERSION
#define NEED_DBIXS_VERSION 93
#include <DBIXS.h>
#endif




static char  rcsid_FreeTDS_h [ ] =
         "$Id: FreeTDS.h,v 1.2 1998/12/31 23:46:07 cts Exp $";
static void *no_unused_FreeTDS_h_warn[]={rcsid_FreeTDS_h, 
                                         no_unused_FreeTDS_h_warn};


#define dbd_init            tds_init
#define dbd_db_login        tds_db_login
#define dbd_db_do           tds_db_do
#define dbd_db_commit       tds_db_commit
#define dbd_db_rollback     tds_db_rollback
#define dbd_db_disconnect   tds_db_disconnect
#define dbd_discon_all      tds_discon_all
#define dbd_db_destroy      tds_db_destroy
#define dbd_db_STORE_attrib tds_db_STORE_attrib
#define dbd_db_FETCH_attrib tds_db_FETCH_attrib
#define dbd_st_prepare      tds_st_prepare
#define dbd_st_rows         tds_st_rows
#define dbd_st_execute      tds_st_execute
#define dbd_st_fetch        tds_st_fetch
#define dbd_st_finish       tds_st_finish
#define dbd_st_destroy      tds_st_destroy
#define dbd_st_blob_read    tds_st_blob_read
#define dbd_st_STORE_attrib tds_st_STORE_attrib
#define dbd_st_FETCH_attrib tds_st_FETCH_attrib
#define dbd_describe        tds_describe
#define dbd_bind_ph         tds_bind_ph


#include "dbdimp.h"

int     tds_db_commit(SV        *dbh,
                      imp_dbh_t *imp_dbh);

void    tds_db_destroy(SV        *dbh,
                       imp_dbh_t *imp_dbh);

int     tds_db_disconnect(SV        *dbh,
                          imp_dbh_t *imp_dbh);

int     tds_db_do(SV   *sv,
                  char *statement);

int     tds_db_login(SV        *dbh,
                     imp_dbh_t *imp_dbh,
                     char      *dbname,
                     char      *uid,
                     char      *pwd);

int     tds_db_rollback(SV        *dbh,
                        imp_dbh_t *imp_dbh);

int     tds_db_STORE_attrib(SV        *dbh,
                            imp_dbh_t *imp_dbh,
                            SV        *keysv,
                            SV        *valuesv);

SV     *tds_db_FETCH_attrib(SV        *dbh,
                            imp_dbh_t *imp_dbh,
                            SV        *keysv);

int     tds_discon_all(SV        *drh,
                       imp_drh_t *imp_drh);

void    tds_init(dbistate_t *dbistate);

int     tds_st_prepare(SV        *sth,
                       imp_sth_t *imp_sth,
                       char      *statement,
                       SV        *attribs);

int     tds_st_blob_read(SV        *sth,
                         imp_sth_t *imp_sth,
                         int        field,
                         long       offset,
                         long       len,
                         SV        *destrv,
                         long       destoffset);

void    tds_st_destroy(SV        *sth,
                       imp_sth_t *imp_sth);

int     tds_st_execute(SV        *sth,
                       imp_sth_t *imp_sth);

AV     *tds_st_fetch  (SV        *sth,
                       imp_sth_t *imp_sth);

int     tds_st_finish (SV        *sth,
                       imp_sth_t *imp_sth);

int     tds_st_rows   (SV        *sth,
                       imp_sth_t *imp_sth);

int     tds_st_STORE_attrib(SV        *sth,
                            imp_sth_t *imp_sth,
                            SV        *keysv,
                            SV        *valuesv);

SV     *tds_st_FETCH_attrib(SV        *sth,
                            imp_sth_t *imp_sth,
                            SV        *keysv);

int     tds_describe(SV        *sth,
                     imp_sth_t *imp_sth);

int     tds_bind_ph (SV        *sth,
                     imp_sth_t *imp_sth,
                     SV        *param,
                     SV        *value,
                     IV        sql_type,
                     SV        *attribs,
                     int        is_inout,
                     IV         maxlen);


/*
 *  Try to eliminate all of the warning messages from the DBD code
 */
extern void XS_DBD__FreeTDS__db__isdead(CV *);
extern void XS_DBD__FreeTDS__dr_discon_all_(CV *);
extern void XS_DBD__FreeTDS__db_commit(CV *);
extern void XS_DBD__FreeTDS__db_disconnect(CV *);
extern void XS_DBD__FreeTDS__db_STORE(CV *);
extern void XS_DBD__FreeTDS__db_FETCH(CV *);
extern void XS_DBD__FreeTDS__db_DESTROY(CV *);
extern void XS_DBD__FreeTDS__st__prepare(CV *);
extern void XS_DBD__FreeTDS__st_rows(CV *);
extern void XS_DBD__FreeTDS__st_bind_param(CV *);
extern void XS_DBD__FreeTDS__st_bind_param_inout(CV *);
extern void XS_DBD__FreeTDS__st_execute(CV *);
extern void XS_DBD__FreeTDS__st_fetchrow_arrayref(CV *);
extern void XS_DBD__FreeTDS__db_rollback(CV *);
extern void XS_DBD__FreeTDS__st_fetchrow_array(CV *);
extern void XS_DBD__FreeTDS__st_finish(CV *);
extern void XS_DBD__FreeTDS__st_blob_read(CV *);
extern void XS_DBD__FreeTDS__st_STORE(CV *);
extern void XS_DBD__FreeTDS__st_FETCH_attrib(CV *);
extern void XS_DBD__FreeTDS__st_DESTROY(CV *);
extern void boot_DBD__FreeTDS(CV *);
extern void XS_DBD__FreeTDS__db__login(CV *);
#endif
