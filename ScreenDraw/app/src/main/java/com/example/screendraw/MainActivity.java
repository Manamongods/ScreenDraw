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
import java.util.concurrent.LinkedBlockingQueue;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.Button;
import android.widget.EditText;

public class MainActivity extends AppCompatActivity {
    int BATCH = 8;

    private EditText ipAddressET, portET;
    private SharedPreferences sharedPreferences;
    private static final String SHARED_PREFS = "screenDrawPrefs", IP_ADDRESS = "ip", PORT = "port";

    String ip = "";
    int port = 8987;

    private static final String TAG = "ScreenDraw";
    private WifiP2pManager wifiP2pManager;
    private WifiP2pManager.Channel channel;
    private Handler handler = new Handler(Looper.getMainLooper());

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
                        d.type = erase ? 2 : 1;
                        d.pressure = event.getHistoricalPressure(pointerIndex, i);
                        try {
                            toSend.put(d);
                        } catch (InterruptedException e) {
                            Log.d(TAG, "Wtf it failed idk??? " + e);
                        }
                    }
                    // (Note there's no break; here)
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_DOWN:
                    Data d = new Data();
                    d.x = event.getX() / viewWidth;
                    d.y = event.getY() / viewHeight;
                    if (event.getActionMasked() == MotionEvent.ACTION_UP)
                        d.type = 0;
                    else
                        d.type = erase ? 2 : 1;
                    d.pressure = event.getPressure();
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
        surfaceView.setVisibility(View.GONE);

        View decorView = getWindow().getDecorView();
        int uiOptions = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;
        decorView.setSystemUiVisibility(uiOptions);

        ipAddressET = findViewById(R.id.ipAddress);
        portET = findViewById(R.id.port);
        Button button = findViewById(R.id.button);
        sharedPreferences = getSharedPreferences(SHARED_PREFS, MODE_PRIVATE);
        ipAddressET.setText(sharedPreferences.getString(IP_ADDRESS, "000.000.0.000"));
        portET.setText(sharedPreferences.getString(PORT, "8987"));

        button.setOnClickListener(v -> {
            ip = ipAddressET.getText().toString();
            port = Integer.parseInt(portET.getText().toString());

            SharedPreferences.Editor editor = sharedPreferences.edit();
            editor.putString(IP_ADDRESS, ip);
            editor.putString(PORT, Integer.toString(port));
            editor.apply();

            ipAddressET.clearFocus();
            portET.clearFocus();

            ipAddressET.setVisibility(View.GONE);
            portET.setVisibility(View.GONE);
            button.setVisibility(View.GONE);

            surfaceView.setVisibility(View.VISIBLE);
            surfaceView.setOnTouchListener(onTouchListener);
        });
    }

    class Data
    {
        int type;
        float x, y, pressure;
    }
    LinkedBlockingQueue<Data> toSend = new LinkedBlockingQueue<Data>();

    private class Do implements Runnable {
        private float prevx = 0, prevy = 0;
        private float average = 0;
        private int counter = 0;

        @Override
        public void run() {
            while(true) {
                try {
                    Log.d(TAG, "A");
                    Socket socket = new Socket(ip, port);
                    Log.d(TAG, "B");
                    OutputStream outputStream = socket.getOutputStream();
                    DataOutputStream dataOutputStream = new DataOutputStream(outputStream);
                    Log.d(TAG, "C");
                    while (socket.isConnected()) {
                        if (!toSend.isEmpty()) {
                            for (int i = 0; i < BATCH; i++) {
                                Data ts = toSend.poll();
                                if (ts == null)
                                    break;
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
                                if (counter++ >= 100) {
                                    counter = 0;
                                    Log.d(TAG, "Smoothness is: " + (1.0f / average));
                                }
                            }
                            dataOutputStream.flush();
                        }
                        try {
                            Thread.sleep(1);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                    Log.d(TAG, "E");
                    outputStream.close();
                    socket.close();
                } catch (Exception e) {
                    e.printStackTrace();
                }

                try {
                    Thread.sleep(1000); // idk
                } catch (InterruptedException e) {
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
