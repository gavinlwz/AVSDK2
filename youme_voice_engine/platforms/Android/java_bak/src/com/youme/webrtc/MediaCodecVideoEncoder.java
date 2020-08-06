/*
 *  Copyright 2013 The WebRTC@AnyRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package com.youme.webrtc;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.graphics.SurfaceTexture;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaCodecInfo.CodecCapabilities;
import android.media.MediaCodecInfo.CodecProfileLevel;
import android.media.MediaCodecList;
import android.media.MediaFormat;
import android.opengl.EGL14;
import android.opengl.EGLContext;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.util.Range;
import android.view.Surface;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import com.youme.mixers.GLESVideoMixer;
import com.youme.voiceengine.NativeEngine;
import com.youme.webrtc.EglBase14.Context;

// Java-side of peerconnection_jni.cc:MediaCodecVideoEncoder.
@SuppressLint("NewApi") // This class is an implementation detail of the Java PeerConnection API.
@TargetApi(19)
@SuppressWarnings("deprecation")
public class MediaCodecVideoEncoder {
  // This class is constructed, operated, and destroyed by its C++ incarnation,
  // so the class and its methods have non-public visibility.  The API this
  // class exposes aims to mimic the webrtc::VideoEncoder API as closely as
  // possibly to minimize the amount of translation work necessary.

  private static final String TAG = "MediaCodecVideoEncoder";

  // Tracks webrtc::VideoCodecType.
  public enum VideoCodecType {
    VIDEO_CODEC_VP8,
    VIDEO_CODEC_VP9,
    VIDEO_CODEC_H264
  }

  private static final float scaleBitrate = 1.0f;
  private static final int MEDIA_CODEC_RELEASE_TIMEOUT_MS = 5000; // Timeout for codec releasing.
  private static final int DEQUEUE_TIMEOUT = 0;  // Non-blocking, no wait.
  private static final int BITRATE_ADJUSTMENT_FPS = 30;
  private static final String AVC_MIME = "video/avc";
  private static boolean isVBR=false;
  private static float abr_avg_bitrate_percent = 0.85f;
  private static float abr_max_bitrate_percent = 0.85f;

  // Active running encoder instance. Set in initEncode() (called from native code)
  // and reset to null in release() call.
  private static MediaCodecVideoEncoder runningInstance = null;
  private static MediaCodecVideoEncoderErrorCallback errorCallback = null;
  private static int codecErrors = 0;
  // List of disabled codec types - can be set from application.
  private static Set<String> hwEncoderDisabledTypes = new HashSet<String>();

  private Thread mediaCodecThread;
  private MediaCodec mediaCodec;
  private ByteBuffer[] outputBuffers;
  private EglBase14 eglBase;
  private int width;
  private int height;
  private Surface inputSurface;
  private GlRectDrawer drawer;
  
  private static Size supportedMinSize = new Size(128,64);
  private static Size supportedMaxSize = new Size(1920,1080);

  private static final String VP8_MIME_TYPE = "video/x-vnd.on2.vp8";
  private static final String VP9_MIME_TYPE = "video/x-vnd.on2.vp9";
  private static final String H264_MIME_TYPE = "video/avc";

  // Class describing supported media codec properties.
  private static class MediaCodecProperties {
    public final String codecPrefix;
    // Minimum Android SDK required for this codec to be used.
    public final int minSdk;
    // Flag if encoder implementation does not use frame timestamps to calculate frame bitrate
    // budget and instead is relying on initial fps configuration assuming that all frames are
    // coming at fixed initial frame rate. Bitrate adjustment is required for this case.
    public final boolean bitrateAdjustmentRequired;

    MediaCodecProperties(
        String codecPrefix, int minSdk, boolean bitrateAdjustmentRequired) {
      this.codecPrefix = codecPrefix;
      this.minSdk = minSdk;
      this.bitrateAdjustmentRequired = bitrateAdjustmentRequired;
    }
  }

  // List of supported HW VP8 encoders.
  private static final MediaCodecProperties qcomVp8HwProperties = new MediaCodecProperties(
      "OMX.qcom.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties exynosVp8HwProperties = new MediaCodecProperties(
      "OMX.Exynos.", Build.VERSION_CODES.M, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties intelVp8HwProperties = new MediaCodecProperties(
      "OMX.Intel.", Build.VERSION_CODES.LOLLIPOP, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties[] vp8HwList = new MediaCodecProperties[] {
    qcomVp8HwProperties, exynosVp8HwProperties, intelVp8HwProperties
  };

  // List of supported HW VP9 encoders.
  private static final MediaCodecProperties qcomVp9HwProperties = new MediaCodecProperties(
      "OMX.qcom.", Build.VERSION_CODES.M, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties exynosVp9HwProperties = new MediaCodecProperties(
      "OMX.Exynos.", Build.VERSION_CODES.M, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties[] vp9HwList = new MediaCodecProperties[] {
    qcomVp9HwProperties, exynosVp9HwProperties
  };

  // List of supported HW H.264 encoders.
  private static final MediaCodecProperties qcomH264HwProperties = new MediaCodecProperties(
      "OMX.qcom.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties exynosH264HwProperties = new MediaCodecProperties(
      "OMX.Exynos.", Build.VERSION_CODES.LOLLIPOP, true /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties rkH264HwProperties = new MediaCodecProperties(
      "OMX.rk.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties k3H264HwProperties = new MediaCodecProperties(
      "OMX.k3.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties mtkH264HwProperties = new MediaCodecProperties(
      "OMX.MTK.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties hiH264HwProperties = new MediaCodecProperties(
	  "OMX.hi.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties intelH264HwProperties = new MediaCodecProperties(
		  "OMX.Intel.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties nvidiaH264HwProperties = new MediaCodecProperties(
		  "OMX.Nvidia.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties ittiamH264HwProperties = new MediaCodecProperties(
		  "OMX.ittiam.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties secH264HwProperties = new MediaCodecProperties(
		  "OMX.SEC.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties imgH264HwProperties = new MediaCodecProperties(
		  "OMX.IMG.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties hisiH264HwProperties = new MediaCodecProperties(
		  "OMX.hisi.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties tiH264HwProperties = new MediaCodecProperties(
		  "OMX.TI.DUCATI1.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties lgH264HwProperties = new MediaCodecProperties(
      "OMX.LG.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  // 全志
  private static final MediaCodecProperties qzH264HwProperties = new MediaCodecProperties(
    "OMX.allwinner.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);

  private static final MediaCodecProperties amlogicH264HwProperties = new MediaCodecProperties(
    "OMX.amlogic.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties marvelH264HwProperties = new MediaCodecProperties(
    "OMX.MARVELL.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);

  // need test
  private static final MediaCodecProperties actionH264HwProperties = new MediaCodecProperties(
    "OMX.Action.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties brcmH264HwProperties = new MediaCodecProperties(
    "OMX.BRCM.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);    
  private static final MediaCodecProperties cosmoH264HwProperties = new MediaCodecProperties(
    "OMX.cosmo.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties duosH264HwProperties = new MediaCodecProperties(
    "OMX.duos.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);    
  private static final MediaCodecProperties hantroH264HwProperties = new MediaCodecProperties(
    "OMX.hantro.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties msH264HwProperties = new MediaCodecProperties(
    "OMX.MS.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);    
  private static final MediaCodecProperties renesasH264HwProperties = new MediaCodecProperties(
    "OMX.RENESAS.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties rtkH264HwProperties = new MediaCodecProperties(
    "OMX.rtk.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);    
  private static final MediaCodecProperties sprdH264HwProperties = new MediaCodecProperties(
    "OMX.sprd.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties stH264HwProperties = new MediaCodecProperties(
    "OMX.ST.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);    
  private static final MediaCodecProperties vpuH264HwProperties = new MediaCodecProperties(
    "OMX.vpu.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);
  private static final MediaCodecProperties wmtH264HwProperties = new MediaCodecProperties(
    "OMX.WMT.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);    
  private static final MediaCodecProperties bluestacksH264HwProperties = new MediaCodecProperties(
    "OMX.bluestacks.", Build.VERSION_CODES.KITKAT, false /* bitrateAdjustmentRequired */);    
      
  private static final MediaCodecProperties[] h264HwList = new MediaCodecProperties[] {
    qcomH264HwProperties, exynosH264HwProperties, rkH264HwProperties, k3H264HwProperties, mtkH264HwProperties,hiH264HwProperties,
    nvidiaH264HwProperties, ittiamH264HwProperties, secH264HwProperties, imgH264HwProperties, hisiH264HwProperties, tiH264HwProperties,
    lgH264HwProperties, intelH264HwProperties, qzH264HwProperties, actionH264HwProperties, brcmH264HwProperties, cosmoH264HwProperties,
    duosH264HwProperties, hantroH264HwProperties, msH264HwProperties, renesasH264HwProperties, rtkH264HwProperties, sprdH264HwProperties,
    stH264HwProperties, vpuH264HwProperties, wmtH264HwProperties, bluestacksH264HwProperties
  };
