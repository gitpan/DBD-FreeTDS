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
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/signal.h>
#include <assert.h>

#include "tdslayer.h"



static char  software_version[]   = "$Id: tdslayer.c,v 1.35 1999/02/17 03:21:42 cts Exp $";
static void *no_unused_var_warn[] = {software_version,
                                     no_unused_var_warn};



static const long msec_per_day          = (1000*60*60*24);
static const long msec_per_second       = 1000;
static const long msec_per_minute       = (60 * 1000);
static const long era_to_tds_day_diff   = 693600;
static const long jiffies_per_second    = 300;
static const int  seconds_per_minute    = 60;
static const int  minutes_per_hour      = 60;
static const int  hours_per_day         = 24;


static const unsigned long fence1 = 0x01234567;
static const unsigned long fence2 = 0x89ABCDEF;
static const unsigned long fence3 = 0xFEDCBA98;

#define MALLOC_TRACE_FNAME_LEN  30

/*
 * The MemAllocLogEntry_t structure helps to debug memory allocation
 * problems.  fence1 and fence2 will help detect memory overwrites.
 * size is the bytes of data allocated.  It does not include the
 * amount of memory taken by the MemAllocLogEntry_t or fence3.
 */
typedef struct tag_MemAllocLogEntry_t
{
   unsigned long  fence1;
   size_t         size;   /* size of data that was allocated. */
#ifdef TRACE_ALLOCMEMORY_LEAKS
   char                            filename[MALLOC_TRACE_FNAME_LEN];    
   int                             lineno;
   struct tag_MemAllocLogEntry_t  *allocation_chain;
#endif
   unsigned long  fence2;
} MemAllocLogEntry_t;

#ifdef TRACE_ALLOCMEMORY_LEAKS
static MemAllocLogEntry_t  *allocation_chain = NULL;
#endif

static const int   SIZE_OF_HEADER    = 8; /* size of a TDS header   */
static const int   PACKET_LEN_OFFSET = 2; /* offset where length is stored */


#define max(a,b)  ((a)>(b) ? (a) : (b))
#define min(a,b)  ((a)<(b) ? (a) : (b))


static void closeStreams(TdsStream_t *tds);


/* ============================= safe_strncpy() ==============================
 *
 * Def:  Wrapper around strncpy.  This function makes sure dst gets null
 *       terminated.
 *
 * Ret:  dst
 *
 * ===========================================================================
 */
static char *safe_strncpy(
   char       *dst,
   const char *src,
   size_t      len)
{
   strncpy(dst, src, len);
   dst[len-1] = '\0';
   return dst;
} /* safe_strncpy()  */


/* ============================= safe_strncat() ==============================
 *
 * Def:  Wrapper around strncat.  This function makes sure dst gets null
 *       terminated.
 *
 * Ret:  dst
 *
 * ===========================================================================
 */
static char *safe_strncat(
   char       *dst,
   const char *src,
   size_t      len)
{
   strncat(dst, src, len);
   dst[len-1] = '\0';
   return dst;
} /* safe_strncat()  */




/* =============================== memheader() ===============================
 *
 * Def:  See AllocMemory() for a description of what the memory
 *       allocation/deallocation routines are trying to accomplish.
 *
 * Ret:  A pointer to the MemAllocLogEntry_t structure that
 *       comes right before the block of memory.
 *
 * ===========================================================================
 */
static MemAllocLogEntry_t *memheader(
   void     *ptr,   /* (I) The user's pointer to the block of memory */
   boolean   check) /* (I) if true we run the sanity checks          */
{
   MemAllocLogEntry_t *result = (((MemAllocLogEntry_t *)ptr) - 1);
   if (check)
   {
      assert((result)->fence1 == fence1);
      assert((result)->size != 0);
      assert((result)->fence2 == fence2);
   }
   return result;
} /* memheader()  */


/* ============================== memtrailer() ===============================
 *
 * Def:  See AllocMemory() for a description of what the memory
 *       allocation/deallocation routines are trying to accomplish.
 *
 * Ret:  A pointer to the MemAllocLogEntry_t structure that
 *       comes right before the block of memory.
 *
 * ===========================================================================
 */
static MemAllocLogEntry_t *memtrailer(
   void     *ptr,   /* (I) The user's pointer to the block of memory */
   boolean   check) /* (I) if true we run the sanity checks          */
{
   MemAllocLogEntry_t *header  = NULL;
   MemAllocLogEntry_t *trailer = NULL;

   header = memheader(ptr, false);
   trailer = (MemAllocLogEntry_t *)((char*)(header+1)+header->size);
   if (check)
   {
      memheader(ptr, true);
      assert(trailer->fence1 == fence1);
      assert(trailer->fence2 == fence2);
      assert(trailer->size != 0);
      assert(header->size == trailer->size);
   }
   return trailer;
} /* trailercheck()  */



/* =========================== AllocMemory_real() ============================
 *
 * Def:  Wrapper around malloc.  This routine allocates a requested amount
 *       of memory and memory overwrite debug aid around the block.
 *
 *       Errors related to dynamic memory generally are either writing
 *       past the block, or using the memory after the block has been
 *       freed.  AllocMemory() allocates enough additional memory to
 *       place debugging information before and after the block of
 *       memory that is given to the user.  Special values are stored
 *       in that additional area of memory.
 *
 *       FreeMemory() will check those debug areas of memory to make
 *       sure that those areas weren't overwritten.  It also writes
 *       garbage over the area of memory that the user is releasing.
 *       That way if the user tries to use the memory after freeing it
 *       they will get unexpected results and probably notice the
 *       problem much sooner than they would otherwise.
 *
 * Ret:  TRUE on success, false otherwise.
 *
 * =========================================================================== */
boolean AllocMemory_real(
   void         **ptr,
   size_t         size,
   const char    *filename,
   int            lineno)
{
   void   *tmp = NULL;


   assert((long)size>0);
   assert(ptr!=NULL);

   tmp = malloc(size + 2*sizeof(MemAllocLogEntry_t));

   if (tmp != NULL)
   {
      *ptr = (void *)(((MemAllocLogEntry_t *)tmp) + 1);
      memheader(*ptr, false)->size = size;

#ifndef NDEBUG
      memheader(*ptr, false)->fence1 = fence1;
      memheader(*ptr, false)->fence2 = fence2;

      memtrailer(*ptr, false)->fence1 = fence1;
      memtrailer(*ptr, false)->size   = size;
      memtrailer(*ptr, false)->fence2 = fence2;

#ifdef TRACE_ALLOCMEMORY_LEAKS
      {
         int     i; 
         
         memheader(*ptr, false)->filename[0] = '\0';
         memtrailer(*ptr, false)->filename[0] = '\0';
         for(i=0; 
             (i+1)<MALLOC_TRACE_FNAME_LEN
                && isprint(filename[i]);
             i++)
         {
            memheader(*ptr, false)->filename[i]   = filename[i];
            memheader(*ptr, false)->filename[i+1] = '\0';
            memtrailer(*ptr, false)->filename[i]   = filename[i];
            memtrailer(*ptr, false)->filename[i+1] = '\0';
         }
         
         memheader(*ptr, false)->lineno = lineno;
         memtrailer(*ptr, false)->lineno = lineno;
         
         memheader(*ptr, false)->allocation_chain = allocation_chain;
         allocation_chain                         = memheader(*ptr, false);
      }
#endif

      memset(((MemAllocLogEntry_t *)tmp) + 1, 0xAB, size);

      memheader(*ptr, true);
      memheader(*ptr, true);
#endif
   }
   else
   {
      *ptr = NULL;
   }

   return *ptr != NULL;
} /* AllocMemory_real()  */



void DumpLeakTrace(FILE *fd)
{
#ifdef TRACE_ALLOCMEMORY_LEAKS

  MemAllocLogEntry_t   *ptr = NULL;

   for(ptr=allocation_chain; 
       ptr!=NULL; 
       ptr = ptr->allocation_chain)
   {
      fprintf(fd, "%s:%d: Didn't free block at %p\n", 
              ptr->filename, ptr->lineno, &(ptr[1]));
   }
#endif
}

#ifdef TRACE_ALLOCMEMORY_LEAKS
static void remove_from_allocation_r(
   MemAllocLogEntry_t   **head,
   MemAllocLogEntry_t    *block)
{
   if (*head == NULL)
   {
      fprintf(stderr, "Error-  Did not find %p in allocation chain", 
              block);
      abort();
   }
   else if (*head == block)
   {
      *head = block->allocation_chain;
   }
   else
   {
      remove_from_allocation_r(&((*head)->allocation_chain), block);
   }
}


static void remove_from_allocation(MemAllocLogEntry_t   *block)
{
   remove_from_allocation_r(&allocation_chain, block);
}
#endif


/* ============================== FreeMemory() ===============================
 *
 * Def:  Free a block of memory that was allocated with AllocMemory()
 *       Unless NDEBUG is set the routine will check the integrity
 *       of the fence markers and dump core if it detects an overwrite.
 *
 * Ret:  void
 *
 * ===========================================================================
 */
void FreeMemory(void **ptr)
{
   MemAllocLogEntry_t   *header;

   assert(ptr!=NULL);
   assert(*ptr!=NULL);

   header = memheader(*ptr, false);

/* fprintf(stderr, "Freeing %p\n", *ptr);
 */


#ifndef NDEBUG
   {
      MemAllocLogEntry_t  *trailer = memtrailer(*ptr, true);

      memheader(*ptr, true);

#ifdef TRACE_ALLOCMEMORY_LEAKS
      remove_from_allocation(header);
#endif

      memset(*ptr, 0xAD, header->size);

      header->fence1 = 0x9a9a9a9a;
      header->size   = 0x9b9b9b9b;
      header->fence2 = 0x9c9c9c9c;

      trailer->fence1 = 0x9d9d9d9d;
      trailer->size   = 0x9e9e9e9e;
      trailer->fence2 = 0x9f9f9f9f;            
   }
#endif

   free(header);
   *ptr = NULL;
} /* FreeMemory()  */


