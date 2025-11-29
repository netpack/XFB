# XFB Accessibility Installation Guide

## Overview

This comprehensive guide walks you through installing XFB Radio Broadcasting Software with full accessibility support. Follow the steps appropriate for your Linux distribution and use case.

## Pre-Installation Checklist

### System Preparation

**1. Verify System Requirements:**
```bash
# Run the system requirements check
curl -fsSL https://install.xfb.com/check-requirements.sh | bash
```

**2. Update System:**
```bash
# Ubuntu/Debian
sudo apt update && sudo apt upgrade -y

# Fedora
sudo dnf update -y

# openSUSE
sudo zypper update -y
```

**3. Enable Accessibility:**
```bash
# Enable system-wide accessibility
gsettings set org.gnome.desktop.interface toolkit-accessibility true

# Verify accessibility is enabled
gsettings get org.gnome.desktop.interface toolkit-accessibility
```

## Installation Methods

### Method 1: Package Repository Installation (Recommended)

This is the easiest and most reliable installation method.

#### Ubuntu/Debian Installation

**Step 1: Add XFB Repository**
```bash
# Add GPG key
wget -qO - https://packages.xfb.com/gpg-key.asc | sudo gpg --dearmor -o /usr/share/keyrings/xfb-archive-keyring.gpg

# Add repository
echo "deb [signed-by=/usr/share/keyrings/xfb-archive-keyring.gpg] https://packages.xfb.com/ubuntu $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/xfb.list

# Update package list
sudo apt update
```

**Step 2: Install XFB with Accessibility Support**
```bash
# Install complete XFB accessibility package
sudo apt install xfb

# This installs:
# - xfb (main application)
# - xfb-accessibility-pack (accessibility enhancements)
# - orca (screen reader)
# - speech-dispatcher (speech synthesis)
# - espeak-ng (speech engine)
# - at-spi2-core (accessibility framework)
```

**Step 3: Verify Installation**
```bash
# Check XFB version
xfb --version

# Test accessibility integration
xfb --test-accessibility

# Verify ORCA integration
orca --version
```

#### Fedora Installation

**Step 1: Add XFB Repository**
```bash
# Add repository configuration
sudo tee /etc/yum.repos.d/xfb.repo << 'EOF'
[xfb]
name=XFB Radio Broadcasting Software
baseurl=https://packages.xfb.com/fedora/$releasever/$basearch/
enabled=1
gpgcheck=1
gpgkey=https://packages.xfb.com/gpg-key.asc
EOF

# Update package cache
sudo dnf makecache
```

**Step 2: Install XFB**
```bash
# Install XFB with accessibility support
sudo dnf install xfb

# Install additional speech engines if desired
sudo dnf install festival festvox-kallpc16k
```

**Step 3: Configure SELinux (if enabled)**
```bash
# Check SELinux status
sestatus

# If SELinux is enforcing, configure XFB policy
sudo setsebool -P allow_execheap 1
sudo setsebool -P allow_execmem 1

# Install XFB SELinux policy
sudo dnf install xfb-selinux-policy
```

#### openSUSE Installation

**Step 1: Add Repository**
```bash
# Add XFB repository
sudo zypper addrepo https://packages.xfb.com/opensuse/$(lsb_release -rs)/ xfb

# Refresh repositories
sudo zypper refresh
```

**Step 2: Install XFB**
```bash
# Install XFB with accessibility
sudo zypper install xfb

# Accept GPG key when prompted
```

### Method 2: Manual Package Installation

If repository installation isn't available for your distribution:

**Step 1: Download Packages**
```bash
# Create download directory
mkdir ~/xfb-install && cd ~/xfb-install

# Download XFB package for your distribution
wget https://releases.xfb.com/latest/xfb-latest-$(lsb_release -cs).deb

# Download accessibility pack
wget https://releases.xfb.com/latest/xfb-accessibility-pack-latest-$(lsb_release -cs).deb
```

**Step 2: Install Dependencies**
```bash
# Ubuntu/Debian
sudo apt install qt6-base qt6-multimedia qt6-declarative \
                 at-spi2-core orca speech-dispatcher espeak-ng \
                 libasound2 libpulse0 libsqlite3-0

# Fedora
sudo dnf install qt6-qtbase qt6-qtmultimedia qt6-qtdeclarative \
                 at-spi2-core orca speech-dispatcher espeak-ng \
                 alsa-lib pulseaudio-libs sqlite
```

