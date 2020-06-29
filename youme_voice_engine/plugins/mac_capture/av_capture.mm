#import <AVFoundation/AVFoundation.h>
#import <AppKit/NSScreen.h>

#include <mutex>
#include <pthread.h>
#include "tsk_time.h"
#include "av_capture.h"
#include "tsk_debug.h"
#include "YouMeConstDefine.h"



std::mutex* youme_osxscreen_capture_mutex = new std::mutex;


enum AVPixelFormat {
	AV_PIX_FMT_NONE = -1,
	AV_PIX_FMT_YUV420P,   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
	AV_PIX_FMT_YUYV422,   ///< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
	AV_PIX_FMT_RGB24,     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
	AV_PIX_FMT_BGR24,     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
	AV_PIX_FMT_YUV422P,   ///< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
	AV_PIX_FMT_YUV444P,   ///< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
	AV_PIX_FMT_YUV410P,   ///< planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
	AV_PIX_FMT_YUV411P,   ///< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
	AV_PIX_FMT_GRAY8,     ///<        Y        ,  8bpp
	AV_PIX_FMT_MONOWHITE, ///<        Y        ,  1bpp, 0 is white, 1 is black, in each byte pixels are ordered from the msb to the lsb
	AV_PIX_FMT_MONOBLACK, ///<        Y        ,  1bpp, 0 is black, 1 is white, in each byte pixels are ordered from the msb to the lsb
	AV_PIX_FMT_PAL8,      ///< 8 bits with AV_PIX_FMT_RGB32 palette
	AV_PIX_FMT_YUVJ420P,  ///< planar YUV 4:2:0, 12bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV420P and setting color_range
	AV_PIX_FMT_YUVJ422P,  ///< planar YUV 4:2:2, 16bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV422P and setting color_range
	AV_PIX_FMT_YUVJ444P,  ///< planar YUV 4:4:4, 24bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV444P and setting color_range
#if FF_API_XVMC
	AV_PIX_FMT_XVMC_MPEG2_MC,///< XVideo Motion Acceleration via common packet passing
	AV_PIX_FMT_XVMC_MPEG2_IDCT,
	AV_PIX_FMT_XVMC = AV_PIX_FMT_XVMC_MPEG2_IDCT,
#endif /* FF_API_XVMC */
	AV_PIX_FMT_UYVY422,   ///< packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
	AV_PIX_FMT_UYYVYY411, ///< packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
	AV_PIX_FMT_BGR8,      ///< packed RGB 3:3:2,  8bpp, (msb)2B 3G 3R(lsb)
	AV_PIX_FMT_BGR4,      ///< packed RGB 1:2:1 bitstream,  4bpp, (msb)1B 2G 1R(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits
	AV_PIX_FMT_BGR4_BYTE, ///< packed RGB 1:2:1,  8bpp, (msb)1B 2G 1R(lsb)
	AV_PIX_FMT_RGB8,      ///< packed RGB 3:3:2,  8bpp, (msb)2R 3G 3B(lsb)
	AV_PIX_FMT_RGB4,      ///< packed RGB 1:2:1 bitstream,  4bpp, (msb)1R 2G 1B(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits
	AV_PIX_FMT_RGB4_BYTE, ///< packed RGB 1:2:1,  8bpp, (msb)1R 2G 1B(lsb)
	AV_PIX_FMT_NV12,      ///< planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V)
	AV_PIX_FMT_NV21,      ///< as above, but U and V bytes are swapped

	AV_PIX_FMT_ARGB,      ///< packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
	AV_PIX_FMT_RGBA,      ///< packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
	AV_PIX_FMT_ABGR,      ///< packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
	AV_PIX_FMT_BGRA,      ///< packed BGRA 8:8:8:8, 32bpp, BGRABGRA...

	AV_PIX_FMT_GRAY16BE,  ///<        Y        , 16bpp, big-endian
	AV_PIX_FMT_GRAY16LE,  ///<        Y        , 16bpp, little-endian
	AV_PIX_FMT_YUV440P,   ///< planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
	AV_PIX_FMT_YUVJ440P,  ///< planar YUV 4:4:0 full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV440P and setting color_range
	AV_PIX_FMT_YUVA420P,  ///< planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A samples)
#if FF_API_VDPAU
	AV_PIX_FMT_VDPAU_H264,///< H.264 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
	AV_PIX_FMT_VDPAU_MPEG1,///< MPEG-1 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
	AV_PIX_FMT_VDPAU_MPEG2,///< MPEG-2 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
	AV_PIX_FMT_VDPAU_WMV3,///< WMV3 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
	AV_PIX_FMT_VDPAU_VC1, ///< VC-1 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
#endif
	AV_PIX_FMT_RGB48BE,   ///< packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for each R/G/B component is stored as big-endian
	AV_PIX_FMT_RGB48LE,   ///< packed RGB 16:16:16, 48bpp, 16R, 16G, 16B, the 2-byte value for each R/G/B component is stored as little-endian

	AV_PIX_FMT_RGB565BE,  ///< packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), big-endian
	AV_PIX_FMT_RGB565LE,  ///< packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), little-endian
	AV_PIX_FMT_RGB555BE,  ///< packed RGB 5:5:5, 16bpp, (msb)1X 5R 5G 5B(lsb), big-endian   , X=unused/undefined
	AV_PIX_FMT_RGB555LE,  ///< packed RGB 5:5:5, 16bpp, (msb)1X 5R 5G 5B(lsb), little-endian, X=unused/undefined

	AV_PIX_FMT_BGR565BE,  ///< packed BGR 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb), big-endian
	AV_PIX_FMT_BGR565LE,  ///< packed BGR 5:6:5, 16bpp, (msb)   5B 6G 5R(lsb), little-endian
	AV_PIX_FMT_BGR555BE,  ///< packed BGR 5:5:5, 16bpp, (msb)1X 5B 5G 5R(lsb), big-endian   , X=unused/undefined
	AV_PIX_FMT_BGR555LE,  ///< packed BGR 5:5:5, 16bpp, (msb)1X 5B 5G 5R(lsb), little-endian, X=unused/undefined

#if FF_API_VAAPI
	/** @name Deprecated pixel formats */
	/**@{*/
	AV_PIX_FMT_VAAPI_MOCO, ///< HW acceleration through VA API at motion compensation entry-point, Picture.data[3] contains a vaapi_render_state struct which contains macroblocks as well as various fields extracted from headers
	AV_PIX_FMT_VAAPI_IDCT, ///< HW acceleration through VA API at IDCT entry-point, Picture.data[3] contains a vaapi_render_state struct which contains fields extracted from headers
	AV_PIX_FMT_VAAPI_VLD,  ///< HW decoding through VA API, Picture.data[3] contains a VASurfaceID
	/**@}*/
	AV_PIX_FMT_VAAPI = AV_PIX_FMT_VAAPI_VLD,
#else
	/**
	 *  Hardware acceleration through VA-API, data[3] contains a
	 *  VASurfaceID.
	 */
	AV_PIX_FMT_VAAPI,
#endif