/* ============================= ResizeMemory() ==============================
 *
 * Def:  Change the size of a memory block that was allocated with
 *       AllocMemory().  Note- If this function fails to allocate
 *       additional memory it won't mess with *ptr.
 *
 * Ret:  true on success, false otherwise
 *
 * ===========================================================================
 */
boolean ResizeMemory(void **ptr, size_t len)
{
   void                *tmp;
   boolean              okay = true;
   MemAllocLogEntry_t  *header = memheader(*ptr, true);
   int                  old_len = header->size;

/*  fprintf(stderr, "Resizing %p, moving to ", *ptr);
 */


   okay = okay && AllocMemory((void **)&tmp, len);
   if (okay)
   {
      memmove(tmp, *ptr, min(len, old_len));
      FreeMemory(ptr);
/*fprintf(stderr, "%p\n", tmp);
 */
      *ptr = tmp;
   }
   return okay;
} /* ResizeMemory()  */


/* ======================= get_year_and_day_of_year() ========================
 * 
 * Def:  Given the number of days since Dec. 31, 1BC determine what
 *       the year is and what day of the year it is.  
 * 
 *       This algorithm is taken from a footnote in
 *       the article "Calendrical Calculations" by Dershowitz & Reingold
 *       published in Sofware Practice and Experience, Vol 20(9) (Sep. 1990)
 *
 * Ret:  
 * 
 * ===========================================================================
 */
static void get_year_and_day_of_year(
   long   date, 
   long   msec, 
   int   *year,
   int   *day)
{
   int n400;
   int d1;
   int n100;
   int d2;
   int n4;
   int d3;
   int n1;
   int d4;

   date = date + (msec / msec_per_day);
   n400 = (date-1) / 146097; /* number of completed 400 year cycles         */
   d1   = (date-1) % 146097; /* days not included in n400                   */
   n100 = (d1 - 1) / 36524;  /* # of 100 year cycles not included in n400   */
   d2   = (d1-1) % 36524;    /* days not included in n400 or n100           */
   n4   = (d2-1) / 1461;     /* # of 4 year cycles not in n400, n100, or n4 */
   d3   = (d2-1) % 1461;     /* days not included in n400, n100, or n4      */
   n1   = (d3-1) / 365;      /* # of years not in n400, n100, or n4         */
   d4   = (d3-1) % 365;      /* days not included in n400, n100, n4, or n1  */

   *year = 400*n400 + 100*n100 + 4*n4 + n1 + 1;
   *day  = d4;
} /* get_year_and_day_of_year()  */


static int days_in_month(
   int    month,   /* (I) January==1, December==12 */
   int    year)
{
   switch (month) 
   {
      case 2:
      {
         if ((((year % 4) == 0) && ((year % 100) != 0))
             || ((year % 400) == 0))
            return 29;
         else
            return 28;
      }
      case 4:
      case 6:
      case 9:
      case 11: 
      {
         return 30;
      }
      default: 
      {
         return 31;
      }
   }
} /* days_in_month()  */

static void get_month_and_day_of_month(
   long  date, 
   long  msec, 
   int  *month,
   int  *day)
{
   int   year;

   get_year_and_day_of_year(date, msec, &year, day);
   
   *month = 1;
   while(*day > days_in_month(*month, year))
   {
      *day = *day - days_in_month(*month, year);
      (*month)++;
   }
} /* get_month_and_day_of_month()  */


/* ============================== tdslib_year() ==============================
 *
 * Def:  
 * 
 * Ret:
 *
 * Rev:
 *    $Crev$   1999/01/05    CTS
 *       Initially coded.
 *
 * ===========================================================================
 */
int tdslib_year(long date, long msec)
{
   int   year;
   int   day;

   get_year_and_day_of_year(date, msec, &year, &day);
   return year;
}

/* ============================= tdslib_month() ==============================
 * 
 * Def:  Given the number of days since Dec. 31, 1BC determine
 *       what month it is.  (January == 1, December==31)
 * 
 * Ret:  
 * 
 * Rev:
 *    $Crev$   1999/01/05    CTS
 *       Initially coded.  
 * 
 * ===========================================================================
 */
int tdslib_month(long date, long msec)
{
   int   month;
   int   day;

   get_month_and_day_of_month(date, msec, &month, &day);

   return month;
}

int tdslib_day(long date, long msec)
{
   int   month;
   int   day;

   get_month_and_day_of_month(date, msec, &month, &day);

   return day;
}
   
int tdslib_hour(long date, long msec)
{
   long   result;
   result = msec / msec_per_second;      /* result is now in seconds */
   result = result / seconds_per_minute; /* result is now in minutes */
   result = result / minutes_per_hour;   /* result is now in hours   */
   result = result % hours_per_day;
   return (int)result;
}

int tdslib_minute(long date, long msec)
{
   long   result;
   result = msec / msec_per_second;      /* result is now in seconds */
   result = result / seconds_per_minute; /* result is now in minutes */
   result = result % minutes_per_hour;   
   return (int)result;
}

int tdslib_second(long date, long msec)
{
   long   result;
   result = msec / msec_per_second;      /* result is now in seconds */
   result = result % seconds_per_minute;
   return (int)result;
} /* tdslib_second()  */

int tdslib_millisecond(long date, long msec)
{
   return msec % msec_per_second;
} /* tdslib_millisecond()  */


/* ============================= namelist_init() =============================
 *
 * Def:
 *
 * Ret:
 *
 * ===========================================================================
 */
void namelist_init(NameList_t *list)
{
   list->count = 0;
   list->first = NULL;
   list->last  = NULL;
} /* namelist_init()  */

/* =========================== namelist_add_name() ===========================
 *
 * Def:  Add a name to the end of the namelist.
 *
 * Ret:  true on success, false on failure.
 *
 * ===========================================================================
 */
boolean namelist_add_name(NameList_t *list, const char *name)
{
   boolean         okay = true;
   NameListItem_t *item;

   assert((list->first==NULL && list->last==NULL)
          || (list->first!=NULL && list->last!=NULL));

   okay = AllocMemory((void **)&item, sizeof(NameListItem_t));
   okay = okay && AllocMemory((void **)&(item->name), strlen(name)+1);
   if(okay)
   {
      /*
       * Add the new item to the end of the list
       */
      safe_strncpy(item->name, name, strlen(name)+1);
      item->next = NULL;

      if (list->last!=NULL)
      {
         list->last->next = item;
      }
      list->last       = item;

      if (list->first == NULL)
      {
         list->first = item;
      }
   }
   if (okay)
   {
      (list->count)++;
   }
   return okay;
} /* namelist_add_name()  */


int namelist_count(NameList_t list)
{
   return list.count;
} /* namelist_count()  */


/* =========================== namelist_nth_name() ===========================
 *
 * Def:  Return the nth name in the name list.  The first name is
 *       at n=1.
 *
 * Ret:
 *
 * ===========================================================================
 */
const char *namelist_nth_name(NameList_t list, int n)
{
   int              i;
   NameListItem_t  *ptr;
   const char      *result;

   for(i=1, ptr=list.first; i<n && ptr!=NULL; i++, ptr=ptr->next)
   {
      /* nop */
   }

   result = ptr==NULL ? NULL : ptr->name;
   return result;
} /* namelist_nth_name()  */


static boolean namelist_copy(NameList_t *dst, const NameList_t src)
{
   NameListItem_t  *ptr;
   boolean          okay = true;

   namelist_init(dst);

   for(ptr=src.first; okay && ptr!=NULL; ptr = ptr->next)
   {
      okay = okay && namelist_add_name(dst, ptr->name);
   }
   return okay;
} /* namelist_copy()  */


void namelist_free(NameList_t *list)
{
   NameListItem_t  *ptr;
   NameListItem_t  *tmp;

   ptr=list->first;
   while (ptr!=NULL)
   {
      FreeMemory((void **)&(ptr->name));
      tmp = ptr;
      ptr=ptr->next;
      FreeMemory((void **)&tmp);
   }
   list->first = NULL;
   list->last  = NULL;
   list->count = 0;
} /* namelist_free()  */


void  colinfo_init(ColumnInfo_t *info)
{
   info->count      = -1;
   info->have_names = false;
   namelist_init(&(info->names));
   info->sizes      = NULL;
   info->types      = NULL;
} /* colinfo_init()  */


void colinfo_free(ColumnInfo_t *info)
{
   if (info->have_names)
   {
      namelist_free(&(info->names));
   }

   if (info->sizes != NULL)
   {
      FreeMemory((void **)&(info->sizes));
   }

   if (info->types != NULL)
   {
      FreeMemory((void **)&(info->types));
   }
} /* colinfo_free()  */


int colinfo_num_columns(const ColumnInfo_t info)
{
   return info.count;
} /* colinfo_num_columns()  */


const char *colinfo_col_name(const ColumnInfo_t info, int column)
{
   const char *result;

   if (column > colinfo_num_columns(info)
       || namelist_count(info.names)<1)
   {
      result = NULL;
   }
   else
   {
      assert(colinfo_num_columns(info) == namelist_count(info.names));

      result = namelist_nth_name(info.names, column);
   }
   return result;
} /* colinfo_col_name()  */


ColumnType_t colinfo_col_type(const ColumnInfo_t info, int column)
{
   ColumnType_t  result;

   if (column > colinfo_num_columns(info))
   {
      result = SYBNONE;
   }
   else
   {
      result = info.types[column-1];
   }
   return result;
} /* colinfo_col_type()  */


/* =========================== colinfo_col_size() ============================
 *
 * Def:  Find the number of bytes taken by a particular column
 *       The first column is numbered 1.
 *
 * Ret:  Number of bytes used to store the column, -1 if the column
 *       doesn't exist
 *
 * ===========================================================================
 */
