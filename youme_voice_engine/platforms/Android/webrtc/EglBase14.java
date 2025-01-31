/*
 *  Copyright 2015 The WebRTC@AnyRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

package com.youme.webrtc;

import android.annotation.TargetApi;
import android.graphics.SurfaceTexture;
import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLExt;
import android.opengl.EGLSurface;
import android.view.Surface;

/**
 * Holds EGL state and utility methods for handling an EGL14 EGLContext, an EGLDisplay,
 * and an EGLSurface.
 */
@TargetApi(18)
public final class EglBase14 extends EglBase {
  private static final String TAG = "EglBase14";
  private static final int EGLExt_SDK_VERSION = android.os.Build.VERSION_CODES.JELLY_BEAN_MR2;
  private static final int CURRENT_SDK_VERSION = android.os.Build.VERSION.SDK_INT;
  private EGLContext eglContext;
  private EGLConfig eglConfig;
  private EGLDisplay eglDisplay;
  private EGLSurface eglSurface = EGL14.EGL_NO_SURFACE;

  // EGL 1.4 is supported from API 17. But EGLExt that is used for setting presentation
  // time stamp on a surface is supported from 18 so we require 18.
  public static boolean isEGL14Supported() {
    Logging.d(TAG, "SDK version: " + CURRENT_SDK_VERSION
        + ". isEGL14Supported: " + (CURRENT_SDK_VERSION >= EGLExt_SDK_VERSION));
    return (CURRENT_SDK_VERSION >= EGLExt_SDK_VERSION);
  }

  public static class Context extends EglBase.Context {
    private final EGLContext egl14Context;

    public Context(EGLContext eglContext) {
      this.egl14Context = eglContext;
    }
    public EGLContext getEGLContext(){return  egl14Context;}
  }

  // Create a new context with the specified config type, sharing data with sharedContext.
  // |sharedContext| may be null.
  public EglBase14(EglBase14.Context sharedContext, int[] configAttributes) {
    eglDisplay = getEglDisplay();
    eglConfig = getEglConfig(eglDisplay, configAttributes);
    eglContext = createEglContext(sharedContext, eglDisplay, eglConfig);
  }

  // Create EGLSurface from the Android Surface.
  @Override
  public void createSurface(Surface surface) {
    createSurfaceInternal(surface);
  }

  // Create EGLSurface from the Android SurfaceTexture.
  @Override
  public void createSurface(SurfaceTexture surfaceTexture) {
    createSurfaceInternal(surfaceTexture);
  }

  // Create EGLSurface from either Surface or SurfaceTexture.
  private void createSurfaceInternal(Object surface) {
    if (!(surface instanceof Surface) && !(surface instanceof SurfaceTexture)) {
      throw new IllegalStateException("Input must be either a Surface or SurfaceTexture");
    }
    checkIsNotReleased();
    if (eglSurface != EGL14.EGL_NO_SURFACE) {
      throw new RuntimeException("Already has an EGLSurface");
    }
    int[] surfaceAttribs = {EGL14.EGL_NONE};
    eglSurface = EGL14.eglCreateWindowSurface(eglDisplay, eglConfig, surface, surfaceAttribs, 0);
    if (eglSurface == EGL14.EGL_NO_SURFACE) {
      throw new RuntimeException("Failed to create window surface");
    }
  }

  @Override
  public void createDummyPbufferSurface() {
    createPbufferSurface(1, 1);
  }

  @Override
  public void createPbufferSurface(int width, int height) {
    checkIsNotReleased();
    if (eglSurface != EGL14.EGL_NO_SURFACE) {
      throw new RuntimeException("Already has an EGLSurface");
    }
    int[] surfaceAttribs = {EGL14.EGL_WIDTH, width, EGL14.EGL_HEIGHT, height, EGL14.EGL_NONE};
    eglSurface = EGL14.eglCreatePbufferSurface(eglDisplay, eglConfig, surfaceAttribs, 0);
    if (eglSurface == EGL14.EGL_NO_SURFACE) {
      throw new RuntimeException(
          "Failed to create pixel buffer surface with size: " + width + "x" + height);
    }
  }

  @Override
  public Context getEglBaseContext() {
    return new EglBase14.Context(eglContext);
  }

  @Override
  public boolean hasSurface() {
    return eglSurface != EGL14.EGL_NO_SURFACE;
  }

  @Override
  public int surfaceWidth() {
    final int widthArray[] = new int[1];
    EGL14.eglQuerySurface(eglDisplay, eglSurface, EGL14.EGL_WIDTH, widthArray, 0);
    return widthArray[0];
  }

  @Override
  public int surfaceHeight() {
    final int heightArray[] = new int[1];
    EGL14.eglQuerySurface(eglDisplay, eglSurface, EGL14.EGL_HEIGHT, heightArray, 0);
    return heightArray[0];
  }

