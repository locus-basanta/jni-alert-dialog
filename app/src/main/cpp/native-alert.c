/*
 * native-alert.c
 *
 * Builds an Android AlertDialog entirely from C via JNI.
 *
 * This demonstrates that you can construct Java UI objects, call their
 * methods, and wire up callbacks — all without writing any dialog logic
 * in Java.  The one thing JNI *cannot* do is implement a Java interface
 * directly, so we use a tiny Java shim (NativeClickListener) for the
 * button callbacks.
 *
 * Flow:
 *   1. Java calls showAlertFromNative() on the UI thread.
 *   2. This C function builds an AlertDialog.Builder via JNI calls.
 *   3. It creates NativeClickListener objects (Java shim) for OK/Cancel.
 *   4. It calls builder.show() to display the dialog.
 *   5. When the user taps a button, the shim calls nativeCallback().
 *   6. nativeCallback() logs the result from C.
 */

#include <jni.h>
#include <android/log.h>

/* Tag used for __android_log_print output (visible in Logcat). */
#define LOG_TAG "NativeAlert"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* --------------------------------------------------------------------------
 * showAlertFromNative
 *
 * Called from MainActivity.  `thiz` is the Activity (our Context).
 * Every JNI step is commented so readers can follow the Java equivalent.
 * ----------------------------------------------------------------------- */