int colinfo_col_size(
   const ColumnInfo_t info,
   int                column)
{
   int  result;

   if (column > colinfo_num_columns(info))
   {
      result = -1;
   }
   else
   {
      result = info.sizes[column-1];
   }
   return result;
} /* colinfo_col_size()  */


boolean colinfo_have_names(const ColumnInfo_t info)
{
   return info.have_names;
} /* colinfo_have_names()  */


boolean colinfo_have_types(const ColumnInfo_t info)
{
   return info.types!=NULL;
} /* colinfo_have_types()  */


boolean colinfo_have_sizes(const ColumnInfo_t info)
{
   return info.sizes!=NULL;
} /* colinfo_have_sizes()  */


int colinfo_data_offset(
   ColumnInfo_t info,
   int          column)
{
   int   result = -1;
   int   i;

   assert(info.sizes != NULL);

   if (info.sizes==NULL)
   {
      /*
       * might as well start trashing memory a byte before
       * whatever we're about to trash.
       */
      result = -1;
   }
   else
   {
      result = 0;
      for(i=0; i<(column-1); i++)
      {
         result += info.sizes[i];
      }
   }
   return result;
} /* colinfo_data_offset()  */


boolean colinfo_merge(
   ColumnInfo_t *dst,
   ColumnInfo_t src)
{
   boolean   okay = true;


   okay = (dst->count==-1 || dst->count == src.count);

   /*
    * set the count in dst if needed
    */
   if (dst->count == -1)
   {
      dst->count = src.count;
   }

   /*
    * handle the names
    */
   if (okay)
   {
      if (dst->have_names && src.have_names)
      {
         /*
          * Need to check that all the names match
          */
         assert(1==0);
      }
      else if (!dst->have_names && src.have_names)
      {
         okay = okay && namelist_copy(&(dst->names), src.names);
         if (okay)
         {
            dst->have_names = src.have_names;
         }
      }
   }

   /*
    * Handle the sizes
    */
   if (okay)
   {
      if (dst->sizes!=NULL && src.sizes!=NULL)
      {
         /*
          * Need to check that all the sizes match
          */
         assert(1==0);
      }
      else if (dst->sizes==NULL && src.sizes!=NULL)
      {
         int   len = sizeof(src.sizes[0])*src.count;
         okay = okay && AllocMemory((void **)&(dst->sizes), len);
         memmove(dst->sizes, src.sizes, len);
      }
   }

   /*
    * Handle the types
    */
   if (okay)
   {
      if (dst->types!=NULL && src.types!=NULL)
      {
         /*
          * Need to check that all the types match
          */
         assert(1==0);
      }
      else if (dst->types==NULL && src.types!=NULL)
      {
         int   len = sizeof(src.types[0])*src.count;
         okay = okay && AllocMemory((void **)&(dst->types), len);
         memmove(dst->types, src.types, len);
      }
   }

   return okay;
} /* colinfo_merge()  */



static boolean get_physical_packet(TdsStream_t tds)
{
   uchar   tmp_buf[SIZE_OF_HEADER];
   int     count;
   int     rc;
   int     len;

   /* read the header */
   count = 0;
   do
   {
      rc = read(tds->sd, tmp_buf, SIZE_OF_HEADER-count);
      if (rc != -1)
      {
         count += rc;
      }
   } while (count < SIZE_OF_HEADER && rc!=-1);

   if (count == SIZE_OF_HEADER)
   {
      /* find out how many bytes are in this packet */
      len = ntohs(*((short *)(tmp_buf + PACKET_LEN_OFFSET))) - SIZE_OF_HEADER;

      /*
       * XXX Need to check that len isn't bigger than buffer and return
       * an error if it is.  for now just crash
       */
      assert(len>=0 && len<=TDS_MAX_PACKET_SIZE);

      /* read in that many bytes into the buffer */
      tds->bytes_received = 0;
      do
      {
         rc = read(tds->sd, tds->input, len-tds->bytes_received);
         if (rc != -1)
         {
            tds->bytes_received += rc;
         }
      } while (rc!=-1 && tds->bytes_received < len);

      tds->input_cursor   = 0;
   }
   return count==SIZE_OF_HEADER && rc!=-1;
} /* get_physical_packet()  */




boolean tds_get_byte(TdsStream_t tds, uchar *b)
{
   boolean result = true;

   if (tds->input_cursor >= tds->bytes_received)
   {
      result = get_physical_packet(tds);
   }
   if (result)
   {
      *b = tds->input[tds->input_cursor];
      tds->input_cursor++;
      tds->total_bytes_read++;
   }
   return result; /* XXX need to check for and return possible errors. */
} /* tds_get_byte()  */

boolean tds_get_marker(TdsStream_t tds, PacketSubType_t *marker)
{
   uchar   tmp;
   boolean result;
   
   result  = tds_get_byte(tds, &tmp);
   *marker = tmp;
   return result;
} /* tds_get_marker()  */

boolean tds_peek(TdsStream_t  tds, uchar *b)
{
   boolean   result;

   result = tds_get_byte(tds, b);
   tds->input_cursor--;
   tds->total_bytes_read--;
   return result;
} /* tds_peek()  */

boolean tds_peek_marker(TdsStream_t tds, PacketSubType_t *marker)
{
   uchar    tmp_byte;
   boolean  result;

   result = tds_peek(tds, &tmp_byte);
   *marker = tmp_byte;
   return result;
} /* tds_peek_marker()  */

boolean tds_get_bytes(TdsStream_t tds, void *buf, int len)
{
   boolean  result;
   int      i;

   assert(len>=0);
   assert(buf!=NULL);

   for(i=0, result = true; result && i<len; i++)
   {
      result = tds_get_byte(tds, &(((uchar *)buf)[i]));
   }
   return result;
} /* tds_get_bytes()  */


boolean tds_skip(TdsStream_t tds, int len)
{
   int       i;
   uchar     garbage;
   boolean   result;

   assert(len>0);

   for(i=0, result = true; result && i<len; i++)
   {
      result = tds_get_byte(tds, &(garbage));
   }
   return result;
} /* tds_skip() */


boolean tds_get_netshort(TdsStream_t tds, short *s)
{
   boolean result;
   short   tmp;

   result = tds_get_bytes(tds, &tmp, 2);

   *s = ntohs(tmp);
   return result;
} /* tds_get_netshort()  */


boolean tds_get_tdsshort(TdsStream_t tds, short *s)
{
   boolean   result;
   uchar     lo;
   uchar     hi;

   result = tds_get_byte(tds, &lo);
   if (result)
   {
      result = tds_get_byte(tds, &hi);
   }

   *s = lo | (hi << 8);

   return result;
} /* tds_get_tdsshort()  */


boolean tds_get_tdsint(TdsStream_t tds, int *i)
{
   boolean result = true;
   uchar   b0 = 0;
   uchar   b1 = 0;
   uchar   b2 = 0;
   uchar   b3 = 0;

   result = result && tds_get_byte(tds, &b0);
   result = result && tds_get_byte(tds, &b1);
   result = result && tds_get_byte(tds, &b2);
   result = result && tds_get_byte(tds, &b3);

   *i = (b0 << 0) | (b1 << 8) | (b2 << 16) | (b3 << 24);
   return result;
} /* tds_get_tdsint()  */



boolean tds_get_len_and_short_string(
   TdsStream_t   tds,
   uchar        *len,
   char        **str)
{
   boolean    result = true;


   result = result && tds_get_byte(tds, len);
   if (result)
   {
      result = result && AllocMemory((void **)str, (*len)+2);
      result = result && tds_get_bytes(tds, *str, *len);
      (*str)[*len] = '\0';
   }
   return result;
} /* tds_get_len_and_short_string()  */


boolean tds_get_len_and_medium_string(
   TdsStream_t   tds,
   short        *len,
   char        **str)
{
   boolean    result = true;


   result = result && tds_get_tdsshort(tds, len);
   if (result)
   {
      result = result && AllocMemory((void **)str, (*len)+2);
      result = result && tds_get_bytes(tds, *str, *len);
      (*str)[*len] = '\0';
   }
   return result;
} /* tds_get_len_and_medium_string()  */


static void cvt_int(void *dst, void *src, int dst_size, int src_size)
{
   /*
    * Right now this code needs to know the sizes of various datatypes.
    * If I was writing it in Ada I could use representation clauses
    * and just let the compiler handle all the low level code.
    */
   assert(sizeof(int)==4);
   assert(sizeof(long)==4);
   assert(sizeof(short)==2);

   switch(src_size)
   {
      case 4:
      {
         if (dst_size==4)
            *(long *)dst = *(long *)src;
         else if (dst_size==2)
            *(short *)dst = *(long *)src;
         else if (dst_size==1)
            *((unsigned char *)dst) = *((long *)src);
         break;
      }
      case 2:
      {
         if (dst_size==4)
            *(long *)dst = *(short *)src;
         else if (dst_size==2)
            *(short *)dst = *(short *)src;
         else if (dst_size==1)
            *(unsigned char *)dst = *(short *)src;
         break;
      }
      case 1:
      {
         if (dst_size==4)
            *(long *)dst = *(unsigned char *)src;
         else if (dst_size==2)
            *(short *)dst = *(unsigned char *)src;
         else if (dst_size==1)
            *(unsigned char *)dst = *(unsigned char *)src;
         break;
      }
      case 0:
      {
         if (dst_size==4)
            *(long *)dst = 0;
         else if (dst_size==2)
            *(short *)dst = 0;
         else if (dst_size==1)
            *(unsigned char *)dst = 0;
         break;
      }
      default:
      {
         assert("Bad"=="size");
         break;
      }
   }
} /* cvt_int()  */