	AV_PIX_FMT_YUV420P16LE,  ///< planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
	AV_PIX_FMT_YUV420P16BE,  ///< planar YUV 4:2:0, 24bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian
	AV_PIX_FMT_YUV422P16LE,  ///< planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
	AV_PIX_FMT_YUV422P16BE,  ///< planar YUV 4:2:2, 32bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian
	AV_PIX_FMT_YUV444P16LE,  ///< planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
	AV_PIX_FMT_YUV444P16BE,  ///< planar YUV 4:4:4, 48bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
#if FF_API_VDPAU
	AV_PIX_FMT_VDPAU_MPEG4,  ///< MPEG-4 HW decoding with VDPAU, data[0] contains a vdpau_render_state struct which contains the bitstream of the slices as well as various fields extracted from headers
#endif
	AV_PIX_FMT_DXVA2_VLD,    ///< HW decoding through DXVA2, Picture.data[3] contains a LPDIRECT3DSURFACE9 pointer

	AV_PIX_FMT_RGB444LE,  ///< packed RGB 4:4:4, 16bpp, (msb)4X 4R 4G 4B(lsb), little-endian, X=unused/undefined
	AV_PIX_FMT_RGB444BE,  ///< packed RGB 4:4:4, 16bpp, (msb)4X 4R 4G 4B(lsb), big-endian,    X=unused/undefined
	AV_PIX_FMT_BGR444LE,  ///< packed BGR 4:4:4, 16bpp, (msb)4X 4B 4G 4R(lsb), little-endian, X=unused/undefined
	AV_PIX_FMT_BGR444BE,  ///< packed BGR 4:4:4, 16bpp, (msb)4X 4B 4G 4R(lsb), big-endian,    X=unused/undefined
	AV_PIX_FMT_YA8,       ///< 8 bits gray, 8 bits alpha

	AV_PIX_FMT_Y400A = AV_PIX_FMT_YA8, ///< alias for AV_PIX_FMT_YA8
	AV_PIX_FMT_GRAY8A = AV_PIX_FMT_YA8, ///< alias for AV_PIX_FMT_YA8

	AV_PIX_FMT_BGR48BE,   ///< packed RGB 16:16:16, 48bpp, 16B, 16G, 16R, the 2-byte value for each R/G/B component is stored as big-endian
	AV_PIX_FMT_BGR48LE,   ///< packed RGB 16:16:16, 48bpp, 16B, 16G, 16R, the 2-byte value for each R/G/B component is stored as little-endian

	/**
	 * The following 12 formats have the disadvantage of needing 1 format for each bit depth.
	 * Notice that each 9/10 bits sample is stored in 16 bits with extra padding.
	 * If you want to support multiple bit depths, then using AV_PIX_FMT_YUV420P16* with the bpp stored separately is better.
	 */
	AV_PIX_FMT_YUV420P9BE, ///< planar YUV 4:2:0, 13.5bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian
	AV_PIX_FMT_YUV420P9LE, ///< planar YUV 4:2:0, 13.5bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
	AV_PIX_FMT_YUV420P10BE,///< planar YUV 4:2:0, 15bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian
	AV_PIX_FMT_YUV420P10LE,///< planar YUV 4:2:0, 15bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
	AV_PIX_FMT_YUV422P10BE,///< planar YUV 4:2:2, 20bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian
	AV_PIX_FMT_YUV422P10LE,///< planar YUV 4:2:2, 20bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
	AV_PIX_FMT_YUV444P9BE, ///< planar YUV 4:4:4, 27bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
	AV_PIX_FMT_YUV444P9LE, ///< planar YUV 4:4:4, 27bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
	AV_PIX_FMT_YUV444P10BE,///< planar YUV 4:4:4, 30bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
	AV_PIX_FMT_YUV444P10LE,///< planar YUV 4:4:4, 30bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
	AV_PIX_FMT_YUV422P9BE, ///< planar YUV 4:2:2, 18bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian
	AV_PIX_FMT_YUV422P9LE, ///< planar YUV 4:2:2, 18bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
	AV_PIX_FMT_VDA_VLD,    ///< hardware decoding through VDA
	AV_PIX_FMT_GBRP,      ///< planar GBR 4:4:4 24bpp
	AV_PIX_FMT_GBR24P = AV_PIX_FMT_GBRP, // alias for #AV_PIX_FMT_GBRP
	AV_PIX_FMT_GBRP9BE,   ///< planar GBR 4:4:4 27bpp, big-endian
	AV_PIX_FMT_GBRP9LE,   ///< planar GBR 4:4:4 27bpp, little-endian
	AV_PIX_FMT_GBRP10BE,  ///< planar GBR 4:4:4 30bpp, big-endian
	AV_PIX_FMT_GBRP10LE,  ///< planar GBR 4:4:4 30bpp, little-endian
	AV_PIX_FMT_GBRP16BE,  ///< planar GBR 4:4:4 48bpp, big-endian
	AV_PIX_FMT_GBRP16LE,  ///< planar GBR 4:4:4 48bpp, little-endian
	AV_PIX_FMT_YUVA422P,  ///< planar YUV 4:2:2 24bpp, (1 Cr & Cb sample per 2x1 Y & A samples)
	AV_PIX_FMT_YUVA444P,  ///< planar YUV 4:4:4 32bpp, (1 Cr & Cb sample per 1x1 Y & A samples)
	AV_PIX_FMT_YUVA420P9BE,  ///< planar YUV 4:2:0 22.5bpp, (1 Cr & Cb sample per 2x2 Y & A samples), big-endian
	AV_PIX_FMT_YUVA420P9LE,  ///< planar YUV 4:2:0 22.5bpp, (1 Cr & Cb sample per 2x2 Y & A samples), little-endian
	AV_PIX_FMT_YUVA422P9BE,  ///< planar YUV 4:2:2 27bpp, (1 Cr & Cb sample per 2x1 Y & A samples), big-endian
	AV_PIX_FMT_YUVA422P9LE,  ///< planar YUV 4:2:2 27bpp, (1 Cr & Cb sample per 2x1 Y & A samples), little-endian
	AV_PIX_FMT_YUVA444P9BE,  ///< planar YUV 4:4:4 36bpp, (1 Cr & Cb sample per 1x1 Y & A samples), big-endian
	AV_PIX_FMT_YUVA444P9LE,  ///< planar YUV 4:4:4 36bpp, (1 Cr & Cb sample per 1x1 Y & A samples), little-endian
	AV_PIX_FMT_YUVA420P10BE, ///< planar YUV 4:2:0 25bpp, (1 Cr & Cb sample per 2x2 Y & A samples, big-endian)
	AV_PIX_FMT_YUVA420P10LE, ///< planar YUV 4:2:0 25bpp, (1 Cr & Cb sample per 2x2 Y & A samples, little-endian)
	AV_PIX_FMT_YUVA422P10BE, ///< planar YUV 4:2:2 30bpp, (1 Cr & Cb sample per 2x1 Y & A samples, big-endian)
	AV_PIX_FMT_YUVA422P10LE, ///< planar YUV 4:2:2 30bpp, (1 Cr & Cb sample per 2x1 Y & A samples, little-endian)
	AV_PIX_FMT_YUVA444P10BE, ///< planar YUV 4:4:4 40bpp, (1 Cr & Cb sample per 1x1 Y & A samples, big-endian)
	AV_PIX_FMT_YUVA444P10LE, ///< planar YUV 4:4:4 40bpp, (1 Cr & Cb sample per 1x1 Y & A samples, little-endian)
	AV_PIX_FMT_YUVA420P16BE, ///< planar YUV 4:2:0 40bpp, (1 Cr & Cb sample per 2x2 Y & A samples, big-endian)
	AV_PIX_FMT_YUVA420P16LE, ///< planar YUV 4:2:0 40bpp, (1 Cr & Cb sample per 2x2 Y & A samples, little-endian)
	AV_PIX_FMT_YUVA422P16BE, ///< planar YUV 4:2:2 48bpp, (1 Cr & Cb sample per 2x1 Y & A samples, big-endian)
	AV_PIX_FMT_YUVA422P16LE, ///< planar YUV 4:2:2 48bpp, (1 Cr & Cb sample per 2x1 Y & A samples, little-endian)
	AV_PIX_FMT_YUVA444P16BE, ///< planar YUV 4:4:4 64bpp, (1 Cr & Cb sample per 1x1 Y & A samples, big-endian)
	AV_PIX_FMT_YUVA444P16LE, ///< planar YUV 4:4:4 64bpp, (1 Cr & Cb sample per 1x1 Y & A samples, little-endian)

