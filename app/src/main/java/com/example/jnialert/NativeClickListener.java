/*
 * NativeClickListener.java
 *
 * A minimal Java shim that bridges JNI and Java interfaces.
 *
 * WHY THIS EXISTS:
 * JNI cannot directly implement a Java interface from C.  Android's
 * AlertDialog.Builder.setPositiveButton() requires a
 * DialogInterface.OnClickListener — a Java interface.  So we need this
 * tiny class to receive the click and forward it to native code.
 *
 * HOW IT WORKS:
 * 1. C code creates a NativeClickListener with an integer id (0=OK, 1=Cancel).
 * 2. C passes it to setPositiveButton / setNegativeButton.
 * 3. When the user taps the button, Android calls onClick() on this object.
 * 4. onClick() calls the native method nativeCallback(id).
 * 5. nativeCallback() is implemented in native-alert.c and logs the result.
 */
package com.example.jnialert;

import android.content.DialogInterface;
import android.view.View;

public class NativeClickListener implements DialogInterface.OnClickListener, View.OnClickListener {

    private final int id;

    /** @param id identifies which button this listener is attached to. */
    public NativeClickListener(int id) {
        this.id = id;
    }

    /** Called when a dialog button is pressed. */
    @Override
    public void onClick(DialogInterface dialog, int which) {
        nativeCallback(id);
    }

    /** Called when a View is clicked (included for completeness). */
    @Override
    public void onClick(View v) {
        nativeCallback(id);
    }

    /** Implemented in native-alert.c — receives the button id. */
    private native void nativeCallback(int id);
}
