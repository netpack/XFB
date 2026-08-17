# APP_ABI is not set here on purpose: the Gradle plugin passes its own from
# defaultConfig.ndk.abiFilters and would override anything written here. That
# list (arm64-v8a, x86_64) is the one that decides what ships.
APP_PLATFORM := android-24
APP_STL := c++_static
