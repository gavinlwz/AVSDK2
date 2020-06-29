
/**@file trtp_rtp_header.c
 * @brief RTP header.
 *
*
 *

 */
#include "tinyrtp/rtp/trtp_rtp_header.h"
#include "tinymedia/tmedia_defaults.h"
#include "tnet_endianness.h"

#include "tsk_memory.h"
#include "tsk_debug.h"

/* RFC 3550 section 5.1 - RTP Fixed Header Fields
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           timestamp                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           synchronization source (SSRC) identifier            |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |            contributing source (CSRC) identifiers             |
   |                             ....                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

/* Create new RTP header */
trtp_rtp_header_t *trtp_rtp_header_create_null ()
{
    return tsk_object_new (trtp_rtp_header_def_t);
}

trtp_rtp_header_t *
trtp_rtp_header_create (uint32_t ssrc, uint16_t seq_num, uint64_t timestamp, uint8_t payload_type, tsk_bool_t marker)
{
    trtp_rtp_header_t *header;
    if ((header = trtp_rtp_header_create_null ()))
    {
		header->version = TRTP_RTP_VERSION;
        header->marker = marker ? 1 : 0;
        header->payload_type = payload_type;
        header->seq_num = seq_num;
        //#ifdef YOUME_FOR_QINIU
        header->timestampl = timestamp;
        //#endif
        header->timestamp = (uint32_t)timestamp;
        header->ssrc = ssrc;
        
        header->video_id=0xff;
        header->frameType=0xff;
        header->frameSerial=0xffff;
        header->frameCount = 0;
    }
    return header;
}



/* guess what is the minimum required size to serialize the header */
tsk_size_t trtp_rtp_header_guess_serialbuff_size (const trtp_rtp_header_t *self)
{
    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }
    return (TRTP_RTP_HEADER_MIN_SIZE + (self->csrc_count << 2));
}

/* serialize the RTP header to a buffer */
// the buffer size must be at least equal to "trtp_rtp_header_guess_serialbuff_size()"
// returns the number of written bytes
tsk_size_t trtp_rtp_header_serialize_to (const trtp_rtp_header_t *self, void *buffer, tsk_size_t size)
{
    tsk_size_t ret;
    tsk_size_t i, j;
    uint8_t *pbuff = (uint8_t *)buffer;

    if (!buffer || (size < (ret = trtp_rtp_header_guess_serialbuff_size (self))))
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return 0;
    }

    // Octet-0: version(2), Padding(1), Extension(1), CSRC Count(4)
    pbuff[0] = (((uint8_t)self->version) << 6) | (((uint8_t)self->padding) << 5) |
               (((uint8_t)self->extension) << 4) | ((uint8_t)self->csrc_count);
    // Octet-1: Marker(1), Payload Type(7)
    pbuff[1] = (((uint8_t)self->marker) << 7) | ((uint8_t)self->payload_type);
    // Octet-2-3: Sequence number (16)
    // *((uint16_t*)&pbuff[2]) = tnet_htons(self->seq_num);
    pbuff[2] = self->seq_num >> 8;
    pbuff[3] = self->seq_num & 0xFF;
    // Octet-4-5-6-7: timestamp (32)
    // ((uint32_t*)&pbuff[4]) = tnet_htonl(self->timestamp);
    pbuff[4] = self->timestamp >> 24;
    pbuff[5] = (self->timestamp >> 16) & 0xFF;
    pbuff[6] = (self->timestamp >> 8) & 0xFF;
    pbuff[7] = self->timestamp & 0xFF;
    
    
    // Octet-8-9-10-11: SSRC (32)
    //((uint32_t*)&pbuff[8]) = tnet_htonl(self->ssrc);
    pbuff[8] = self->ssrc >> 24;
    pbuff[9] = (self->ssrc >> 16) & 0xFF;
    pbuff[10] = (self->ssrc >> 8) & 0xFF;
    pbuff[11] = self->ssrc & 0xFF;
    
    // Octet-12-13-14-15-****: CSRC
    for (i = 0, j = 12; i < self->csrc_count; ++i, j+=4)
    {
        // *((uint32_t*)&pbuff[12+i]) = tnet_htonl(self->csrc[i]);
        pbuff[j] = self->csrc[i] >> 24;
        pbuff[j + 1] = (self->csrc[i] >> 16) & 0xFF;
        pbuff[j + 2] = (self->csrc[i] >> 8) & 0xFF;
        pbuff[j + 3] = self->csrc[i] & 0xFF;
    }

    //#ifdef YOUME_FOR_QINIU
    //8个字节的毫秒时间戳
    uint64_t uNetOrder = tnet_htonll(self->timestampl);
    memcpy(pbuff+12+self->csrc_count*4  , &uNetOrder, sizeof(uint64_t));
    //#endif
    return ret;
}

