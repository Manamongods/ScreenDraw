package com.example.screendraw;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.util.Log;
import android.view.SurfaceHolder;

public class HoverView extends SurfaceView implements SurfaceHolder.Callback {

    private boolean surfaceReady = false;

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
        getHolder().addCallback(this);
        setFocusable(true);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        // The surface is created. Initialize drawing here.
        surfaceReady = true;
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        // The surface size or format has changed
        // Respond appropriately to surface changes, e.g., adjusting drawing parameters
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        // The surface is being destroyed. Cleanup or release resources here.
        surfaceReady = false;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        //Log.d("TAG", "Hofevent " + event.getAction());

        switch (event.getAction()) {
            case MotionEvent.ACTION_HOVER_ENTER:
                // Handle hover enter event
                // Example: Log.d("CustomSurfaceView", "Hover enter event");
                break;
            case MotionEvent.ACTION_HOVER_MOVE:
                // Handle hover move event
                // Example: Log.d("CustomSurfaceView", "Hover move event");
                break;
            case MotionEvent.ACTION_HOVER_EXIT:
                // Handle hover exit event
                // Example: Log.d("CustomSurfaceView", "Hover exit event");
                break;
        }
        return false;
    }
}