/*
 * Copyright 1998, 1999 CDS Networks, Inc., Medford Oregon
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

#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>

#include "FreeTDS.h"
#include "dbdimp.h"
#include "tdslayer.h"


static char  software_version[]   = "$Id: dbdimp.c,v 1.35 1999/02/18 18:57:00 cts Exp $";
static void *no_unused_var_warn[] = {software_version,
                                     no_unused_var_warn};


#ifndef MAXHOSTNAMELEN
/* 
 * XXX punt for now, figure out portable way later 
*/
#define MAXHOSTNAMELEN   1024  
#endif


static const int XXX_NEED_ERROR_NUMBER = 1;


DBISTATE_DECLARE;


#define NotImplemented(h)                                            \
do                                                                   \
{                                                                    \
   static char    buf[276];                                          \
   SV  *errstr = DBIc_ERRSTR(h);                                     \
   fprintf(stderr, "%s:%d: Not implemented\n", __FILE__, __LINE__);  \
   sprintf(buf, "\n%s:%d: Not Implemented!\n", __FILE__, __LINE__);  \
   sv_setiv(DBIc_ERR(h), (IV)1);                                     \
   sv_setpv(errstr, buf);                                            \
} while(0)                                                           \



/* ============================= safe_strncpy() ==============================
 *
 * Def:
 *
 * Ret:
 *
 * ===========================================================================
 */
static char *safe_strncpy(char *dst, const char *src, size_t len)
{
   strncpy(dst, src, len);
   dst[len-1] = '\0';
   return dst;
} /* safe_strncpy() */


static void check_imp_sth(imp_sth_t  *imp_sth)
{
   if (imp_sth == NULL)
   {
      abort();
   }
}

#ifdef  TRACE_ALLOCMEMORY_LEAKS
static void show_leaks()
{
   DumpLeakTrace(stderr);
}
#endif


static void signal_dbh_error(
   SV           *dbh,
   imp_dbh_t    *imp_dbh,
   int           error_number,
   const char   *error_message)
{
   SV   *errstr = DBIc_ERRSTR(imp_dbh);
   sv_setiv(DBIc_ERR(imp_dbh), 
            (IV)(error_number));
   sv_setpv(errstr, error_message);
   if (dbh!=NULL)
   {
      DBIh_EVENT2(dbh, ERROR_event, DBIc_ERR(imp_dbh), errstr);
   }
} /* signal_sth_error()  */


static void signal_sth_error(
   SV           *sth,
   imp_sth_t    *imp_sth,
   int           error_number,
   const char   *error_message)
{
   SV   *errstr = DBIc_ERRSTR(imp_sth);
   sv_setiv(DBIc_ERR(imp_sth), 
            (IV)(error_number));
   sv_setpv(errstr, error_message);
   if (sth!=NULL)
   {
      DBIh_EVENT2(sth, ERROR_event, DBIc_ERR(imp_sth), errstr);
   }
} /* signal_error()  */


int tds_db_commit(
   SV        *dbh,
   imp_dbh_t *imp_dbh)
{
   void *unused[] = {unused, dbh, imp_dbh};
   NotImplemented(imp_dbh);
   return 0;
} /* tds_db_commit()  */


void tds_db_destroy(
   SV        *dbh,
   imp_dbh_t *imp_dbh)
{
   void *unused[] = {unused, dbh, imp_dbh};

   if (DBIc_ACTIVE(imp_dbh))
   {
      /*
       *  xxx Do we need to do this and are we permitted to do this?
       */
      tds_db_disconnect(dbh, imp_dbh);
   }

   if (DBIc_IMPSET(imp_dbh))
   {
      if (imp_dbh->dbhost != NULL)
      {
         imp_dbh->dbhost[0] = '\0';
         FreeMemory((void **)&(imp_dbh->dbhost));
      }
      if (imp_dbh->dbport_str != NULL)
      {
         imp_dbh->dbport_str[0] = '\0';
         FreeMemory((void **)&(imp_dbh->dbport_str));
      }
      
      DBIc_IMPSET_off(imp_dbh);	
   }

   return;
} /* tds_db_destroy()  */