**Step 3: Install Packages**
```bash
# Ubuntu/Debian
sudo dpkg -i xfb-latest-*.deb xfb-accessibility-pack-latest-*.deb
sudo apt-get install -f  # Fix any dependency issues

# Fedora (convert .deb to .rpm first)
sudo alien -r xfb-latest-*.deb
sudo dnf install xfb-*.rpm
```

### Method 3: Source Compilation

For advanced users or unsupported distributions:

**Step 1: Install Build Dependencies**
```bash
# Ubuntu/Debian
sudo apt install build-essential cmake git pkg-config \
                 qt6-base-dev qt6-multimedia-dev qt6-declarative-dev \
                 libatspi2.0-dev libspeechd-dev libasound2-dev \
                 libpulse-dev libsqlite3-dev

# Fedora
sudo dnf install gcc-c++ cmake git pkgconfig \
                 qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qtdeclarative-devel \
                 at-spi2-core-devel speech-dispatcher-devel \
                 alsa-lib-devel pulseaudio-libs-devel sqlite-devel
```

**Step 2: Clone and Build**
```bash
# Clone repository
git clone https://github.com/xfb/xfb.git
cd xfb

# Create build directory
mkdir build && cd build

# Configure build with accessibility support
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_ACCESSIBILITY=ON \
    -DENABLE_ORCA_INTEGRATION=ON \
    -DENABLE_BRAILLE_SUPPORT=ON \
    -DCMAKE_INSTALL_PREFIX=/usr/local

# Build (use all available cores)
make -j$(nproc)

# Run tests
make test

# Install
sudo make install
```

**Step 3: Post-Build Configuration**
```bash
# Update library cache
sudo ldconfig

# Create desktop entry
sudo make install-desktop-files

# Install accessibility configuration
sudo make install-accessibility-config
```

## Post-Installation Configuration

### Initial Setup

**Step 1: Launch XFB Setup Wizard**
```bash
# Run first-time setup
xfb --setup

# Or launch with accessibility mode
xfb --accessibility-mode --setup
```

**Step 2: Configure ORCA for XFB**
```bash
# Launch ORCA setup
orca --setup

# Key settings to configure:
# - Speech rate: 50-70% (adjust to preference)
# - Verbosity: Verbose (for detailed information)
# - Punctuation: Some or Most
# - Progress bars: Enable updates every 10%
```

**Step 3: Test Integration**
```bash
# Test XFB with ORCA
orca &
sleep 3
xfb --test-mode

# You should hear: "XFB Radio Broadcasting Software loaded with accessibility support"
```

### Audio System Configuration

**PulseAudio Setup (Recommended):**
```bash
# Install PulseAudio if not present
sudo apt install pulseaudio pulseaudio-utils

# Configure PulseAudio for XFB
mkdir -p ~/.config/pulse
cat >> ~/.config/pulse/default.pa << 'EOF'
# XFB Audio Configuration
.include /etc/pulse/default.pa

# Load role-based modules for accessibility
load-module module-role-cork
load-module module-role-ducking trigger_roles=phone ducking_roles=music,video

# XFB-specific sink for broadcasting
load-module module-null-sink sink_name=xfb_broadcast sink_properties=device.description="XFB Broadcast Output"
EOF

# Restart PulseAudio
pulseaudio --kill
pulseaudio --start
```

**JACK Setup (Professional Use):**
```bash
# Install JACK
sudo apt install jackd2 qjackctl

# Configure JACK for low latency
sudo usermod -a -G audio $USER

# Create JACK configuration
mkdir -p ~/.config/jack
cat > ~/.config/jack/conf.xml << 'EOF'
<?xml version="1.0"?>
<jack>
    <engine>
        <option name="realtime">true</option>
        <option name="realtime-priority">70</option>
        <option name="temporary">false</option>
        <option name="verbose">false</option>
        <option name="client-timeout">0</option>
        <option name="clock-source">0</option>
        <option name="port-max">256</option>
    </engine>
    <drivers>
        <driver name="alsa">
            <option name="device">hw:0</option>
            <option name="capture">hw:0</option>
            <option name="playback">hw:0</option>
            <option name="rate">48000</option>
            <option name="period">128</option>
            <option name="nperiods">2</option>
        </driver>
    </drivers>
</jack>
EOF

# Note: Log out and back in for group membership to take effect
```

