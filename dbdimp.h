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

#ifndef DBDIMP_h
#define DBDIMP_h



#include "FreeTDS.h"
#include "tdslayer.h"



static char  rcsid_dbdimp_h [ ] =
         "$Id: dbdimp.h,v 1.4 1999/02/10 23:41:19 cts Exp $";
static void *no_unused_dbdimp_h_warn[]={rcsid_dbdimp_h,
                                        no_unused_dbdimp_h_warn};


/* ROWDATA_GUARDCHECK is used to help find overwrites of the row data */
static const int ROWDATA_GUARDCHECK = 0xBADC0ADD;  /* arbitrary */

/*
 * 
 */
struct imp_drh_st 
{
   dbih_drc_t com;   /* MUST be first element in structure */

   /* Add any driver specific dr handle stuff here */
};




/* 
 * 
 */

struct imp_dbh_st 
{
   dbih_dbc_t com;  /* MUST be first element in structure	*/

   /* 
    *  Add any database specific dbh info here 
    */
   TdsConnection_t    cx;
   char              *dbhost;
   char              *dbport_str;
   int                auto_commit; /* boolean value */
   int                print_error; /* boolean value */
};


   
/* 
 * 
 */
struct imp_sth_st 
{
   dbih_stc_t  com;       /* MUST be first element in structure */

   /*
    * Add any driver specific statement handle stuff here.
    *
    * NOTE-  Any fields added to this structure should be initialized 
    *        in the init_impsth() function.
    */
   TdsStream_t    tds;

   char          *raw_query;
                
/*   ColumnInfo_t   metadata; 
 */
   int            row_data_size;
   int           *column_offset;
   char          *row_data;
   boolean       *is_null;
};


#endif