int tds_db_disconnect(
   SV        *dbh,
   imp_dbh_t *imp_dbh)
{
   void *unused[] = {unused, dbh};


   if (DBIc_ACTIVE(imp_dbh))
   {
      /*
       * Close all database connections for this handle.
       */
      tds_closeConnection(&(imp_dbh->cx));

      /*
       * Let DBI know this handle is closed.
       */
      DBIc_ACTIVE_off(imp_dbh);
   }
   return 1;
} /* tds_db_disconnect()  */


int tds_db_do(
   SV   *sv,
   char *statement)
{
   void *unused[] = {unused, sv, statement};
   assert(1==0);
   return 0;
} /* tds_db_do()  */


/* ============================= tds_db_login() ==============================
 *
 * Def:  This function connects to the database.
 *
 * Ret:
 *
 * ===========================================================================
 */
int tds_db_login(
   SV        *dbh,
   imp_dbh_t *imp_dbh,
   char      *dbname,
   char      *uid,
   char      *pwd)
{
   void       *unused[]    = {unused, dbh};
   char        clientname[MAXHOSTNAMELEN+1];
   int         result      = 0;
   char       *hostname    = NULL;
   char       *port_str    = NULL;
   short       port_num    = 0;
   ErrorMsg_t  err;

   static int  dejavu      = 0;

   assert(imp_dbh != NULL);

   if (!dejavu)
   {
#ifdef TRACE_ALLOCMEMORY_LEAKS
      dejavu = 1;
      if (-1 == atexit(show_leaks))
      {
         fprintf(stderr, "Coulnd't init the atexit routine to show leaks\n");
         exit(1);
      }
#endif
   }

   gethostname(clientname, sizeof(clientname)-1);
   clientname[sizeof(clientname)-1] = '\0';

   if (strlen(dbname) > 256)
   {
      result = 0;
   }
   else
   {
      hostname = imp_dbh->dbhost;
      {
         struct servent *service;
         port_str = imp_dbh->dbport_str;
         service = getservbyname(port_str, "tcp");
         if (service!=NULL)
         {
            port_num = service->s_port;
         }
         else
         {
            port_num = atoi(port_str);
         }
      }
      
      if (hostname == NULL)
      {
         result = 0;
      }
      else if (port_num==0)
      {
         result = 0;
      }
      else
      {
/*           fprintf(stderr, "database is %s\n", dbname); */
/*           fprintf(stderr, "server is %s\n", hostname); */
/*           fprintf(stderr, "port name is %s\n", port_str); */
/*           fprintf(stderr, "port num is %d\n", port_num); */
/*           fprintf(stderr, "uid is %s\n", uid); */
/*           fprintf(stderr, "password is %s\n", pwd); */
/*           fflush(stderr); */
         
         imp_dbh->cx = tds_createConnection(
            tds42,
            hostname,
            port_num,
            dbname, 
            uid,
            pwd,
            clientname,
            &err);
            
            result = imp_dbh->cx != NULL;
            if (result)
            {
               DBIc_set(imp_dbh,DBIcf_AutoCommit, 1);
            
               DBIc_IMPSET_on(imp_dbh);
               DBIc_ACTIVE_on(imp_dbh);
            }
            else
            {
               signal_dbh_error(dbh, imp_dbh, err.error, err.msg);
            }
         }
      
   }
   return result;
} /* tds_db_login() */


int tds_db_rollback(
   SV        *dbh,
   imp_dbh_t *imp_dbh)
{
   void *unused[] = {unused, dbh};
   NotImplemented(imp_dbh);
   return 0;
} /* tds_db_rollback()  */