static boolean tds_get_value_char(
   TdsStream_t   tds,
   void         *dst,
   int           dst_len,
   boolean      *is_null)
{
   boolean  okay = true;
   uchar    len;

   *is_null = true;
   memset(dst, '\0', dst_len);
   okay = okay && tds_get_byte(tds, &len);
   if (okay)
   {
      *is_null = len==0;
      okay = okay && tds_get_bytes(tds, dst, min(dst_len, len));
      if (len>dst_len)
      {
         tds_skip(tds, len-dst_len);
      }
   }

   return okay;
} /* tds_get_value_char()  */


static boolean tds_get_value_varbinary(
   TdsStream_t   tds,
   ColumnType_t  type,
   void         *dst,
   int           dst_len,
   boolean      *is_null)
{
   boolean  okay = true;
   uchar    len;
   uchar    buf[256];
   char    *ptr;
   int      i;

   *is_null = true;
   memset(dst, '\0', dst_len);
   okay = okay && tds_get_byte(tds, &len);
   if (okay)
   {
      *is_null = len==0;
      okay = okay && tds_get_bytes(tds, buf, min(dst_len, len));
      if (len>dst_len)
      {
         tds_skip(tds, len-dst_len);
      }
   }

   ptr = (char *)dst;
   *(ptr++) = '0';
   *(ptr++) = 'x';
   for(i=0; i<len; i++)
   {
      int   nibble;
      char  digits[] = "0123456789abcdef";
      nibble   = (buf[i] & 0xf0) >> 4;
      *(ptr++) = digits[nibble];
      nibble   = (buf[i] & 0x0f);
      *(ptr++) = digits[nibble];
   }
   return okay;
} /* tds_get_value_varbinary()  */


/* ============================== extract_bit() ==============================
 * 
 * Def:  
 *
 * Note:  The most significant bit of buf is in buf[0], the
 *        least significant is in buf[buf_size-1].  We assume an
 *        8 bit character.
 * 
 * Ret:  
 * 
 * Rev:
 *    $Crev$   1999/01/06    CTS
 *       Initially coded.  
 * 
 * ===========================================================================
 */
static int extract_bit(
   unsigned char   *buf,
   int              buf_size,
   int              bit_number)
{
   int   index = -1;
   int   result;
   
   assert(bit_number>=0);
   assert((buf_size * 8) > bit_number);

   index = (buf_size - (bit_number / 8)) - 1;
   bit_number = bit_number % 8;

   result = buf[index];
   result = result >> bit_number;
   result = result & 0x01;
   
   return result;
} /* extract_bit()  */


static double cvt_tds_to_native_double(
   uchar   *buf,
   int      buf_len,
   int      sign_bit,
   int      start_exponent,
   int      end_exponent,
   int      start_mantissa,
   int      end_mantissa,
   int      bias)
{
   double   value    = 0.0;
   int      i        = 0;
   int      exponent = 0;

   /*
    *  The exponent must be big enough to hold all bits 
    *  in the exponent.
    */
   assert((start_exponent-end_exponent)/8 <= sizeof(exponent));

   /*
    *  Store the mantissa
    */
   value = 1.0; /* This is the implied mantissa bit */
   for(i=start_mantissa; i>=end_mantissa; i--)
   {
      value = value * 2;
      value = value + extract_bit(buf, buf_len, i);
   }

   /*
    *  Find out what the exponent is
    */
   for(i=start_exponent, exponent=0; i>=end_exponent; i--)
   {
      exponent = exponent*2 + extract_bit(buf, buf_len, i);
   }
   exponent = exponent - bias;
   exponent = exponent - ((start_mantissa - end_mantissa)+1);
   
   /* 
    *  Shift the mantissa by the exponent.
    */
   if (exponent > 0)
   {
/*        value = value * (1<<exponent); */
      for(i=0; i<exponent; i++)
      {
         value = value * 2;
      }
   }
   else if (exponent < 0)
   {
/*        value = value / (1<<(-exponent));  */
      for(i=exponent; i<0; i++)
      {
         value = value / 2;
      }
   }


   /* 
    * Handle the sign bit
    */
   if (extract_bit(buf, buf_len, sign_bit))
   {
      value = value * -1;
   }
   return value;
} /* cvt_tds_to_native_double()  */


static boolean tds_get_value_floatn(
   TdsStream_t   tds,
   ColumnType_t  type,
   void         *dst,
   int           dst_len,
   boolean      *is_null)
{
   boolean  okay = true;
   uchar    len  = 0;
   double   value = 0.0;
   uchar    buf[16];


   /*
    *  If this assert fails then it probably means that different
    *  parts of the code are using different amounts of space to 
    *  store the floating point numbers.
    */
   assert(dst_len==sizeof(double));

   *is_null = false;

   if (type == SYBFLTN)
   {
      okay = okay && tds_get_byte(tds, &len);
   }
   else if (type == SYBFLT8)
   {
      len = 8;
   }
   else if (type == SYBREAL)
   {
      len = 4;
   }
   else
   {
      assert("Somebody passed wrong 'type' parameter"=="problem");
      len = 0;
   }

   switch(len)
   {
      case 8:
      {
         okay = tds_get_bytes(tds, &buf[7], 1);
         okay = tds_get_bytes(tds, &buf[6], 1);
         okay = tds_get_bytes(tds, &buf[5], 1);
         okay = tds_get_bytes(tds, &buf[4], 1);
         okay = tds_get_bytes(tds, &buf[3], 1);
         okay = tds_get_bytes(tds, &buf[2], 1);
         okay = tds_get_bytes(tds, &buf[1], 1);
         okay = tds_get_bytes(tds, &buf[0], 1);

         value = cvt_tds_to_native_double(buf, 8, 63, 62, 52, 51, 0, 1023);
         break;
      }
      case 4:
      {
         /*
          *  Read the data into memory
          */
         okay = tds_get_bytes(tds, &buf[3], 1);
         okay = tds_get_bytes(tds, &buf[2], 1);
         okay = tds_get_bytes(tds, &buf[1], 1);
         okay = tds_get_bytes(tds, &buf[0], 1);

         value = cvt_tds_to_native_double(buf, 4, 31, 30, 23, 22, 0, 127);
         break;
      }
      case 0:
      {
         *is_null = true;
         break;
      }
      default:
      {
         /* !!! len doesn't have a meaningful value !!! */
         assert(1==0);
         break;
      }
   }
   
   ((double *)dst)[0] = value;

   return okay;
} /* tds_get_value_floatn()  */



/* ======================== tds_get_value_datetime() =========================
 *
 * Def:  Retrieve a SYBDATETIME, SYBDATETIME4, OR SYBDATETIMN from the
 *       tds stream and store it in the destination as two long integers,
 *       the first is the number of days since December 31, 1 BC,
 *       the second is the number of milliseconds into the current day.
 *
 * Ret:
 *
 * Rev:
 *    $Crev$   1999/01/05    CTS
 *       Initially coded.
 *
 * ===========================================================================
 */
static boolean tds_get_value_datetime(
   TdsStream_t   tds,
   ColumnType_t  type,
   void         *dst,
   int           dst_len,
   boolean      *is_null)
{
   boolean  okay = true;
   uchar    len;
   int      tds_days     = 0; /* number of days since Jan 1, 1900           */
   int      tds_jiffies  = 0; /* 1/300th seconds since the start of the day */
   long     days         = 0; /* days since December 31, 1 BC               */
   long     milliseconds = 0; /* milliseconds since start of day            */
   int      more_days    = 0; /* # of days that were in  milliseconds field */ 

   assert(sizeof(long)==4);
   assert(dst_len==8);

   if (type == SYBDATETIMN)
   {
      okay = okay && tds_get_byte(tds, &len);
   }
   else if (type == SYBDATETIME4)
   {
      len = 4;
   }
   else if (type == SYBDATETIME)
   {
      len = 8;
   }
   else
   {
      /*
       * Somebody must have called this with the wrong datatype
       */
      assert(1==0);
   }

   memset(dst, '\0', dst_len);
   *is_null = false;
   switch (len)
   {
      case 8:
      {
         /*
          *  It appears that a datetime is made of of 2 32bit ints
          *  The first one is the number of days since 1900-01-01
          *  The second is the number of seconds*300
          */
         okay = okay && tds_get_tdsint(tds, &tds_days);
         okay = okay && tds_get_tdsint(tds, &tds_jiffies);

         days = tds_days + era_to_tds_day_diff;

         milliseconds = (tds_jiffies / jiffies_per_second) * msec_per_second;
         break;
      }
      case 4:
      {
         /*
          *  Accroding to Transact SQL Reference
          *  a smalldatetime is two small integers.
          *  The first is the number of days past January 1, 1900,
          *  the second smallint is the number of minutes past
          *  midnight.
          */
         unsigned short   tmp_days;
         unsigned short   tmp_minutes;

         okay = okay && tds_get_tdsshort(tds, &tmp_days);
         okay = okay && tds_get_tdsshort(tds, &tmp_minutes);

         days         = (long)tmp_days + era_to_tds_day_diff;
         milliseconds = (long)tmp_minutes * msec_per_minute;
         break;
      }
      case 0:
      {
         *is_null = true;
         break;
      }
      default:
      {
         /*
          * The size must be set in the code above!
          */
         assert(1==0);
         okay = false;
         break;
      }
   }

   /*
    * XXX This (just like SQLServer) ignores time zone issues. :-(
    */
   more_days = milliseconds / msec_per_day;
   milliseconds = milliseconds % msec_per_day;

   days = days + more_days;

   ((long *)dst)[0] = days;
   ((long *)dst)[1] = milliseconds;

   return okay;
} /* tds_get_value_datetime()  */


   
static boolean tds_get_value_int(
   TdsStream_t   tds,
   ColumnType_t  type,
   void         *dst,
   int           dst_len,
   boolean      *is_null)
{
   uchar     buf[8];
   boolean   okay = true;
   uchar     len;

   /*
    * Right now this code needs to know the sizes of various datatypes.
    * If I was writing it in Ada I could use representation clauses
    * and just let the compiler handle all the low level details.
    */
   assert(sizeof(int)==4);
   assert(sizeof(long)==4);
   assert(sizeof(short)==2);
   assert(dst_len==4);

   *is_null = false;
   switch(type)
   {
      case SYBINTN:
      {
         okay = okay && tds_get_byte(tds, &len);
         break;
      }
      case SYBINT4:
      {
         len = 4;
         break;
      }
      case SYBINT2:
      {
         len = 2;
         break;
      }
      case SYBINT1:
      {
         len = 1;
         break;
      }
      default:
      {
         assert("Bogus"=="integer type");
         len = 0;
      }
   }
   if (len==4)
   {
      okay = okay && tds_get_tdsint(tds, (int*)buf);
   }
   else if (len==2 && dst_len>=2)
   {
      okay = okay && tds_get_tdsshort(tds, (short*)buf);
   }
   else if (len==1 && dst_len>=1)
   {
      okay = okay && tds_get_byte(tds, buf);
   }
   else if (len==0)
   {
      *is_null = true;
   }
   else
   {
      assert("Unhandled"=="length");
      okay = false;
   }
   if (okay)
   {
      cvt_int(dst, buf, dst_len, len);
   }
   return okay;
} /* tds_get_value_int()  */


