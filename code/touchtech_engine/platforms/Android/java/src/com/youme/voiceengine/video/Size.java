package com.youme.voiceengine.video;

/**
 * Created by fire on 2017/2/14.
 */

/**
 * Class for representing size of an object. Very similar to android.util.Size but available on all
 * devices.
 */
public class Size {
    public int width;
    public int height;

    public Size(int width, int height) {
        this.width = width;
        this.height = height;
    }

    @Override
    public String toString() {
        return width + "x" + height;
    }

    @Override
    public boolean equals(Object other) {
        if (!(other instanceof Size)) {
            return false;
        }
        final Size otherSize = (Size) other;
        return width == otherSize.width && height == otherSize.height;
    }

    @Override
    public int hashCode() {
        // Use prime close to 2^16 to avoid collisions for normal values less than 2^16.
        return 1 + 65537 * width + height;
    }
}