//  private static final MediaCodecProperties[] h264HwList = new MediaCodecProperties[] {
//     qcomH264HwProperties
//  };
  
  private static final String [] bitrateAdjustmentRequiredDevice = new String[]{
	  "vivo X5Pro D"
  };
  

  // List of devices with poor H.264 encoder quality.
  private static final String[] H264_HW_EXCEPTION_MODELS = new String[] {
    // HW H.264 encoder on below devices has poor bitrate control - actual
    // bitrates deviates a lot from the target value.
    "SAMSUNG-SGH-I337",
    "Nexus 7",
    "Nexus 4"
  };

  // Bitrate modes - should be in sync with OMX_VIDEO_CONTROLRATETYPE defined
  // in OMX_Video.h
  private static final int VIDEO_ControlRateConstant = 2;
  // NV12 color format supported by QCOM codec, but not declared in MediaCodec -
  // see /hardware/qcom/media/mm-core/inc/OMX_QCOMExtns.h
  private static final int
    COLOR_QCOM_FORMATYUV420PackedSemiPlanar32m = 0x7FA30C04;
  // Allowable color formats supported by codec - in order of preference.
  private static final int[] supportedColorList = {
    CodecCapabilities.COLOR_FormatYUV420Planar,
    CodecCapabilities.COLOR_FormatYUV420SemiPlanar,
    CodecCapabilities.COLOR_QCOM_FormatYUV420SemiPlanar,
    COLOR_QCOM_FORMATYUV420PackedSemiPlanar32m
  };
  private static final int[] supportedSurfaceColorList = {
    CodecCapabilities.COLOR_FormatSurface
  };
  private VideoCodecType type;
  private int colorFormat;  // Used by native code.
  private boolean bitrateAdjustmentRequired;
  private boolean configureH264HighProfile;

  // SPS and PPS NALs (Config frame) for H.264.
  private ByteBuffer configData = null;

  // MediaCodec error handler - invoked when critical error happens which may prevent
  // further use of media codec API. Now it means that one of media codec instances
  // is hanging and can no longer be used in the next call.
  public static interface MediaCodecVideoEncoderErrorCallback {
    void onMediaCodecVideoEncoderCriticalError(int codecErrors);
  }

  public static void setErrorCallback(MediaCodecVideoEncoderErrorCallback errorCallback) {
    Logging.d(TAG, "Set error callback");
    MediaCodecVideoEncoder.errorCallback = errorCallback;
  }

  // Functions to disable HW encoding - can be called from applications for platforms
  // which have known HW decoding problems.
  public static void disableVp8HwCodec() {
    Logging.w(TAG, "VP8 encoding is disabled by application.");
    hwEncoderDisabledTypes.add(VP8_MIME_TYPE);
  }

  public static void disableVp9HwCodec() {
    Logging.w(TAG, "VP9 encoding is disabled by application.");
    hwEncoderDisabledTypes.add(VP9_MIME_TYPE);
  }

  public static void disableH264HwCodec() {
    Logging.w(TAG, "H.264 encoding is disabled by application.");
    hwEncoderDisabledTypes.add(H264_MIME_TYPE);
  }

  // Functions to query if HW encoding is supported.
  public static boolean isVp8HwSupported() {
    return !hwEncoderDisabledTypes.contains(VP8_MIME_TYPE) &&
        (findHwEncoder(VP8_MIME_TYPE, vp8HwList, supportedColorList) != null);
  }

  public static boolean isVp9HwSupported() {
    return !hwEncoderDisabledTypes.contains(VP9_MIME_TYPE) &&
        (findHwEncoder(VP9_MIME_TYPE, vp9HwList, supportedColorList) != null);
  }

  public static boolean isH264HwSupported() {
    return !hwEncoderDisabledTypes.contains(H264_MIME_TYPE) &&
        (findHwEncoder(H264_MIME_TYPE, h264HwList, supportedColorList) != null);
  }

  public static boolean isVp8HwSupportedUsingTextures() {
    return !hwEncoderDisabledTypes.contains(VP8_MIME_TYPE) &&
        (findHwEncoder(VP8_MIME_TYPE, vp8HwList, supportedSurfaceColorList) != null);
  }

  public static boolean isVp9HwSupportedUsingTextures() {
    return !hwEncoderDisabledTypes.contains(VP9_MIME_TYPE) &&
        (findHwEncoder(VP9_MIME_TYPE, vp9HwList, supportedSurfaceColorList) != null);
  }

  public static boolean isH264HwSupportedUsingTextures() {
    return !hwEncoderDisabledTypes.contains(H264_MIME_TYPE) &&
        (findHwEncoder(H264_MIME_TYPE, h264HwList, supportedSurfaceColorList) != null);
  }

  // Helper struct for findHwEncoder() below.
  private static class EncoderProperties {
    public EncoderProperties(String codecName, int colorFormat, boolean bitrateAdjustment, boolean configureH264HighProfile) {
      this.codecName = codecName;
      this.colorFormat = colorFormat;
      this.bitrateAdjustment = bitrateAdjustment;
      this.configureH264HighProfile = configureH264HighProfile;
    }
    public final String codecName; // OpenMax component name for HW codec.
    public final int colorFormat;  // Color format supported by codec.
    public final boolean bitrateAdjustment; // true if bitrate adjustment workaround is required.
    public final boolean configureH264HighProfile;
  }

  @SuppressLint("NewApi") private static EncoderProperties findHwEncoder(
      String mime, MediaCodecProperties[] supportedHwCodecProperties, int[] colorList) {
    // MediaCodec.setParameters is missing for JB and below, so bitrate
    // can not be adjusted dynamically.
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT) {
      return null;
    }
    try {
    // Check if device is in H.264 exception list.
    if (mime.equals(H264_MIME_TYPE)) {
      List<String> exceptionModels = Arrays.asList(H264_HW_EXCEPTION_MODELS);
      if (exceptionModels.contains(Build.MODEL)) {
        Logging.w(TAG, "Model: " + Build.MODEL + " has black listed H.264 encoder.");
        return null;
      }
    }

    for (int i = 0; i < MediaCodecList.getCodecCount(); ++i) {
      MediaCodecInfo info = MediaCodecList.getCodecInfoAt(i);
      if (!info.isEncoder()) {
        continue;
      }
      String name = null;
      for (String mimeType : info.getSupportedTypes()) {
    	  Log.d(TAG, "mediacodec encode getSupportedTypes:" + mimeType);
        if (mimeType.equals(mime)) {
          name = info.getName();
          break;
        }
      }
      if (name == null) {
        continue;  // No HW support in this codec; try the next one.
      }
      Log.d(TAG, "Found candidate encoder " + name);

      // Check if this is supported HW encoder.
      boolean supportedCodec = false;
      boolean bitrateAdjustmentRequired = false;
      for (MediaCodecProperties codecProperties : supportedHwCodecProperties) {
        if (name.startsWith(codecProperties.codecPrefix)) {
          if (Build.VERSION.SDK_INT < codecProperties.minSdk) {
            Log.d(TAG, "Codec " + name + " is disabled due to SDK version " +
                Build.VERSION.SDK_INT);
            supportedCodec = false;
            continue;
          }
          if (codecProperties.bitrateAdjustmentRequired) {
            Log.d(TAG, "Codec " + name + " does not use frame timestamps.");
            bitrateAdjustmentRequired = true;
          }
          supportedCodec = true;
          break;
        }
      }
      if (!supportedCodec) {
        continue;
      }
     
      // Check if HW codec supports known color format.
      CodecCapabilities capabilities = info.getCapabilitiesForType(mime);
      for (int colorFormat : capabilities.colorFormats) {
        Log.d(TAG, "   Color: 0x" + Integer.toHexString(colorFormat));
      }
    
      int colorFormat = 0;
      boolean adjustmentRequire = false;
      boolean _configureH264HighProfile = false;
      for (int supportedColorFormat : colorList) {
        for (int codecColorFormat : capabilities.colorFormats) {
          if (codecColorFormat == supportedColorFormat) {
            // Found supported HW encoder.
            Log.d(TAG, "Found target encoder for mime " + mime + " : " + name +
                ". Color: 0x" + Integer.toHexString(codecColorFormat));
            colorFormat = codecColorFormat;
            adjustmentRequire = bitrateAdjustmentRequired;
            break;
           // return new EncoderProperties(name, codecColorFormat, bitrateAdjustmentRequired);
          }
        }
        if(colorFormat !=0)
        	break;
      }
      
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
      {
         MediaCodecInfo.VideoCapabilities videoCaps = capabilities.getVideoCapabilities();
         if (videoCaps != null) 
    	  {
        	    Range<Integer> widthRange = videoCaps.getSupportedWidths();
                Range<Integer> heightRange = videoCaps.getSupportedHeights();
                supportedMinSize.width = widthRange.getLower();
                supportedMinSize.height = heightRange.getLower();
                supportedMaxSize.width = widthRange.getUpper();
                supportedMaxSize.height = heightRange.getUpper();
                Log.d(TAG, "mediaencode supportedMinSize w:"+ supportedMinSize.width + " h:"+supportedMinSize.height + 
                		" supportedMaxSize w:" + supportedMaxSize.width + " h:" +supportedMaxSize.height);
      
    	  }
       }
      
      //判断机型码率控制
      if(adjustmentRequire == false)
      {
    	  String phoneName = android.os.Build.MODEL;
          for (String mode : bitrateAdjustmentRequiredDevice) {
             if(phoneName.equals(mode) == true)
             {
            	 adjustmentRequire = true;
            	 break;
             }
          }
      }
      
      if(_configureH264HighProfile == false)
      {
    	  if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
                  && testAvcProfileLevel(MediaCodecInfo.CodecProfileLevel.AVCProfileHigh,
                                         CodecProfileLevel.AVCLevel42) )
    		  _configureH264HighProfile = true;
      }

      if(colorFormat != 0)
      {
    	  String msg = "Found target encoder for mime " + mime + " : " + name +
                  ". Color: 0x" + Integer.toHexString(colorFormat) + " bitrate adjustment require:"+adjustmentRequire;
    	  Log.d(TAG, msg);
    	  NativeEngine.logcat(NativeEngine.LOG_INFO, TAG, msg);
    	  return new EncoderProperties(name, colorFormat, adjustmentRequire, _configureH264HighProfile);
      }
     
    }
    }catch (Exception e) {
        Log.d(TAG, "Media encoder found failed", e);
      }
    NativeEngine.logcat(NativeEngine.LOG_INFO, TAG, "No HW encoder found for mime " + mime);
    return null;  // No HW encoder.
  }

  private void checkOnMediaCodecThread() {
    if (mediaCodecThread.getId() != Thread.currentThread().getId()) {
      throw new RuntimeException(
          "MediaCodecVideoEncoder previously operated on " + mediaCodecThread +
          " but is now called on " + Thread.currentThread());
    }
  }

  public static void printStackTrace() {
    if (runningInstance != null && runningInstance.mediaCodecThread != null) {
      StackTraceElement[] mediaCodecStackTraces = runningInstance.mediaCodecThread.getStackTrace();
      if (mediaCodecStackTraces.length > 0) {
        Log.d(TAG, "MediaCodecVideoEncoder stacks trace:");
        for (StackTraceElement stackTrace : mediaCodecStackTraces) {
          Log.d(TAG, stackTrace.toString());
        }
      }
    }
  }

  static MediaCodec createByCodecName(String codecName) {
    try {
      // In the L-SDK this call can throw IOException so in order to work in
      // both cases catch an exception.
      return MediaCodec.createByCodecName(codecName);
    } catch (Exception e) {
      return null;
    }
  }
  
  boolean initEncode(VideoCodecType type, int width, int height, int kbps, int fps, int interval,
      EglBase14.Context sharedContext, int max_quality, int min_quality, int profile_level,
                     float abravg_bitrate_percent, float abrmax_bitrate_percent ) {
    final boolean useSurface = sharedContext != null;
    Log.d(TAG, "Java initEncode: " + type + " : " + width + " x " + height +
        ". @ " + kbps + " kbps. Fps: " + fps + ". Encode from texture : " + useSurface +
    " , abr_avg:"+ abravg_bitrate_percent + " , abr_max:" + abrmax_bitrate_percent );

    int max_kbps = kbps;

    this.abr_avg_bitrate_percent = abravg_bitrate_percent < 0.85f ? 0.85f : abravg_bitrate_percent;
    this.abr_max_bitrate_percent = abrmax_bitrate_percent;

    this.width = width;
    this.height = height;
    if (mediaCodecThread != null) {
      throw new RuntimeException("Forgot to release()?");
    }

    EncoderProperties properties = null;
    String mime = null;
    int keyFrameIntervalSec = interval;
    if (type == VideoCodecType.VIDEO_CODEC_VP8) {
      mime = VP8_MIME_TYPE;
      properties = findHwEncoder(
          VP8_MIME_TYPE, vp8HwList, useSurface ? supportedSurfaceColorList : supportedColorList);
      //keyFrameIntervalSec = 100;
    } else if (type == VideoCodecType.VIDEO_CODEC_VP9) {
      mime = VP9_MIME_TYPE;
      properties = findHwEncoder(
          VP9_MIME_TYPE, vp9HwList, useSurface ? supportedSurfaceColorList : supportedColorList);
      //keyFrameIntervalSec = 100;
    } else if (type == VideoCodecType.VIDEO_CODEC_H264) {
      mime = H264_MIME_TYPE;
      properties = findHwEncoder(
          H264_MIME_TYPE, h264HwList, useSurface ? supportedSurfaceColorList : supportedColorList);
      //keyFrameIntervalSec = 20;
    }
    if (properties == null) {
      throw new RuntimeException("Can not find HW encoder for " + type);
    }
    
    int w = Math.max(width, height);
    int h = Math.min(width, height);
    if((w < supportedMinSize.width || h < supportedMinSize.height) ||
       (w > supportedMaxSize.width || h > supportedMaxSize.height) )
    {
  	  //throw new IllegalStateException("Non supported  size");
  	  return false;
    }
    
    if(keyFrameIntervalSec == 0)
    	keyFrameIntervalSec = 1;
    runningInstance = this; // Encoder is now running and can be queried for stack traces.
    colorFormat = properties.colorFormat;
    // profile_level is config by server, 1 means use high profile
    configureH264HighProfile = (profile_level == 1) ? properties.configureH264HighProfile : false;
    isVBR = false;
    if(configureH264HighProfile) {
      kbps *= 0.85f;
    }else if( max_quality != -1 && min_quality != -1 ){
      //动态码率模式降低码率设置
      isVBR = true;
      kbps *= abr_avg_bitrate_percent;
      max_kbps *= abr_max_bitrate_percent;
    }

    bitrateAdjustmentRequired = properties.bitrateAdjustment;
    if (bitrateAdjustmentRequired) {
      kbps = BITRATE_ADJUSTMENT_FPS/fps*kbps;
      max_kbps =   BITRATE_ADJUSTMENT_FPS/fps*max_kbps;
      fps = BITRATE_ADJUSTMENT_FPS;
    }
    Log.d(TAG, "Color format: " + colorFormat +
        ". Bitrate adjustment: " + bitrateAdjustmentRequired + " gop:"+keyFrameIntervalSec);

    kbps = (int)(kbps * scaleBitrate * 1000);
    max_kbps = (int)(max_kbps * scaleBitrate * 1000 );
    mediaCodecThread = Thread.currentThread();
    try {
      MediaFormat format = MediaFormat.createVideoFormat(mime, width, height);
      if( max_quality != -1 && min_quality != -1 )
      {
        format.setInteger(MediaFormat.KEY_BIT_RATE, kbps);
        format.setInteger(MediaFormat.KEY_BITRATE_MODE, MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_VBR);     //0:CQ 1:VBR 2:CBR
        //max-bitrate 设置没啥用，注掉了
//        format.setInteger( "max-bitrate", max_kbps );

        Log.d(TAG,"Media Format: bitrate:" + kbps + ", max_bitrate:" + max_kbps );
      }
      else
      {
        format.setInteger(MediaFormat.KEY_BIT_RATE, kbps);
        format.setInteger(MediaFormat.KEY_BITRATE_MODE, MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_CBR);     //0:CQ 1:VBR 2:CBR
      }

      format.setInteger(MediaFormat.KEY_COLOR_FORMAT, properties.colorFormat);
      format.setInteger(MediaFormat.KEY_FRAME_RATE, fps);
//      if(configureH264HighProfile){
//        format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, keyFrameIntervalSec * 2);  //GOP
//      }else {
      format.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, keyFrameIntervalSec);  //GOP
//      }
      int profile = MediaCodecInfo.CodecProfileLevel.AVCProfileBaseline;
      int level = -1;
      if (configureH264HighProfile) {
        profile = CodecProfileLevel.AVCProfileHigh;
        if(width * height < 720 * 1280) {
          level = MediaCodecInfo.CodecProfileLevel.AVCLevel31; // max support 1280*720
        }else {
          if(fps <=30 ) {
            level = MediaCodecInfo.CodecProfileLevel.AVCLevel4; // max support 1920x1080
          }else{
            level = MediaCodecInfo.CodecProfileLevel.AVCLevel42; // max support 2,048×1,080@60
          }
        }
      } else {
        profile = CodecProfileLevel.AVCProfileBaseline;
        if(width * height > 720 * 1280){
          if(testAvcProfileLevel(MediaCodecInfo.CodecProfileLevel.AVCProfileBaseline,
                  MediaCodecInfo.CodecProfileLevel.AVCLevel42)) {
            if (fps <= 30) {
              level = MediaCodecInfo.CodecProfileLevel.AVCLevel4; // max support 1920x1080
            } else {
              level = MediaCodecInfo.CodecProfileLevel.AVCLevel42; // max support 2,048×1,080@60
            }
          }
        }else{
          if(testAvcProfileLevel(MediaCodecInfo.CodecProfileLevel.AVCProfileBaseline,
                  CodecProfileLevel.AVCLevel31)) {
            level = MediaCodecInfo.CodecProfileLevel.AVCLevel31;
          }
        }
      } 
      
      if(level != -1){
        format.setInteger(MediaFormat.KEY_PROFILE, profile);
        format.setInteger(MediaFormat.KEY_LEVEL, level);
      }

      Log.d(TAG, "hw encoder profile: "+ profile + " level: "+level);


      Log.d(TAG, "  Format: " + format);
      mediaCodec = createByCodecName(properties.codecName);
      this.type = type;
      if (mediaCodec == null) {
        Log.d(TAG, "Can not create media encoder");
        return false;
      }
     
      mediaCodec.configure(
          format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);

      if (useSurface) {
        eglBase = new EglBase14(sharedContext, EglBase.CONFIG_RECORDABLE);
//        // Create an input surface and keep a reference since we must release the surface when done.
        inputSurface = mediaCodec.createInputSurface();
        eglBase.createSurface(inputSurface);
        drawer = new GlRectDrawer();
      }
      mediaCodec.start();
      outputBuffers = mediaCodec.getOutputBuffers();
      Log.d(TAG, "Output buffers: " + outputBuffers.length + " threadid:"+Thread.currentThread().getId());

    } catch (IllegalStateException e) {
      if (drawer != null) {
          drawer.release();
          drawer = null;
        }
        if (eglBase != null) {
          eglBase.release();
          eglBase = null;
        }
        if (inputSurface != null) {
          inputSurface.release();
          inputSurface = null;
        }
        mediaCodec = null;
        mediaCodecThread = null;
        Log.d(TAG, "initEncode failed", e);
        return false;
    }
    return true;
  }

  ByteBuffer[]  getInputBuffers() {
    ByteBuffer[] inputBuffers = mediaCodec.getInputBuffers();
    Logging.d(TAG, "Input buffers: " + inputBuffers.length);
    return inputBuffers;
  }

  boolean encodeBuffer(
      boolean isKeyframe, int inputBuffer, int size,
      long presentationTimestampUs) {
    checkOnMediaCodecThread();
    try {
      if (isKeyframe) {
        // Ideally MediaCodec would honor BUFFER_FLAG_SYNC_FRAME so we could
        // indicate this in queueInputBuffer() below and guarantee _this_ frame
        // be encoded as a key frame, but sadly that flag is ignored.  Instead,
        // we request a key frame "soon".
        Logging.d(TAG, "Sync frame request");
        Bundle b = new Bundle();
        b.putInt(MediaCodec.PARAMETER_KEY_REQUEST_SYNC_FRAME, 0);
        mediaCodec.setParameters(b);
      }
      mediaCodec.queueInputBuffer(
          inputBuffer, 0, size, presentationTimestampUs, 0);
      return true;
    }
    catch (IllegalStateException e) {
      Logging.e(TAG, "encodeBuffer failed", e);
      return false;
    }
  }

  boolean encodeTextureRgb(boolean isKeyframe, int textureId, float[] transformationMatrix,
	      long presentationTimestampUs) {
	    checkOnMediaCodecThread();
	    try {
	      if (isKeyframe) {
	        Logging.d(TAG, "Sync frame request");
	        Bundle b = new Bundle();
	        b.putInt(MediaCodec.PARAMETER_KEY_REQUEST_SYNC_FRAME, 0);
	        mediaCodec.setParameters(b);
	      }
	      eglBase.makeCurrent();
	      // TODO(perkj): glClear() shouldn't be necessary since every pixel is covered anyway,
	      // but it's a workaround for bug webrtc:5147.
	      GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
	      drawer.drawRgb(textureId, transformationMatrix, width, height, 0, 0, width, height);
	      eglBase.swapBuffers(TimeUnit.MICROSECONDS.toNanos(presentationTimestampUs));
	      return true;
	    }
	    catch (RuntimeException e) {
	      Logging.e(TAG, "encodeTexture failed", e);
	      return false;
	    }
	  }
  
  boolean encodeTextureOes(boolean isKeyframe, int oesTextureId, float[] transformationMatrix,
      long presentationTimestampUs) {
    checkOnMediaCodecThread();
    try {
      if (isKeyframe) {
        Logging.d(TAG, "Sync frame request");
        Bundle b = new Bundle();
        b.putInt(MediaCodec.PARAMETER_KEY_REQUEST_SYNC_FRAME, 0);
        mediaCodec.setParameters(b);
      }
      eglBase.makeCurrent();
      // TODO(perkj): glClear() shouldn't be necessary since every pixel is covered anyway,
      // but it's a workaround for bug webrtc:5147.
      GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
      drawer.drawOes(oesTextureId, transformationMatrix, width, height, 0, 0, width, height);
      eglBase.swapBuffers(TimeUnit.MICROSECONDS.toNanos(presentationTimestampUs));
      return true;
    }
    catch (RuntimeException e) {
      Logging.e(TAG, "encodeTexture failed", e);
      return false;
    }
  }

  void release() {
    Logging.d(TAG, "Java releaseEncoder");
    checkOnMediaCodecThread();

    // Run Mediacodec stop() and release() on separate thread since sometime
    // Mediacodec.stop() may hang.
    final CountDownLatch releaseDone = new CountDownLatch(1);

    Runnable runMediaCodecRelease = new Runnable() {
      @Override
      public void run() {
        try {
          Logging.d(TAG, "Java releaseEncoder on release thread");
          mediaCodec.stop();
          mediaCodec.release();
          Logging.d(TAG, "Java releaseEncoder on release thread done");
        } catch (Exception e) {
          Logging.e(TAG, "Media encoder release failed", e);
        }
        releaseDone.countDown();
      }
    };
    new Thread(runMediaCodecRelease).start();

    if (!ThreadUtils.awaitUninterruptibly(releaseDone, MEDIA_CODEC_RELEASE_TIMEOUT_MS)) {
      Log.e(TAG, "Media encoder release timeout");
      codecErrors++;
      if (errorCallback != null) {
        Logging.e(TAG, "Invoke codec error callback. Errors: " + codecErrors);
        errorCallback.onMediaCodecVideoEncoderCriticalError(codecErrors);
      }
    }

    mediaCodec = null;
    mediaCodecThread = null;
    if (drawer != null) {
      drawer.release();
      drawer = null;
    }
    if (eglBase != null) {
      eglBase.release();
      eglBase = null;
    }
    if (inputSurface != null) {
      inputSurface.release();
      inputSurface = null;
    }
    runningInstance = null;
    Logging.d(TAG, "Java releaseEncoder done");
  }

  private boolean setRates(int kbps, int frameRate) {
    checkOnMediaCodecThread();
    int codecBitrate = (int)(1000 * kbps * scaleBitrate);
    int max_codecBitrate = codecBitrate;
    if(configureH264HighProfile)
    {
        codecBitrate *= 0.85f;
    }else if( isVBR ){
      //动态码率模式降低码率设置
        codecBitrate *= abr_avg_bitrate_percent;
        max_codecBitrate *= abr_max_bitrate_percent;
    }


    if (bitrateAdjustmentRequired && frameRate > 0) {
      codecBitrate = BITRATE_ADJUSTMENT_FPS * codecBitrate / frameRate;
      max_codecBitrate = BITRATE_ADJUSTMENT_FPS * max_codecBitrate / frameRate;
      Logging.v(TAG, "setRates: " + kbps + " -> " + (codecBitrate / 1000)
          + " kbps. Fps: " + frameRate);
    } else {
      Logging.v(TAG, "setRates: " + kbps);
    }
    try {
      Bundle params = new Bundle();
      params.putInt(MediaCodec.PARAMETER_KEY_VIDEO_BITRATE, codecBitrate);
      //无法动态调整最大码率
      mediaCodec.setParameters(params);
      return true;
    } catch (IllegalStateException e) {
      Logging.e(TAG, "setRates failed", e);
      return false;
    }
  }

  // Dequeue an input buffer and return its index, -1 if no input buffer is
  // available, or -2 if the codec is no longer operative.
  int dequeueInputBuffer() {
    checkOnMediaCodecThread();
    try {
      return mediaCodec.dequeueInputBuffer(DEQUEUE_TIMEOUT);
    } catch (IllegalStateException e) {
      Logging.e(TAG, "dequeueIntputBuffer failed", e);
      return -2;
    }
  }

  // Helper struct for dequeueOutputBuffer() below.
  static class OutputBufferInfo {
    public OutputBufferInfo(
        int index, ByteBuffer buffer,
        boolean isKeyFrame, long presentationTimestampUs) {
      this.index = index;
      this.buffer = buffer;
      this.isKeyFrame = isKeyFrame;
      this.presentationTimestampUs = presentationTimestampUs;
    }

    public final int index;
    public final ByteBuffer buffer;
    public final boolean isKeyFrame;
    public final long presentationTimestampUs;
  }

  // Dequeue and return an output buffer, or null if no output is ready.  Return
  // a fake OutputBufferInfo with index -1 if the codec is no longer operable.
  OutputBufferInfo dequeueOutputBuffer() {
    checkOnMediaCodecThread();
    try {
      MediaCodec.BufferInfo info = new MediaCodec.BufferInfo();
      int result = mediaCodec.dequeueOutputBuffer(info, DEQUEUE_TIMEOUT);
      // Check if this is config frame and save configuration data.
      if (result >= 0) {
        boolean isConfigFrame =
            (info.flags & MediaCodec.BUFFER_FLAG_CODEC_CONFIG) != 0;
        if (isConfigFrame) {
          Logging.d(TAG, "Config frame generated. Offset: " + info.offset +
              ". Size: " + info.size);
          configData = ByteBuffer.allocateDirect(info.size);
          outputBuffers[result].position(info.offset);
          outputBuffers[result].limit(info.offset + info.size);
          configData.put(outputBuffers[result]);
          // Release buffer back.
          mediaCodec.releaseOutputBuffer(result, false);
          // Query next output.
          result = mediaCodec.dequeueOutputBuffer(info, DEQUEUE_TIMEOUT);
        }
      }
      if (result >= 0) {
        // MediaCodec doesn't care about Buffer position/remaining/etc so we can
        // mess with them to get a slice and avoid having to pass extra
        // (BufferInfo-related) parameters back to C++.
        ByteBuffer outputBuffer = outputBuffers[result].duplicate();
        outputBuffer.position(info.offset);
        outputBuffer.limit(info.offset + info.size);
        // Check key frame flag.
        boolean isKeyFrame =
            (info.flags & MediaCodec.BUFFER_FLAG_SYNC_FRAME) != 0;
        if (isKeyFrame) {
 //         Logging.d(TAG, "Sync frame generated");
        }
        if (isKeyFrame && type == VideoCodecType.VIDEO_CODEC_H264) {
//          Logging.d(TAG, "Appending config frame of size " + configData.capacity() +
//              " to output buffer with offset " + info.offset + ", size " +
//              info.size);
          // For H.264 key frame append SPS and PPS NALs at the start
          ByteBuffer keyFrameBuffer = ByteBuffer.allocateDirect(
              configData.capacity() + info.size);
          configData.rewind();
          keyFrameBuffer.put(configData);
          keyFrameBuffer.put(outputBuffer);
          keyFrameBuffer.position(0);
          return new OutputBufferInfo(result, keyFrameBuffer,
              isKeyFrame, info.presentationTimeUs);
        } else {
          return new OutputBufferInfo(result, outputBuffer.slice(),
              isKeyFrame, info.presentationTimeUs);
        }
      } else if (result == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
        outputBuffers = mediaCodec.getOutputBuffers();
        return dequeueOutputBuffer();
      } else if (result == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
        return dequeueOutputBuffer();
      } else if (result == MediaCodec.INFO_TRY_AGAIN_LATER) {
        return null;
      }
      throw new RuntimeException("dequeueOutputBuffer: " + result);
    } catch (IllegalStateException e) {
      Logging.e(TAG, "dequeueOutputBuffer failed", e);
      return new OutputBufferInfo(-1, null, false, -1);
    }
  }

  // Release a dequeued output buffer back to the codec for re-use.  Return
  // false if the codec is no longer operable.
  boolean releaseOutputBuffer(int index) {
    checkOnMediaCodecThread();
    try {
      mediaCodec.releaseOutputBuffer(index, false);
      return true;
    } catch (IllegalStateException e) {
      Logging.e(TAG, "releaseOutputBuffer failed", e);
      return false;
    }
  }

  public static boolean testAvcProfileLevel(int profile, int level)  {
    if (!supports(AVC_MIME, profile, level)) {
      NativeEngine.logcat(NativeEngine.LOG_INFO, TAG, "AvcProfile：" + profile + ", level:" + level + " not supported");
      return false ;
    }
    NativeEngine.logcat(NativeEngine.LOG_INFO, TAG, "AvcProfile：" + profile + ", level:" + level + " supported");
    return true;
  }

  private static boolean supports(String mimeType, int profile, int level) {
      int numCodecs = MediaCodecList.getCodecCount();
      for (int i = 0; i < numCodecs; i++) {
        MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);
        if (!codecInfo.isEncoder()) {
          continue;
        }
        //Log.e(TAG, "codecInfo name: "+codecInfo.getName());
        try {
          for (String mime : codecInfo.getSupportedTypes()) {
            if (mimeType.equals(mime)) {
              CodecCapabilities capabilities = codecInfo.getCapabilitiesForType(mimeType);
              for (CodecProfileLevel profileLevel : capabilities.profileLevels) {
                if (profileLevel.profile == profile
                        && profileLevel.level >= level) {
                  return true;
                }
              }
            }
          }
        } catch (IllegalArgumentException e) {
          continue;
        }
      }
      return false;
  }

}