	AV_PIX_FMT_VDPAU,     ///< HW acceleration through VDPAU, Picture.data[3] contains a VdpVideoSurface

	AV_PIX_FMT_XYZ12LE,      ///< packed XYZ 4:4:4, 36 bpp, (msb) 12X, 12Y, 12Z (lsb), the 2-byte value for each X/Y/Z is stored as little-endian, the 4 lower bits are set to 0
	AV_PIX_FMT_XYZ12BE,      ///< packed XYZ 4:4:4, 36 bpp, (msb) 12X, 12Y, 12Z (lsb), the 2-byte value for each X/Y/Z is stored as big-endian, the 4 lower bits are set to 0
	AV_PIX_FMT_NV16,         ///< interleaved chroma YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
	AV_PIX_FMT_NV20LE,       ///< interleaved chroma YUV 4:2:2, 20bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
	AV_PIX_FMT_NV20BE,       ///< interleaved chroma YUV 4:2:2, 20bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian

	AV_PIX_FMT_RGBA64BE,     ///< packed RGBA 16:16:16:16, 64bpp, 16R, 16G, 16B, 16A, the 2-byte value for each R/G/B/A component is stored as big-endian
	AV_PIX_FMT_RGBA64LE,     ///< packed RGBA 16:16:16:16, 64bpp, 16R, 16G, 16B, 16A, the 2-byte value for each R/G/B/A component is stored as little-endian
	AV_PIX_FMT_BGRA64BE,     ///< packed RGBA 16:16:16:16, 64bpp, 16B, 16G, 16R, 16A, the 2-byte value for each R/G/B/A component is stored as big-endian
	AV_PIX_FMT_BGRA64LE,     ///< packed RGBA 16:16:16:16, 64bpp, 16B, 16G, 16R, 16A, the 2-byte value for each R/G/B/A component is stored as little-endian

	AV_PIX_FMT_YVYU422,   ///< packed YUV 4:2:2, 16bpp, Y0 Cr Y1 Cb

	AV_PIX_FMT_VDA,          ///< HW acceleration through VDA, data[3] contains a CVPixelBufferRef

	AV_PIX_FMT_YA16BE,       ///< 16 bits gray, 16 bits alpha (big-endian)
	AV_PIX_FMT_YA16LE,       ///< 16 bits gray, 16 bits alpha (little-endian)

	AV_PIX_FMT_GBRAP,        ///< planar GBRA 4:4:4:4 32bpp
	AV_PIX_FMT_GBRAP16BE,    ///< planar GBRA 4:4:4:4 64bpp, big-endian
	AV_PIX_FMT_GBRAP16LE,    ///< planar GBRA 4:4:4:4 64bpp, little-endian
	/**
	 *  HW acceleration through QSV, data[3] contains a pointer to the
	 *  mfxFrameSurface1 structure.
	 */
	AV_PIX_FMT_QSV,
	/**
	 * HW acceleration though MMAL, data[3] contains a pointer to the
	 * MMAL_BUFFER_HEADER_T structure.
	 */
	AV_PIX_FMT_MMAL,

	AV_PIX_FMT_D3D11VA_VLD,  ///< HW decoding through Direct3D11, Picture.data[3] contains a ID3D11VideoDecoderOutputView pointer

	/**
	 * HW acceleration through CUDA. data[i] contain CUdeviceptr pointers
	 * exactly as for system memory frames.
	 */
	AV_PIX_FMT_CUDA,

	AV_PIX_FMT_0RGB = 0x123 + 4, ///< packed RGB 8:8:8, 32bpp, XRGBXRGB...   X=unused/undefined
	AV_PIX_FMT_RGB0,        ///< packed RGB 8:8:8, 32bpp, RGBXRGBX...   X=unused/undefined
	AV_PIX_FMT_0BGR,        ///< packed BGR 8:8:8, 32bpp, XBGRXBGR...   X=unused/undefined
	AV_PIX_FMT_BGR0,        ///< packed BGR 8:8:8, 32bpp, BGRXBGRX...   X=unused/undefined

	AV_PIX_FMT_YUV420P12BE, ///< planar YUV 4:2:0,18bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian
	AV_PIX_FMT_YUV420P12LE, ///< planar YUV 4:2:0,18bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
	AV_PIX_FMT_YUV420P14BE, ///< planar YUV 4:2:0,21bpp, (1 Cr & Cb sample per 2x2 Y samples), big-endian
	AV_PIX_FMT_YUV420P14LE, ///< planar YUV 4:2:0,21bpp, (1 Cr & Cb sample per 2x2 Y samples), little-endian
	AV_PIX_FMT_YUV422P12BE, ///< planar YUV 4:2:2,24bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian
	AV_PIX_FMT_YUV422P12LE, ///< planar YUV 4:2:2,24bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
	AV_PIX_FMT_YUV422P14BE, ///< planar YUV 4:2:2,28bpp, (1 Cr & Cb sample per 2x1 Y samples), big-endian
	AV_PIX_FMT_YUV422P14LE, ///< planar YUV 4:2:2,28bpp, (1 Cr & Cb sample per 2x1 Y samples), little-endian
	AV_PIX_FMT_YUV444P12BE, ///< planar YUV 4:4:4,36bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
	AV_PIX_FMT_YUV444P12LE, ///< planar YUV 4:4:4,36bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
	AV_PIX_FMT_YUV444P14BE, ///< planar YUV 4:4:4,42bpp, (1 Cr & Cb sample per 1x1 Y samples), big-endian
	AV_PIX_FMT_YUV444P14LE, ///< planar YUV 4:4:4,42bpp, (1 Cr & Cb sample per 1x1 Y samples), little-endian
	AV_PIX_FMT_GBRP12BE,    ///< planar GBR 4:4:4 36bpp, big-endian
	AV_PIX_FMT_GBRP12LE,    ///< planar GBR 4:4:4 36bpp, little-endian
	AV_PIX_FMT_GBRP14BE,    ///< planar GBR 4:4:4 42bpp, big-endian
	AV_PIX_FMT_GBRP14LE,    ///< planar GBR 4:4:4 42bpp, little-endian
	AV_PIX_FMT_YUVJ411P,    ///< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples) full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV411P and setting color_range

