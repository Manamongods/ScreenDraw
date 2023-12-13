package com.example.screendraw;

import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.os.Bundle;

import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbDeviceConnection;
import java.io.OutputStream;
import android.content.Context;
import android.view.MotionEvent;
import android.view.SurfaceView;
import android.view.View;
import java.io.FileDescriptor;
import java.io.FileOutputStream;
import java.io.IOException;
import android.os.ParcelFileDescriptor;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import android.content.Context;
import android.net.wifi.p2p.WifiP2pConfig;
import android.net.wifi.p2p.WifiP2pDevice;
import android.net.wifi.p2p.WifiP2pInfo;
import android.net.wifi.p2p.WifiP2pManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import androidx.appcompat.app.AppCompatActivity;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Objects;

public class MainActivity extends AppCompatActivity {
    float threshold = 0.05f;

    private static final String TAG = "Wi-Fi Direct Example";
    private WifiP2pManager wifiP2pManager;
    private WifiP2pManager.Channel channel;
    private ServerSocket serverSocket;
    private Handler handler = new Handler(Looper.getMainLooper());

    boolean down = false;
    View.OnTouchListener onTouchListener = new View.OnTouchListener() {
        @Override
        public boolean onTouch(View v, MotionEvent event) {
            float pr = event.getPressure();

            int action = event.getActionMasked();
            String eventName = "M";
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
                            eventName = "D";
                        }
                    }
                    else
                    {
                        if (down)
                        {
                            down = false;
                            eventName = "U";
                        }
                    }

                    break;
                case MotionEvent.ACTION_UP:
                    down = false;
                    eventName = "U";
                    break;
            }
            Log.v("TAG", eventName + " - X: " + event.getX() + ", Y: " + event.getY() + ", Pressure: " + (pr - threshold)); // + " DISTANCE: " + event.getAxisValue(MotionEvent.AXIS_DISTANCE)
            toSend = eventName + " - X: " + event.getX() + ", Y: " + event.getY() + ", Pressure: " + (pr - threshold);

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

        // Discover peers and initiate connection
        discoverPeers();

        HoverView surfaceView = findViewById(R.id.hoverView);
        surfaceView.setOnTouchListener(onTouchListener);
    }

    private void discoverPeers() {
        wifiP2pManager.discoverPeers(channel, new WifiP2pManager.ActionListener() {
            @Override
            public void onSuccess() {
                Log.d(TAG, "Discovery started successfully.");
            }

            @Override
            public void onFailure(int reasonCode) {
                Log.d(TAG, "Discovery failed. Reason: " + reasonCode);
            }
        });
    }

    private void connectToDevice(final WifiP2pDevice device) {
        WifiP2pConfig config = new WifiP2pConfig();
        config.deviceAddress = device.deviceAddress;

        wifiP2pManager.connect(channel, config, new WifiP2pManager.ActionListener() {
            @Override
            public void onSuccess() {
                Log.d(TAG, "Connection to " + device.deviceName + " initiated.");
            }

            @Override
            public void onFailure(int reason) {
                Log.d(TAG, "Connection to " + device.deviceName + " failed. Reason: " + reason);
            }
        });
    }

    private void startServerSocket() {
        try {
            serverSocket = new ServerSocket(8888);
            new Thread(new ServerThread()).start();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    String toSend;

    private class ServerThread implements Runnable {
        String sent;

        @Override
        public void run() {
            try {
                Log.d(TAG, "ServerSocket started. Waiting for a client...");

                Socket clientSocket = serverSocket.accept();
                Log.d(TAG, "Client connected.");

                OutputStream outputStream = clientSocket.getOutputStream();
                while(true) {
                    if (!Objects.equals(toSend, sent)) {
                        sent = toSend;
                        byte[] buffer = toSend.getBytes(); //String.valueOf(event.getX()+","+event.getY()+","+(pr - threshold))
                        outputStream.write(buffer);
                        outputStream.flush();
                    }
                }

                //outputStream.close();
                //clientSocket.close();
                //serverSocket.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        startServerSocket();
    }

    @Override
    protected void onPause() {
        super.onPause();
        try {
            if (serverSocket != null) {
                serverSocket.close();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}