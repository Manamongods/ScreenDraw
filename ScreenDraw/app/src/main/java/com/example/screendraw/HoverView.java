package com.example.screendraw;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.util.Log;

public class HoverView extends SurfaceView {

    public HoverView(Context context) {
        super(context);
        init();
    }

    public HoverView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public HoverView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    private void init() {
        // Perform initialization if needed
    }
}