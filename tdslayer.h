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





/*
 *---------------- WARNING! ACHTUNG! !ADVERTENCIA! ----------------------
 *
 * The dynamic memory in the following structures should be allocated
 * with AllocMemory() and freed with FreeMemory().  Using malloc() or
 * free() could result in program instability, global warming, nuclear
 * winter, and/or male pattern baldness.
 *
 *---------------- WARNING! ACHTUNG! !ADVERTENCIA! ----------------------
 */




#ifndef TDS_h
#define TDS_h





static char  rcsid_tds_h [ ] =
         "$Id: tdslayer.h,v 1.16 1999/02/11 20:30:52 cts Exp $";
static void *no_unused_tds_h_warn[]={rcsid_tds_h, no_unused_tds_h_warn};


#ifndef HAVE_BOOLEAN
typedef int   boolean;
#endif
#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif

#ifndef uchar
#define uchar     unsigned char
#endif

#define TDS_MAX_PACKET_SIZE   512 /* XXX should be user specifiable */


/*
 * The first byte of every TDS packet is one of the following values.
 */
typedef enum {
   NONE    = 0,
   QUERY   = 1,
   LOGON   = 2,
   PROC    = 3,
   REPLY   = 4,
   CANCEL  = 6
} PacketType_t;


typedef enum {
   TDS_RET_STAT_TOKEN  = 121, /* 0x79 */
   TDS_PROCID          = 124, /* 0x7C */
   TDS_COL_NAME_TOKEN  = 160, /* 0xA0 */
   TDS_COL_INFO_TOKEN  = 161, /* 0xA1 */
   TDS_TABNAME         = 164, /* 0xA4 */
   TDS_UNKNOWN_165     = 165, /* 0xA5 */
   TDS_ORDER           = 169, /* 0xA9 */
   TDS_ERR_TOKEN       = 170, /* 0xAA */
   TDS_MSG_TOKEN       = 171, /* 0xAB */
   TDS_LOGIN_ACK_TOKEN = 173, /* 0xAD */
   TDS_CONTROL         = 174, /* 0xAE */
   TDS_ROW_TOKEN       = 209, /* 0xD1 */
   TDS_UNKNOWN_0xE2    = 226, /* 0xE2 */
   TDS_ENV_CHG_TOKEN   = 227, /* 0xE3 */
   TDS_MSG50_TOKEN     = 229, /* 0xE5 */
   TDS_RESULT_TOKEN    = 238, /* 0xEE */
   TDS_END_TOKEN       = 253, /* 0xFD */
   TDS_DONEPROC        = 254, /* 0xFE */
   TDS_DONEINPROC      = 255  /* 0xFF */
} PacketSubType_t;

typedef enum
{
   SYBNONE        =   0,   /*                               */
   SYBVOID        =  31,   /* 0x1F                          */
   SYBIMAGE       =  34,   /* 0x22                          */
   SYBTEXT        =  35,   /* 0x23                          */
   SYBVARBINARY   =  37,   /* 0x25                          */
   SYBINTN        =  38,   /* 0x26 (does not allow nulls?)  */
   SYBVARCHAR     =  39,   /* 0x27                          */
   SYBBINARY      =  45,   /* 0x2D                          */
   SYBCHAR        =  47,   /* 0x2F                          */
   SYBINT1        =  48,   /* 0x30                          */
   SYBBIT         =  50,   /* 0x32                          */
   SYBINT2        =  52,   /* 0x34                          */
   SYBINT4        =  56,   /* 0x38                          */
   SYBDATETIME4   =  58,   /* 0x3A                          */
   SYBREAL        =  59,   /* 0x3B                          */
   SYBMONEY       =  60,   /* 0x3C (does not allow nulls?)  */
   SYBDATETIME    =  61,   /* 0x3D                          */
   SYBFLT8        =  62,   /* 0x3E                          */
   SYBDECIMAL     = 106,   /* 0x6A                          */
   SYBNUMERIC     = 108,   /* 0x6C                          */
   SYBFLTN        = 109,   /* 0x6D                          */
   SYBMONEYN      = 110,   /* 0x6E                          */
   SYBDATETIMN    = 111,   /* 0x6F                          */
   SYBMONEY4      = 112,   /* 0x70                          */
   SYBSMALLMONEY  = 122    /* 0x7A                          */
} ColumnType_t;

