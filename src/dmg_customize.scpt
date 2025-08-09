tell application "Finder"
    tell disk "<VOLUME_NAME>"
        open
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        set the bounds of container window to {100, 100, 700, 400}
        set view options of container window to icon view options
        set arrangement of icon view options of container window to not arranged
        set icon size of icon view options of container window to 128
        set background picture of icon view options of container window to file ".background:dmg_background.png"
        delay 2
        set position of item "<APP_NAME>.app" of container window to {100, 150}
        set position of item "Applications" of container window to {400, 150}
        close
        open
        update without registering applications
        delay 2
        eject
    end tell
end tell
