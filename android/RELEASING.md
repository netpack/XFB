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

`package-companion.sh` stages the two files twice: into XFB's drop directory, so
*this* machine serves them straight away, and into `packaging/companion/`, which
is what the desktop packaging reads. Neither file is committed — they are build
products of another toolchain, 8 MB a release, and the key that makes them
installable is not in the repository.

Every channel treats them as optional. With nothing staged the packages build
exactly as before and XFB says it has no app to hand out.

| channel | lands at | wired in |
|---|---|---|
| macOS | `XFB.app/Contents/Resources/` | `cmake/StageCompanionApk.cmake`, run at build time — `build-macos.sh` takes the bundle out of the build tree and never runs `cmake --install` |
| Debian | `/usr/share/xfb/` | `install(FILES ... OPTIONAL)` in `src/CMakeLists.txt`; the Docker build `COPY . /src` brings the staged files in |
| Windows | beside `XFB.exe` | `build-windows.bat` copies them into the dist folder, and NSIS packs the folder whole |
| Arch | `/usr/share/xfb/` | the PKGBUILD **downloads them from the release** — it builds from a git tag and never sees `packaging/companion/` |

So the order for a full release is: build and stage the APK first, then build the
desktop packages, then publish.

### Windows needs the file carried over

The Android build runs on the Mac. Copy `packaging/companion/xfb-companion.apk`
and `xfb-companion.json` into the same directory on the Windows machine before
running `build-windows.bat`, or that installer ships without the phone app.

### Arch needs the hashes

Upload both files as assets on the GitHub release:

```bash
gh release upload "v${VERSION}" packaging/companion/xfb-companion.apk packaging/companion/xfb-companion.json --repo netpack/XFB --clobber
```

Then replace `REPLACE_WITH_APK_SHA256` and
`REPLACE_WITH_SIDECAR_SHA256` in both `PKGBUILD` and `aur-xfb/PKGBUILD`.
`package-companion.sh` prints both hashes when it finishes.

### Checking a package actually carries it

```bash
unzip -l XFB-*-Setup.exe | grep companion
ls "XFB.app/Contents/Resources/" | grep companion
dpkg -c xfb_*.deb | grep companion
```

Then, from the installed copy rather than a build tree, open Options › Sync to
Phone: the last line names the APK it is offering and its version. That line
reading "none to hand out" is the whole failure mode, and it is the only check
that exercises the lookup the phone actually depends on.