### Accessibility Service Configuration

**AT-SPI Configuration:**
```bash
# Ensure AT-SPI is enabled system-wide
sudo mkdir -p /etc/at-spi2
sudo cat > /etc/at-spi2/accessibility.conf << 'EOF'
[D-BUS Service]
Name=org.a11y.atspi.Registry
Exec=/usr/libexec/at-spi2-registryd --use-gnome-session

[org.a11y.atspi.Registry]
toolkit-accessibility=true
EOF

# Enable AT-SPI service for current user
systemctl --user enable at-spi-dbus-bus.service
systemctl --user start at-spi-dbus-bus.service
```

**Speech Dispatcher Configuration:**
```bash
# Configure Speech Dispatcher for XFB
mkdir -p ~/.config/speech-dispatcher
cat > ~/.config/speech-dispatcher/speechd.conf << 'EOF'
# XFB Speech Dispatcher Configuration
LogLevel 3
LogDir "default"
DefaultModule espeak-ng
AudioOutputMethod pulse
AudioPulseServer "unix:/run/user/$(id -u)/pulse/native"

# Module configuration
AddModule "espeak-ng" "sd_espeak-ng" "espeak-ng.conf"
AddModule "festival" "sd_festival" "festival.conf"

# Default voice settings
DefaultVoiceType "MALE1"
DefaultLanguage "en"
DefaultRate 0
DefaultPitch 0
DefaultVolume 100
EOF

# Test speech dispatcher
spd-say "XFB accessibility test"
```

### XFB-Specific Configuration

**Create XFB Configuration Directory:**
```bash
# Create configuration directory
mkdir -p ~/.config/xfb/{logs,cache,plugins,accessibility}

# Set proper permissions
chmod 755 ~/.config/xfb
chmod 755 ~/.config/xfb/*
```

**Configure XFB Accessibility Settings:**
```bash
# Create accessibility configuration
cat > ~/.config/xfb/accessibility.conf << 'EOF'
[Accessibility]
Enabled=true
VerbosityLevel=Normal
AnnouncementTiming=Immediate
AudioFeedbackEnabled=true
BrailleSupport=false

[KeyboardNavigation]
TabOrderOptimized=true
ArrowKeyNavigation=true
CustomShortcutsEnabled=true
EscapeKeyBehavior=Cancel

[AudioFeedback]
ActionConfirmations=true
StatusUpdates=true
ProgressReports=true
ErrorNotifications=true
VolumeLevel=80
InterruptBehavior=Queue

[LiveRegions]
PlaybackAnnouncements=true
SystemStatusUpdates=true
ProgressUpdates=true
CriticalAlerts=true
UpdateThrottling=true

[Performance]
LazyInitialization=true
CacheAccessibilityData=true
ThrottleUpdates=true
MaxCacheSize=100MB
EOF
```

**Configure Keyboard Shortcuts:**
```bash
# Create keyboard shortcuts configuration
cat > ~/.config/xfb/keyboard-shortcuts.conf << 'EOF'
[GlobalShortcuts]
PlayPause=Space
Stop=Ctrl+Space
NextTrack=Ctrl+Right
PreviousTrack=Ctrl+Left
VolumeUp=Ctrl+Up
VolumeDown=Ctrl+Down
Mute=Ctrl+M

[NavigationShortcuts]
FocusLibrary=Alt+L
FocusPlaylist=Alt+P
FocusControls=Alt+C
FocusStatusBar=Alt+S

[AccessibilityShortcuts]
AnnounceTime=Ctrl+T
AnnounceRemainingTime=Ctrl+Shift+T
AnnounceTrackInfo=Ctrl+D
WhereAmI=Ctrl+Question
RepeatLastAnnouncement=Ctrl+Alt+A

[FileOperations]
NewPlaylist=Ctrl+N
OpenFile=Ctrl+O
SavePlaylist=Ctrl+S
SaveAs=Ctrl+Shift+S
EOF
```

## Verification and Testing

### Basic Functionality Test

