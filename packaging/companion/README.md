# The companion APK, staged for packaging

`android/package-companion.sh` drops the signed `xfb-companion.apk` and its
`xfb-companion.json` sidecar here. Every desktop package that can carry the
phone app reads them from this directory:

| channel | where it ends up | who puts it there |
|---|---|---|
| macOS | `XFB.app/Contents/Resources/` | `cmake/StageCompanionApk.cmake`, at build time |
| Debian | `/usr/share/xfb/` | the `install(FILES ... OPTIONAL)` rule in `src/CMakeLists.txt` |
| Windows | beside `XFB.exe` | `build-windows.bat`, into the dist folder NSIS packs |
| Arch | `/usr/share/xfb/` | the PKGBUILD, which downloads the release asset instead — it builds from a git tag and never sees this directory |

The two files are **deliberately not committed**. They are build products of a
different toolchain, they are 8 MB a release, and the signing key that makes
them installable is not in the repository either. Every step above is optional:
with nothing staged the packages build exactly as before, and XFB's Sync to
Phone dialog says it has no app to hand out.

Both files must travel together. Without the sidecar the APK is offered as a
download but never recognised as an *update*, so nobody's phone is ever told a
newer one exists.
