package com.enjoy.plthook;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

import com.enjoy.plthook.databinding.ActivityMainBinding;

import java.io.BufferedOutputStream;
import java.io.FileDescriptor;
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.io.PrintStream;
import java.io.UnsupportedEncodingException;
import java.util.Properties;

import dalvik.system.PathClassLoader;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'plthook' library on application startup.
    static {
        System.loadLibrary("plthook");
        System.loadLibrary("elf_test");
        System.loadLibrary("memory_hook");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        PathClassLoader classLoader = (PathClassLoader) getClassLoader();
        String enjoy_test = classLoader.findLibrary("enjoy_test");
        Log.i("vance", "onCreate: "+enjoy_test);

//        PLTHook.testHook1();
//        PLTHook.testHook2();
        PLTHook.startLoop();
        PLTHook.testMemory();
        PLTHook.testMemory2();


    }


}