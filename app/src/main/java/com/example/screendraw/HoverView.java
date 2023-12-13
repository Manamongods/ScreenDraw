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

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int action = event.getActionMasked();
        Log.d("TAG", "onTouchEvent");

        switch (action) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_MOVE:
                float x = event.getX();
                float y = event.getY();

                // Now x and y contain the stylus position
                // Use the position information as needed

                break;
            case MotionEvent.ACTION_UP:
                // Handle touch release if needed
                break;
            // ... other cases for different touch events

        }

        return true; // indicate that the event has been handled
    }

    @Override
    public boolean onHoverEvent(MotionEvent event) {
        Log.d("TAG", "Hover enter event");
        switch (event.getAction()) {
            case MotionEvent.ACTION_HOVER_ENTER:
                // Handle hover enter event
                Log.d("CustomHoverView", "Hover enter event");
                break;
            case MotionEvent.ACTION_HOVER_MOVE:
                // Handle hover move event
                Log.d("CustomHoverView", "Hover move event");
                break;
            case MotionEvent.ACTION_HOVER_EXIT:
                // Handle hover exit event
                Log.d("CustomHoverView", "Hover exit event");
                break;
        }
        return true;
    }
}