int tds_db_STORE_attrib(
   SV        *dbh,
   imp_dbh_t *imp_dbh,
   SV        *keysv,
   SV        *valuesv)
{
   void   *unused[] = {unused, dbh, valuesv};
   STRLEN  kl;
   char   *key;
   char   *value;
   STRLEN  vl;
   int     result = FALSE;

   key    = SvPV(keysv, kl);
   value  = SvPV(valuesv, vl);
                        

   if (0 == strcmp(key, "database"))
   {
      /* nop */
   }
   else if (0 == strcmp(key, "host"))
   {
      if (imp_dbh->dbhost != NULL)
      {
         FreeMemory((void **)&imp_dbh->dbhost);
      }
      result = AllocMemory((void **)&imp_dbh->dbhost, vl+2);
      strncpy(imp_dbh->dbhost, value, vl+1);
   }
   else if (0 == strcmp(key, "port"))
   {
      if (imp_dbh->dbport_str != NULL)
      {
         FreeMemory((void **)&imp_dbh->dbport_str);
      }
      result = AllocMemory((void **)&imp_dbh->dbport_str, vl+2);
      strncpy(imp_dbh->dbport_str, value, vl+1);
   }
   else if (0 == strcmp(key, "PrintError"))
   {
      imp_dbh->print_error = SvTRUE(valuesv);
      result = TRUE;
   }
   else if (0 == strcmp(key, "AutoCommit"))
   {
      DBIc_set(imp_dbh, DBIcf_AutoCommit, SvTRUE(valuesv));
      imp_dbh->auto_commit = SvTRUE(valuesv);
      result = TRUE;
   }
   
   return result;
} /* tds_db_STORE_attrib()  */



SV *tds_db_FETCH_attrib(
   SV        *dbh,
   imp_dbh_t *imp_dbh,
   SV        *keysv)
{
   void *unused[] = {unused, dbh, keysv};
   NotImplemented(imp_dbh);
   return 0;
} /* tds_db_FETCH_attrib()  */



int tds_discon_all(
   SV        *drh,
   imp_drh_t *imp_drh)
{
   void *unused[] = {unused, drh, imp_drh};

   /*
    * XXX Need to implement this
    */

   return 1;
} /* tds_discon_all()  */



void tds_init(dbistate_t *dbistate)
{
   /* XXX Need to find out what next statement does. */
   DBIS = dbistate;

} /* tds_init()  */


static void init_impsth(imp_sth_t  *imp_sth)
{
   imp_sth->raw_query      = NULL;
/*   colinfo_init(&(imp_sth->metadata));
 */
   imp_sth->row_data_size  = -1;
   imp_sth->column_offset  = NULL;
   imp_sth->row_data       = NULL;
   imp_sth->is_null        = NULL;
}

/* ============================ tds_st_prepare() =============================
 *
 * Def:  Prepare a statement for execution.  The statement is not
 *       actually executed yet, only prepared.  Typically you
 *       would call tds_st_execute() to send the query to the database
 *       server.
 *
 * Ret:  who knows?  DBD doesn't document the return code, but I'm
 *       going to assume that true means success, false on failure.
 *
 * ===========================================================================
 */
int tds_st_prepare(
   SV        *sth,
   imp_sth_t *imp_sth,
   char      *statement,
   SV        *attribs)
{
   void *unused[] = {unused, sth, attribs};
   int    isOkay;
   D_imp_dbh_from_sth;

/* fprintf(stderr, "tds_st_prepare    %.60s\n", statement);
 */

   if (DBIc_ACTIVE(imp_sth))
   {
      tds_st_finish(sth, imp_sth);
   }
   if (DBIc_is(imp_sth, DBIcf_IMPSET))
   {
      tds_releaseStream(imp_dbh->cx, imp_sth->tds);
   }

   /*
    * XXX the next line might be a memory leak, but I don't know where
    * we first get the imp_sth to initialize it.
    */
   init_impsth(imp_sth);

   if (!AllocMemory((void**)&(imp_sth->raw_query), strlen(statement)+1))
   {
      signal_dbh_error(NULL, imp_dbh, XXX_NEED_ERROR_NUMBER, 
                   "Couldn't allocate memory for query");
      isOkay = 0;
   }
   else
   {
      D_imp_dbh_from_sth;

      isOkay = 1;
      safe_strncpy(imp_sth->raw_query, statement, strlen(statement)+1);

      imp_sth->tds = tds_allocateStream(imp_dbh->cx, imp_sth->raw_query);
      if (imp_sth->tds!=NULL)
      {
         isOkay = 1;
      }
      else
      {
         signal_dbh_error(NULL, imp_dbh, XXX_NEED_ERROR_NUMBER, 
                      "Out of TDS streams.  (Too many open statements)");
         isOkay = 0;
      }
   }
   if (isOkay)
   {
      DBIc_on(imp_sth, DBIcf_IMPSET);
   }
   return isOkay;
} /* tds_st_prepare()  */



