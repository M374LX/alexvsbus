## Environment setup

In order to build the game for Android, the Java Development Kit (JDK) and the
Android SDK are required.

For instructions on installing the JDK, see https://openjdk.org/install. The
recommended version at the moment is 17.

The Android SDK can be downloaded from https://developer.android.com/studio.
The command-line tools, without Android Studio, are enough.

The required SDK packages, which can be installed by running
``sdkmanager --install <package>``, are listed below. The versions here are the
ones that have been tested, but later versions might also work. The versions
of some of the packages can be changed by editing the file
``android/app/build.gradle``.

The packages are:
```
build-tools;33.0.2
ndk;25.1.8937393
patcher;v4
platform-tools
platforms;android-34
```

You need to point to the location of the Android SDK. To do so, you have two
alternatives:
1. Set the environment variable ``ANDROID_HOME`` to the location of the SDK.
2. Create the file ``android/local.properties`` and add a line like
   ``sdk.dir=/path/to/android/sdk`` to it.


## Building

The commands from now on should be run from the ``android`` directory. So,
``cd`` into it.

The build type can be debug or release. The command to build a debug .apk file
for Android on Unix systems (including Linux) is:
```
./gradlew assembleDebug
```

Similarly, the command to build a release .apk is:
```
./gradlew assembleRelease
```

On Windows, replace ``./gradlew`` with ``gradlew.bat``.

When running any of the above commands for the first time, the appropriate
Gradle version will be downloaded automatically, which means that an Internet
connection is required.

The built .apk file will be at
``android/app/build/outputs/apk/debug/app-debug.apk`` (debug build) or
``android/app/build/outputs/apk/release/app-release-unsigned.apk`` (release
build).

A debug build is automatically signed with a debug key and is ready to run on
an Android device. A release build, however, can run only after you sign it.


## Signing a release build

The tools needed to sign an Android app are ``keytool``, ``zipalign``, and
``apksigner``. While ``keytool`` is part of the JDK, ``zipalign`` and
``apksigner`` are found in the Android SDK's ``build-tools`` directory.

The steps to sign an app are:

1. If you don't have a key already, generate one with ``keytool``:

   ```keytool -genkey -v -keystore my-release-key.jks -keyalg RSA -keysize 2048 -validity 10000 -alias my-alias```

   The command above prompts for a password and distinguished name fields
   (first and last name, organizational unit, organization, city or locality,
   state or province, and two-letter country code).

2. Align the APK with ``zipalign``:

   ```zipalign -v -p 4 app-release-unsigned.apk app-release-unsigned-aligned.apk```

3. Finally, sign the aligned APK with ``apksigner``:

   ```apksigner sign --ks my-release-key.jks --out app-release.apk app-release-unsigned-aligned.apk```

You can verify the signature by running:

```apksigner verify app-release.apk```

It might show some warnings about files in the ``META-INF`` directory being
unprotected. These files are unimportant and the warnings can be ignored.

For more details, see
https://developer.android.com/build/building-cmdline#sign_cmdline.


## Cleaning

To clean the source tree after the project is built, the following command can
be used on Unix systems (including Linux):
```
./gradlew clean
```

On Windows, replace ``./gradlew`` with ``gradlew.bat``.

In case you experience problems, try manually deleting the ``android/app/build``
directory.