	AV_PIX_FMT_BAYER_BGGR8,    ///< bayer, BGBG..(odd line), GRGR..(even line), 8-bit samples */
	AV_PIX_FMT_BAYER_RGGB8,    ///< bayer, RGRG..(odd line), GBGB..(even line), 8-bit samples */
	AV_PIX_FMT_BAYER_GBRG8,    ///< bayer, GBGB..(odd line), RGRG..(even line), 8-bit samples */
	AV_PIX_FMT_BAYER_GRBG8,    ///< bayer, GRGR..(odd line), BGBG..(even line), 8-bit samples */
	AV_PIX_FMT_BAYER_BGGR16LE, ///< bayer, BGBG..(odd line), GRGR..(even line), 16-bit samples, little-endian */
	AV_PIX_FMT_BAYER_BGGR16BE, ///< bayer, BGBG..(odd line), GRGR..(even line), 16-bit samples, big-endian */
	AV_PIX_FMT_BAYER_RGGB16LE, ///< bayer, RGRG..(odd line), GBGB..(even line), 16-bit samples, little-endian */
	AV_PIX_FMT_BAYER_RGGB16BE, ///< bayer, RGRG..(odd line), GBGB..(even line), 16-bit samples, big-endian */
	AV_PIX_FMT_BAYER_GBRG16LE, ///< bayer, GBGB..(odd line), RGRG..(even line), 16-bit samples, little-endian */
	AV_PIX_FMT_BAYER_GBRG16BE, ///< bayer, GBGB..(odd line), RGRG..(even line), 16-bit samples, big-endian */
	AV_PIX_FMT_BAYER_GRBG16LE, ///< bayer, GRGR..(odd line), BGBG..(even line), 16-bit samples, little-endian */
	AV_PIX_FMT_BAYER_GRBG16BE, ///< bayer, GRGR..(odd line), BGBG..(even line), 16-bit samples, big-endian */
#if !FF_API_XVMC
	AV_PIX_FMT_XVMC,///< XVideo Motion Acceleration via common packet passing
#endif /* !FF_API_XVMC */
	AV_PIX_FMT_YUV440P10LE, ///< planar YUV 4:4:0,20bpp, (1 Cr & Cb sample per 1x2 Y samples), little-endian
	AV_PIX_FMT_YUV440P10BE, ///< planar YUV 4:4:0,20bpp, (1 Cr & Cb sample per 1x2 Y samples), big-endian
	AV_PIX_FMT_YUV440P12LE, ///< planar YUV 4:4:0,24bpp, (1 Cr & Cb sample per 1x2 Y samples), little-endian
	AV_PIX_FMT_YUV440P12BE, ///< planar YUV 4:4:0,24bpp, (1 Cr & Cb sample per 1x2 Y samples), big-endian
	AV_PIX_FMT_AYUV64LE,    ///< packed AYUV 4:4:4,64bpp (1 Cr & Cb sample per 1x1 Y & A samples), little-endian
	AV_PIX_FMT_AYUV64BE,    ///< packed AYUV 4:4:4,64bpp (1 Cr & Cb sample per 1x1 Y & A samples), big-endian

	AV_PIX_FMT_VIDEOTOOLBOX, ///< hardware decoding through Videotoolbox

	AV_PIX_FMT_P010LE, ///< like NV12, with 10bpp per component, data in the high bits, zeros in the low bits, little-endian
	AV_PIX_FMT_P010BE, ///< like NV12, with 10bpp per component, data in the high bits, zeros in the low bits, big-endian

	AV_PIX_FMT_GBRAP12BE,  ///< planar GBR 4:4:4:4 48bpp, big-endian
	AV_PIX_FMT_GBRAP12LE,  ///< planar GBR 4:4:4:4 48bpp, little-endian

	AV_PIX_FMT_GBRAP10BE,  ///< planar GBR 4:4:4:4 40bpp, big-endian
	AV_PIX_FMT_GBRAP10LE,  ///< planar GBR 4:4:4:4 40bpp, little-endian

	AV_PIX_FMT_MEDIACODEC, ///< hardware decoding through MediaCodec

	AV_PIX_FMT_GRAY12BE,   ///<        Y        , 12bpp, big-endian
	AV_PIX_FMT_GRAY12LE,   ///<        Y        , 12bpp, little-endian
	AV_PIX_FMT_GRAY10BE,   ///<        Y        , 10bpp, big-endian
	AV_PIX_FMT_GRAY10LE,   ///<        Y        , 10bpp, little-endian

	AV_PIX_FMT_P016LE, ///< like NV12, with 16bpp per component, little-endian
	AV_PIX_FMT_P016BE, ///< like NV12, with 16bpp per component, big-endian

	AV_PIX_FMT_NB         ///< number of pixel formats, DO NOT USE THIS if you want to link with shared libav* because the number of formats might differ between versions
};

#define AV_PIX_FMT_YUV422P16 AV_PIX_FMT_NE(YUV422P16BE, YUV422P16LE)
#define AV_PIX_FMT_YUV422P10 AV_PIX_FMT_NE(YUV422P10BE, YUV422P10LE)
#define AV_PIX_FMT_YUV444P10 AV_PIX_FMT_NE(YUV444P10BE, YUV444P10LE)

struct AVFPixelFormatSpec {
	enum AVPixelFormat ff_id;
	OSType avf_id;
};