int tds_st_blob_read(
   SV        *sth,
   imp_sth_t *imp_sth,
   int        field,
   long       offset,
   long       len,
   SV        *destrv,
   long       destoffset)
{
   void *unused[] = {unused, sth, &field, &offset, &len, destrv, &destoffset};
   NotImplemented(imp_sth);
   return 0;
} /* tds_st_blob_read()  */



void tds_st_destroy(
   SV        *sth,
   imp_sth_t *imp_sth)
{
   dTHR;
   D_imp_dbh_from_sth;
   void *unused[] = {unused, sth, imp_dbh};
   /* XXX should we at least check to see if they need to be cleaned up? */

/* fprintf(stderr, "tds_st_destroy    %.60s\n", imp_sth->raw_query);   
 */
   if (DBIc_ACTIVE(imp_dbh))
   {
      DBIc_ACTIVE_off(imp_sth);
   }
   if (DBIc_is(imp_sth, DBIcf_IMPSET))
   {
      DBIc_IMPSET_off(imp_sth);
   }

   return;
} /* tds_st_destroy()  */



/* ============================= get_col_info() ==============================
 * 
 * Def:  This function should be called after sending a query 
 *       to the server.  It will read the TDS_COL_*_TOKEN and 
 *       TDS_RESULT_TOKEN subpackets and place that information 
 *       into the imp_sth->metadata variable.
 * 
 * Ret:  true on success, false on failure.
 * 
 * ===========================================================================
 */
static boolean get_col_info(
   SV          *sth,
   imp_sth_t   *imp_sth)
{
   boolean   result       = true;
   boolean   done         = false;
   boolean   okay         = true;
   boolean   got_col_info = false;

   done = false;
   okay = true;
   while(okay && !done)
   {
      PacketSubType_t  marker;

      okay = okay && tds_peek_marker(imp_sth->tds, &marker);
      done = (marker==TDS_ROW_TOKEN
              || marker==TDS_ROW_TOKEN
              || marker==TDS_END_TOKEN
              || marker==TDS_DONEPROC
              || marker==TDS_DONEINPROC);

      if (!done)
      {
         okay = okay && tds_process_subpacket(imp_sth->tds);
         if (okay)
         {
            switch(imp_sth->tds->info.type)
            {
               case TDS_COL_INFO_TOKEN:
               case TDS_COL_NAME_TOKEN:
               {
                  got_col_info = true;
/*                  colinfo_merge(&(imp_sth->metadata),
                                imp_sth->tds->info.info.colinfo);
*/
                  break;
               }        
               case TDS_CONTROL:
               case TDS_ORDER:
               {
                  break;
               }
               case TDS_MSG_TOKEN:
               {
                  /* XXX do we need to propogate this back to DBI? */
                  break;
               }
               case TDS_PROCID:
               {
                  break;
               }
               case TDS_ERR_TOKEN:
               {
                  signal_sth_error(sth, imp_sth, 
                               imp_sth->tds->info.info.msg.number,
                               imp_sth->tds->info.info.msg.message);
                  result = false;
                  break;
               }
               default:
               {
                  fprintf(stderr, "\n%s:%d: Unhandled type 0x%02x\n",
                          __FILE__, __LINE__, imp_sth->tds->info.type);
                  abort();
               }
            }            
         }
      }
   }

   if (! got_col_info)
   {
      imp_sth->tds->info.info.colinfo.count = 0;
   }

   return okay && result;
} /* get_col_info()  */