boolean tds_get_value_generic(
   TdsStream_t   tds,
   ColumnType_t  type,
   void         *dst,
   int           dst_len,
   boolean      *is_null)
{
   boolean   okay = true;

   switch(type)
   {
      case SYBNONE:
      {
         assert("Can't put value"=="in SYBNONE");
         break;
      }
      case SYBINT1:
      case SYBINT2:
      case SYBINT4:
      case SYBINTN:
      {
         okay = okay && tds_get_value_int(tds, type, dst, dst_len, is_null);
         break;
      }
      case SYBCHAR:
      case SYBVARCHAR:
      {
         okay = okay && tds_get_value_char(tds, dst, dst_len, is_null);
         break;
      }
      case SYBDATETIMN:
      case SYBDATETIME4:
      case SYBDATETIME:
      {
         okay = okay && tds_get_value_datetime(tds, type, dst, dst_len, is_null);
         break;
      }
      case SYBFLTN:
      case SYBREAL:
      case SYBFLT8:
      {
         okay = okay && tds_get_value_floatn(tds, type, dst, dst_len, is_null);
         break;
      }
      case SYBVARBINARY:
      {
         okay = okay && tds_get_value_varbinary(tds, type, dst, dst_len, 
                                                is_null);
         break;
      }
      case SYBBIT:
      {
         uchar   bit;

         assert(dst_len == 1);
         *is_null = false;
         okay = okay && tds_get_byte(tds, &bit);
         *(uchar *)dst = bit ? 1 : 0;
         break;
      }
      case SYBVOID:
      case SYBIMAGE:
      case SYBTEXT:
      case SYBBINARY:
      case SYBMONEY:
      case SYBDECIMAL:
      case SYBNUMERIC:
      case SYBMONEYN:
      case SYBMONEY4:
      case SYBSMALLMONEY:
      {
         fprintf(stderr, "%s:%d: Not implemented for data type %d\n", 
                 __FILE__, __LINE__,
                 type);
         break;
      }
      default:
      {
         assert("unknown"=="column type");
         break;
      }
   }
   return okay;
} /* tds_get_value_generic()  */


boolean tds_is_result_set(PacketSubType_t marker)
{
   /*
    *  XXX we need to expand our view of what a result
    *  set is.  TDS 5.0 can start a result set with other
    *  tokens.
    */
   return marker == TDS_COL_NAME_TOKEN;
}

boolean tds_is_end_marker(PacketSubType_t marker)
{
   return marker==TDS_END_TOKEN || marker==TDS_DONEPROC;
}


static void free_info(TdsStream_t tds)
{
   switch(tds->info.type)
   {
      case NONE:
      {
         /* nop */
         break;
      }
      case TDS_MSG_TOKEN:
      case TDS_MSG50_TOKEN:
      case TDS_ERR_TOKEN:
      {
         FreeMemory((void **)&(tds->info.info.msg.message));
         FreeMemory((void **)&(tds->info.info.msg.server));
         FreeMemory((void **)&(tds->info.info.msg.proc_name));
         break;
      }
      case TDS_ENV_CHG_TOKEN:
      case TDS_LOGIN_ACK_TOKEN:
      case TDS_END_TOKEN:
      case TDS_DONEPROC:
      case TDS_DONEINPROC:
      case TDS_PROCID:
      case TDS_RET_STAT_TOKEN:
      case TDS_ORDER:
      case TDS_CONTROL:
      {
         /*
          * These packets don't have any data in the info field of
          * the TdsStatus_t structure.
          */
         break;
      }
      case TDS_COL_INFO_TOKEN:
      case TDS_COL_NAME_TOKEN:
      {
         colinfo_free(&(tds->info.info.colinfo));
         break;
      }
      case TDS_TABNAME:
      case TDS_UNKNOWN_165:
      case TDS_ROW_TOKEN:
      case TDS_UNKNOWN_0xE2:
      case TDS_RESULT_TOKEN:
      {
         fprintf(stderr, "\n%s:%d: Unhandled type 0x%02x\n",
                 __FILE__, __LINE__, tds->info.type);
         break;
      }
      default:
      {
         fprintf(stderr, "Unknown type 0x%02x\n", tds->info.type);
         assert("Unrecognized info"=="type");
         break;
      }
   }
   tds->info.type = NONE;
} /* free_info()  */


static boolean is_fixed_size(ColumnType_t type)
{
   switch (type)
   {
      case SYBINT1:
      case SYBINT2:
      case SYBINT4:
      case SYBFLT8:
      case SYBDATETIME:
      case SYBBIT:
      case SYBMONEY:
      case SYBMONEY4:
      case SYBSMALLMONEY:
      case SYBREAL:
      case SYBDATETIME4:
      {
         return true;
      }
      case SYBINTN:
      case SYBMONEYN:
      case SYBVARCHAR:
      case SYBDATETIMN:
      case SYBFLTN:
      case SYBCHAR:
      case SYBIMAGE:
      case SYBVARBINARY:
      case SYBBINARY:
      {
         return false;
      }
      default:
      {
         assert("Bad"=="type");
         return false;
      }
   }
} /* is_fixed_size()  */


static int lookup_db_size(ColumnType_t type)
{
   switch(type)
   {
      case SYBINT1:         return 1;
      case SYBINT2:         return 2;
      case SYBINT4:         return 4;
      case SYBREAL:         return 4;
      case SYBFLT8:         return 8;
      case SYBDATETIME:     return 8;
      case SYBDATETIME4:    return 8;
      case SYBBIT:          return 1;
      case SYBMONEY:        return 8;
      case SYBMONEY4:       return 4;
      case SYBSMALLMONEY:   return 4;
      default:
      {
         assert("Bad"=="type");
         return -1;
      }
   }
} /* lookup_db_size()  */

static int lookup_storage_size(ColumnType_t type)
{
   switch(type)
   {
      case SYBINT1:         return sizeof(int);
      case SYBINT2:         return sizeof(int);
      case SYBINT4:         return sizeof(int);
      case SYBINTN:         return sizeof(int);
      case SYBREAL:         return sizeof(double);
      case SYBFLT8:         return sizeof(double);
      case SYBFLTN:         return sizeof(double);
      case SYBDATETIME:     return 8;
      case SYBDATETIME4:    return 8;
      case SYBBIT:          return 1;
      case SYBMONEY:        return 8;
      case SYBMONEY4:       return 4;
      case SYBSMALLMONEY:   return 4;
      default:
      {
         assert("Bad"=="type");
         return -1;
      }
   }
} /* lookup_storage_size()  */


static boolean process_msg(TdsStream_t tds)
{
   boolean result = true;
   short   len;
   short   tmp_len   = 0;
   uchar   tmp_byte;

   result = result && tds_get_tdsshort(tds, &len);

   result = result && tds_get_tdsint(tds, &tds->info.info.msg.number);
   result = result && tds_get_byte(tds, &tds->info.info.msg.state);
   result = result && tds_get_byte(tds, &tds->info.info.msg.level);
   result = result && tds_get_tdsshort(tds, &tmp_len);
   result = result && (tmp_len<len);
   if (result)
   {
      result = result && AllocMemory((void **)&tds->info.info.msg.message,
                                     tmp_len+2);
      result = result && tds_get_bytes(tds, tds->info.info.msg.message, tmp_len);
      tds->info.info.msg.message[tmp_len] = '\0';
   }
   result = result &&
      tds_get_len_and_short_string(tds,
                                   &tmp_byte,
                                   &tds->info.info.msg.server);
   result = result &&
      tds_get_len_and_short_string(tds,
                                   &tmp_byte,
                                   &tds->info.info.msg.proc_name);
   result = result && (tmp_byte<len);
   result = result && tds_get_byte(tds, &tds->info.info.msg.line);
   result = result && tds_get_byte(tds, &tmp_byte); /* unknown byte */

   return result;
} /* process_msg()  */

static boolean process_end(TdsStream_t tds)
{
   boolean result = true;

   result = result && tds_get_byte(tds, &tds->info.info.end.status);
   result = result && tds_skip(tds, 3);
   result = result && tds_get_tdsint(tds, &tds->info.info.end.row_count);

   tds->hit_end              = true;
   tds->more_results         = (tds->info.info.end.status & 0x01) == 0x01;
   tds->checked_more_results = false;

   /*
    * XXX This is also the place we will need to handle cancels.
    */
   /* handle cancels */

   return  result;
} /* process_end()  */