static const struct AVFPixelFormatSpec avf_pixel_formats[] = {
	{ AV_PIX_FMT_YUV420P,      kCVPixelFormatType_420YpCbCr8Planar },
	{ AV_PIX_FMT_MONOBLACK,    kCVPixelFormatType_1Monochrome },
	{ AV_PIX_FMT_RGB555BE,     kCVPixelFormatType_16BE555 },
	{ AV_PIX_FMT_RGB555LE,     kCVPixelFormatType_16LE555 },
	{ AV_PIX_FMT_RGB565BE,     kCVPixelFormatType_16BE565 },
	{ AV_PIX_FMT_RGB565LE,     kCVPixelFormatType_16LE565 },
	{ AV_PIX_FMT_RGB24,        kCVPixelFormatType_24RGB },
	{ AV_PIX_FMT_BGR24,        kCVPixelFormatType_24BGR },
	{ AV_PIX_FMT_0RGB,         kCVPixelFormatType_32ARGB },
	{ AV_PIX_FMT_BGR0,         kCVPixelFormatType_32BGRA },
	{ AV_PIX_FMT_0BGR,         kCVPixelFormatType_32ABGR },
	{ AV_PIX_FMT_RGB0,         kCVPixelFormatType_32RGBA },
	{ AV_PIX_FMT_BGR48BE,      kCVPixelFormatType_48RGB },
	{ AV_PIX_FMT_UYVY422,      kCVPixelFormatType_422YpCbCr8 },
	{ AV_PIX_FMT_YUVA444P,     kCVPixelFormatType_4444YpCbCrA8R },
	{ AV_PIX_FMT_YUVA444P16LE, kCVPixelFormatType_4444AYpCbCr16 },
	{ AV_PIX_FMT_YUV444P,      kCVPixelFormatType_444YpCbCr8 },
	//{ AV_PIX_FMT_YUV422P16,    kCVPixelFormatType_422YpCbCr16 },
	//{ AV_PIX_FMT_YUV422P10,    kCVPixelFormatType_422YpCbCr10 },
//    { AV_PIX_FMT_YUV444P10,    kCVPixelFormatType_444YpCbCr10 },
	{ AV_PIX_FMT_NV12,         kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange },
	{ AV_PIX_FMT_YUYV422,      kCVPixelFormatType_422YpCbCr8_yuvs },
#if !TARGET_OS_IPHONE && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
	{ AV_PIX_FMT_GRAY8,        kCVPixelFormatType_OneComponent8 },
#endif
	{ AV_PIX_FMT_NONE, 0 }
};



#if 0
typedef struct {

	pthread_mutex_t frame_lock;
	// pthread_cond_t  frame_wait_cond;

	// pthread_t  		capture_thread;
	int 			pthread_exit_flag;	// pthread exit flag, default false
	int             num_video_devices;
	id              avf_delegate;
	int             frames_captured;
	ScreenCb 		videoCb;

	int 			frame_rate;
	int             width;
	int 			height;

	enum AVPixelFormat pixel_format;

	AVCaptureSession         *capture_session;
	AVCaptureVideoDataOutput *video_output;
	CMSampleBufferRef         current_frame;
} AVFContext;


static int avf_get_frame(AVFContext* ctx);

static void lock_frames(AVFContext* ctx) {
   pthread_mutex_lock(&ctx->frame_lock);
}

static void unlock_frames(AVFContext* ctx) {
   pthread_mutex_unlock(&ctx->frame_lock);
}

/** FrameReciever class - delegate for AVCaptureSession
 */
@interface AVFFrameReceiver : NSObject {
	AVFContext* _context;
}

- (id)initWithContext:(AVFContext*)context;

- (void)  captureOutput:(AVCaptureOutput *)captureOutput
    didOutputSampleBuffer:(CMSampleBufferRef)videoFrame
    fromConnection:(AVCaptureConnection *)connection;

@end

@implementation AVFFrameReceiver

- (id)initWithContext:(AVFContext*)context {
	if (self = [super init]) {
		_context = context;
	}
	return self;
}

-(void)releaseContext
{
    _context = nil;
}


- (void)  captureOutput:(AVCaptureOutput *)captureOutput
    didOutputSampleBuffer:(CMSampleBufferRef)videoFrame
    fromConnection:(AVCaptureConnection *)connection {
    {
        std::lock_guard<std::mutex> lock(*youme_osxscreen_capture_mutex);
        if(_context){
            lock_frames(_context);
            // callback from system, then send frame to caller
            if (_context->current_frame != nil) {
                CFRelease(_context->current_frame);
                _context->current_frame = nil;
            }

            _context->current_frame = (CMSampleBufferRef)CFRetain(videoFrame);

            if (!_context->pthread_exit_flag) {
                avf_get_frame(_context);
            }
            unlock_frames(_context);
            
            ++_context->frames_captured;
        }
    }
}

@end

/**
 * Configure the video device.
 *
 * Configure the video device using a run-time approach to access properties
 * since formats, activeFormat are available since  iOS >= 7.0 or OSX >= 10.7
 * and activeVideoMaxFrameDuration is available since i0S >= 7.0 and OSX >= 10.9.
 *
 * The NSUndefinedKeyException must be handled by the caller of this function.
 *
 */
static int configure_video_device(AVFContext *data, AVCaptureDevice *video_device) {
	AVFContext *ctx = (AVFContext*)data;

	int framerate = ctx->frame_rate;
	NSObject *range = nil;
	NSObject *format = nil;
	NSObject *selected_range = nil;
	NSObject *selected_format = nil;

	for (format in [video_device valueForKey:@"formats"]) {
		CMFormatDescriptionRef formatDescription;
		CMVideoDimensions dimensions;

		formatDescription = (__bridge CMFormatDescriptionRef) [format performSelector:@selector(formatDescription)];
		dimensions = CMVideoFormatDescriptionGetDimensions(formatDescription);

		if ((ctx->width == 0 && ctx->height == 0) ||
		        (dimensions.width == ctx->width && dimensions.height == ctx->height)) {

			selected_format = format;

			for (range in [format valueForKey:@"videoSupportedFrameRateRanges"]) {
				double max_framerate;

				[[range valueForKey:@"maxFrameRate"] getValue:&max_framerate];
				if (fabs (framerate - max_framerate) < 0.01) {
					selected_range = range;
					break;
				}
			}
		}
	}

	if (!selected_format) {
		TSK_DEBUG_ERROR("Selected video size (%dx%d) is not supported by the device\n",
		                ctx->width, ctx->height);
		goto unsupported_format;
	}

	if (!selected_range) {
		TSK_DEBUG_ERROR("Selected framerate (%f) is not supported by the device\n",
		                framerate);
		goto unsupported_format;
	}

	if ([video_device lockForConfiguration:NULL] == YES) {
		NSValue *min_frame_duration = [selected_range valueForKey:@"minFrameDuration"];

		[video_device setValue:selected_format forKey:@"activeFormat"];
		[video_device setValue:min_frame_duration forKey:@"activeVideoMinFrameDuration"];
		[video_device setValue:min_frame_duration forKey:@"activeVideoMaxFrameDuration"];
	} else {
		TSK_DEBUG_ERROR("Could not lock device for configuration");
		return -1;
	}

	return 0;

unsupported_format:

	TSK_DEBUG_INFO("Supported modes:\n");
	for (format in [video_device valueForKey:@"formats"]) {
		CMFormatDescriptionRef formatDescription;
		CMVideoDimensions dimensions;

		formatDescription = (__bridge CMFormatDescriptionRef) [format performSelector:@selector(formatDescription)];
		dimensions = CMVideoFormatDescriptionGetDimensions(formatDescription);

		for (range in [format valueForKey:@"videoSupportedFrameRateRanges"]) {
			double min_framerate;
			double max_framerate;

			[[range valueForKey:@"minFrameRate"] getValue:&min_framerate];
			[[range valueForKey:@"maxFrameRate"] getValue:&max_framerate];
			TSK_DEBUG_INFO("  %dx%d@[%f %f]fps\n",
			               dimensions.width, dimensions.height,
			               min_framerate, max_framerate);
		}
	}
	return -1;
}