static int start_result_set(
   SV        *sth,
   imp_sth_t *imp_sth)
{
   int              num_columns = -1;
   boolean          okay = true;
   PacketSubType_t  marker;
   int              result;

   okay = okay && tds_peek_marker(imp_sth->tds, &marker);

   if (okay && marker == TDS_DONEINPROC)
   {
      okay = okay && tds_process_subpacket(imp_sth->tds);
      if (!okay)
      {
         signal_sth_error(sth, imp_sth, XXX_NEED_ERROR_NUMBER, 
                          "Error processing tds DONEINPROC packet");
      }
   }
   if (okay && marker == TDS_PROCID)
   {
      okay = okay && tds_process_subpacket(imp_sth->tds);
      if (!okay)
      {
         signal_sth_error(sth, imp_sth, XXX_NEED_ERROR_NUMBER, 
                          "Error processing tds DONEINPROC packet");
      }
   }


   if (okay)
   {
      colinfo_free(&(imp_sth->tds->colinfo));
      colinfo_init(&(imp_sth->tds->colinfo));
      if (!get_col_info(sth, imp_sth))
      {
         signal_sth_error(sth, imp_sth, XXX_NEED_ERROR_NUMBER, 
                          "Error processing tds packet.  "
                          "Coulnd't get column information");
      }
      else
      {
         num_columns =  colinfo_num_columns(imp_sth->tds->colinfo);
         
         if (num_columns > 0)
         {
            /* XXX We should modify this to not use row_data for staging
             * tds_st_fetch will store the incoming data in the row_data field.
             * We must figure out the offset of each column in that buffer
             * and store those offsets in the column_offset array
             */
            if (imp_sth->column_offset != NULL)
            {
               FreeMemory((void **)&(imp_sth->column_offset));
            }
            if (imp_sth->row_data != NULL)
            {
               FreeMemory((void **)&(imp_sth->row_data));
            }
            if (imp_sth->is_null != NULL)
            {
               FreeMemory((void **)&(imp_sth->is_null));
            }
            if (num_columns > 0)
            {
               okay = okay && AllocMemory((void **)&(imp_sth->column_offset),
                                          (sizeof(imp_sth->column_offset[0])
                                           * num_columns));
               okay = okay && AllocMemory((void **)&(imp_sth->is_null),
                                          (sizeof(imp_sth->is_null[0]) 
                                           * num_columns));
               if (!okay)
               {
                  signal_sth_error(sth, imp_sth, XXX_NEED_ERROR_NUMBER, 
                                   "Couldn't allocate memory for result set.");
               }
            }
            if (okay)
            {
               int  i;
               int  offset = 0;
               
               for(i=0; i<num_columns; i++)
               {
                  imp_sth->column_offset[i] = offset;
                  offset += colinfo_col_size(imp_sth->tds->colinfo, i+1);
               }
               imp_sth->row_data_size = offset;
               okay = AllocMemory((void **)&(imp_sth->row_data), 
                                  imp_sth->row_data_size);
               
               /* 
                * Let DBI know how many columns are in the result set.
                */
               DBIc_NUM_FIELDS(imp_sth) = colinfo_num_columns(imp_sth->tds->colinfo);
            }
         }
         okay = okay && tds_peek_marker(imp_sth->tds, &marker);
         if (okay)
         {
            if (marker != TDS_ROW_TOKEN)
            {
               result = 0;  /* This signifies no rows returned by query */
            }
            else
            {
               result = 1; /* 1 signifies that rows were returned. */
            }
         }
         else
         {
            result = -2;  /* signifies an error */
         }
      }
   }

   return result;
} /* start_result_set()  */


/* ============================ tds_st_execute() =============================
 *
 * Def:
 *
 * Ret:  As best as I can tell DBD wants this function to return
 *       <= -2                on a failure.
 *           0                if successful but no rows returned.
 *           something else   on success with rows returned.
 *
 * ===========================================================================
 */
