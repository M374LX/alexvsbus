package com.github.m374lx.alexvsbus;

import android.app.NativeActivity;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;

public class NativeLoader extends NativeActivity {
    private static NativeLoader instance;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        instance = this;
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setImmersiveMode();
        System.loadLibrary("main");
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) setImmersiveMode();
    }

    private void setImmersiveMode() {
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_FULLSCREEN              |
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION         |
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY        |
            View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN       |
            View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION  |
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        );
    }

    public static NativeLoader getInstance() {
        return instance;
    }
}