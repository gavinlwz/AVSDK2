//
//  tmedia_utils.c
//  youme_voice_engine
//
//  Created by mac on 2017/5/19.
//  Copyright © 2017年 Youme. All rights reserved.
//

#include "tsk_string.h"
#include "tmedia_defaults.h"
#include "tmedia_utils.h"

int32_t tmedia_utils_file_size(const char *fileName) {
    FILE* fp = fopen(fileName, "r");
    if(!fp) return -1;
    fseek(fp, 0L, SEEK_END);
    int32_t size = ftell(fp);
    fclose(fp);
    return size;
}

uint32_t tmedia_utils_dump(const char *fileName, const void *data, uint32_t size, tsk_bool_t *create)
{
    uint32_t ret = -1;
    char filePath[256];
    FILE* file;
    const char *baseDir = tmedia_defaults_get_app_document_path();
    uint32_t baseLen = 0;
    if (NULL != baseDir)
    {
        strncpy(filePath, baseDir, sizeof(filePath) - 1);
        baseLen = strlen(filePath) + 1;
        strncat(filePath, fileName, sizeof(filePath) - baseLen);
        
        int32_t file_size = tmedia_utils_file_size(filePath);
        if((file_size < 0) || (file_size > TMEDIA_DUMP_FILE_MAX_SIZE)) {
            file = fopen (filePath, "wb");
        } else {
            file = fopen (filePath, "ab");
        }
        if(file) {
            ret = fwrite(data, 1, size, file);
            fclose(file);
        }
    }
    return ret;
}

uint32_t tmedia_utils_dump_log(const char *filename, tsk_bool_t *create, const char *pszFormat, ...)
{
    char log[256];
    
    va_list args;
    va_start(args, pszFormat);
    int size = vsnprintf(log, sizeof(log), pszFormat, args);
    va_end(args);
    
    return tmedia_utils_dump(filename, log, size, create);
}

uint32_t tmedia_utils_dump_rtp_packet(const char *filename, trtp_rtp_packet_t *packet, tsk_bool_t *create)
{
    char *log = tsk_null;
    int size = 0;
    
    char* data = (char*)(packet->payload.data)?packet->payload.data:(char*)packet->payload.data_const;
    
    size = tsk_sprintf(&log, "rtp: session_id:%d type:%d ver:%d marker:%d seq_num:%d timestamp:%d ssrc:%d data_size:%ld data:%d\n",
                       packet->header->csrc[0],
                       packet->header->payload_type,
                       packet->header->version,
                       packet->header->marker,
                       packet->header->seq_num,
                       packet->header->timestamp,
                       packet->header->ssrc,
                       packet->payload.size,
                       data[0]);

    tmedia_utils_dump(filename, log, size, create);
    if(log) {
        tsk_free((void **)&log);
    }
    return 0;
}
