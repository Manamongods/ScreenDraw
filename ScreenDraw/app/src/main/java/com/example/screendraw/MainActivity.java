package com.example.screendraw;

import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.os.Bundle;

import java.io.OutputStream;
import java.io.DataOutputStream;
import android.content.Context;
import android.view.MotionEvent;
import android.view.View;
import android.util.Log;

import android.net.wifi.p2p.WifiP2pManager;
import android.os.Handler;
import android.os.Looper;
import java.net.Socket;
import java.util.Objects;
import java.util.concurrent.LinkedBlockingQueue;

public class MainActivity extends AppCompatActivity {
    float threshold = 0.15f;

    private static final String TAG = "Wi-Fi Direct Example";
    private WifiP2pManager wifiP2pManager;
    private WifiP2pManager.Channel channel;
    private Handler handler = new Handler(Looper.getMainLooper());

    private float startX, startY;

    boolean down = false;
    View.OnTouchListener onTouchListener = new View.OnTouchListener() {
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            int toolType = event.getToolType(0);
            boolean erase = false;
            if (toolType == MotionEvent.TOOL_TYPE_STYLUS)
                erase = false;
            else if (toolType == MotionEvent.TOOL_TYPE_ERASER)
                erase = true;
            else if (toolType == MotionEvent.TOOL_TYPE_FINGER)
                return true;

            int pointerIndex = event.getActionIndex();
            int viewWidth = v.getWidth();
            int viewHeight = v.getHeight();

            switch (event.getActionMasked()) {
                case MotionEvent.ACTION_MOVE:
                    int historySize = event.getHistorySize(); // Get the history size
                    for (int i = 0; i < historySize; i++) {
                        Data d = new Data();
                        d.x = event.getHistoricalX(pointerIndex, i) / viewWidth;
                        d.y = event.getHistoricalY(pointerIndex, i) / viewHeight;
                        /*
                        float dx = d.x-startX;
                        float dy = d.y-startY;
                        float dist = (float)Math.sqrt(dx*dx+dy*dy);
                        if (dist < 0.01) //d.x == startX && d.y == startY)
                            continue; //????????????????????????????????????
                         */
                        d.type = 0;
                        float pr = event.getHistoricalPressure(pointerIndex, i);
                        if (pr > threshold) {
                            if (!down) {
                                startX = d.x;
                                startY = d.y;
                                down = true;
                                d.type = 1;
                            }
                        } else {
                            if (down) {
                                down = false;
                                d.type = 2;
                            }
                        }
                        if (erase)
                            d.type = -d.type;
                        d.pressure = (pr - threshold);
                        try {
                            toSend.put(d);
                        } catch (InterruptedException e) {
                            Log.d(TAG, "Wtf it failed idk??? " + e);
                        }
                    }
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_DOWN:
                    Data d = new Data();
                    d.x = event.getX() / viewWidth;
                    d.y = event.getY() / viewHeight;
                    float pr = event.getPressure();
                    if (event.getActionMasked() == MotionEvent.ACTION_UP) {
                        d.type = 2;
                        startX = -1000000;
                        startY = -1000000;
                    } else {
                        d.type = 0;
                        if (pr > threshold) {
                            if (!down) {
                                startX = d.x;
                                startY = d.y;
                                down = true;
                                d.type = 1;
                            }
                        } else {
                            if (down) {
                                down = false;
                                d.type = 2;
                            }
                        }
                    }
                    if (erase)
                        d.type = -d.type;
                    d.pressure = (pr - threshold);
                    try {
                        toSend.put(d);
                    } catch (InterruptedException e) {
                        Log.d(TAG, "Wtf it failed idk??? " + e);
                    }
                    break;
                default:
                    break;
            }

            return true;
        }
    };

    @SuppressLint("ClickableViewAccessibility")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        wifiP2pManager = (WifiP2pManager) getSystemService(Context.WIFI_P2P_SERVICE);
        channel = wifiP2pManager.initialize(this, getMainLooper(), null);

        Thread t = new Thread(new Do());
        t.setPriority(10);
        t.start();

        HoverView surfaceView = findViewById(R.id.hoverView);
        surfaceView.setOnTouchListener(onTouchListener);
    }

    class Data
    {
        int type;
        float x,y,pressure;
    }
    LinkedBlockingQueue<Data> toSend = new LinkedBlockingQueue<Data>();

    private class Do implements Runnable {
        float prevx = 0, prevy = 0;
        float average = 0;
        int counter = 0;

        @Override
        public void run() {
            while(true) {
                try {
                    Log.d(TAG, "A");

                    Socket socket = new Socket("***REMOVED***", 8987);
                    Log.d(TAG, "B");

                    OutputStream outputStream = socket.getOutputStream();

                    DataOutputStream dataOutputStream = new DataOutputStream(outputStream);

                    Log.d(TAG, "C");

                    //DataOutputStream outputStream = new DataOutputStream(socket.getOutputStream());
                    while (socket.isConnected()) {
                        if (!toSend.isEmpty()) {
                            while(true) {
                                Data ts = toSend.poll();
                                if (ts == null)
                                    break;
                                //Log.d(TAG, "D");
                                dataOutputStream.writeInt(Integer.reverseBytes(478934687));
                                dataOutputStream.writeInt(Integer.reverseBytes(ts.type));
                                dataOutputStream.writeInt(Integer.reverseBytes(Float.floatToIntBits(ts.x)));
                                dataOutputStream.writeInt(Integer.reverseBytes(Float.floatToIntBits(ts.y)));
                                dataOutputStream.writeInt(Integer.reverseBytes(Float.floatToIntBits(ts.pressure)));

                                float dx = ts.x-prevx;
                                float dy = ts.y-prevy;
                                prevx = ts.x;
                                prevy = ts.y;
                                float move = (float)Math.sqrt(dx*dx+dy*dy);
                                float lerp = 0.025f;
                                average = (1.0f - lerp) * average + lerp * move;
                                if (counter++ >= 100)
                                {
                                    counter = 0;
                                    Log.d(TAG, "Smoothness is: " + (1.0f / average));
                                }
                            }
                            dataOutputStream.flush();
                        }
                        //sleep
                    }

                    Log.d(TAG, "E");

                    outputStream.close();
                    socket.close();
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }
}