static boolean process_column_info(TdsStream_t tds)
{
   boolean         result        = true;
   short           total_len     = -1;
   char           *tmp_name      = NULL;
   int             bytes_read    = -1;
   uchar           tmp_byte      = 0xff;
   ColumnType_t    type          = SYBNONE;
   int             size          = -1;
   int             num_allocated = 0;
   int            *sizes         = NULL;
   ColumnType_t   *types         = NULL;
   int             num_columns   = 0;

   num_allocated = 32;
   result = result && AllocMemory((void **)&sizes,
                                  num_allocated*sizeof(sizes[0]));
   result = result && AllocMemory((void **)&types,
                                  num_allocated*sizeof(types[0]));

   result = result && tds_get_tdsshort(tds, &total_len);
   bytes_read  = 0;
   num_columns = 0;
   while(result && bytes_read < total_len)
   {
      result = result && tds_skip(tds, 4);
      bytes_read += 4;

      result = result && tds_get_byte(tds, &tmp_byte);
      /*
       * XXX need to check the result from tds_get_byte() before
       * we process the data.
       */
      bytes_read++;
      type = tmp_byte;

/*fprintf(stderr, "Processing column type %02x\n", type);
 */

      if (type == SYBTEXT
            || type == SYBIMAGE)
      {
         short   name_len;

         result = result && tds_skip(tds, 4);
         bytes_read += 4;

         result = result && tds_get_len_and_medium_string(tds,
                                                         &name_len,
                                                         &tmp_name);
         bytes_read += 2+name_len;
         size = (2<<31) - 1;
      }
      else if (is_fixed_size(type))
      {
         size = lookup_storage_size(type);
      }
      else if (type == SYBINTN)
      {
         result = result && tds_get_byte(tds, &tmp_byte);
         bytes_read += 1;
/*         size   = tmp_byte;
 */
         size = sizeof(int);
      }
      else if (type == SYBFLTN)
      {
         result = result && tds_get_byte(tds, &tmp_byte);
         bytes_read += 1;
         size   = sizeof(double);
      }
      else if (type == SYBVARBINARY)
      {
         result = result && tds_get_byte(tds, &tmp_byte);
         bytes_read++;
         size = 2 + 2*(int)tmp_byte;
      }
      else if (type == SYBDATETIMN)
      {
         result = result && tds_get_byte(tds, &tmp_byte);
         bytes_read += 1;
         size   = 2 * sizeof(long);
      }
      else
      {
         result = result && tds_get_byte(tds, &tmp_byte);
         bytes_read += 1;
         size   = tmp_byte;
      }

      if (num_columns==num_allocated)
      {
         num_allocated *= 2;
         result = result && ResizeMemory((void **)&sizes,
                                         num_allocated*sizeof(sizes[0]));
         result = result && ResizeMemory((void **)&types,
                                         num_allocated*sizeof(types[0]));
      }
      sizes[num_columns] = size;
      types[num_columns] = type;
      num_columns++;
   }


   if (result)
   {
      colinfo_init(&(tds->info.info.colinfo));

      /*
       * transfer the temporary sizes and types arrays
       * into colinfo
       */
      tds->info.info.colinfo.sizes = sizes;
      tds->info.info.colinfo.types = types;
      tds->info.info.colinfo.count = num_columns;
   }

   colinfo_merge(&(tds->colinfo), tds->info.info.colinfo);
   
   return result;
} /* process_column_info()  */


/* ========================= process_column_names() ==========================
 *
 * Def:  Handle TDS_COL_NAME packets.
 *
 * Ret:
 *
 * ===========================================================================
 */
static boolean process_column_names(TdsStream_t tds)
{
   boolean result = true;
   short   total_len;
   uchar   name_len;
   char   *tmp_name;
   int     bytes_read;

   colinfo_init(&(tds->info.info.colinfo));

   result = result && tds_get_tdsshort(tds, &total_len);
   bytes_read = 0;
   while(result && bytes_read < total_len)
   {
      result = result && tds_get_len_and_short_string(tds,
                                                      &name_len,
                                                      &tmp_name);
      bytes_read += name_len+1;
      namelist_add_name(&(tds->info.info.colinfo.names), tmp_name);
      FreeMemory((void **)&tmp_name);
   }
   assert(tds->info.info.colinfo.count==-1
          || (namelist_count(tds->info.info.colinfo.names)
              ==tds->info.info.colinfo.count));
   if (colinfo_num_columns(tds->info.info.colinfo)==-1)
   {
      tds->info.info.colinfo.count =
         namelist_count(tds->info.info.colinfo.names);
      tds->info.info.colinfo.have_names = true;
   }

   colinfo_merge(&(tds->colinfo), tds->info.info.colinfo);
   
   return result;
} /* process_column_names()  */


static boolean process_doneinproc(TdsStream_t tds)
{
   PacketSubType_t  marker;
   boolean          okay = true;
   

   okay = process_end(tds);

   if (okay && (! tds->more_results))
   {
      assert("Confused."=="We should have more stuff coming");
      okay = false;
   }
   else if (okay)
   {
      while (okay && tds->more_results && tds_peek_marker(tds, &marker)
             && (marker==TDS_PROCID
                 || marker==TDS_RET_STAT_TOKEN
                 || marker==TDS_DONEINPROC))
      {
         okay = okay && tds_process_subpacket(tds);
      }
   }
   else
   {
      /* nop */
   }
   return okay;
}


boolean tds_process_subpacket(TdsStream_t tds)
{
   PacketSubType_t    packet_type;
   boolean            result      = false;

   tds_get_marker(tds, &packet_type);

/*
   fprintf(stderr, "!Processing a subpacket at byte %d type is 0x%02x\n", 
           tds->total_bytes_read, packet_type); 
   fflush(stderr);
*/


   free_info(tds);
   tds->info.type = packet_type;
   switch(packet_type)
   {
      case TDS_ERR_TOKEN:
      case TDS_MSG_TOKEN:
      case TDS_MSG50_TOKEN:
      {
         result = process_msg(tds);
         break;
      }
      case TDS_END_TOKEN:
      case TDS_DONEPROC:
      {
         result = process_end(tds);
         break;
      }
      case TDS_COL_NAME_TOKEN:
      {
         result = process_column_names(tds);
         break;
      }
      case TDS_COL_INFO_TOKEN:
      {
         result = process_column_info(tds);
         break;
      }
      case TDS_CONTROL:
      case TDS_ENV_CHG_TOKEN:
      case TDS_LOGIN_ACK_TOKEN:
      {
         /*
          * XXX It might be nice to actually handle these case
          * instead of just skipping over them.
          */
         short    len;
         tds_get_tdsshort(tds, &len);
         tds_skip(tds, len);
         result = true;
         break;
      }
      case TDS_PROCID:
      {
         /* XXX still need to know what this means */
         tds_skip(tds, 8);
         result = true;
         break;
      }
      case TDS_DONEINPROC:
      {
         result = process_doneinproc(tds);
         break;
      }
      case TDS_RET_STAT_TOKEN:
      {
         int      i = 0;
         /* XXX We need to squirrel the return status  away somewhere. */
         result = tds_get_tdsint(tds, &i);
         break;
      }
      case TDS_ORDER:
      {
         short    len;
         tds_get_tdsshort(tds, &len);
         tds_skip(tds, len);
         result = true;
         break;
      }
      default:
      {
         fprintf(stderr, "\n%s:%d: Unhandled subpacket type 0x%02x\n",
                 __FILE__, __LINE__, tds->info.type);
         abort();
         break;
      }
   }
   return result;
} /* tds_process_subpacket()  */


static boolean skip_row(TdsStream_t tds)
{
   int       num_columns = colinfo_num_columns(tds->colinfo);
   boolean   okay        = true;
   int       i;

   /* 
    * We already peeked and found that the next byte will 
    * be a TDS_ROW_TOKEN.  Just skip it.
    */
   okay = okay && tds_skip(tds, 1);

   for(i=0; i<num_columns; i++)
   {
      uchar   buf[257];

      assert(colinfo_col_size(tds->colinfo, i+1) < sizeof(buf));
      okay = okay &&
         tds_get_value_generic(tds, 
                               colinfo_col_type(tds->colinfo, i+1),
                               (void*)buf, 
                               colinfo_col_size(tds->colinfo, i+1),
                               (boolean*)buf);
   }
   return okay;
} /* skip_row()  */


boolean tds_skip_result_set(TdsStream_t tds)
{
   boolean          okay     = true;
   boolean          done     = false;
   PacketSubType_t  marker;

   while (okay && !done)
   {
      okay   = okay && tds_peek_marker(tds, &marker);

      if (okay && tds_is_end_marker(marker))
      {
         okay = okay && tds_process_subpacket(tds);
         done = true;
      }
      else if (okay && marker==TDS_ROW_TOKEN)
      {
         okay = okay && skip_row(tds);
      }
      else
      {
         okay = okay && tds_process_subpacket(tds);
      }
   }
   return okay;
} /* tds_skip_result_set()  */


static boolean changeDB(
   TdsStream_t   tds,
   const char   *dbname,
   ErrorMsg_t   *err)
{
   int     i;
   boolean name_is_okay = true;
   boolean result       = true;
   boolean done         = false;

      
   /*
    * Check that the datbase name is a reasonable length and 
    * doesn't have funky characters.
    */
   i            = -1;
   name_is_okay = true;
   do 
   {
      char   ch;
      
      i++;
      ch = dbname[i];
      //
      name_is_okay = i<32 && (isalpha(ch)
                              || (i>0 && (ch=='_' || isdigit(ch)))
                              || (i>0 && (ch=='\0')));
   } while(dbname[i]!='\0' && name_is_okay);

   if (! name_is_okay)
   {
      result = false;
   }
   else
   {
      char   sql[64];
      sprintf(sql, "use %s", dbname);
      tds_start_packet(tds, QUERY);
      tds_write_bytes(tds, sql, strlen(sql));
      result = -1 != tds_send_packet(tds);
   
      done = false;
      do
      {
         tds_process_subpacket(tds);
         switch(tds->info.type)
         {
            case TDS_ERR_TOKEN:
            {
               err->error = tds->info.info.msg.number;
               safe_strncpy(err->msg, tds->info.info.msg.message,
                            sizeof(err->msg));
               result = false;
               break;
            }
            case TDS_MSG_TOKEN:
            case TDS_MSG50_TOKEN:
            {
               break;
            }
            case TDS_END_TOKEN:
            {
               done = true;
               break;
            }
            case TDS_ENV_CHG_TOKEN:
            {
               /* XXX Need to track the changes. */
               break;
            }
            case TDS_LOGIN_ACK_TOKEN:
            {
               result = true;
               break;
            }
            default:
            {
               fprintf(stderr, "Unrecognized token 0x%02x\n", tds->info.type);
               assert("Unrecognized"=="token");
               break;
            }
         }
      } while (!done);
   }
   return result;
} /* changeDb()  */