typedef enum {tds42, tds50} TdsVersion_t;

typedef struct
{
   int     number;    /* error code returned by server           */
   uchar   state;     /*                                         */
   uchar   level;     /*                                         */
   char   *message;   /* message/error text sent back by server  */
   char   *server;    /* ?name of server?                        */
   char   *proc_name; /* name of procedure where error occured   */
   uchar   line;      /* line number where error occured         */
} Message_t;

typedef struct
{
   /*
    * status flags.  (I only know what 2 of the bits mean.  Sorry.)
    *   0x01   There is another result set from this query
    *   0x20   The query was canceled before it finished.
    */
   uchar   status;       /* status flags.  see tds documentation */
   /*
    * number of rows affected
    */
   int     row_count;
} EndInfo_t;

typedef struct _tag_NameListItem_t
{
   char                        *name;
   struct _tag_NameListItem_t  *next;
} NameListItem_t;


typedef struct
{
   int               count; /* number of names in the list.      */
   NameListItem_t   *first; /* pointer to the first name in list */
   NameListItem_t   *last;  /* pointer to last name in the list  */
} NameList_t;

typedef struct
{
   int            count;       /* number of columns in the result set */
   boolean        have_names;  /* do we know the column names yet?    */
   NameList_t     names;       /* names of the columns                */
   int           *sizes;       /* array of max sizes for each column  */
   ColumnType_t  *types;       /* array of the type of each column    */
} ColumnInfo_t;

typedef struct
{
   PacketSubType_t     type;  /* this determines the union data */
   union
   {
      Message_t     msg;      /* TDS_MSG50_TOKEN TDS_ERR_TOKEN TDS_MSG_TOKEN */
      EndInfo_t     end;      /* TDS_END_TOKEN TDS_DONEPROC                  */
      /*
       * TDS_COL_NAME_TOKEN, TDS_COL_INFO_TOKEN and TDS_RESULT_TOKEN
       */
      ColumnInfo_t                colinfo;
   } info;
} TdsStatus_t;


typedef struct _tag_TdsStream
{
   int                         sd;              /* socket descriptor         */
   boolean                     creating_packet; /* are we making a packet?   */
   boolean                     more_results;    /* are there results to read?*/
   boolean                     hit_end;         /* have we hit end token yet?*/
   boolean                     checked_more_results; 
   ColumnInfo_t                colinfo;
   uchar                       input[TDS_MAX_PACKET_SIZE];
   int                         bytes_received;  /* number of bytes in buffer */
   int                         total_bytes_read;
   int                         input_cursor;    /* index of next byte to get */
   uchar                       output[TDS_MAX_PACKET_SIZE];
   int                         bytes_to_send;   /* # bytes in output buffer  */
   PacketType_t                packet_type;     /* type of packet being made */
   TdsStatus_t                 info;            /* info back from server     */
   struct _tag_TdsConnection  *cx;              /* TDS connection            */
   void                       *debug_info;      /*                           */
   struct _tag_TdsStream      *next;            /* next item in linked list  */
} *TdsStream_t;


/*
 * A TDS connection is a set of TDS streams.  All of the streams have
 * the same user and are generally connected to the same database.
 */
typedef struct _tag_TdsConnection
{
   char          server[256];      /* name of server to connect to    */
   short         port;             /* port number in host byte order  */
   char          dbname[256];      /* database name to connect to     */
   char          username[256];
   char          password[256];
   char          client[256];      /* client name to tell to server   */
   int           total_streams;    /* total number of streams created */
   TdsStream_t   availableStreams; /* list of streams not being used. */
   TdsStream_t   inUseStreams;     /* list of streams in active use   */
} *TdsConnection_t;


#define MISC_ERR    274

typedef struct 
{
   int    error;
   char   msg[512];
} ErrorMsg_t;


extern TdsConnection_t  tds_createConnection(TdsVersion_t     protocol,
                                             const char      *server,
                                             short            port,
                                             const char      *dbname,
                                             const char      *user,
                                             const char      *password,
                                             const char      *client,
                                             ErrorMsg_t      *err);
extern void          tds_closeConnection(TdsConnection_t  *cx);


