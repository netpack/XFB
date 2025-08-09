#!/bin/bash

# Script to create a .dmg installer for the XFB macOS app

APP_NAME="XFB"
APP_BUNDLE="XFB.app"
DMG_NAME="${APP_NAME}.dmg"
VOLUME_NAME="${APP_NAME} Installer"

# Check if app bundle exists
if [ ! -d "$APP_BUNDLE" ]; then
  echo "App bundle not found at $APP_BUNDLE"
  exit 1
fi

# Create a temporary staging directory
STAGING_DIR=$(mktemp -d)

# Copy the app bundle to the staging directory
cp -R "$APP_BUNDLE" "$STAGING_DIR/"

# Copy the install dependencies script and command file to the staging directory
cp install_dependencies_mac.sh "$STAGING_DIR/.install_dependencies_mac.sh"
cp install_dependencies_mac.command "$STAGING_DIR/"

# Make the scripts executable
chmod +x "$STAGING_DIR/.install_dependencies_mac.sh"
chmod +x "$STAGING_DIR/install_dependencies_mac.command"

# Create Applications alias in staging directory
ln -s /Applications "$STAGING_DIR/Applications"

# Create the .dmg file
hdiutil create -volname "$VOLUME_NAME" -srcfolder "$STAGING_DIR" -ov -format UDZO "$DMG_NAME"

# Clean up
rm -rf "$STAGING_DIR"

echo "Created $DMG_NAME successfully."
