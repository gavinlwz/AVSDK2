//
//  FilterFactory.cpp
//  VideoCodecTest
//
//  Created by bhb on 2017/8/29.
//  Copyright © 2017年 youme. All rights reserved.
//



#include "FilterFactory.h"
#include "Basic/BasicVideoFilterBGRA.h"
#include "Basic/BasicVideoFilterToBGRA.h"
#include "Basic/BeautyVideoFilter.h"
#include "Basic/BasicVideoFilterYUVToBGRA.h"

namespace videocore {
    std::map<std::string, InstantiateFilter>* FilterFactory::s_registration = nullptr ;
    
    FilterFactory::FilterFactory() {
        {
            filters::BasicVideoFilterBGRA b;
            filters::BasicVideoFilterToBGRA tb;
            filters::BasicVideoFilterYUVToBGRA yuv;
            filters::BeautyVideoFilter beauty;
            
        }
    }
    
    void  FilterFactory::remove(IFilter* filter)
    {
        auto it = m_filters.begin();
        while (it != m_filters.end()) {
            if ((*it).get() == filter) {
                m_filters.erase(it);
                break;
            }
            it++;
        }
    }
    
    void  FilterFactory::clear()
    {
        m_filters.clear();
    }
    
    
    IFilter* FilterFactory::filter(std::string name) {
        IFilter* filter = nullptr;
        
        auto it = FilterFactory::s_registration->find(name);
        if(it != FilterFactory::s_registration->end()) {
            std::unique_ptr<IFilter> uq( it->second() );
            m_filters.push_back(std::move(uq));
            filter = m_filters.back().get();
        }
        
        return filter;
    }
    
    void FilterFactory::_register(std::string name, InstantiateFilter instantiation)
    {
        if(!s_registration) {
            s_registration = new std::map<std::string, InstantiateFilter>();
        }
        s_registration->emplace(std::make_pair(name, instantiation));
    }
}


