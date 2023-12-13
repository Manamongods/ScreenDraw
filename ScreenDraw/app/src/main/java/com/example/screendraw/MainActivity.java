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

    boolean down = false;
    View.OnTouchListener onTouchListener = new View.OnTouchListener() {
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            float pr = event.getPressure();

            int action = event.getActionMasked();
            int eventType = 0;
            switch (action) {
                case MotionEvent.ACTION_DOWN:
                case MotionEvent.ACTION_MOVE:
                    float x = event.getX();
                    float y = event.getY();

                    if (pr > threshold)
                    {
                        if (!down)
                        {
                            down = true;
                            eventType = 1;
                        }
                    }
                    else
                    {
                        if (down)
                        {
                            down = false;
                            eventType = 2;
                        }
                    }

                    break;
                case MotionEvent.ACTION_UP:
                    down = false;
                    eventType = 2;
                    break;
            }
            //Log.v("TAG", eventName + " - X: " + event.getX() + ", Y: " + event.getY() + ", Pressure: " + (pr - threshold)); // + " DISTANCE: " + event.getAxisValue(MotionEvent.AXIS_DISTANCE)
            Data d = new Data();
            d.type = eventType;
            d.x = event.getX();
            d.y = event.getY();
            d.pressure = (pr - threshold);
            try {
                toSend.put(d);
            } catch (InterruptedException e) {
                Log.d(TAG, "Wtf it failed idk??? " + e);
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

        new Thread(new Do()).start();

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
                            Data ts = toSend.take();
                            //Log.d(TAG, "D");
                            dataOutputStream.writeInt(478934687);
                            dataOutputStream.writeInt(Integer.reverseBytes(ts.type));
                            dataOutputStream.writeInt(Integer.reverseBytes(Float.floatToIntBits(ts.x)));
                            dataOutputStream.writeInt(Integer.reverseBytes(Float.floatToIntBits(ts.y)));
                            dataOutputStream.writeInt(Integer.reverseBytes(Float.floatToIntBits(ts.pressure)));
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