int tds_st_execute(
   SV        *sth,
   imp_sth_t *imp_sth)
{
   boolean            okay     = true;
   void              *unused[] = {unused, sth};
   int                result   = -2;
   PacketSubType_t    marker;

   /* 
    * Send the query
    */
   tds_start_packet(imp_sth->tds, QUERY);
   tds_write_bytes(imp_sth->tds, imp_sth->raw_query, 
                   strlen(imp_sth->raw_query));
   okay = tds_send_packet(imp_sth->tds);
   colinfo_free(&(imp_sth->tds->colinfo));
   colinfo_init(&(imp_sth->tds->colinfo));

   

   /*
    * Figure out if there were any errors and if the query returned 
    * any rows.  Set result to the appropriate value.  If 
    * we did find rows we must also let DBI know how many columns there
    * are and prepare imp_sth to receive data.
    */
   okay = okay && tds_peek_marker(imp_sth->tds, &marker);
   if (!okay)
   {
      signal_sth_error(sth, imp_sth, XXX_NEED_ERROR_NUMBER, 
                   "Problem executing query");
      result = -2; /* it appears that DBD's should return -2 for an error */
   }
   else
   {
      result = start_result_set(sth, imp_sth);
   }

   if (result != -2)
   {
          DBIc_ACTIVE_on(imp_sth);
   }
   if (result == 0)
   {
      D_imp_dbh_from_sth;

      DBIc_ACTIVE_off(imp_sth);
      tds_releaseStream(imp_dbh->cx, imp_sth->tds);
      FreeMemory((void**)&(imp_sth->raw_query));
   }
   return result;
} /* tds_st_execute()  */



static AV *get_row(imp_sth_t  *imp_sth)
{
   AV       *result;
   boolean   okay;
   int       num_columns = colinfo_num_columns(imp_sth->tds->colinfo);
   int       i;
   SV       *sv;
   int       offset;
   void     *data     = NULL;         

   /* 
    * We already peeked and found that the next byte will 
    * be a TDS_ROW_TOKEN.  Just skip it.
    */
   okay = okay && tds_skip(imp_sth->tds, 1);
         
   result = DBIS->get_fbav(imp_sth);

   /*
    * DBI doesn't know anything about multiple result sets
    * so we will have to do manually expand av if this result
    * set has more columns than the previous one.
    */
   if(av_len(result)+1 != num_columns)
   {
      int read_only = SvREADONLY(result);
      if(read_only)
      {
         SvREADONLY_off(result);
      }
      for(i=av_len(result)+1; i<num_columns; i++)
      {
         av_store(result, i, newSV(0));
      }
      if(read_only)
      {
         SvREADONLY_on(result);  
      }
   }

   for(i=0; i<num_columns; i++)
   {
      sv     = AvARRAY(result)[i];
      offset = colinfo_data_offset(imp_sth->tds->colinfo, i+1);
      data   = (imp_sth->row_data + offset);
            
      okay = okay &&
         tds_get_value_generic(imp_sth->tds, 
                               colinfo_col_type(imp_sth->tds->colinfo, i+1),
                               data, 
                               colinfo_col_size(imp_sth->tds->colinfo, i+1),
                               &(imp_sth->is_null[i]));
            
      if (imp_sth->is_null[i])
      {
         (void)SvOK_off(sv);
      }
      else
      {
         switch(colinfo_col_type(imp_sth->tds->colinfo, i+1))
         {
            case SYBNONE:
            {
               assert("column type"=="can't be none");
            }
            case SYBREAL:
            case SYBFLTN:
            case SYBFLT8:
            {
               check_imp_sth(imp_sth);
               sv_setnv(sv, *(double *)data);
               check_imp_sth(imp_sth);
               break;
            }                  
            case SYBBIT:
            {
               check_imp_sth(imp_sth);
               sv_setiv(sv, *(uchar *)data);
               check_imp_sth(imp_sth);
               break;
            }
            case SYBINTN:
            case SYBINT4:
            case SYBINT2:
            case SYBINT1:
            {
               check_imp_sth(imp_sth);
               sv_setiv(sv, *(int *)data);
               check_imp_sth(imp_sth);
               break;
            }
            case SYBCHAR:
            case SYBVARCHAR:
            {
               int   len;
               for(len=0; 
                   ((char *)data)[len] != '\0' 
                      && len<colinfo_col_size(imp_sth->tds->colinfo, i+1); 
                   len++)
               {
                  /* nop */
               }
               check_imp_sth(imp_sth);
               sv_setpvn(sv, (char *)data, len);
               check_imp_sth(imp_sth);
               break;
            }
            case SYBVARBINARY:
            {
               int   len = colinfo_col_size(imp_sth->tds->colinfo, i+1);
                     
               check_imp_sth(imp_sth);
               sv_setpvn(sv, (char *)data, len);
               check_imp_sth(imp_sth);
               break;
            }
            case SYBDATETIMN:
            case SYBDATETIME4:
            case SYBDATETIME:
            {
               char    buf[40];
               int     year;
               int     month;
               int     day;
               int     hour;
               int     minute;
               int     second;
                     
               /* 
                *  get the year, month, day, hour, minute, and second
                *  from *data and convert it to human readable
                *  string of the form yyyy-mm-dd hh:mm:ss.
                */
               year   = tdslib_year(((long *)data)[0], ((long *)data)[1]);
               month  = tdslib_month(((long *)data)[0], ((long *)data)[1]);
               day    = tdslib_day(((long *)data)[0], ((long *)data)[1]);
               hour   = tdslib_hour(((long *)data)[0], ((long *)data)[1]);
               minute = tdslib_minute(((long *)data)[0], ((long *)data)[1]);
               second = tdslib_second(((long *)data)[0], ((long *)data)[1]);
               sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d", 
                       year, month, day, hour, minute, second);
               /*
                *  Place it in the perl data structures.
                */
               check_imp_sth(imp_sth);
               sv_setpvn(sv, buf, strlen(buf));
               check_imp_sth(imp_sth);
               break;
            }
            default:
            {
               fprintf(stderr, "%s:%d Not implemented for type %d", 
                       __FILE__, __LINE__, 
                       colinfo_col_type(imp_sth->tds->colinfo, i+1));
               assert(1==0);
               break;
            }
         }
      }
   }  
   return result;
}


