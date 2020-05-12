//
//  ImageUtils.hpp
//  youme_voice_engine
//
//  Created by mac on 2017/8/25.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef ImageUtils_hpp
#define ImageUtils_hpp

#include <memory>
#include <stdio.h>
#include <stdlib.h>
namespace youme_voice_engine {
    class Image
    {
    public:
        int width;
        int height;
        void* data;
        int size;
        int iSlot;
        
    public:
        Image(int width, int height, void* data);
        Image(int width, int height);
        ~Image();
    };

    class ImageUtils
    {
    public:
        static Image* transpose(Image* src);
        static Image* mirror(Image* src);
        static Image* rotate(Image* src, int rotation);
        static Image* crop(Image* src, int width, int height);
        static Image* scale(Image* src, int width, int height);
        static Image* centerScale(Image* src, int width, int height);
        //by bhb 2017-09-11
        static void centerScale_new(char * in_buff, int in_width, int in_height, char * out_buff, int out_width, int out_height);
    };
} // namespace youme_voice_engine

#endif /* ImageUtils_hpp */
