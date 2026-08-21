import java.util.Properties

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

/**
 * Release signing details, kept out of the repository.
 *
 * Android will not install an unsigned APK, and the desktop hands this one out
 * itself rather than a store doing it, so the release build has to be signed
 * here. Create android/keystore.properties with storeFile, storePassword,
 * keyAlias and keyPassword; without it the release build still runs and says
 * plainly that what comes out cannot be installed.
 */
val signingProperties = Properties().apply {
    val file = rootProject.file("keystore.properties")
    if (file.exists()) file.inputStream().use { load(it) }
}
val hasSigningKey = signingProperties.getProperty("storeFile") != null

android {
    namespace = "pt.netpack.xfb.companion"
    compileSdk = 34

    defaultConfig {
        applicationId = "pt.netpack.xfb.companion"
        minSdk = 24
        targetSdk = 34
        // The update check compares versionCode; versionName is what the
        // desktop's pairing page and the app's own update prompt show.
        versionCode = 2
        versionName = "1.0"

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

    signingConfigs {
        if (hasSigningKey) {
            create("release") {
                storeFile = rootProject.file(signingProperties.getProperty("storeFile"))
                storePassword = signingProperties.getProperty("storePassword")
                keyAlias = signingProperties.getProperty("keyAlias")
                keyPassword = signingProperties.getProperty("keyPassword")
            }
        }
    }

    buildTypes {
        release {
            // R8 is deliberately off. Nothing here is short of space, the app
            // is handed out by the desk rather than downloaded over and over,
            // and shrinking a Media3 player is a class of bug nobody needs for
            // a saving nobody will notice.
            isMinifyEnabled = false

            if (hasSigningKey) {
                signingConfig = signingConfigs.getByName("release")
            } else {
                logger.warn(
                    "XFB companion: no android/keystore.properties, so the " +
                    "release APK will be unsigned and Android will refuse to " +
                    "install it. See RELEASING.md."
                )
            }
        }
    }

    lint {
        // Lint's own artifacts are not in the Gradle cache, so lintVital — which
        // the release build runs — cannot resolve them offline and fails before
        // anything is packaged. Run `lint` deliberately when there is a network.
        checkReleaseBuilds = false
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
