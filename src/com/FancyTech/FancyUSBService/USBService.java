package com.FancyTech.FancyUSBService;

import android.app.IntentService;
import android.content.Intent;
import android.content.Context;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.util.Log;
import android.widget.Toast;

import java.util.HashMap;
import java.util.Iterator;

public class USBService extends IntentService {

  
	static String TAG = "FancyUSBService";
	
    private UsbInterface gyroDataIntf_;
    private UsbInterface hidInputIntf_;
    private UsbEndpoint hidEndpoint_;
    private UsbDeviceConnection connection;
    private boolean forceClaim = true;
    

    public USBService() {
        super("USBService");

        Log.d(TAG, "USBService");
    }    
    

    @Override
    protected void onHandleIntent(Intent intent) {
        Toast.makeText(this, "service starting", Toast.LENGTH_SHORT).show();

        Log.d(TAG, "onHandleIntent 111 " + intent.getAction());

        InitUsbDevice();        
        
//        if (intent != null) {
//            final String action = intent.getAction();
//            if (ACTION_FOO.equals(action)) {
//                final String param1 = intent.getStringExtra(EXTRA_PARAM1);
//                final String param2 = intent.getStringExtra(EXTRA_PARAM2);
//                handleActionFoo(param1, param2);
//            } else if (ACTION_BAZ.equals(action)) {
//                final String param1 = intent.getStringExtra(EXTRA_PARAM1);
//                final String param2 = intent.getStringExtra(EXTRA_PARAM2);
//                handleActionBaz(param1, param2);
//            }
//        }
    }


    protected void InitUsbDevice() {

        UsbManager manager = (UsbManager) getSystemService(Context.USB_SERVICE);
        HashMap<String, UsbDevice> deviceList = manager.getDeviceList();

        Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();

        while(deviceIterator.hasNext()){
            UsbDevice device = deviceIterator.next();

            int deviceType = 0;

            if (device.getVendorId() == 10291 && device.getProductId() == 1) {
                deviceType = 1;
            } else if (	(device.getVendorId() == 1155 && device.getProductId() == 22336) ||
                    (device.getVendorId() == 1155 && device.getProductId() == 22352) ||
                    (device.getVendorId() == 949 && device.getProductId() == 1)
                    ) {
                deviceType = 0;
            } else {
                continue;
            }

            int nInterface = device.getInterfaceCount();
            if (nInterface < 1)	{
                continue;
            }

            Log.d(TAG, "usb device found interface count " + nInterface);
            for (int i = 0; i < nInterface; i++) {
                UsbInterface intf = device.getInterface(i);
                String str = String.format("interface %d %d.%d.%d", intf.getId(), intf.getInterfaceClass(),
                        intf.getInterfaceSubclass(), intf.getInterfaceProtocol());
                Log.i(TAG, str);

                if (intf.getInterfaceClass() == 10 && intf.getInterfaceSubclass() == 0 && intf.getInterfaceProtocol() == 0) {
                    gyroDataIntf_ = intf;
                    Log.i(TAG, "found cdc interface " + intf.toString());
                }

                if (intf.getInterfaceClass() == 0x03 && intf.getInterfaceSubclass() == 0 && intf.getInterfaceProtocol() == 0) {
                    this.hidInputIntf_ = intf;
                    Log.i(TAG, "found hid interface " + intf.toString());
                }

                int numEndpoint = intf.getEndpointCount();
                for (int j = 0; j < intf.getEndpointCount(); j++) {
                    UsbEndpoint endpoint = intf.getEndpoint(j);
                    String endpointInfo = String.format("endpoint %d addr 0x%x number %d attri %d", j,
                            endpoint.getAddress(), endpoint.getEndpointNumber(), endpoint.getAttributes());
                    Log.i(TAG, endpointInfo);
                    if (this.hidInputIntf_ == intf) {
                        this.hidEndpoint_ = endpoint;
                    }
                }
            }

            connection = manager.openDevice(device);
            if (gyroDataIntf_ != null) {
                boolean bRes = connection.claimInterface(gyroDataIntf_, forceClaim);
                Log.i(TAG, String.format("claim gyro data interface %s", bRes ? "succeeded" : "failed"));
            }

            if (hidInputIntf_ != null) {
                boolean bRes = connection.claimInterface(hidInputIntf_, forceClaim);
                Log.i(TAG, String.format("claim hid input interface %s", bRes ? "succeeded" : "failed"));
            }

            int fd = connection.getFileDescriptor();
            Log.i(TAG, String.format("FileDescriptor %d", fd));
            
            setupUsbDevice(fd);

            //setupUsbDevice(fd, deviceType, false);

            // use native thread
            //StartUsbIOThread();

            // 0xa1 : device to host / Type : class / Recipien : interface
            // request : 1 (get report)
            // value : 0x300 HID_FEATURE  0x3 FEATURE_CALIBRATE

            //size = connection.controlTransfer(0xa1, 1, 0x303, 0, bytes, 256, 0);
            //addText("control read " + size);

            //size = connection.controlTransfer(0xa1, 1, 0x304, 0, bytes, 256, 0);
            //addText("control read " + size);

        }

    }

    void HandleHidInputData(byte[] data, int size)
    {
        if (size == 0){
            return;
        }

        if (data[6] == (byte)0xf) {
            //VrActivity.nativeJoypadAxis(appPtr, 0, -0.5f, 0, 0);
        }
        if (data[6] == (byte)0xf0) {
            //VrActivity.nativeJoypadAxis(appPtr, 0, 0, 0, 0);
        }

//		if (data[6] == (byte)0xf) {
//			this.buttonDown_ = true;
//			VrActivity.nativeKeyEvent(appPtr, KeyEvent.KEYCODE_FORWARD, true, 1);
//		} else if (buttonDown_ && data[6] == ) {
//			this.buttonDown_ = false;
//			VrActivity.nativeKeyEvent(appPtr, KeyEvent.KEYCODE_FORWARD, false, 1);
//			Log.i(TAG, "button clicked!!!");
//		}
    }

//    void StartUsbIOThread() {
//        bytes = new byte[256];
//
//        new Thread(new Runnable() {
//            public void run() {
//
//                while (true) {
//                    int size = connection.bulkTransfer(hidEndpoint_, bytes, bytes.length, 0);
//                    HandleHidInputData(bytes, size);
//                    //PushData(bytes, size);
//                }
//            }
//        }).start();
//    }
//
//
//    public native void PushData(byte[] buffer, int length);
//    public native void setupUsbDevice(int fd, int deviceType, boolean startThread);
    
    public native void setupUsbDevice(int fd);
}