**Step 1: Launch XFB with ORCA**
```bash
# Ensure ORCA is running
orca &
sleep 3

# Launch XFB
xfb

# Expected: ORCA announces "XFB Radio Broadcasting Software loaded with accessibility support"
```

**Step 2: Test Navigation**
```bash
# Test keyboard navigation
# Press Tab - should move through interface elements
# Press Alt+M - should access menu bar
# Press Alt+L - should focus music library
# Press Escape - should return to main interface
```

**Step 3: Test Audio Feedback**
```bash
# Test button feedback
# Click any button - should hear confirmation
# Press Ctrl+N - should hear "New playlist created"
# Press Ctrl+O - should hear file dialog opening
```

### Advanced Feature Testing

**Test Library Scanning:**
```bash
# Add a test music directory
mkdir -p ~/Music/test
cp /usr/share/sounds/alsa/*.wav ~/Music/test/

# Add directory to XFB library
# Press Ctrl+Shift+L to open library management
# Press Ctrl+A to add directory
# Navigate to ~/Music/test and press Enter
# Should hear progress announcements during scan
```

**Test Playback with Announcements:**
```bash
# Select a track in library
# Press Enter to play
# Should hear "Playback started" announcement
# Press Ctrl+T - should announce current time
# Press Space to pause - should hear "Playback paused"
```

**Test Keyboard Shortcuts:**
```bash
# Test all configured shortcuts
# Press F1 - should open help
# Press Ctrl+, - should open preferences
# Press Ctrl+F - should open search dialog
```

### Accessibility Compliance Testing

**Run Built-in Accessibility Tests:**
```bash
# Run comprehensive accessibility test suite
xfb --test-accessibility --verbose

# Test specific components
xfb --test-accessibility --component=navigation
xfb --test-accessibility --component=audio-feedback
xfb --test-accessibility --component=keyboard-shortcuts
```

**Manual ORCA Testing:**
```bash
# Test ORCA-specific features
# Navigate through interface with Tab
# Use arrow keys in lists and grids
# Test context menus with Menu key
# Verify all controls are announced properly
```

## Troubleshooting Installation Issues

### Common Installation Problems

**Problem: Package Dependencies Not Met**
```bash
# Ubuntu/Debian
sudo apt --fix-broken install
sudo apt update && sudo apt upgrade

# Fedora
sudo dnf check
sudo dnf update
```

**Problem: ORCA Not Detecting XFB**
```bash
# Verify AT-SPI is running
systemctl --user status at-spi-dbus-bus.service

# Restart AT-SPI if needed
systemctl --user restart at-spi-dbus-bus.service

# Restart ORCA
orca --quit
sleep 2
orca &
```

**Problem: Audio System Issues**
```bash
# Test audio system
speaker-test -t sine -f 1000 -l 1

# Check PulseAudio
pulseaudio --check
pulseaudio --start

# Verify XFB audio configuration
xfb --test-audio
```

**Problem: Permission Issues**
```bash
# Fix XFB configuration permissions
sudo chown -R $USER:$USER ~/.config/xfb
chmod -R 755 ~/.config/xfb

# Add user to audio group
sudo usermod -a -G audio $USER
# Log out and back in for changes to take effect
```

### Getting Help

**Built-in Diagnostics:**
```bash
# Run XFB diagnostic tool
xfb --diagnose

# Generate system report
xfb --system-report > ~/xfb-system-report.txt
```

**Log Files:**
```bash
# Check XFB logs
tail -f ~/.config/xfb/logs/xfb.log

# Check accessibility logs
tail -f ~/.config/xfb/logs/accessibility.log

# Check system logs for AT-SPI issues
journalctl --user -u at-spi-dbus-bus.service
```

**Community Support:**
- XFB Accessibility Forum: https://forum.xfb.com/accessibility
- Documentation: https://docs.xfb.com/accessibility
- Bug Reports: https://github.com/xfb/xfb/issues

## Next Steps

After successful installation:

1. **Read the User Guide**: Complete the XFB Accessibility User Guide
2. **Configure Your Setup**: Customize settings for your broadcasting needs
3. **Import Music Library**: Add your music collection to XFB
4. **Create Playlists**: Set up playlists for your shows
5. **Test Broadcasting**: Configure streaming settings and test your setup

For detailed usage instructions, see the XFB Accessibility User Guide.