  @Override
  public void releaseSurface() {
    if (eglSurface != EGL14.EGL_NO_SURFACE) {
      EGL14.eglDestroySurface(eglDisplay, eglSurface);
      eglSurface = EGL14.EGL_NO_SURFACE;
    }
  }

  private void checkIsNotReleased() {
    if (eglDisplay == EGL14.EGL_NO_DISPLAY || eglContext == EGL14.EGL_NO_CONTEXT
        || eglConfig == null) {
      throw new RuntimeException("This object has been released");
    }
  }

  @Override
  public void release() {
    checkIsNotReleased();
    releaseSurface();
    detachCurrent();
    EGL14.eglDestroyContext(eglDisplay, eglContext);
    EGL14.eglReleaseThread();
    EGL14.eglTerminate(eglDisplay);
    eglContext = EGL14.EGL_NO_CONTEXT;
    eglDisplay = EGL14.EGL_NO_DISPLAY;
    eglConfig = null;
  }

  @Override
  public void makeCurrent() {
    checkIsNotReleased();
    if (eglSurface == EGL14.EGL_NO_SURFACE) {
      throw new RuntimeException("No EGLSurface - can't make current");
    }
    synchronized (EglBase.lock) {
      if (!EGL14.eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext)) {
        throw new RuntimeException("eglMakeCurrent failed");
      }
    }
  }

  // Detach the current EGL context, so that it can be made current on another thread.
  @Override
  public void detachCurrent() {
    synchronized (EglBase.lock) {
      if (!EGL14.eglMakeCurrent(
          eglDisplay, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_CONTEXT)) {
        throw new RuntimeException("eglDetachCurrent failed");
      }
    }
  }

  @Override
  public void swapBuffers() {
    checkIsNotReleased();
    if (eglSurface == EGL14.EGL_NO_SURFACE) {
      throw new RuntimeException("No EGLSurface - can't swap buffers");
    }
    synchronized (EglBase.lock) {
      EGL14.eglSwapBuffers(eglDisplay, eglSurface);
    }
  }

  public void swapBuffers(long timeStampNs) {
    checkIsNotReleased();
    if (eglSurface == EGL14.EGL_NO_SURFACE) {
      throw new RuntimeException("No EGLSurface - can't swap buffers");
    }
    synchronized (EglBase.lock) {
      // See https://android.googlesource.com/platform/frameworks/native/+/tools_r22.2/opengl/specs/EGL_ANDROID_presentation_time.txt
      EGLExt.eglPresentationTimeANDROID(eglDisplay, eglSurface, timeStampNs);
      EGL14.eglSwapBuffers(eglDisplay, eglSurface);
    }
  }

  // Return an EGLDisplay, or die trying.
  private static EGLDisplay getEglDisplay() {
    EGLDisplay eglDisplay = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
    if (eglDisplay == EGL14.EGL_NO_DISPLAY) {
      throw new RuntimeException("Unable to get EGL14 display");
    }
    int[] version = new int[2];
    if (!EGL14.eglInitialize(eglDisplay, version, 0, version, 1)) {
      throw new RuntimeException("Unable to initialize EGL14");
    }
    return eglDisplay;
  }

  // Return an EGLConfig, or die trying.
  private static EGLConfig getEglConfig(EGLDisplay eglDisplay, int[] configAttributes) {
    EGLConfig[] configs = new EGLConfig[1];
    int[] numConfigs = new int[1];
    if (!EGL14.eglChooseConfig(
        eglDisplay, configAttributes, 0, configs, 0, configs.length, numConfigs, 0)) {
      throw new RuntimeException("eglChooseConfig failed");
    }
    if (numConfigs[0] <= 0) {
      throw new RuntimeException("Unable to find any matching EGL config");
    }
    final EGLConfig eglConfig = configs[0];
    if (eglConfig == null) {
      throw new RuntimeException("eglChooseConfig returned null");
    }
    return eglConfig;
  }

  // Return an EGLConfig, or die trying.
  private static EGLContext createEglContext(
          EglBase14.Context sharedContext, EGLDisplay eglDisplay, EGLConfig eglConfig) {
    if (sharedContext != null && sharedContext.egl14Context == EGL14.EGL_NO_CONTEXT) {
      throw new RuntimeException("Invalid sharedContext");
    }
    int[] contextAttributes = {EGL14.EGL_CONTEXT_CLIENT_VERSION, 2, EGL14.EGL_NONE};
    EGLContext rootContext =
        sharedContext == null || sharedContext.egl14Context == null ? EGL14.EGL_NO_CONTEXT : sharedContext.egl14Context;
    final EGLContext eglContext;
    synchronized (EglBase.lock) {
      eglContext = EGL14.eglCreateContext(eglDisplay, eglConfig, rootContext, contextAttributes, 0);
    }
    if (eglContext == EGL14.EGL_NO_CONTEXT) {
      throw new RuntimeException("Failed to create EGL context");
    }
    return eglContext;
  }
}