static void* avf_capture_init(uint32_t screenid) {
	AVFContext *ctx         	  = nil;
	AVCaptureDevice *video_device = nil;
	uint32_t num_screens = 0;
	int capture_screen = 0;

	TSK_DEBUG_INFO("@@ avf_capture_init start");

	// get screen num, normally more than 0
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070	
	CGGetActiveDisplayList(0, NULL, &num_screens);
#endif

	if (screenid >= num_screens ) {
		screenid = 0;	// get screen 0 when input screenid is invalid
	}
	
	ctx = (AVFContext *)malloc(sizeof(AVFContext));
	ctx->pthread_exit_flag = 1;
	ctx->num_video_devices = num_screens;
    ctx->frames_captured = 0;

    ctx->capture_session = NULL;
	ctx->video_output    = NULL;
	ctx->avf_delegate    = NULL;
    ctx->current_frame   = NULL;
	ctx->frame_rate 	 = 30;	// default frame rate is 30

    NSRect screenRect;
    NSArray *screenArray = [NSScreen screens];
    unsigned screenCount = [screenArray count];
    unsigned index = 0;
    float screenScaleFactor = 1.0f;
    for (index; index < screenCount; index++)
    {
        if(index == screenid){
            NSScreen *screen = [screenArray objectAtIndex: index];
            screenRect = [screen visibleFrame];
            screenScaleFactor = [screen backingScaleFactor];
            break;
        }
    }
    //return [NSString stringWithFormat:@"%.1fx%.1f",screenRect.size.width, screenRect.size.height];

    pthread_mutex_init(&ctx->frame_lock, NULL);
    

	if (num_screens > 0) {
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
		CGDirectDisplayID screens[num_screens];
		CGGetActiveDisplayList(num_screens, screens, &num_screens);
		AVCaptureScreenInput* capture_screen_input = [[AVCaptureScreenInput alloc] initWithDisplayID:screens[screenid]];

		if (ctx->frame_rate > 0) {
			capture_screen_input.minFrameDuration = CMTimeMake(ctx->frame_rate, 1000);
		}
        //scale
        float wScale = 1920.0f / screenRect.size.width / screenScaleFactor;
        float hScale = 1080.0f / screenRect.size.height/ screenScaleFactor;
        
        capture_screen_input.scaleFactor = wScale > hScale ? wScale : hScale;
        capture_screen_input.scaleFactor = capture_screen_input.scaleFactor > 1.0f ? 1.0f : capture_screen_input.scaleFactor;
        TSK_DEBUG_INFO("scale: %f",capture_screen_input.scaleFactor);
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
		capture_screen_input.capturesCursor = YES;
#endif
		capture_screen_input.capturesMouseClicks = YES;

		video_device = (AVCaptureDevice*) capture_screen_input;
		capture_screen = 1;
#endif
	}

	AVCaptureInput* capture_input = (AVCaptureInput*) video_device;

	// Initialize capture session
	ctx->capture_session = [[AVCaptureSession alloc] init];

	// add_video_device
	if ([ctx->capture_session canAddInput:capture_input]) {
		[ctx->capture_session addInput:capture_input];
	} else {
		TSK_DEBUG_ERROR("can't add video input to capture session\n");
		return nullptr;
	}

	// Attaching output
	ctx->video_output = [[AVCaptureVideoDataOutput alloc] init];

	if (!ctx->video_output) {
		TSK_DEBUG_ERROR("Failed to init AV video output\n");
		return nullptr;
	}

	// Configure device framerate and video size
	@try {
		if (configure_video_device(ctx, video_device) < 0) {
			return nullptr;
		}
	} @catch (NSException *exception) {
		if (![[exception name] isEqualToString:NSUndefinedKeyException]) {
			TSK_DEBUG_ERROR("An error occurred: %s", [exception.reason UTF8String]);
			return nullptr;
		}
	}

	// select pixel format
	// default is AV_PIX_FMT_BGR0, swicth to other format if not support 32BGRA
	struct AVFPixelFormatSpec pxl_fmt_spec;
	pxl_fmt_spec.ff_id = AV_PIX_FMT_BGR0;
	ctx->pixel_format  = AV_PIX_FMT_BGR0;

	for (int i = 0; avf_pixel_formats[i].ff_id != AV_PIX_FMT_NONE; i++) {
		if (ctx->pixel_format == avf_pixel_formats[i].ff_id) {
			pxl_fmt_spec = avf_pixel_formats[i];
			break;
		}
	}
	TSK_DEBUG_INFO("Selected pixel format (%d) \n", pxl_fmt_spec.ff_id);

	// check if selected pixel format is supported by AVFoundation
	if (pxl_fmt_spec.ff_id == AV_PIX_FMT_NONE) {
		TSK_DEBUG_ERROR("Selected pixel format (%d) is not supported by AVFoundation.\n", pxl_fmt_spec.ff_id);
		return nullptr;
	}

	// check if the pixel format is available for this device
	if ([[ctx->video_output availableVideoCVPixelFormatTypes] indexOfObject:[NSNumber numberWithInt:pxl_fmt_spec.avf_id]] == NSNotFound) {
		TSK_DEBUG_WARN("Selected pixel format (%d) is not supported by the input device.\n", pxl_fmt_spec.ff_id);

		pxl_fmt_spec.ff_id = AV_PIX_FMT_NONE;

		TSK_DEBUG_INFO("Supported pixel formats:\n");
		for (NSNumber * pxl_fmt in [ctx->video_output availableVideoCVPixelFormatTypes]) {
			struct AVFPixelFormatSpec pxl_fmt_dummy;
			pxl_fmt_dummy.ff_id = AV_PIX_FMT_NONE;
			for (int i = 0; avf_pixel_formats[i].ff_id != AV_PIX_FMT_NONE; i++) {
				if ([pxl_fmt intValue] == avf_pixel_formats[i].avf_id) {
					pxl_fmt_dummy = avf_pixel_formats[i];
					break;
				}
			}

			if (pxl_fmt_dummy.ff_id != AV_PIX_FMT_NONE) {
				TSK_DEBUG_INFO("pix_fmt:%d, avf_id:0x%x\n", pxl_fmt_dummy.ff_id, pxl_fmt_dummy.avf_id);

				// select first supported pixel format instead of user selected (or default) pixel format
				if (pxl_fmt_spec.ff_id == AV_PIX_FMT_NONE) {
					pxl_fmt_spec = pxl_fmt_dummy;
				}
			}
		}

		// fail if there is no appropriate pixel format or print a warning about overriding the pixel format
		if (pxl_fmt_spec.ff_id == AV_PIX_FMT_NONE) {
			return nullptr;
		} else {
			TSK_DEBUG_INFO("Overriding selected pixel format to use %d instead.\n", pxl_fmt_spec.ff_id);
		}
	}

	NSNumber *pixel_format;
	NSDictionary *capture_dict;
	dispatch_queue_t queue;

	ctx->pixel_format          = pxl_fmt_spec.ff_id;
	pixel_format = [NSNumber numberWithUnsignedInt:pxl_fmt_spec.avf_id];
	capture_dict = [NSDictionary dictionaryWithObject:pixel_format
	                						   forKey:(id)kCVPixelBufferPixelFormatTypeKey];

	[ctx->video_output setVideoSettings:capture_dict];
	[ctx->video_output setAlwaysDiscardsLateVideoFrames:YES];

	ctx->avf_delegate = [[AVFFrameReceiver alloc] initWithContext:ctx];

	queue = dispatch_queue_create("avf_queue", NULL);
	[ctx->video_output setSampleBufferDelegate:ctx->avf_delegate queue:queue];
	dispatch_release(queue);

	if ([ctx->capture_session canAddOutput:ctx->video_output]) {
		[ctx->capture_session addOutput:ctx->video_output];

		#if 0 // use for ios, no use for macos
		AVCaptureConnection *connection = [ctx->video_output connectionWithMediaType:AVMediaTypeVideo];
		// 设置视频的方向
        connection.videoOrientation = AVCaptureVideoOrientationPortraitUpsideDown;

		// 视频稳定设置
		if ([connection isVideoStabilizationSupported]) {
		   connection.preferredVideoStabilizationMode = AVCaptureVideoStabilizationModeAuto;
		}
		connection.videoScaleAndCropFactor = connection.videoMaxScaleAndCropFactor;
        #endif
	} else {
		TSK_DEBUG_ERROR("can't add video output to capture session\n");
		return nullptr;
	}

	[ctx->capture_session startRunning];

	/* Unlock device configuration only after the session is started so it
	 * does not reset the capture formats */
	if (!capture_screen) {
		[video_device unlockForConfiguration];
	}

	// get video config
	if (video_device) {
		// Take stream info from the first frame.
		while (ctx->frames_captured < 1) {
			CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, YES);
		}

        lock_frames(ctx);
		CVImageBufferRef image_buffer = CMSampleBufferGetImageBuffer(ctx->current_frame);
		CGSize image_buffer_size = CVImageBufferGetEncodedSize(image_buffer);
		ctx->width      = (int)image_buffer_size.width;
		ctx->height     = (int)image_buffer_size.height;

		if (ctx->current_frame != nil) {
			CFRelease(ctx->current_frame);
			ctx->current_frame = nil;
		}
        unlock_frames(ctx);
	}

	return ctx;
}
#endif
static int avf_format_transfer_I420(char *data, int *size, int width, int height, int fmt) {
	//int out_size = width * height * 3 / 2;
	//int out_data =  malloc(out_size);
	if (!data) {
		TSK_DEBUG_ERROR("avf_format_transfer_i420 input buffer invalid\n");
		return -1;
	}
    
	switch (fmt) {
		case AV_PIX_FMT_NV12:
		{
		    int yLen = width * height;
		    int vLen = yLen / 4;

		    uint8_t *udata = (uint8_t*)data + yLen;
			uint8_t *vData = (uint8_t *)malloc(yLen / 4);
		    
		    for (int index = 0; index < vLen; ++index) {
		        *(udata + index) = *(udata + index * 2);
		        *(vData + index) = *(udata + index * 2 + 1);
		    }

		    memcpy(udata + vLen, vData, vLen);
		    *size = width * height * 3 / 2;
		    free(vData);
			break;
		}

		case AV_PIX_FMT_YUYV422:
		{
			int out_size = width * height * 3 / 2;
			uint8_t * out_data = (uint8_t*)malloc(out_size);

			int ylen = width * height;
			int ulen = ylen / 4;

			uint8_t *y_data = out_data;
			uint8_t *u_data = (uint8_t*)y_data + ylen;
			uint8_t *v_data = (uint8_t*)u_data + ulen;

			for (int index = 0; index < (width * height * 2 / 4); ++index) {
				*(y_data + 2 * index) = *(data + 4*index);
				*(y_data + 2 * index + 1) = *(data + 4*index + 2);
				if (0 == index%2) {
					*(u_data + index/2) = *(data + 4*index + 1);
					*(v_data + index/2) = *(data + 4*index + 3);				
				}
			}

			memcpy(data, out_data, out_size);
			*size = out_size;

			free(out_data);
			break;
		}
		case AV_PIX_FMT_UYVY422 :
		{
			int out_size = width * height * 3 / 2;
			uint8_t * out_data = (uint8_t*)malloc(out_size);

			int ylen = width * height;
			int ulen = ylen / 4;

			uint8_t *y_data = out_data;
			uint8_t *u_data = (uint8_t*)y_data + ylen;
			uint8_t *v_data = (uint8_t*)u_data + ulen;

			for (int index = 0; index < (width * height * 2 / 4); ++index) {
				*(y_data + 2*index) 	= *(data + 4*index + 1);
				*(y_data + 2*index + 1) = *(data + 4*index + 3);
				if (0 == index%2) {
					*(u_data + index/2) = *(data + 4*index);
					*(v_data + index/2) = *(data + 4*index + 2);				
				}
			}

			memcpy(data, out_data, out_size);
			// memset(data + out_size, 0, width * height / 2);
			*size = out_size;
			if (out_data) {
				free(out_data);
				out_data = nil;
			}
				
			break;
		}
		case AV_PIX_FMT_YUV420P:
		{
			// do nothing
			break;
		}
	}
    return 0;
}
#if 0
static int avf_get_frame(AVFContext* ctx) {
	do {
		CVImageBufferRef image_buffer;
		uint64_t   ts = 0;
        
		if (ctx->current_frame != nil) {
			CMItemCount count;
			CMSampleTimingInfo timing_info;

			image_buffer = CMSampleBufferGetImageBuffer(ctx->current_frame);
			CVPixelBufferLockBaseAddress(image_buffer, kCVPixelBufferLock_ReadOnly);
			CMFormatDescriptionRef formatDesc = CMSampleBufferGetFormatDescription(ctx->current_frame);
    		CMMediaType mediaType = CMFormatDescriptionGetMediaType(formatDesc);
			if (mediaType != kCMMediaType_Video) {
				TSK_DEBUG_ERROR("avf_get_frame not video data, pass!\n");
			    break;
			}

			int width = (int)CVPixelBufferGetWidth(image_buffer);
            int height = (int)CVPixelBufferGetHeight(image_buffer);

			if(ctx->videoCb) ctx->videoCb((char*)image_buffer, 0, width, height, ts);

			CVPixelBufferUnlockBaseAddress(image_buffer, kCVPixelBufferLock_ReadOnly);

		}
	} while (0);
    
	// free memory
	if (ctx->current_frame != nil) {
		CFRelease(ctx->current_frame);
		ctx->current_frame = nil;		
	}
	
	return 0;
}