JNIEXPORT void JNICALL
Java_com_example_jnialert_MainActivity_showAlertFromNative(JNIEnv *env,
                                                           jobject thiz)
{
    LOGI("showAlertFromNative: building AlertDialog from C…");

    /* ---- Step 1: Find the AlertDialog.Builder class ----
     *
     * Java equivalent:
     *   AlertDialog.Builder builder = new AlertDialog.Builder(this);
     *
     * In JNI, inner classes use '$' as separator.
     */
    jclass builderClass = (*env)->FindClass(env,
            "android/app/AlertDialog$Builder");
    if (!builderClass) {
        LOGE("FindClass failed for AlertDialog$Builder");
        return;
    }

    /* ---- Step 2: Get the Builder(Context) constructor ---- */
    jmethodID builderCtor = (*env)->GetMethodID(env, builderClass,
            "<init>", "(Landroid/content/Context;)V");
    if (!builderCtor) {
        LOGE("GetMethodID failed for Builder constructor");
        return;
    }

    /* ---- Step 3: Create a new Builder, passing the Activity as Context ---- */
    jobject builder = (*env)->NewObject(env, builderClass, builderCtor, thiz);
    if (!builder) {
        LOGE("NewObject failed for Builder");
        return;
    }

    /* ---- Step 4: builder.setTitle("Hello from C!") ----
     *
     * setTitle returns the Builder itself (for chaining), so the
     * return type in the JNI signature is the Builder class.
     */
    jmethodID setTitle = (*env)->GetMethodID(env, builderClass,
            "setTitle",
            "(Ljava/lang/CharSequence;)Landroid/app/AlertDialog$Builder;");
    if (!setTitle) {
        LOGE("GetMethodID failed for setTitle");
        return;
    }

    jstring title = (*env)->NewStringUTF(env, "Hello from C!");
    (*env)->CallObjectMethod(env, builder, setTitle, title);

    /* ---- Step 5: builder.setMessage(…) ---- */
    jmethodID setMessage = (*env)->GetMethodID(env, builderClass,
            "setMessage",
            "(Ljava/lang/CharSequence;)Landroid/app/AlertDialog$Builder;");
    if (!setMessage) {
        LOGE("GetMethodID failed for setMessage");
        return;
    }

    jstring message = (*env)->NewStringUTF(env,
            "This dialog was built entirely in C.\nPick an option:");
    (*env)->CallObjectMethod(env, builder, setMessage, message);

    /* ---- Step 6: Create NativeClickListener objects ----
     *
     * JNI cannot implement a Java interface on its own, so we use
     * NativeClickListener — a tiny Java class that implements
     * DialogInterface.OnClickListener and delegates to a native method.
     *
     * We create two instances:
     *   okListener     (id = 0)   →  for the positive button
     *   cancelListener (id = 1)   →  for the negative button
     */
    jclass listenerClass = (*env)->FindClass(env,
            "com/example/jnialert/NativeClickListener");
    if (!listenerClass) {
        LOGE("FindClass failed for NativeClickListener");
        return;
    }

    jmethodID listenerCtor = (*env)->GetMethodID(env, listenerClass,
            "<init>", "(I)V");
    if (!listenerCtor) {
        LOGE("GetMethodID failed for NativeClickListener constructor");
        return;
    }

    /* id=0 → OK button */
    jobject okListener = (*env)->NewObject(env, listenerClass,
            listenerCtor, (jint)0);
    /* id=1 → Cancel button */
    jobject cancelListener = (*env)->NewObject(env, listenerClass,
            listenerCtor, (jint)1);

    if (!okListener || !cancelListener) {
        LOGE("NewObject failed for listener(s)");
        return;
    }

    /* ---- Step 7: builder.setPositiveButton("OK", okListener) ----
     *
     * Signature: setPositiveButton(CharSequence, DialogInterface.OnClickListener)
     *            → returns Builder
     */
    jmethodID setPositiveButton = (*env)->GetMethodID(env, builderClass,
            "setPositiveButton",
            "(Ljava/lang/CharSequence;Landroid/content/DialogInterface$OnClickListener;)"
            "Landroid/app/AlertDialog$Builder;");
    if (!setPositiveButton) {
        LOGE("GetMethodID failed for setPositiveButton");
        return;
    }

    jstring okText = (*env)->NewStringUTF(env, "OK");
    (*env)->CallObjectMethod(env, builder, setPositiveButton,
            okText, okListener);

    /* ---- Step 8: builder.setNegativeButton("Cancel", cancelListener) ---- */
    jmethodID setNegativeButton = (*env)->GetMethodID(env, builderClass,
            "setNegativeButton",
            "(Ljava/lang/CharSequence;Landroid/content/DialogInterface$OnClickListener;)"
            "Landroid/app/AlertDialog$Builder;");
    if (!setNegativeButton) {
        LOGE("GetMethodID failed for setNegativeButton");
        return;
    }

    jstring cancelText = (*env)->NewStringUTF(env, "Cancel");
    (*env)->CallObjectMethod(env, builder, setNegativeButton,
            cancelText, cancelListener);

    /* ---- Step 9: builder.show() ----
     *
     * This both creates the AlertDialog *and* displays it.
     * Return type is AlertDialog.
     */
    jmethodID show = (*env)->GetMethodID(env, builderClass,
            "show", "()Landroid/app/AlertDialog;");
    if (!show) {
        LOGE("GetMethodID failed for show");
        return;
    }

    (*env)->CallObjectMethod(env, builder, show);

    LOGI("showAlertFromNative: dialog shown!");

    /* ---- Clean up local references ----
     *
     * Strictly optional inside a JNI call (they are freed on return),
     * but good practice, especially if you create many refs in a loop.
     */
    (*env)->DeleteLocalRef(env, title);
    (*env)->DeleteLocalRef(env, message);
    (*env)->DeleteLocalRef(env, okText);
    (*env)->DeleteLocalRef(env, cancelText);
    (*env)->DeleteLocalRef(env, okListener);
    (*env)->DeleteLocalRef(env, cancelListener);
    (*env)->DeleteLocalRef(env, builder);
}

/* --------------------------------------------------------------------------
 * nativeCallback
 *
 * Called from NativeClickListener.onClick() when the user taps a button.
 * `id` tells us which button: 0 = OK, 1 = Cancel.
 * ----------------------------------------------------------------------- */
JNIEXPORT void JNICALL
Java_com_example_jnialert_NativeClickListener_nativeCallback(JNIEnv *env,
                                                              jobject thiz,
                                                              jint id)
{
    switch (id) {
        case 0:
            LOGI("nativeCallback: user pressed OK (id=%d)", id);
            break;
        case 1:
            LOGI("nativeCallback: user pressed Cancel (id=%d)", id);
            break;
        default:
            LOGI("nativeCallback: unknown button (id=%d)", id);
            break;
    }
}
