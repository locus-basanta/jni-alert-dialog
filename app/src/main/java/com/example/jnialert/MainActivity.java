/*
 * MainActivity.java
 *
 * The only Activity in the app.  It shows a single button; tapping
 * it calls showAlertFromNative(), which is implemented entirely in C.
 *
 * No dialog logic lives here — it is all in native-alert.c.
 */
package com.example.jnialert;

import android.os.Bundle;
import android.widget.Button;
import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    /* Load the native library built from native-alert.c */
    static {
        System.loadLibrary("native-alert");
    }

    /**
     * Implemented in native-alert.c.
     * Builds and shows an AlertDialog entirely from C via JNI.
     */
    public native void showAlertFromNative();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button button = findViewById(R.id.btn_show_alert);
        button.setOnClickListener(v -> showAlertFromNative());
    }
}