static int StartScreenStream(void * data) {
	if (!data) {
		return -1;
	}

	AVFContext *ctx = (AVFContext *)data;

	ctx->pthread_exit_flag = 0;

	return 0;
}

static int StopScreenStream(void * data) {
	if (!data) {
		return -1;
	}

	AVFContext *ctx = (AVFContext *)data;

	[ctx->capture_session stopRunning];

	ctx->pthread_exit_flag = 1;

	return 0;
}

static int SetScreenCaptureFramerate(void *data, int fps) {
	// sanity check
	if (!data) {
		return -1;
	}

	// fps: up to 60 
	// default frame rate is 15
	if (fps < 1 || fps > 60) {
		fps = 15;
	}

	AVFContext *ctx = (AVFContext *)data;
	ctx->frame_rate = fps;

	return 0;
}

static void SetScreenCaptureVideoCallback(void *data, ScreenCb cb) {
	if (!data) {
		return;
	}

	AVFContext *ctx = (AVFContext *)data;
	ctx->videoCb = cb;
}

static void DestroyScreenMedia(void *data) {
	AVFContext *ctx = (AVFContext *)data;

	[ctx->capture_session stopRunning];

   	[ctx->capture_session release];
   	[ctx->video_output    release];
    [ctx->avf_delegate  releaseContext];
   	[ctx->avf_delegate    release];

	ctx->capture_session = NULL;
	ctx->video_output    = NULL;
	ctx->avf_delegate    = NULL;
    ctx->videoCb         = NULL;


   pthread_mutex_destroy(&ctx->frame_lock);
//    pthread_cond_destroy(&ctx->frame_wait_cond);

	if (ctx->current_frame != nil) {
		CFRelease(ctx->current_frame);
		ctx->current_frame = nil;
	}

	if (ctx) {
		free(ctx);
		ctx = NULL;
	}
}