/* ============================= tds_st_fetch() ==============================
 * 
 * Def:  
 * 
 * Ret:  NULL if there wasn't a row to fetch, an AV if there was?
 * 
 * ===========================================================================
 */
AV *tds_st_fetch(
   SV        *sth,
   imp_sth_t *imp_sth)
{
   void         *unused[] = {unused, sth};
   AV           *av       = NULL;
   boolean       okay     = true;
   ColumnType_t  marker;


   check_imp_sth(imp_sth);

   if (imp_sth->tds->hit_end)
   {
      /* 
       *  see if have more result sets 
       *  and see if we should implicitly open it
       */
      if (imp_sth->tds->more_results && imp_sth->tds->checked_more_results)
      {
         int  rc;

         rc = start_result_set(sth, imp_sth);
         if (rc == -2)
         {
            assert("Need to throw " == "an error to DBI");
         }
         else
         {
            imp_sth->tds->hit_end = false;
            av =  tds_st_fetch(sth, imp_sth);
         }
      }
      else
      {
         av = NULL;
      }
   }
   else 
   {
      /*
       * See if there is a row to be read and read it.
       */
      while (okay                                   && 
             tds_peek_marker(imp_sth->tds, &marker) && 
             marker!=TDS_ROW_TOKEN                  && 
             !tds_is_end_marker(marker))
      {
         okay = okay && tds_process_subpacket(imp_sth->tds);
      }
            
      if (okay && marker==TDS_ROW_TOKEN)
      {
         av = get_row(imp_sth);
      }
      else if (okay && tds_is_end_marker(marker))
      {
         okay = okay && tds_process_subpacket(imp_sth->tds);
         av = NULL;
      }      
      else
      {
         fprintf(stderr, "%s:%d: Not implemented.  Marker is %d", 
                 __FILE__, __LINE__, marker);
         assert(1==0);
         exit(1);
      }
   }

   check_imp_sth(imp_sth);

   return av;
} /* tds_st_fetch()  */