extern TdsStream_t   tds_allocateStream(TdsConnection_t  cx, void *debug_aid);
extern void          tds_releaseStream(TdsConnection_t cx,
                                       TdsStream_t tds);

extern void          tds_start_packet(TdsStream_t tds,
                                      PacketType_t packet_type);
extern int           tds_write_byte(TdsStream_t tds, uchar b);
extern int           tds_write_bytes(TdsStream_t tds,
                                     void *buf,
                                     int len);
extern int           tds_write_padded_string(TdsStream_t tds,
                                             const char *s,
                                             int         len,
                                             char        pad);
extern int           tds_write_netshort(TdsStream_t tds, short s);
extern int           tds_write_netlong(TdsStream_t tds, long l);
extern int           tds_write_tdsshort(TdsStream_t tds, short s);
extern int           tds_write_tdslong(TdsStream_t tds, long l);
extern boolean       tds_send_packet(TdsStream_t tds);

extern boolean       tds_get_byte(TdsStream_t tds, uchar *b);
extern boolean       tds_get_marker(TdsStream_t tds, PacketSubType_t *m);
extern boolean       tds_peek(TdsStream_t  tds, uchar *b);
extern boolean       tds_peek_marker(TdsStream_t  tds, PacketSubType_t *m);
extern boolean       tds_get_bytes(TdsStream_t tds, void *buf, int len);
extern boolean       tds_get_len_and_short_string(TdsStream_t   tds,
                                                  uchar        *len,
                                                  char        **str);
extern boolean       tds_get_len_and_medium_string(TdsStream_t   tds,
                                                   short        *len,
                                                   char        **str);
extern boolean       tds_skip(TdsStream_t tds, int len);
extern boolean       tds_get_netshort(TdsStream_t tds, short *s);
extern boolean       tds_get_tdsshort(TdsStream_t tds, short *s);
extern boolean       tds_get_tdsint(TdsStream_t tds, int *i);

extern boolean       tds_process_subpacket(TdsStream_t tds);
extern boolean       tds_skip_result_set(TdsStream_t tds);

extern boolean       tds_get_value_generic(TdsStream_t   tds,
                                         ColumnType_t  type,
                                         void         *data,
                                         int           data_len,
                                         boolean      *is_null);

extern boolean       tds_is_end_marker(PacketSubType_t marker);
extern boolean       tds_is_result_set(PacketSubType_t marker);


extern void          namelist_init(NameList_t *list);
extern void          namelist_free(NameList_t *list);
extern boolean       namelist_add_name(NameList_t *list, const char *name);
extern const char   *namelist_nth_name(NameList_t list, int n);
extern int           namelist_count(NameList_t list);
extern void          namelist_free(NameList_t *list);

extern void          colinfo_init(ColumnInfo_t *info);
extern void          colinfo_free(ColumnInfo_t *info);
extern int           colinfo_num_columns(const ColumnInfo_t info);
extern const char   *colinfo_col_name(const ColumnInfo_t info, int column);
extern ColumnType_t  colinfo_col_type(const ColumnInfo_t info, int column);
extern int           colinfo_col_size(const ColumnInfo_t info, int column);
extern boolean       colinfo_have_names(const ColumnInfo_t info);
extern boolean       colinfo_have_types(const ColumnInfo_t info);
extern boolean       colinfo_have_sizes(const ColumnInfo_t info);
extern boolean       colinfo_merge(ColumnInfo_t *dst, ColumnInfo_t src);
extern int           colinfo_data_offset(ColumnInfo_t info, int column);


/* extern boolean       AllocMemory(void   **ptr, size_t size);
 */
extern boolean       ResizeMemory(void **ptr, size_t len);
extern void          FreeMemory(void **ptr);
extern boolean       AllocMemory_real(void   **ptr, size_t size,
                                      const char *fname, int lineno);
#define AllocMemory(ptr, size) AllocMemory_real((ptr), (size),           \
                                                __FILE__, __LINE__)

extern void DumpLeakTrace(FILE *fd);


extern int           tdslib_year(long days, long msec);
extern int           tdslib_month(long days, long msec);
extern int           tdslib_day(long days, long msec);
extern int           tdslib_hour(long days, long msec);
extern int           tdslib_minute(long days, long msec);
extern int           tdslib_second(long days, long msec);
extern int           tdslib_millisecond(long days, long msec);

#endif