#endif

typedef struct {
	int mode;
	void * handle;
} av_capture;

void* InitMedia(int mode, void * setting) {
	av_capture * av_capture_h = nullptr;
	do {
		if (mode != VIDEO_SHARE_TYPE_SCREEN && mode != VIDEO_SHARE_TYPE_WINDOW) {
			TSK_DEBUG_ERROR("capture mode is invalid:%d", mode);
			break;
		}

		// window capture must set windows setting property
		if (VIDEO_SHARE_TYPE_WINDOW == mode && !setting) {
			TSK_DEBUG_ERROR("window setting pointer is null");
			break;
		}

		av_capture_h = (av_capture *)malloc(sizeof(av_capture));
		if (VIDEO_SHARE_TYPE_SCREEN == mode) {
			int screenid = ((obs_data_t *)setting)->window_id;
//			av_capture_h->handle = avf_capture_init(screenid);
            av_capture_h->handle = InitScreenMedia(screenid);
		} else {
			av_capture_h->handle = InitWindowMedia((obs_data_t *)setting);
		}

		av_capture_h->mode = mode;

		if (av_capture_h->handle) {
			break;
		} else {
			free(av_capture_h);
		}
    } while(0);

	return av_capture_h;
}

void SetShareContext(void* context )
{
    setWindowShareContext( context );
}

int StartStream(void * data) {
	if (!data) {
		TSK_DEBUG_ERROR("StartStream input invalid");
		return -1;
	}

	av_capture * av_capture_h = (av_capture *)data;
	if (VIDEO_SHARE_TYPE_SCREEN == av_capture_h->mode) {
		StartScreenStream(av_capture_h->handle);
	} else {
		StartWindowStream(av_capture_h->handle);
	}

	return 0;
}

int StopStream(void * data) {
	if (!data) {
		TSK_DEBUG_ERROR("StopStream input invalid");
		return -1;
	}

	av_capture * av_capture_h = (av_capture *)data;
	if (VIDEO_SHARE_TYPE_SCREEN == av_capture_h->mode) {
		StopScreenStream(av_capture_h->handle);
	} else {
		StopWindowStream(av_capture_h->handle);
	}

	return 0;
}

void DestroyMedia(void *data) {
	if (!data) {
		TSK_DEBUG_ERROR("DestroyMedia input invalid");
		return;
	}
    {
        std::lock_guard<std::mutex> lock(*youme_osxscreen_capture_mutex);
        av_capture * av_capture_h = (av_capture *)data;
        if (VIDEO_SHARE_TYPE_SCREEN == av_capture_h->mode) {
            DestroyScreenMedia(av_capture_h->handle);
        } else {
            DestroyWindowMedia(av_capture_h->handle);
        }

        free(av_capture_h);
    }
}

int SetCaptureFramerate(void *data, int fps) {
	if (!data) {
		TSK_DEBUG_ERROR("SetCaptureFramerate input invalid");
		return -1;
	}

	av_capture * av_capture_h = (av_capture *)data;
	if (VIDEO_SHARE_TYPE_SCREEN == av_capture_h->mode) {
		SetScreenCaptureFramerate(av_capture_h->handle, fps);
	} else {
		SetWindowCaptureFramerate(av_capture_h->handle, fps);
	}
	return 0;
}

int SetExclusiveWnd( void * data, int wnd )
{
    if (!data) {
        TSK_DEBUG_ERROR("SetExclusiveWnd input invalid");
        return -1;
    }
    av_capture * av_capture_h = (av_capture *)data;
    if (VIDEO_SHARE_TYPE_SCREEN == av_capture_h->mode) {
        SetScreenExclusiveWnd(av_capture_h->handle, wnd);
    }
    
    return 0;
}

void SetCaptureVideoCallback(void *data, ScreenCb cb) {
	if (!data) {
		TSK_DEBUG_ERROR("SetCaptureVideoCallback input invalid");
		return;
	}

	av_capture * av_capture_h = (av_capture *)data;
	if (VIDEO_SHARE_TYPE_SCREEN == av_capture_h->mode) {
		SetScreenCaptureVideoCallback(av_capture_h->handle, cb);
	} else {
		SetWindowCaptureVideoCallback(av_capture_h->handle, cb);
	}
}

void GetWindowList(int mode, obs_data_t *info, int *count) {
	if (VIDEO_SHARE_TYPE_SCREEN == mode) {
		uint32_t num_screens = 0;
		int capture_screen = 0;

		TSK_DEBUG_INFO("@@ avf_capture_init start");

		// get screen num, normally more than 0
		CGGetActiveDisplayList(0, NULL, &num_screens);

		*count= num_screens;
	} else {
		// enumerate all windows
		int win_count = 0;
		EnumerateWindowInfo(info, &win_count);
        *count = win_count;
	}
}

