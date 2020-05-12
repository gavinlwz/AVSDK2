
//
//  FilterFactory.h
//  VideoCodecTest
//
//  Created by bhb on 2017/8/29.
//  Copyright © 2017年 youme. All rights reserved.
//

#ifndef __videocore__FilterFactory__
#define __videocore__FilterFactory__
#include "IFilter.h"
#include <map>
#include <list>
#include <memory>
#include <functional>

#define GL_GLEXT_PROTOTYPES 
namespace videocore {
    
    using InstantiateFilter = std::function<IFilter*()> ;
    
    class FilterFactory {
    public:
        FilterFactory();
        virtual ~FilterFactory() {};
        
        IFilter* filter(std::string name);
        
        void  remove(IFilter* filter);
        void  clear();
    public:
        static void _register(std::string name, InstantiateFilter instantiation );
        
    private:
        std::list<std::unique_ptr<IFilter>> m_filters;
    private:
        static std::map<std::string, InstantiateFilter>* s_registration;
    };
    
}

#endif
