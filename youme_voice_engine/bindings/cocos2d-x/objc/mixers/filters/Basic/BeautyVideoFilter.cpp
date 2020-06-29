//
//  BeautyVideoFilter.cpp
//  VideoCodecTest
//
//  Created by bhb on 2017/8/29.
//  Copyright © 2017年 youmi. All rights reserved.
//

#include <TargetConditionals.h>
#ifdef MAC_OS
#import <OpenGL/gl3.h>
#else
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES3/gl.h>
#endif

#include "BeautyVideoFilter.h"
#include "../../GLESUtil.h"
#include "../FilterFactory.h"

namespace videocore { namespace filters {
    
    enum LIVE_BEAUTY_LEVEL
    {
        LIVE_BEAUTY_LEVEL1,
        LIVE_BEAUTY_LEVEL2,
        LIVE_BEAUTY_LEVEL3,
        LIVE_BEAUTY_LEVEL4,
        LIVE_BEAUTY_LEVEL5
    };
    
    static void getBeautyParam(GLfloat* fBeautyParam, enum LIVE_BEAUTY_LEVEL level){
        switch (level) {
            case LIVE_BEAUTY_LEVEL1:
                fBeautyParam[0] = 1.0;
                fBeautyParam[1] = 1.0;
                fBeautyParam[2] = 0.15;
                fBeautyParam[3] = 0.15;
                break;
            case LIVE_BEAUTY_LEVEL2:
                fBeautyParam[0] = 0.8;
                fBeautyParam[1] = 0.9;
                fBeautyParam[2] = 0.2;
                fBeautyParam[3] = 0.2;
                break;
            case LIVE_BEAUTY_LEVEL3:
                fBeautyParam[0] = 0.6;
                fBeautyParam[1] = 0.8;
                fBeautyParam[2] = 0.25;
                fBeautyParam[3] = 0.25;
                break;
            case LIVE_BEAUTY_LEVEL4:
                fBeautyParam[0] = 0.4;
                fBeautyParam[1] = 0.7;
                fBeautyParam[2] = 0.38;
                fBeautyParam[3] = 0.3;
                break;
            case LIVE_BEAUTY_LEVEL5:
                fBeautyParam[0] = 0.33/1.5;
                fBeautyParam[1] = 0.63/1.5;
                fBeautyParam[2] = 0.4/1.5;
                fBeautyParam[3] = 0.35/1.5;
                break;
            default:
                break;
        }
    }
    
    void BeautyVideoFilter::getBeautyParam(float* fBeautyParam)
    {
        fBeautyParam[0] = -m_level*0.8 + 1.0;
        fBeautyParam[1] = -m_level*0.5 + 1.0;
        fBeautyParam[2] = 0.3*m_level + 0.15;
        fBeautyParam[3] = 0.25*m_level + 0.15;
    }
    
    

    bool BeautyVideoFilter::s_registered = BeautyVideoFilter::registerFilter();
    
    bool
    BeautyVideoFilter::registerFilter()
    {
        FilterFactory::_register("com.videocore.filters.beauty", []() { return new BeautyVideoFilter(); });
        return true;
    }
    
    BeautyVideoFilter::BeautyVideoFilter()
    : IVideoFilter(), m_initialized(false), m_bound(false),m_level(0.5f)
    {
        
    }
    BeautyVideoFilter::~BeautyVideoFilter()
    {
        if(m_program)
          glDeleteProgram(m_program);
    }
    
    void BeautyVideoFilter::setBeautyLevel(float level)
    {
        m_level = level;
    }
    
    const char * const
    BeautyVideoFilter::vertexKernel() const
    {
        
        KERNEL(GL_ES2_3, m_language,
               uniform mat4   position;
               attribute vec2 aPos;
               attribute vec2 texCoordinate;
               varying vec2   textureCoordinate;
               void main(void) {
                   gl_Position =  position*vec4(aPos,0.,1.);
                   textureCoordinate = texCoordinate;
               }
               )
        
        return nullptr;
    }
    
