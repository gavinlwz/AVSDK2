package com.youme.mixers;

import android.graphics.Rect;

/**
 * Created by bhb on 2018/1/9.
 */

public class MixInfo {
    private int z;
    private Rect rect;
    private String userId;

    public MixInfo(String userId, int z, Rect rect){
        this.userId = userId;
        this.z = z;
        this.rect = rect;
    }

    public String getUserId(){return  userId;}

    public int getZ(){return  z;}

    public Rect getRect(){return  rect;}

}
