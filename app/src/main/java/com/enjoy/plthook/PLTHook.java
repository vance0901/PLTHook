package com.enjoy.plthook;

import android.os.Handler;
import android.os.HandlerThread;

public class PLTHook {

    public static native void testHook1();
    public static native void testHook2();

    public static native void hookElf();

    private static Handler handler;

    public static void startLoop(){
        hookMemory();
        getHandler().post(new Runnable() {
            @Override
            public void run() {
                dumpMemory();
                getHandler().postDelayed(this,15_000);
            }
        });
    }

    private static Handler getHandler(){
        if (handler==null){
            HandlerThread handlerThread = new HandlerThread("native-leak");
            handlerThread.start();
            handler = new Handler(handlerThread.getLooper());
        }
        return handler;
    }

    private static native  void hookMemory();
    private static native  void dumpMemory();


    public static native  void testMemory();
    public static native  void testMemory2();
}