    const char * const
    BeautyVideoFilter::pixelKernel() const
    {
#if TARGET_OS_OSX
        KERNEL(GL_ES2_3, m_language,
               uniform  vec4 params;
               varying  vec2 textureCoordinate;
               const  vec3 W = vec3(0.299,0.587,0.114);
#else
        KERNEL2(GL_ES2_3, m_language,
                precision highp float;,
                uniform highp vec4 params;
                varying highp vec2 textureCoordinate;
                const highp vec3 W = vec3(0.299,0.587,0.114);
#endif
               uniform sampler2D inputImageTexture;
               uniform vec2 singleStepOffset;

               const mat3 saturateMatrix = mat3(
                                                1.1102,-0.0598,-0.061,
                                                -0.0774,1.0826,-0.1186,
                                                -0.0228,-0.0228,1.1772);
               
               float hardlight(float color)
               {
                   if(color <= 0.5)
                   {
                       color = color * color * 2.0;
                   }
                   else
                   {
                       color = 1.0 - ((1.0 - color)*(1.0 - color) * 2.0);
                   }
                   return color;
               }
               
               void main(){
                   vec2 blurCoordinates[24];
                   
                   blurCoordinates[0] = textureCoordinate.xy + singleStepOffset * vec2(0.0, -10.0);
                   blurCoordinates[1] = textureCoordinate.xy + singleStepOffset * vec2(0.0, 10.0);
                   blurCoordinates[2] = textureCoordinate.xy + singleStepOffset * vec2(-10.0, 0.0);
                   blurCoordinates[3] = textureCoordinate.xy + singleStepOffset * vec2(10.0, 0.0);
                   
                   blurCoordinates[4] = textureCoordinate.xy + singleStepOffset * vec2(5.0, -8.0);
                   blurCoordinates[5] = textureCoordinate.xy + singleStepOffset * vec2(5.0, 8.0);
                   blurCoordinates[6] = textureCoordinate.xy + singleStepOffset * vec2(-5.0, 8.0);
                   blurCoordinates[7] = textureCoordinate.xy + singleStepOffset * vec2(-5.0, -8.0);
                   
                   blurCoordinates[8] = textureCoordinate.xy + singleStepOffset * vec2(8.0, -5.0);
                   blurCoordinates[9] = textureCoordinate.xy + singleStepOffset * vec2(8.0, 5.0);
                   blurCoordinates[10] = textureCoordinate.xy + singleStepOffset * vec2(-8.0, 5.0);
                   blurCoordinates[11] = textureCoordinate.xy + singleStepOffset * vec2(-8.0, -5.0);
                   
                   blurCoordinates[12] = textureCoordinate.xy + singleStepOffset * vec2(0.0, -6.0);
                   blurCoordinates[13] = textureCoordinate.xy + singleStepOffset * vec2(0.0, 6.0);
                   blurCoordinates[14] = textureCoordinate.xy + singleStepOffset * vec2(6.0, 0.0);
                   blurCoordinates[15] = textureCoordinate.xy + singleStepOffset * vec2(-6.0, 0.0);
                   
                   blurCoordinates[16] = textureCoordinate.xy + singleStepOffset * vec2(-4.0, -4.0);
                   blurCoordinates[17] = textureCoordinate.xy + singleStepOffset * vec2(-4.0, 4.0);
                   blurCoordinates[18] = textureCoordinate.xy + singleStepOffset * vec2(4.0, -4.0);
                   blurCoordinates[19] = textureCoordinate.xy + singleStepOffset * vec2(4.0, 4.0);
                   
                   blurCoordinates[20] = textureCoordinate.xy + singleStepOffset * vec2(-2.0, -2.0);
                   blurCoordinates[21] = textureCoordinate.xy + singleStepOffset * vec2(-2.0, 2.0);
                   blurCoordinates[22] = textureCoordinate.xy + singleStepOffset * vec2(2.0, -2.0);
                   blurCoordinates[23] = textureCoordinate.xy + singleStepOffset * vec2(2.0, 2.0);
                   
                   
                   float sampleColor = texture2D(inputImageTexture, textureCoordinate).g * 22.0;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[0]).g;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[1]).g;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[2]).g;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[3]).g;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[4]).g;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[5]).g;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[6]).g;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[7]).g;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[8]).g;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[9]).g;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[10]).g;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[11]).g;
                   
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[12]).g * 2.0;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[13]).g * 2.0;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[14]).g * 2.0;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[15]).g * 2.0;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[16]).g * 2.0;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[17]).g * 2.0;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[18]).g * 2.0;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[19]).g * 2.0;
                   
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[20]).g * 3.0;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[21]).g * 3.0;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[22]).g * 3.0;
                   sampleColor += texture2D(inputImageTexture, blurCoordinates[23]).g * 3.0;
                   
                   sampleColor = sampleColor / 62.0;
                   
                   vec3 centralColor = texture2D(inputImageTexture, textureCoordinate).rgb;
                   
                   float highpass = centralColor.g - sampleColor + 0.5;
                   
                   for(int i = 0; i < 5;i++)
                   {
                       highpass = hardlight(highpass);
                   }
                   float lumance = dot(centralColor, W);
                   
                   float alpha = pow(lumance, params.r);
                   
                   vec3 smoothColor = centralColor + (centralColor-vec3(highpass))*alpha*0.1;
                   
                   smoothColor.r = clamp(pow(smoothColor.r, params.g),0.0,1.0);
                   smoothColor.g = clamp(pow(smoothColor.g, params.g),0.0,1.0);
                   smoothColor.b = clamp(pow(smoothColor.b, params.g),0.0,1.0);
                   
                   vec3 lvse = vec3(1.0)-(vec3(1.0)-smoothColor)*(vec3(1.0)-centralColor);
                   vec3 bianliang = max(smoothColor, centralColor);
                   vec3 rouguang = 2.0*centralColor*smoothColor + centralColor*centralColor - 2.0*centralColor*centralColor*smoothColor;
                   
                   gl_FragColor = vec4(mix(centralColor, lvse, alpha), 1.0);
                   gl_FragColor.rgb = mix(gl_FragColor.rgb, bianliang, alpha);
                   gl_FragColor.rgb = mix(gl_FragColor.rgb, rouguang, params.b);
                   
                   vec3 satcolor = gl_FragColor.rgb * saturateMatrix;
                   gl_FragColor.rgb = mix(gl_FragColor.rgb, satcolor, params.a);
               }
               )
        
        return nullptr;
    }
    void
    BeautyVideoFilter::initialize()
    {
        switch(m_language) {
            case GL_ES2_3:
            case GL_2: {
                setProgram(build_program(vertexKernel(), pixelKernel()));
                m_uMatrix = glGetUniformLocation(m_program, "position");
                m_uWH     = glGetUniformLocation(m_program, "singleStepOffset");
                m_beautyParam = glGetUniformLocation(m_program, "params");
                m_attrpos = glGetAttribLocation(m_program, "aPos");
                m_attrtex = glGetAttribLocation(m_program, "texCoordinate");
                int unitex = glGetUniformLocation(m_program, "inputImageTexture");
                glUniform1i(unitex, 0);
                m_initialized = true;
            }
                break;
            case GL_3:
                break;
        }
    }
    void
    BeautyVideoFilter::bind()
    {
        switch(m_language) {
            case GL_ES2_3:
            case GL_2:
                if(!m_bound) {
                    if(!initialized()) {
                        initialize();
                    }
                    glUseProgram(m_program);
                }
                //glUniformMatrix4fv(m_uMatrix, 1, GL_FALSE, m_matrix);
                
                glUniform1f(m_uWidth, m_dimensions.w);
                glUniform1f(m_uHeight, m_dimensions.h);
                GLfloat fWHArray[2];
                fWHArray[0] = m_dimensions.w;
                fWHArray[1] = m_dimensions.h;
                glUniform2fv(m_uWH, 1, fWHArray);
                
                GLfloat fBeautyParam[4];
                //getBeautyParam(fBeautyParam, LIVE_BEAUTY_LEVEL5);
                getBeautyParam(fBeautyParam);
                glUniform4fv(m_beautyParam, 1, fBeautyParam);
                
               IVideoFilter::bind();

                break;
            case GL_3:
                break;
        }
    }
    void
    BeautyVideoFilter::unbind()
    {
        m_bound = false;
    }
}
}
