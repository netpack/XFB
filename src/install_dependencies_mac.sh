#!/bin/bash

# Script to install optional dependencies for XFB on macOS using Homebrew

# Check if Homebrew is installed
if ! command -v brew &> /dev/null
then
    echo "Homebrew not found. Installing Homebrew first..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi

echo "Updating Homebrew..."
brew update

# List of optional dependencies
dependencies=(
    audacity
    yt-dlp
    sox
    exiftool
    ffmpeg
    icecast
    mplayer
    soundconverter
    mediainfo
)

echo "The following optional dependencies will be installed:"
for dep in "${dependencies[@]}"; do
    echo " - $dep"
done

read -p "Do you want to proceed with the installation? (y/n) " answer
if [[ "$answer" != "y" && "$answer" != "Y" ]]; then
    echo "Installation aborted."
    exit 0
fi

# Install dependencies
for dep in "${dependencies[@]}"; do
    echo "Installing $dep..."
    brew install "$dep"
done

echo "All selected dependencies have been installed."