int tds_st_finish(
   SV        *sth,
   imp_sth_t *imp_sth)
{
   void *unused[] = {unused, sth};
   int   result = 1;
   D_imp_dbh_from_sth;


   if (DBIc_ACTIVE(imp_sth))
   {
      /*
       * close the statement connection
       */
      if (DBIc_ACTIVE(imp_dbh))
      {
         tds_releaseStream(imp_dbh->cx, imp_sth->tds);
      }
      
      if (dbis->debug >= 2)
      {
         fprintf(DBILOGFP, "%s:%d: Calling DBIc_ACTIVE_off\n", 
                 __FILE__, __LINE__);
      }
      DBIc_ACTIVE_off(imp_sth);
      if (dbis->debug >= 2)
      {
         fprintf(DBILOGFP, "%s:%d: Back from DBIc_ACTIVE_off\n", 
                 __FILE__, __LINE__);
      }
   }

   if (imp_sth->raw_query != NULL)
   {
      FreeMemory((void**)&(imp_sth->raw_query));
   }
   else
   {
      assert("It should"=="not be null");
   }

   return result;
} /* tds_st_finish()  */


int tds_st_rows(
   SV        *sth,
   imp_sth_t *imp_sth)
{
   void     *unused[] = {unused, sth};

   return tds_is_end_marker(imp_sth->tds->info.type) 
      ? imp_sth->tds->info.info.end.row_count 
      : -1;
} /* tds_st_rows()  */


int tds_st_STORE_attrib(
   SV        *sth,
   imp_sth_t *imp_sth,
   SV        *keysv,
   SV        *valuesv)
{
   void *unused[] = {unused, sth, keysv, valuesv};
   NotImplemented(imp_sth);
   return 0;
} /* tds_st_STORE_attrib()  */



SV *tds_st_FETCH_attrib(
   SV        *sth,
   imp_sth_t *imp_sth,
   SV        *keysv)
{
   void   *unused[] = {unused, sth, keysv};
   STRLEN  kl;
   char   *key = SvPV(keysv,kl);
   SV     *result = NULL;

   if (0==strncmp(key, "LENGTH", kl))
   {
      NotImplemented(imp_sth);
   }
   else if (0==strncmp(key, "NAME", kl))
   {
      int   i;
      AV   *av = NULL;
      const int num_fields = DBIc_NUM_FIELDS(imp_sth);

      av = newAV();
      result = newRV(sv_2mortal((SV*)av));

      for(i=0; i<num_fields; i++)
      {
         av_store(av, i, 
                  newSVpv((char*)colinfo_col_name(imp_sth->tds->colinfo, i+1),
                          0));
      }
   }
   else if (0==strncmp(key, "NULLABLE", kl))
   {
      NotImplemented(imp_sth);
   }
   else if (0==strncmp(key, "NUM_OF_FIELDS", kl))
   {
      result = newSViv(DBIc_NUM_FIELDS(imp_sth));
   }
   else if (0==strncmp(key, "NUM_OF_PARAMS", kl))
   {
      NotImplemented(imp_sth);
   }
   else if (0==strncmp(key, "PRECISION", kl))
   {
      NotImplemented(imp_sth);
   }
   else if (0==strncmp(key, "SCALE", kl))
   {
      NotImplemented(imp_sth);
   }
   else if (0==strncmp(key, "TYPE", kl))
   {
      NotImplemented(imp_sth);
   }
   else if (0==strncmp(key, "more_results", kl))
   {
      if (imp_sth->tds->hit_end)
      {
         imp_sth->tds->checked_more_results = true;
         result = newSViv(imp_sth->tds->more_results);
      }
      else
      {
         result = newSViv(1);
      }
   }
   else
   {
      result = Nullsv;
   }
   return result;
} /* tds_st_FETCH_attrib()  */


int tds_describe(
   SV        *sth,
   imp_sth_t *imp_sth)
{
   void *unused[] = {unused, sth};
   NotImplemented(imp_sth);
   return 0;
} /* tds_describe()  */


int tds_bind_ph (
   SV        *sth,
   imp_sth_t *imp_sth,
   SV        *param,
   SV        *value,
   IV        sql_type,
   SV        *attribs,
   int        is_inout,
   IV         maxlen)
{
   void *unused[] = {unused, sth, param, value, &sql_type, attribs, &is_inout, 
                     &maxlen};

   NotImplemented(imp_sth);
   assert(1==0);
   return 0;
} /* tds_bind_ph()  */
