
#ifdef HAVE_CONFIG_H
#if ANDROID
#include "android_config.h"
#else
#include "ios_config.h"
#endif
#endif

#include "stdlib.h"
#include "stdio.h"
#include "youme_buffer.h"

struct YOUMEBuffer_
{
   char *data;
   int   size;
   int   read_ptr;
   int   write_ptr;
   int   available;
};

YOUMEBuffer *youme_buffer_init(int size)
{
   YOUMEBuffer *st = malloc(sizeof(YOUMEBuffer));
   st->data = malloc(size);
   st->size = size;
   st->read_ptr = 0;
   st->write_ptr = 0;
   st->available = 0;
   return st;
}

void youme_buffer_destroy(YOUMEBuffer *st)
{
   free(st->data);
   free(st);
}

int youme_buffer_write(YOUMEBuffer *st, void *_data, int len)
{
   int end;
   int end1;
   char *data = _data;
   if (len > st->size)
   {
      data += len-st->size;
      len = st->size;
   }
   end = st->write_ptr + len;
   end1 = end;
   if (end1 > st->size)
      end1 = st->size;
   YOUME_COPY(st->data + st->write_ptr, data, end1 - st->write_ptr);
   if (end > st->size)
   {
      end -= st->size;
      YOUME_COPY(st->data, data+end1 - st->write_ptr, end);
   }
   st->available += len;

   st->write_ptr += len;
   if (st->write_ptr > st->size)
      st->write_ptr -= st->size;
    
    if (st->available > st->size)
    {
        st->available = st->size;
        st->read_ptr = st->write_ptr;
    }
   return len;
}

int youme_buffer_writezeros(YOUMEBuffer *st, int len)
{
   /* This is almost the same as for youme_buffer_write() but using 
   YOUME_MEMSET() instead of youme_COPY(). Update accordingly. */
   int end;
   int end1;
   if (len > st->size)
   {
      len = st->size;
   }
   end = st->write_ptr + len;
   end1 = end;
   if (end1 > st->size)
      end1 = st->size;
   YOUME_MEMSET(st->data + st->write_ptr, 0, end1 - st->write_ptr);
   if (end > st->size)
   {
      end -= st->size;
      YOUME_MEMSET(st->data, 0, end);
   }
   st->available += len;
   if (st->available > st->size)
   {
      st->available = st->size;
      st->read_ptr = st->write_ptr;
   }
   st->write_ptr += len;
   if (st->write_ptr > st->size)
      st->write_ptr -= st->size;
   return len;
}

int youme_buffer_read(YOUMEBuffer *st, void *_data, int len)
{
   int end, end1;
   char *data = _data;
   if (len > st->available)
   {
      YOUME_MEMSET(data+st->available, 0, st->size-st->available);
      len = st->available;
   }
   end = st->read_ptr + len;
   end1 = end;
   if (end1 > st->size)
      end1 = st->size;
   YOUME_COPY(data, st->data + st->read_ptr, end1 - st->read_ptr);

   if (end > st->size)
   {
      end -= st->size;
      YOUME_COPY(data+end1 - st->read_ptr, st->data, end);
   }
   st->available -= len;
   st->read_ptr += len;
   if (st->read_ptr > st->size)
      st->read_ptr -= st->size;
   return len;
}

int youme_buffer_get_available(YOUMEBuffer *st)
{
   return st->available;
}

int youme_buffer_resize(YOUMEBuffer *st, int len)
{
   int old_len = st->size;
   if (len > old_len)
   {
      st->data = realloc(st->data, len);
   }
   else
   {
      st->data = realloc(st->data, len);
   }
   return len;
}
