plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "pt.netpack.xfb.companion"
    compileSdk = 34

    defaultConfig {
        applicationId = "pt.netpack.xfb.companion"
        minSdk = 24
        targetSdk = 34
        versionCode = 1
        versionName = "0.1"

        ndk {
            // The two ABIs any current phone actually is. AGP passes APP_ABI to
            // ndk-build from here, overriding whatever Application.mk asks for,
            // so this is the list that counts.
            abiFilters += listOf("arm64-v8a", "x86_64")
        }
    }

    // ndk-build rather than CMake: the NDK brings its own make and toolchain,
    // so the native side builds offline with nothing extra installed. This SDK
    // has no cmake package and the system CMake is newer than AGP accepts.
    ndkVersion = "25.1.8937393"

    externalNativeBuild {
        ndkBuild {
            path = file("src/main/jni/Android.mk")
        }
    }

    buildFeatures {
        // versionCode is what the update check compares against.
        buildConfig = true
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
    }

}

// Deliberately a plain View/AppCompat app with no Compose, CameraX, OkHttp or
// view binding. Pairing arrives through the xfb:// deep link that XFB's own
// pairing page offers, so no barcode library is needed, and the network layer
// is HttpURLConnection with org.json — both part of the platform.
dependencies {
    implementation("androidx.core:core-ktx:1.12.0")
    implementation("androidx.appcompat:appcompat:1.6.1")
    implementation("com.google.android.material:material:1.11.0")
    implementation("androidx.constraintlayout:constraintlayout:2.1.4")
    implementation("androidx.recyclerview:recyclerview:1.1.0")
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.7.3")

    // Playback. media3-session is what gives the lock screen, the notification
    // and Android Auto their controls without hand-rolling any of it.
    implementation("androidx.media3:media3-exoplayer:1.4.1")
    implementation("androidx.media3:media3-session:1.4.1")
}
