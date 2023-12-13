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
import android.os.Bundle;
import android.view.View;
import android.widget.Button;

import android.view.InputDevice;


import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.util.Log;

import java.util.HashMap;

public class MainActivity extends AppCompatActivity {
    float threshold = 0.05f;

    private UsbManager usbManager;
    private UsbAccessory usbAccessory;
    private ParcelFileDescriptor fileDescriptor;
    private FileOutputStream outputStream;

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
            String dataToSend = eventName + " - X: " + event.getX() + ", Y: " + event.getY() + ", Pressure: " + (pr - threshold);
            sendDataOverUsb(dataToSend);
            return true;
        }
    };

    @SuppressLint("ClickableViewAccessibility")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);

        HoverView surfaceView = findViewById(R.id.hoverView);

        surfaceView.setOnTouchListener(onTouchListener);


        Context context = surfaceView.getContext();

        /*
        UsbManager usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);

        if (usbManager != null) {
            HashMap<String, UsbDevice> deviceList = usbManager.getDeviceList();

            Log.d("TAG", "IDK");
            for (UsbDevice device : deviceList.values()) {
                int vendorID = device.getVendorId();
                int productID = device.getProductId();

                Log.d("TAG", "Device found: Vendor ID (VID) = " + vendorID + ", Product ID (PID) = " + productID);
            }
        } else {
            Log.e("TAG", "USB Manager is null.");
        }
         */
    }

    private void sendDataOverUsb(String data) {
        if (outputStream != null) {
            try {
                byte[] buffer = data.getBytes();
                outputStream.write(buffer);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();

        UsbAccessory[] accessories = usbManager.getAccessoryList();
        if (accessories != null) {
            usbAccessory = accessories[0];
            fileDescriptor = usbManager.openAccessory(usbAccessory);
            if (fileDescriptor != null) {
                FileDescriptor fd = fileDescriptor.getFileDescriptor();
                outputStream = new FileOutputStream(fd);
            }
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        closeUsbAccessory();
    }

    private void closeUsbAccessory() {
        try {
            if (outputStream != null) {
                outputStream.close();
            }
            if (fileDescriptor != null) {
                fileDescriptor.close();
            }
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            usbAccessory = null;
            fileDescriptor = null;
            outputStream = null;
        }
    }
}