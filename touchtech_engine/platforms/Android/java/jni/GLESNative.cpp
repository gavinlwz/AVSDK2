//
//  GLESNative.cpp
//  youme_voice_engine
//
//  Created by bhb on 2018/3/21.
//  Copyright © 2018年 Youme. All rights reserved.
//

#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <iostream>
#include <GLES2/gl2.h>
#include <android/log.h>
#include "com_youme_mixers_VideoMixerNative.h"


typedef GLvoid*   (*pfnGLMapBufferRange) (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
pfnGLMapBufferRange  fGLMapBufferRange = nullptr;

static int ympixelBufferWidth = 0;
static int ympixelBufferHeight = 0;
static char* ympixelBufferData = nullptr;

bool load_gl3_pfn(){
    if(fGLMapBufferRange)
        return true;
    void *handle = dlopen("system/lib/libGLESv3.so", RTLD_LAZY);
    if (!handle) {
        handle = dlopen("system/lib64/libGLESv3.so", RTLD_LAZY);
    }
    
    if(!handle)
        return false;
    
    fGLMapBufferRange = (pfnGLMapBufferRange)dlsym(handle, "glMapBufferRange");
    dlclose(handle);
    return fGLMapBufferRange != nullptr;
}
static void youme_yuv_crop(char* dstBuffer, int dstW, int dstH, char* srcBuffer, int srcW, int srcH){
    
    int width  = dstW;
    int height = dstH;
    int stride = srcW;
    char * pSrcY = srcBuffer;
    char * pSrcU = pSrcY+stride * height;
    char * pSrcV = pSrcU;
    
    char * pDstY = dstBuffer;
    char * pDstU = pDstY+width * height;
    char * pDstV = pDstU+width * height/4;
    
    int uvWidth = width;
    int uvHieght = height/4;
    
    for (int i = 0 ; i < uvHieght; ++i){
        //Y
        memcpy(pDstY + i*4*width, pSrcY+i*4*stride, width);
        memcpy(pDstY + (i*4+1)*width, pSrcY+(i*4+1)*stride, width);
        memcpy(pDstY + (i*4+2)*width, pSrcY+(i*4+2)*stride, width);
        memcpy(pDstY + (i*4+3)*width, pSrcY+(i*4+3)*stride, width);
        
        //U
        memcpy(pDstU + i*uvWidth, pSrcU+i*2*stride, uvWidth/2);
        memcpy(pDstU + i*uvWidth+uvWidth/2, pSrcU+(i*2+1)*stride, uvWidth/2);
        
        //V
        memcpy(pDstV + i*uvWidth, pSrcV+i*2*stride + uvWidth/2, uvWidth/2);
        memcpy(pDstV + i*uvWidth+uvWidth/2, pSrcV+(i*2+1)*stride + uvWidth/2, uvWidth/2);
        
    }
    
}


/*
 * Class:     com_youme_mixers_VideoMixerNative
 * Method:    isSupportGL3
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_youme_mixers_VideoMixerNative_isSupportGL3(JNIEnv *env, jclass jobj){
    void *handle = dlopen("system/lib/libGLESv3.so", RTLD_LAZY);
    if (handle) {
      //  TSK_DEBUG_INFO("found arm32 libGLESv3!");
        dlclose(handle);
        return true;
    }
    
    handle = dlopen("system/lib64/libGLESv3.so", RTLD_LAZY);
    if (handle) {
      //  TSK_DEBUG_INFO("found arm64 libGLESv3!");
        dlclose(handle);
        return true;
    }
  //  TSK_DEBUG_INFO("Couldn't load libGLESv3.so !");
    return false;
}

/*
 * Class:     com_youme_mixers_VideoMixerNative
 * Method:    glReadPixels
 * Signature: (IIIIII[B)V
 */
JNIEXPORT void JNICALL Java_com_youme_mixers_VideoMixerNative_glReadPixels(JNIEnv *env, jclass jobj,  jint x, jint y, jint width, jint height, jint format, jint type, jbyteArray array){
    if(array == nullptr){
        glReadPixels(x, y, width, height, format, type, nullptr);
    }
    else{
        char * tmpBuffer = (char*)env->GetByteArrayElements(array, 0);
        if(tmpBuffer == nullptr)
            return;
        if(ympixelBufferWidth != width || ympixelBufferHeight != height){
            if(ympixelBufferData)
                free(ympixelBufferData);
            ympixelBufferData = (char*)malloc(width*height*4);
            ympixelBufferWidth = width;
            ympixelBufferHeight = height;
        }
        glReadPixels(x, y, width, height, format, type, ympixelBufferData);
        char* tmpp = (char*)env->GetByteArrayElements(array, 0);
        youme_yuv_crop(tmpp, width*4, height*2/3, ympixelBufferData, width*4, height*2/3);
        env->ReleaseByteArrayElements(array, (jbyte*)tmpp, 0);
    }
}

/*
 * Class:     com_youme_mixers_VideoMixerNative
 * Method:    glMapBufferRange
 * Signature: (IIIIIII[B)I
 */
JNIEXPORT jint JNICALL Java_com_youme_mixers_VideoMixerNative_glMapBufferRange(JNIEnv *env, jclass jobj, jint width, jint height, jint stride,  jint target, jint offset, jint length, jint access, jbyteArray array){
    
    //long long tm1 = getSystemTimer();
    if(!load_gl3_pfn())
        return -1;
    char* buffer = (char*) fGLMapBufferRange(target, offset, length, access);
    if(buffer == nullptr)
        return -2;
    //long long tm2 = getSystemTimer();
    width  = width*4;
    height = height*2/3;
    
    char* tmpp = (char*)env->GetByteArrayElements(array, 0);
    youme_yuv_crop(tmpp, width, height, buffer, stride, height);
    env->ReleaseByteArrayElements(array, (jbyte*)tmpp, 0);
    //long long tm3 = getSystemTimer();
    //LOGD("bindPixelBuffer ~ ~ ~ ~ ~ tm1:%lld, tm2:%lld\n", tm2 - tm1, tm3 - tm2);
    return 0;
    
}




