package com.youme.mixers;

import java.util.Collection;
import java.util.HashMap;
import java.util.Map;


public class FilterManager {

    public  Map<String,IFilter> mFilters = new HashMap<String,IFilter>();

    public static FilterManager sFilterManager;
    public static FilterManager getInstance(){
         if(sFilterManager == null)
             sFilterManager = new FilterManager();
        return sFilterManager;
    }
    private FilterManager() {
    }

    public  void releaseFilter(){
        Collection<IFilter> filter_Value = mFilters.values();
        for (IFilter value : filter_Value) {
            value.releaseProgram();
        }
        mFilters.clear();
    }

    public  IFilter getCameraFilter(String userId, FilterType filterType) {

        IFilter filter = mFilters.get(userId);
        if (filter != null) {
            if (filter.getFilterType() == filterType.ordinal())
                return filter;
            filter.releaseProgram();
            mFilters.remove(userId);
        }
        try{
        switch (filterType) {
            default:
            case Normal:
                filter = new CameraFilter(filterType.ordinal(), false);
                break;
            case NormalOES:
                filter = new CameraFilter(filterType.ordinal(), true);
                break;
            case Beautify:
                filter = new CameraFilterBeauty(filterType.ordinal(), false);
                break;
            case BeautifyOES:
                filter = new CameraFilterBeauty(filterType.ordinal(), true);
                break;
            case YV12:
                filter = new YV12Filter(filterType.ordinal(), false);
                break;
            case YV21:
                filter = new YV21Filter(filterType.ordinal(), false);
                break;
            case NV21:
                filter = new NV21Filter(filterType.ordinal(), false);
                break;
            case TEXTURETOYUV:
                filter = new TextureToYUV(filterType.ordinal(), false);
                break;
            case TEXTURETOYUVOES:
                filter = new TextureToYUV(filterType.ordinal(), true);
                break;
        }
        }catch(Exception e){
        	e.printStackTrace();
        	return null;
        }
        mFilters.put(userId, filter);
        return filter;
    }

    public enum FilterType {
        Normal, NormalOES, Beautify, BeautifyOES, YV21, YV12, NV21, TEXTURETOYUV, TEXTURETOYUVOES,
    }
}
