package com.youme.mixers;

import android.content.Context;

import com.youme.gles.GlUtil;

/**
 * Created by bhb on 2018/1/26.
 */

public class YV12Filter extends YV21Filter {

    private static final   String sVertexShaderFilter = "uniform mat4 uMVPMatrix;  // MVP 的变换矩阵（整体变形）\n" +
            "uniform mat4 uTexMatrix;  // Texture 的变换矩阵 （只对texture变形）\n" +
            "\n" +
            "attribute vec4 aPosition;\n" +
            "attribute vec4 aTextureCoord;\n" +
            "\n" +
            "varying vec2 vTextureCoord;\n" +
            "\n" +
            "void main() {\n" +
            "    gl_Position = uMVPMatrix * aPosition;\n" +
            "    vTextureCoord = (uTexMatrix*aTextureCoord).xy;\n" +
            "}";

    private static final   String sFragmentShaderFilter = "\n" +
            "precision mediump float; //指定默认精度\n" +
            "\n" +
            "varying vec2 vTextureCoord;\n" +
            "uniform sampler2D uTexture;\n" +
            "uniform sampler2D uTextureU;\n" +
            "uniform sampler2D uTextureV;\n" +
            "\n" +
            "void main() {\n" +
            "mediump vec3 yuv;\n"+
            "lowp vec3 rgb;\n"+
            "yuv.x = texture2D(uTexture, vTextureCoord).r;\n"+
            "yuv.y = texture2D(uTextureV, vTextureCoord).r - 0.5;\n"+
            "yuv.z = texture2D(uTextureU, vTextureCoord).r - 0.5;\n"+
            "rgb = mat3(1, 1, 1, 0, -0.39465, 2.03211, 1.13983, -0.58060, 0) * yuv;\n"+
            "gl_FragColor = vec4(rgb, 1);\n"+
            "}";

    public YV12Filter(int type, boolean useOES) {
        super(type, useOES);
    }

    @Override
    protected int createProgram(Context applicationContext) {
        return GlUtil.createProgram(sVertexShaderFilter, sFragmentShaderFilter);
    }
}