static boolean login(
   TdsStream_t   tds,
   const char   *dbname,
   ErrorMsg_t   *err)
{
   boolean   done   = false;
   boolean   result = false;

   tds_start_packet(tds, LOGON);

   /* hostname  (offset0) */
   tds_write_padded_string(tds, tds->cx->client, 30, '\0');
   tds_write_byte(tds, min(strlen(tds->cx->client), 30)); /* length of client name */

   /* username (offset 32) */
   tds_write_padded_string(tds, tds->cx->username, 30, '\0');
   tds_write_byte(tds, min(30, strlen(tds->cx->username)));

   /* password (offset 48) */
   tds_write_padded_string(tds, tds->cx->password, 30, '\0');
   tds_write_byte(tds, min(30, strlen(tds->cx->password)));

   /* hostproc (offset64) */
   tds_write_padded_string(tds, "00000116", 8, '\0');

   /* unused */
   tds_write_padded_string(tds, "", (30-14), '\0');

   /* apptype */
   tds_write_byte(tds, 0x0);
   tds_write_byte(tds, 0xA0);
   tds_write_byte(tds, 0x24);
   tds_write_byte(tds, 0xCC);
   tds_write_byte(tds, 0x50);
   tds_write_byte(tds, 0x12);

   /* hostproc length (offset96) */
   tds_write_byte(tds, 8);

   /* type of int2 */
   tds_write_byte(tds, 3);

   /* type of int4 */
   tds_write_byte(tds, 1);

   /* type of char */
   tds_write_byte(tds, 6);

   /* type of flt */
   tds_write_byte(tds, 10);

   /* type of date */
   tds_write_byte(tds, 9);

   /* notify of use db */
   tds_write_byte(tds, 1);

   /* disallow dump/load and bulk insert */
   tds_write_byte(tds, 1);

   /* sql interface type */
   tds_write_byte(tds, 0);

   /* type of network connection */
   tds_write_byte(tds, 0);

   /* spare[7] */
   tds_write_padded_string(tds, "", 7, '\0');

   /* appname */
   /* XXX perhaps we should use a different appname */
   tds_write_padded_string(tds, "freetds_dbd", 30, '\0');
   tds_write_byte(tds, 11);

   /* server name */
   tds_write_padded_string(tds, tds->cx->server, 30, '\0');
   tds_write_byte(tds, min(30, strlen(tds->cx->server)));

   /* remote passwords */
   tds_write_padded_string(tds, tds->cx->password, 255, '\0');
   tds_write_byte(tds, min(255, strlen(tds->cx->password)));

      /* tds version */
   tds_write_byte(tds, 4);
   tds_write_byte(tds, 2);
   tds_write_byte(tds, 0);
   tds_write_byte(tds, 0);

   /* prog name */
   /* XXX Perhaps this name isn't appropriate either */
   tds_write_padded_string(tds, "Perl", 10, '\0');
   tds_write_byte(tds, 4);

   /* prog version */
   tds_write_byte(tds, 0); /* major version */
   tds_write_byte(tds, 1); /* minor version */
   tds_write_byte(tds, 0);
   tds_write_byte(tds, 0);

      /* auto convert short */
   tds_write_byte(tds, 0);

   /* type of flt4 */
   tds_write_byte(tds, 0x0D);

   /* type of date4 */
   tds_write_byte(tds, 0x11);

   /* language */
   tds_write_padded_string(tds, "us_english", 30, '\0');
   tds_write_byte(tds, 10);

      /* notify on lang change */
   tds_write_byte(tds, 1);

   /* security label hierachy */
   tds_write_tdsshort(tds, 0);

   /* security components */
   tds_write_padded_string(tds, "", 8, '\0');

   /* security spare */
   tds_write_tdsshort(tds, 0);

   /* security login role */
   tds_write_byte(tds, 0);

   /* charset */
   tds_write_padded_string(tds, "iso_1", 30, '\0');
   tds_write_byte(tds, 5);

      /* notify on charset change */
   tds_write_byte(tds, 0);

   /* length of tds packets */
   tds_write_padded_string(tds, "512", 6, '\0');
   tds_write_byte(tds, 3);

      /* pad out to a longword */
   tds_write_padded_string(tds, "", 8, '\0');

   tds_send_packet(tds);

   done   = false;
   result = false;
   do
   {
      tds_process_subpacket(tds);
      switch(tds->info.type)
      {
         case TDS_ERR_TOKEN:
         {
            err->error = tds->info.info.msg.number;
            safe_strncpy(err->msg, tds->info.info.msg.message,
                         sizeof(err->msg));
            result = false;
            /* XXX Somehow propagate this error upward */
            break;
         }
         case TDS_MSG_TOKEN:
         case TDS_MSG50_TOKEN:
         {
            break;
         }
         case TDS_END_TOKEN:
         {
            done = true;
            break;
         }
         case TDS_ENV_CHG_TOKEN:
         {
            /* XXX Need to track the changes. */
            break;
         }
         case TDS_LOGIN_ACK_TOKEN:
         {
            result = true;
            break;
         }
         default:
         {
            fprintf(stderr, "Unrecognized token 0x%02x\n", tds->info.type);
            assert("Unrecognized"=="token");
            break;
         }
      }
   } while (!done);

   if (result)
   {
      result = changeDB(tds, dbname, err);
   }

   return result;
} /* login()  */


static int countStreams(
   TdsStream_t      s)
{
   if (s==NULL)
   {
      return 0;
   }
   else
   {
      return 1 + countStreams(s->next);
   }
}

static boolean newStream(
   TdsConnection_t  cx,
   ErrorMsg_t      *err)
{
   boolean             okay    = true;
   struct hostent *    host    = gethostbyname(cx->server);
   TdsStream_t         tds     = NULL;
   struct sockaddr_in  sin;
   int                 one     = 1;



   /*
    * Prepare the address to connect to
    */
   if (host == NULL)
   {
      okay = false;
      err->error = MISC_ERR;
      safe_strncpy(err->msg, "Couldn't find host ", sizeof(err->msg));
      safe_strncat(err->msg, cx->server,  sizeof(err->msg));
   }
   else
   {
      memcpy(&sin.sin_addr.s_addr, host->h_addr, sizeof(sin.sin_addr.s_addr));
      sin.sin_family      = AF_INET;
      sin.sin_port        = htons(cx->port);
      
      /*
       * Allocate and initialize a TDS structure
       */
      if (okay)
      {
         if (AllocMemory((void **)&tds, sizeof(*tds)))
         {
            tds->sd                   = -1;
            tds->creating_packet      = false;
            tds->more_results         = false;
            tds->hit_end              = false;
            tds->checked_more_results = false;
            colinfo_init(&(tds->colinfo));
            tds->bytes_received       = 0;
            tds->input_cursor         = 0;
            tds->bytes_to_send        = 0;
            tds->total_bytes_read     = 0;
            tds->packet_type          = NONE;
            tds->info.type            = NONE;
            tds->cx                   = cx;
            tds->next                 = NULL;
         }
         else
         {
            okay = false;
            err->error = MISC_ERR;
            safe_strncpy(err->msg, "Couldn't allocate memory",sizeof(err->msg));
         }
      }
      
      /*
       * Connect the socket
       */
      if (okay)
      {
         if (-1 == (tds->sd = socket (AF_INET, SOCK_STREAM, 0)))
         {
            okay = false;
            err->error = MISC_ERR;
            sprintf(err->msg, "Couldn't open socket.  (errno=%d)",
                    errno);
         }
      }
      if (okay && 0 != setsockopt(tds->sd, IPPROTO_TCP, TCP_NODELAY, 
                                  (char*)&one, sizeof(int)))
      {
         err->error = MISC_ERR;
         sprintf(err->msg, "Couldn't set socket option. (errno=%d)",
                 errno);
      }
      if (okay)
      {
         if (-1 == connect(tds->sd,
                           (struct sockaddr *) &sin, sizeof(struct sockaddr)))
         {
            okay = false;
            err->error = MISC_ERR;
            sprintf(err->msg, "Couldn't connect socket.  (errno=%d)",
                    errno);
         }
      }
      
      if (okay && !login(tds, cx->dbname, err))
      {
         okay = false;
      }
      
      /*
       * Now add this stream to the availableStreams
       * of the parent connection 
       */
      if (okay)
      {
         tds->next            = cx->availableStreams;
         cx->availableStreams = tds;
      }
   }

#ifndef NDEBUG
   {
      static int  total                   = 0;
      int         available               = countStreams(cx->availableStreams);
      int         inUseStreams            = countStreams(cx->inUseStreams);

      if (okay)
      {
         total++;
         (cx->total_streams)++;
      }
      
      assert(total<6);
      assert(cx->total_streams == (available + inUseStreams));
   }
#endif

   return okay;
} /* newStream()  */


/* ========================= tds_createConnection() ==========================
 *
 * Def:
 *
 * Ret:
 *
 * ===========================================================================
 */
