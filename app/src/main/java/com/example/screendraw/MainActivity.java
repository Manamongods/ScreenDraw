package com.example.screendraw;

import androidx.appcompat.app.AppCompatActivity;

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

public class MainActivity extends AppCompatActivity {

    private UsbManager usbManager;
    private UsbAccessory usbAccessory;
    private ParcelFileDescriptor fileDescriptor;
    private FileOutputStream outputStream;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        usbManager = (UsbManager) getSystemService(Context.USB_SERVICE);

        HoverView surfaceView = findViewById(R.id.hoverView);

        surfaceView.setOnHoverListener(new View.OnHoverListener() {
            @Override
            public boolean onHover(View v, MotionEvent event) {
                // Handle hover input here
                Log.v("TAG", "Hover - X: " + event.getX() + ", Y: " + event.getY() + ", Pressure: " + event.getPressure());
                String dataToSend = "Hover - X: " + event.getX() + ", Y: " + event.getY() + ", Pressure: " + event.getPressure();
                sendDataOverUsb(dataToSend);
                return true;
            }
        });

        surfaceView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                /*
                InputDevice.MotionRange x =event.getDevice().getMotionRange(MotionEvent.AXIS_X);
                InputDevice.MotionRange y =event.getDevice().getMotionRange(MotionEvent.AXIS_Y);
                InputDevice.MotionRange z =event.getDevice().getMotionRange(MotionEvent.AXIS_DISTANCE);
                Log.v("TAG", "X:" + x.getMin() + " " + x.getMax());
                Log.v("TAG", "y:" + y.getMin() + " " + y.getMax());
                Log.v("TAG", "z:" + z.getMin() + " " + z.getMax());
                 */

                // Handle touch input here
                MotionEvent.PointerCoords coords = new MotionEvent.PointerCoords();
                event.getPointerCoords(0, coords);
                Log.v("TAG", "IDK - X: " + coords.x + ", Y: " + coords.y + ", orientation: " + coords.orientation);

                Log.v("TAG", "Touch - X: " + event.getX() + ", Y: " + event.getY() + ", Pressure: " + event.getPressure() + " DISTANCE: " + event.getAxisValue(MotionEvent.AXIS_DISTANCE));
                String dataToSend = "Touch - X: " + event.getX() + ", Y: " + event.getY() + ", Pressure: " + event.getPressure();
                sendDataOverUsb(dataToSend);
                return true;
            }
        });

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