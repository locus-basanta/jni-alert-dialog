# JNI Alert Dialog

> An Android AlertDialog built **entirely from C** via JNI — with click handlers.

---

## Why?

Android's `AlertDialog.Builder` is a Java API.  Normally you'd build dialogs in
Java or Kotlin.  But what if your app's core logic lives in C and you want to
trigger UI from native code?

JNI lets you call *any* Java method from C — `FindClass`, `GetMethodID`,
`CallObjectMethod` — so you can construct a `Builder`, set its title, message,
and buttons, and call `show()`, all from a `.c` file.

**The catch:** JNI cannot implement Java interfaces directly.  When you call
`setPositiveButton("OK", listener)`, the `listener` must be an object that
implements `DialogInterface.OnClickListener`.  You can't create that in C.

The solution is a ~12-line Java shim (`NativeClickListener`) that implements the
interface and immediately calls back into C.  That's the *only* Java needed
beyond `MainActivity`.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│  User taps "Show Alert from JNI"                                    │
└──────────────────────┬──────────────────────────────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────────────────────────────┐
│  MainActivity.java                                                   │
│    └─ button.onClick → showAlertFromNative()   (native method)       │
└──────────────────────┬───────────────────────────────────────────────┘
                       │  JNI call
                       ▼
┌──────────────────────────────────────────────────────────────────────┐
│  native-alert.c                                                      │
│    1. FindClass("AlertDialog$Builder")                               │
│    2. NewObject(builder, context)                                    │
│    3. CallObjectMethod → setTitle("Hello from C!")                   │
│    4. CallObjectMethod → setMessage("…")                             │
│    5. NewObject(NativeClickListener, id=0)   ← OK button             │
│    6. NewObject(NativeClickListener, id=1)   ← Cancel button         │
│    7. CallObjectMethod → setPositiveButton("OK", listener0)          │
│    8. CallObjectMethod → setNegativeButton("Cancel", listener1)      │
│    9. CallObjectMethod → show()                                      │
└──────────────────────┬───────────────────────────────────────────────┘
                       │  Dialog appears
                       ▼
┌──────────────────────────────────────────────────────────────────────┐
│  User taps OK or Cancel                                              │
└──────────────────────┬───────────────────────────────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────────────────────────────┐
│  NativeClickListener.java                                            │
│    onClick(dialog, which) → nativeCallback(id)   (native method)     │
└──────────────────────┬───────────────────────────────────────────────┘
                       │  JNI call
                       ▼
┌──────────────────────────────────────────────────────────────────────┐
│  native-alert.c :: nativeCallback(id)                                │
│    → __android_log_print("user pressed OK/Cancel")                   │
└──────────────────────────────────────────────────────────────────────┘
```

---

## Project Structure

```
jni-alert-dialog/
├── README.md                       ← You are here
├── LICENSE                         ← MIT
├── .gitignore
├── build.gradle                    ← Top-level: AGP + Kotlin plugin
├── settings.gradle                 ← includes :app
├── gradle.properties               ← AndroidX, Jetifier
└── app/
    ├── build.gradle                ← compileSdk 34, CMake, dependencies
    └── src/main/
        ├── AndroidManifest.xml
        ├── java/com/example/jnialert/
        │   ├── MainActivity.java       ← Button + loadLibrary
        │   └── NativeClickListener.java← Interface shim → native
        ├── cpp/
        │   ├── CMakeLists.txt          ← Builds libnative-alert.so
        │   └── native-alert.c          ← ALL dialog logic lives here
        └── res/
            ├── layout/activity_main.xml
            └── values/
                ├── strings.xml
                ├── colors.xml
                └── themes.xml
```

---

## How It Works

| File | Role |
|------|------|
| **`native-alert.c`** | The star of the show. Builds an `AlertDialog` from scratch using only JNI calls: `FindClass`, `GetMethodID`, `NewObject`, `CallObjectMethod`. Also handles the button callback via `nativeCallback()`. |
| **`NativeClickListener.java`** | Tiny (~12 lines of logic) Java class that implements `DialogInterface.OnClickListener` and `View.OnClickListener`. Its sole job is to receive `onClick()` and call `native void nativeCallback(int id)`. |
| **`MainActivity.java`** | Loads `libnative-alert.so`, sets up one button, and calls `showAlertFromNative()`. Contains zero dialog logic. |
| **`CMakeLists.txt`** | Tells the NDK to compile `native-alert.c` into a shared library and link Android's log library. |

---

## The Key Insight

JNI is powerful — you can call any Java method, create any Java object, and
access any field from C.  But there is **one thing you cannot do**: implement a
Java interface in C.

Java interfaces require a class that declares `implements SomeInterface`, and
JNI's `DefineClass` is not supported on Android's ART runtime.  So when an
Android API demands a callback object (like `OnClickListener`), you need a
real Java class.

`NativeClickListener` is that class.  It is the thinnest possible bridge:

```java
public class NativeClickListener implements DialogInterface.OnClickListener {
    private final int id;
    public NativeClickListener(int id) { this.id = id; }
    public void onClick(DialogInterface d, int w) { nativeCallback(id); }
    private native void nativeCallback(int id);
}
```

Everything else — constructing the dialog, setting its title, message, buttons,
and showing it — happens in C.

---

## Build Instructions

### Android Studio (recommended)

1. Open the `jni-alert-dialog/` folder in Android Studio.
2. Let Gradle sync (it will download the NDK if needed).
3. Click **Run ▶** on an emulator or device (API 24+).

### Command Line

```bash
cd jni-alert-dialog/
./gradlew assembleDebug
# APK → app/build/outputs/apk/debug/app-debug.apk
adb install app/build/outputs/apk/debug/app-debug.apk
```

### What to look for

- Tap **"Show Alert from JNI"**.
- A dialog appears with title **"Hello from C!"** and two buttons.
- Tap OK or Cancel.
- Check Logcat for `NativeAlert`:
  ```
  I/NativeAlert: nativeCallback: user pressed OK (id=0)
  ```

---

## Requirements

- Android Studio Hedgehog (2023.1.1) or later
- Android SDK 34
- NDK (any recent version; CMake 3.22+)
- Device or emulator running Android 7.0+ (API 24)

---

## License

MIT — see [LICENSE](LICENSE).
