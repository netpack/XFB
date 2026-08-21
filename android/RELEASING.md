# Releasing the XFB companion

The desktop is this app's store. XFB serves whatever sits beside it as
`xfb-companion.apk`, and the phone downloads it from the pairing page or as an
in-app update. There is no Play Store in the loop, which removes a review queue
and adds two obligations: the APK must be **signed by us**, and it must be
accompanied by a sidecar saying what version it is.

## 1. The signing key — once, and then never lose it

Android identifies an app by the key it was signed with. An update signed with a
different key is not an update; it is a different app that will not install over
the old one. Everyone who has the companion would have to uninstall and lose
their downloaded music.

So: make the keystore once, back it up, and keep the passwords somewhere they
will outlive the laptop.

```bash
keytool -genkeypair -v \
  -keystore ~/keys/xfb-companion.jks \
  -alias xfb-companion \
  -keyalg RSA -keysize 4096 -validity 10950 \
  -dname "CN=XFB, O=Netpack - Online Solutions, C=PT"
```

`keytool` will ask for a keystore password and a key password. Then write
`android/keystore.properties` — it is gitignored, and it is the only place the
passwords appear:

```properties
storeFile=/Users/you/keys/xfb-companion.jks
storePassword=…
keyAlias=xfb-companion
keyPassword=…
```

Without that file the release build still runs, warns, and produces
`app-release-unsigned.apk`, which Android refuses to install.

## 2. The version

Both numbers live in `app/build.gradle.kts`:

- `versionCode` — an integer. **This is what the update check compares.** It has
  to go up every release or phones will not offer the new build.
- `versionName` — what people see, on the pairing page and in the update prompt.

## 3. Build and place it

```bash
./package-companion.sh
```

That builds `assembleRelease`, refuses to go on if the result is not signed,
copies it to XFB's drop directory as `xfb-companion.apk`, and writes
`xfb-companion.json` with the APK's own version numbers. Pass a directory to
put it somewhere else.

XFB looks for the APK in this order, taking the first that exists:

1. `MobileSync/ApkPath` in `xfb.conf`, if set
2. next to the XFB executable
3. `XFB.app/Contents/Resources/` (macOS)
4. the app data directory — `~/Library/Application Support/Netpack - Online Solutions/XFB`
   on macOS, `~/.local/share/Netpack - Online Solutions/XFB` on Linux
5. `/usr/share/xfb/`

Options › Sync to Phone names the directory it is watching and says when it has
nothing to hand out.

## 4. Check it before anyone else does

```bash
adb install -r app/build/outputs/apk/release/app-release.apk
```

A release build differs from the debug one in ways that only show up at runtime
— the signature, and any resource shrinking — so install the actual artifact and
play a set with it. Then, from a phone that already has the previous version,
open the app and confirm the update banner appears and installs.

`INSTALL_FAILED_UPDATE_INCOMPATIBLE` means the phone has a debug-signed build on
it. Uninstall that one first; it is not a problem with the key.

## 5. Shipping it inside the desktop packages

Steps 1–4 put the APK on **your** machine, so your XFB hands it out. For
everybody else's XFB to hand it out, the APK has to travel inside the desktop
packages, and that is not wired up yet. What each channel needs:

- **macOS** — copy into `XFB.app/Contents/Resources/` alongside the icon, in the
  same CMake install step that assembles the bundle.
- **Windows** — `installer.nsi` should place it beside `XFB.exe`.
- **Debian / Arch** — install to `/usr/share/xfb/`.

The APK is a build product of a different toolchain, so it cannot simply be a
CMake target: either commit the signed APK for the packaging steps to pick up,
or build it first and point the packaging at
`android/app/build/outputs/apk/release/`. Both put the sidecar next to it.