TdsConnection_t  tds_createConnection(
   TdsVersion_t     protocol,
   const char      *server,
   short            port,
   const char      *dbname,
   const char      *user,
   const char      *password,
   const char      *client,
   ErrorMsg_t      *err)
{
   TdsConnection_t   result = NULL;

   if (! AllocMemory((void **)&result, sizeof(*result)))
   {
      err->error = MISC_ERR;
      safe_strncpy(err->msg, "Couldn't allocate memory", sizeof(err->msg));
      result = NULL;
   }
   else
   {
      safe_strncpy(result->server, server, sizeof(result->server));
      result->port = port;
      safe_strncpy(result->dbname, dbname, sizeof(result->dbname));
      safe_strncpy(result->username, user, sizeof(result->username));
      safe_strncpy(result->password, password, sizeof(result->password));
      safe_strncpy(result->client, client, sizeof(result->client));

      result->total_streams    = 0;
      result->availableStreams = NULL;
      result->inUseStreams     = NULL;

      if (! newStream(result, err))
      {
         result = NULL;
      }
   }

   return result;
} /* tds_createConnection()  */


void tds_closeConnection(TdsConnection_t  *cx)
{
   closeStreams(&((*cx)->availableStreams));
   closeStreams(&((*cx)->inUseStreams));
   (*cx)->server[0]   = '\0';
   (*cx)->dbname[0]   = '\0';
   (*cx)->username[0] = '\0';
   (*cx)->password[0] = '\0';

   FreeMemory((void**)cx);
} /* tds_closeConnection()  */




TdsStream_t tds_allocateStream(TdsConnection_t  cx, void *misc)
{
   TdsStream_t  result = NULL;
   ErrorMsg_t   err;


   if (cx->availableStreams == NULL)
   {
      newStream(cx, &err);
   }

   if (cx->availableStreams != NULL)
   {
      result               = cx->availableStreams;
      result->debug_info   = misc;
      cx->availableStreams = result->next;
      result->next         = cx->inUseStreams;
      cx->inUseStreams     = result;
   }
   return result;
} /* tds_allocateStream()  */


static void closeStreams(
   TdsStream_t     *tds)
{
   if (*tds == NULL)
   {
      /* nop */
   }
   else
   {
      closeStreams(&(*tds)->next);
      close((*tds)->sd);
      FreeMemory((void **)&(*tds));
   }
} /* closeStreams()  */



static void releaseStream(
   TdsStream_t  *availableList,
   TdsStream_t  *inUseList,
   TdsStream_t   tds)
{
   if (*inUseList == NULL)
   {
      /*
       * XXX
       * in pre-production code we want to core dump
       * in production code we'll have to do something
       */
      assert(*inUseList != NULL);
   }
   else
   {
      if (*inUseList != tds)
      {
         releaseStream(availableList, &((*inUseList)->next), tds);
      }
      else
      {
         /*
          * Move tds out of the list it is in and over to the
          * availableStreams list
          */
         *inUseList           = tds->next;
         tds->debug_info      = NULL;
         tds->next            = *availableList;
         *availableList       = tds;
      }
   }
} /* releaseStream()  */


void tds_releaseStream(
   TdsConnection_t cx,
   TdsStream_t     tds)
{
   /*
    * XXX Should we just send a cancel and be done with it?
    */

   /*
    * See if the stream is expecting any results.
    * If so we need to cancel the stream and skip over all tokens upto and
    * including the cancel acknowledgment.  Otherwise when this stream is
    * reused the new connection will see old data.
    */
   while (!tds->hit_end || tds->more_results)
   {
      if (!tds_skip_result_set(tds))
      {
         /* XXX How should this be handled in production code? */
         fprintf(stderr, "Couldn't skip the result set.");
      }
   }

   releaseStream(&(cx->availableStreams), &(cx->inUseStreams), tds);
} /* tds_releaseStream()  */



/* =========================== tds_start_packet() ============================
 *
 * Def:  Start a logical TDS packet.
 *
 * Ret:  void
 *
 * ===========================================================================
 */
void tds_start_packet(
   TdsStream_t tds,
   PacketType_t packet_type)
{
   assert(tds!=NULL);

   memset(tds->output, 0x99, sizeof(tds->output));
   tds->creating_packet = true;
   tds->packet_type     = packet_type;
   tds->bytes_to_send   = SIZE_OF_HEADER;
} /* tds_start_packet()  */


/* ============================== send_packet() ==============================
 *
 * Def:  The higher level routines (the tds_write_* routines)
 *       deal with logical data.  A logical packet can be any length
 *       but that logical packet has to be broken into 512 byte
 *       physical packets.
 *
 *       This is the lower level routine that sends the actual physical
 *       packets.
 *
 * Ret:  -1 on failure, otherwise the number of physical bytes sent.
 *
 * ===========================================================================
 */
static int send_packet(TdsStream_t  tds, boolean last)
{
   int   rc;
   int   bytes_left;
   int   tmp_count = tds->bytes_to_send;

   /*
    * Put in the header info into the packet
    */
   tds->bytes_to_send = 0;
   tds_write_byte(tds, tds->packet_type);
   tds_write_byte(tds, last ? 1 : 0);
   tds_write_netshort(tds, tmp_count);
   tds_write_padded_string(tds, "", 4, '\0');
   tds->bytes_to_send = tmp_count;

   /*
    * send the packet.
    */
   bytes_left = tds->bytes_to_send;
   do
   {
      rc = write(tds->sd, tds->output, bytes_left);
      bytes_left -= rc;
   } while (rc!=-1 && bytes_left>0);

   /*
    * Make sure to reset all of the bookeeping information.
    */
   tds->bytes_to_send        = -1;
   tds->creating_packet      = false;
   tds->more_results         = false;
   tds->checked_more_results = false;
   tds->hit_end              = false;

   return rc==-1 ? -1 : tmp_count;
} /* send_packet()  */


/* ============================ tds_write_byte() =============================
 *
 * Def:  Write a single byte to the TDS stream.
 *
 * Ret:  true on success, false otherwise
 *
 * ===========================================================================
 */
int tds_write_byte(
   TdsStream_t tds,
   uchar       b)
{
   boolean  okay = true;

   /* XXX change sizeof(tds->buffer) to be the packet size */

   /*
    * See if the output buffer is full.
    */
   if (tds->bytes_to_send == sizeof(tds->output))
   {
      /*
       * if so, flush it.
       */
      okay = -1 != send_packet(tds, false);
      tds->bytes_to_send = SIZE_OF_HEADER;
   }

   /*
    * Place the byte into the output buffer.
    */
   tds->output[tds->bytes_to_send] = b;
   tds->bytes_to_send++;

   return okay ? 1 : -1;
} /* tds_write_byte()  */


/* ============================ tds_write_bytes() ============================
 *
 * Def:  Write 'len' bytes from 'buf' to the TDS stream.
 *
 * Ret:  true on success, false on failure
 *
 * ===========================================================================
 */
int tds_write_bytes(
   TdsStream_t  tds,
   void        *buf,
   int          len)
{
   int  i;
   for(i=0; i<len; i++)
   {
      tds_write_byte(tds, ((char *)buf)[i]);
   }
   return len;
} /* tds_write_bytes()  */


/* ======================== tds_write_padded_string() ========================
 *
 * Def:  Write a character string out to the TDS stream up to length
 *       'len'.  If the string is shorter than pad the output with 'pad'.
 *
 * Ret:  true on success, false otherwise
 *
 * ===========================================================================
 */
int tds_write_padded_string(
   TdsStream_t tds,
   const char *s,
   int         len,
   char        pad)
{
   int  i;
   for(i=0; i<len && s[i]!='\0'; i++)
   {
      tds_write_byte(tds, s[i]);
   }
   for(; i<len; i++)
   {
      tds_write_byte(tds, pad);
   }
   return len;
} /* tds_write_padded_string()  */



/* ========================== tds_write_netshort() ===========================
 *
 * Def:  Write a 16 bit integer in network byte order.
 *
 * Ret:  true on success, false otherwise.
 *
 * ===========================================================================
 */
int tds_write_netshort(
   TdsStream_t tds,
   short s)
{
   tds_write_byte(tds, (uchar)((s>>8)&0xff));
   tds_write_byte(tds, (uchar)((s>>0)&0xff));
   return 2;
} /* tds_write_netshort()  */


/* =========================== tds_write_netlong() ===========================
 *
 * Def:  Write a 32 bit integer in network byte order
 *
 * Ret:  true on success, false on failure
 *
 * ===========================================================================
 */
int tds_write_netlong(
   TdsStream_t tds,
   long l)
{
   tds_write_byte(tds, (uchar)((l>>24)&0xff));
   tds_write_byte(tds, (uchar)((l>>16)&0xff));
   tds_write_byte(tds, (uchar)((l>>8)&0xff));
   tds_write_byte(tds, (uchar)((l>>0)&0xff));
   return 4;
} /* tds_write_netlong()  */


/* ========================== tds_write_tdsshort() ===========================
 *
 * Def:  Write a 16 bit integer in TDS byte order.
 *
 * Ret:  true on success, false otherwise.
 *
 * ===========================================================================
 */
int tds_write_tdsshort(
   TdsStream_t tds,
   short s)
{
   tds_write_byte(tds, (uchar)((s>>0)&0xff));
   tds_write_byte(tds, (uchar)((s>>8)&0xff));
   return 2;
} /* tds_write_tdsshort()  */


/* =========================== tds_write_tdslong() ===========================
 *
 * Def:
 *
 * Ret:
 *
 * ===========================================================================
 */
int tds_write_tdslong(
   TdsStream_t tds,
   long        l)
{
   abort();
} /* tds_write_tdslong()  */


/* ============================ tds_send_packet() ============================
 *
 * Def:  send the packet that has been built.
 *
 * Ret:  true on success.
 *
 * ===========================================================================
 */
boolean tds_send_packet(TdsStream_t tds)
{
   return send_packet(tds, true);
} /* tds_send_packet()  */


