#!/bin/bash
# XFB macOS Uninstaller
# Run this script to completely remove XFB from your Mac

set -e

echo "╔══════════════════════════════════════════╗"
echo "║     XFB Uninstaller for macOS           ║"
echo "╚══════════════════════════════════════════╝"
echo ""

# Check if running as the correct user
if [ "$EUID" -eq 0 ]; then
    echo "Please run this script as your normal user (not sudo)."
    exit 1
fi

APP_PATH="/Applications/XFB.app"
USER_CONFIG="$HOME/Library/Application Support/Netpack - Online Solutions/XFB"
USER_PREFS="$HOME/Library/Preferences/pt.netpack.xfb.plist"
USER_CACHE="$HOME/Library/Caches/XFB"
TOR_DATA="$HOME/Library/Application Support/XFB/tor_data"
DOWNLOADS="$HOME/Downloads/XFB_Torrents"

echo "This will remove:"
echo "  • XFB.app from /Applications"
echo "  • Configuration files"
echo "  • Cache files"
echo "  • Tor data directory"
echo ""
echo "Your music library and playlists will NOT be deleted."
echo ""

read -p "Continue with uninstallation? (y/N) " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Uninstallation cancelled."
    exit 0
fi

echo ""
echo "Stopping XFB if running..."
pkill -x "XFB" 2>/dev/null || true
sleep 1

# Remove the application bundle
if [ -d "$APP_PATH" ]; then
    echo "Removing $APP_PATH..."
    rm -rf "$APP_PATH"
    echo "  ✓ Application removed"
else
    echo "  • XFB.app not found in /Applications (already removed?)"
fi

# Remove user configuration
if [ -d "$USER_CONFIG" ]; then
    echo "Removing configuration..."
    rm -rf "$USER_CONFIG"
    echo "  ✓ Configuration removed"
fi

# Remove preferences plist
if [ -f "$USER_PREFS" ]; then
    echo "Removing preferences..."
    rm -f "$USER_PREFS"
    echo "  ✓ Preferences removed"
fi

# Remove cache
if [ -d "$USER_CACHE" ]; then
    echo "Removing cache..."
    rm -rf "$USER_CACHE"
    echo "  ✓ Cache removed"
fi

# Remove Tor data
if [ -d "$TOR_DATA" ]; then
    echo "Removing Tor data..."
    rm -rf "$TOR_DATA"
    echo "  ✓ Tor data removed"
fi

# Ask about downloads
if [ -d "$DOWNLOADS" ]; then
    echo ""
    read -p "Remove downloaded torrents in $DOWNLOADS? (y/N) " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm -rf "$DOWNLOADS"
        echo "  ✓ Downloads removed"
    else
        echo "  • Downloads preserved at: $DOWNLOADS"
    fi
fi

echo ""
echo "╔══════════════════════════════════════════╗"
echo "║     XFB has been uninstalled            ║"
echo "╚══════════════════════════════════════════╝"
echo ""
echo "Thank you for using XFB!"
echo ""
