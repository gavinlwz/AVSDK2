

#ifndef YOUME_BUFFER_H
#define YOUME_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
/** Copy n elements from src to dst. The 0* term provides compile-time type checking  */

#define YOUME_COPY(dst, src, n) (memcpy ((dst), (src), (n) * sizeof (*(dst)) + 0 * ((dst) - (src))))

    
/** Copy n elements from src to dst, allowing overlapping regions. The 0* term provides compile-time type checking */
#define YOUME_MOVE(dst, src, n) (memmove ((dst), (src), (n) * sizeof (*(dst)) + 0 * ((dst) - (src))))

    
/** For n elements worth of memory, set every byte to the value of c, starting at address dst */

#define YOUME_MEMSET(dst, c, n) (memset ((dst), (c), (n) * sizeof (*(dst))))

struct YOUMEBuffer_;
typedef struct YOUMEBuffer_ YOUMEBuffer;

YOUMEBuffer *youme_buffer_init(int size);

void youme_buffer_destroy(YOUMEBuffer *st);

int youme_buffer_write(YOUMEBuffer *st, void *data, int len);

int youme_buffer_writezeros(YOUMEBuffer *st, int len);

int youme_buffer_read(YOUMEBuffer *st, void *data, int len);

int youme_buffer_get_available(YOUMEBuffer *st);

int youme_buffer_resize(YOUMEBuffer *st, int len);

#ifdef __cplusplus
}
#endif

#endif