/** Serialize rtp header object into binary buffer */
tsk_buffer_t *trtp_rtp_header_serialize (const trtp_rtp_header_t *self)
{
    tsk_buffer_t *buffer;
    tsk_size_t size;

    if (!self)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return tsk_null;
    }

    size = trtp_rtp_header_guess_serialbuff_size (self);
    if (!(buffer = tsk_buffer_create (tsk_null, size)))
    {
        TSK_DEBUG_ERROR ("Failed to create new buffer");
        TSK_OBJECT_SAFE_FREE (buffer);
    }
    else
    {
        trtp_rtp_header_serialize_to (self, buffer->data, buffer->size);
    }

    return buffer;
}

/** Deserialize rtp header object from binary buffer */
trtp_rtp_header_t *trtp_rtp_header_deserialize (const void *data, tsk_size_t size)
{
    trtp_rtp_header_t *header = tsk_null;
    const uint8_t *pdata = (const uint8_t *)data;
    uint8_t csrc_count, i;

    if (!data)
    {
        TSK_DEBUG_ERROR ("Invalid parameter");
        return tsk_null;
    }

    if (size < TRTP_RTP_HEADER_MIN_SIZE)
    {
        TSK_DEBUG_ERROR ("Too short to contain RTP header");
        return tsk_null;
    }

    /* Before starting to deserialize, get the "csrc_count" and check the length validity
    * CSRC count (4 last bits)
    */
    csrc_count = (*pdata & 0x0F);
    if (size < (tsk_size_t)TRTP_RTP_HEADER_MIN_SIZE + (csrc_count << 2))
    {
        TSK_DEBUG_ERROR ("Too short to contain RTP header");
        return tsk_null;
    }

    if (!(header = trtp_rtp_header_create_null ()))
    {
        TSK_DEBUG_ERROR ("Failed to create new RTP header");
        return tsk_null;
    }

    /* version (2bits) */
    header->version = (*pdata >> 6);
    /* Padding (1bit) */
    header->padding = ((*pdata >> 5) & 0x01);
    /* Extension (1bit) */
    header->extension = ((*pdata >> 4) & 0x01);
    /* CSRC Count (4bits) */
    header->csrc_count = csrc_count;
    // skip octet
    ++pdata;

    /* Marker (1bit) */
    header->marker = (*pdata >> 7);
    /* Payload Type (7bits) */
    header->payload_type = (*pdata & 0x7F);
    // skip octet
    ++pdata;

    /* Sequence Number (16bits) */
    header->seq_num = pdata[0] << 8 | pdata[1];
    // skip octets
    pdata += 2;
    
    
    /* timestamp (32bits) */
    header->timestamp = pdata[0] << 24 | pdata[1] << 16 | pdata[2] << 8 | pdata[3];
    // skip octets
    pdata += 4;
    
    /* synchronization source (SSRC) identifier (32bits) */
    header->ssrc = pdata[0] << 24 | pdata[1] << 16 | pdata[2] << 8 | pdata[3];
    // skip octets
    pdata += 4;
    
    /* contributing source (CSRC) identifiers */
    for (i = 0; i < csrc_count; i++, pdata += 4)
    {
        header->csrc[i] = pdata[0] << 24 | pdata[1] << 16 | pdata[2] << 8 | pdata[3];
    }

    //#ifdef YOUME_FOR_QINIU
    /* timestamp (32bits) */
   // uint64_t uNetOrder = *(uint64_t*)pdata;
	uint64_t uNetOrder = 0;
	memcpy(&uNetOrder, pdata, 8);
	
	header->timestampl = tnet_ntohll(uNetOrder); // skip octets
    //#endif
    header->bkaud_source = BKAUD_SOURCE_NULL;
    return header;
}


//=================================================================================================
//	RTP header object definition
//
static tsk_object_t *trtp_rtp_header_ctor (tsk_object_t *self, va_list *app)
{
    trtp_rtp_header_t *header = self;
    if (header)
    {
    }
    return self;
}

static tsk_object_t *trtp_rtp_header_dtor (tsk_object_t *self)
{
    trtp_rtp_header_t *header = self;
    if (header)
    {
    }

    return self;
}

static const tsk_object_def_t trtp_rtp_header_def_s = {
    sizeof (trtp_rtp_header_t),
    trtp_rtp_header_ctor,
    trtp_rtp_header_dtor,
    tsk_null,
};
const tsk_object_def_t *trtp_rtp_header_def_t = &trtp_rtp_header_def_s;
