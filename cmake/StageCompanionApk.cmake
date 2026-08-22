# Copies the staged companion APK into a bundle or dist directory.
#
# A script rather than `cmake -E copy_if_different` because the APK is
# optional: it is built by the Android toolchain, not this one, and a desktop
# build with none staged has to succeed and simply offer no phone app. A plain
# copy command fails the build when the source is missing.
#
# Expects APK_DIR and DEST.

set(_apk "${APK_DIR}/xfb-companion.apk")
set(_sidecar "${APK_DIR}/xfb-companion.json")

if(NOT EXISTS "${_apk}")
    message(STATUS "No companion APK staged in ${APK_DIR} — "
                   "Sync to Phone will have no app to hand out")
    return()
endif()

file(MAKE_DIRECTORY "${DEST}")
file(COPY "${_apk}" DESTINATION "${DEST}")

if(EXISTS "${_sidecar}")
    file(COPY "${_sidecar}" DESTINATION "${DEST}")
    message(STATUS "Companion APK staged into ${DEST}")
else()
    # Worth shouting about: the APK alone is offered as a download but never
    # recognised as an update, so no phone is ever told a newer one exists.
    message(WARNING "Staged ${_apk} without xfb-companion.json — the app will "
                    "be offered as a download but never as an update")
endif()
