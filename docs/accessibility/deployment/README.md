# XFB Accessibility Deployment Guide

## Overview

This guide provides comprehensive instructions for deploying XFB Radio Broadcasting Software with full accessibility support in various environments. It covers system requirements, installation procedures, configuration steps, and integration with assistive technologies.

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Installation Methods](#installation-methods)
3. [Accessibility Configuration](#accessibility-configuration)
4. [System Integration](#system-integration)
5. [Network Deployment](#network-deployment)
6. [Testing and Validation](#testing-and-validation)
7. [Maintenance](#maintenance)
8. [Troubleshooting](#troubleshooting)

## System Requirements

### Minimum Requirements

**Operating System:**
- Ubuntu 20.04 LTS or newer
- Debian 11 (Bullseye) or newer
- Fedora 34 or newer
- openSUSE Leap 15.3 or newer
- Any Linux distribution with Qt 6.0+ support

**Hardware:**
- CPU: 2 GHz dual-core processor
- RAM: 4 GB minimum, 8 GB recommended
- Storage: 2 GB available space for application
- Audio: ALSA or PulseAudio compatible sound card
- Network: Ethernet connection for streaming (optional)

**Accessibility Software:**
- ORCA Screen Reader 3.36 or newer
- AT-SPI 2.36 or newer
- Speech Dispatcher 0.10 or newer

### Recommended Configuration

**Operating System:**
- Ubuntu 22.04 LTS (best tested compatibility)
- GNOME Desktop Environment (optimal ORCA integration)

**Hardware:**
- CPU: 4 GHz quad-core processor or better
- RAM: 16 GB for large music libraries
- Storage: SSD with 10 GB available space
- Audio: Professional audio interface (USB/Firewire)
- Network: Gigabit Ethernet for high-quality streaming

**Accessibility Software:**
- ORCA Screen Reader 40.0 or newer
- AT-SPI 2.40 or newer
- Speech Dispatcher 0.11 or newer
- Braille display with BrlTTY support (optional)

### Dependencies

**Required Packages:**
```bash
# Core dependencies
qt6-base-dev
qt6-multimedia-dev
qt6-declarative-dev
libasound2-dev
libpulse-dev
libspeechd-dev

# Accessibility dependencies
at-spi2-core
libatspi2.0-dev
orca
speech-dispatcher
espeak-ng

# Audio dependencies
pulseaudio
alsa-utils
jackd2 (optional, for professional audio)

# Database dependencies
libsqlite3-dev
```

## Installation Methods

### Method 1: Package Installation (Recommended)

**Ubuntu/Debian:**
```bash
# Add XFB repository
wget -qO - https://packages.xfb.com/key.gpg | sudo apt-key add -
echo "deb https://packages.xfb.com/ubuntu $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/xfb.list

# Update package list
sudo apt update

# Install XFB with accessibility support
sudo apt install xfb xfb-accessibility-pack

# Install ORCA if not already present
sudo apt install orca speech-dispatcher espeak-ng
```

**Fedora:**
```bash
# Add XFB repository
sudo dnf config-manager --add-repo https://packages.xfb.com/fedora/xfb.repo

# Install XFB
sudo dnf install xfb xfb-accessibility-pack

# Install ORCA
sudo dnf install orca speech-dispatcher espeak-ng
```

### Method 2: Manual Installation

**Download and Install:**
```bash
# Download XFB package
wget https://releases.xfb.com/latest/xfb-latest.deb

# Install package
sudo dpkg -i xfb-latest.deb

# Install missing dependencies
sudo apt-get install -f

# Verify installation
xfb --version
```

### Method 3: Source Compilation

**Prerequisites:**
```bash
# Install build dependencies
sudo apt install build-essential cmake qt6-base-dev qt6-multimedia-dev git

# Clone repository
git clone https://github.com/xfb/xfb.git
cd xfb

# Configure build with accessibility support
mkdir build && cd build
cmake .. -DENABLE_ACCESSIBILITY=ON -DCMAKE_BUILD_TYPE=Release

# Compile
make -j$(nproc)

# Install
sudo make install
```

## Accessibility Configuration

### Initial Setup

**1. Enable System Accessibility:**
```bash
# Enable accessibility in GNOME
gsettings set org.gnome.desktop.interface toolkit-accessibility true

# Start AT-SPI service
systemctl --user enable at-spi-dbus-bus.service
systemctl --user start at-spi-dbus-bus.service
```

**2. Configure ORCA:**
```bash
# Start ORCA configuration
orca --setup

# Or use GUI
gnome-control-center universal-access
```

**3. XFB Accessibility Setup:**
```bash
# Create XFB configuration directory
mkdir -p ~/.config/xfb

# Copy default accessibility configuration
cp /usr/share/xfb/config/accessibility.conf ~/.config/xfb/

# Set appropriate permissions
chmod 644 ~/.config/xfb/accessibility.conf
```

### Configuration Files

**Main Accessibility Configuration (`~/.config/xfb/accessibility.conf`):**
```ini
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

[AudioFeedback]
ActionConfirmations=true
StatusUpdates=true
ProgressReports=true
ErrorNotifications=true
VolumeLevel=80

[LiveRegions]
PlaybackAnnouncements=true
SystemStatusUpdates=true
ProgressUpdates=true
CriticalAlerts=true

[Performance]
LazyInitialization=true
CacheAccessibilityData=true
ThrottleUpdates=true
```

**ORCA Application-Specific Settings:**
```bash
# Create ORCA settings directory for XFB
mkdir -p ~/.local/share/orca/app-settings

# Create XFB-specific ORCA configuration
cat > ~/.local/share/orca/app-settings/xfb.py << 'EOF'
# ORCA settings for XFB Radio Broadcasting Software
import orca.settings

# Speech settings
orca.settings.enableSpeech = True
orca.settings.speechVerbosityLevel = orca.settings.VERBOSITY_LEVEL_VERBOSE
orca.settings.enableSpeechIndentation = False
orca.settings.speakBlankLines = False

# Braille settings
orca.settings.enableBraille = True
orca.settings.brailleVerbosityLevel = orca.settings.VERBOSITY_LEVEL_BRIEF

# Keyboard settings
orca.settings.enableKeyEcho = True
orca.settings.enableAlphabeticKeys = True
orca.settings.enableNumericKeys = True
orca.settings.enablePunctuationKeys = True
orca.settings.enableSpace = True
orca.settings.enableModifierKeys = True

# Progress bar settings
orca.settings.progressBarVerbosity = orca.settings.PROGRESS_BAR_ALL
orca.settings.progressBarUpdateInterval = 10

# Application-specific settings
orca.settings.enableMnemonicSpeaking = True
orca.settings.enablePositionSpeaking = True
orca.settings.enableTutorialMessages = False
EOF
```

### Audio System Configuration

**PulseAudio Setup:**
```bash
# Configure PulseAudio for accessibility
cat >> ~/.config/pulse/default.pa << 'EOF'
# XFB Accessibility Audio Configuration
load-module module-role-cork
load-module module-role-ducking trigger_roles=phone ducking_roles=music,video
EOF

# Restart PulseAudio
pulseaudio --kill
pulseaudio --start
```

**ALSA Configuration (if using ALSA directly):**
```bash
# Create ALSA configuration for XFB
cat > ~/.asoundrc << 'EOF'
pcm.!default {
    type pulse
}
ctl.!default {
    type pulse
}

# XFB-specific audio device
pcm.xfb {
    type hw
    card 0
    device 0
}
EOF
```

## System Integration

### Desktop Environment Integration

**GNOME Integration:**
```bash
# Create desktop entry with accessibility metadata
cat > ~/.local/share/applications/xfb-accessible.desktop << 'EOF'
[Desktop Entry]
Version=1.0
Type=Application
Name=XFB Radio (Accessible)
Comment=Radio Broadcasting Software with Full Accessibility Support
Exec=env ORCA_ENABLED=1 xfb --accessibility-mode
Icon=xfb
Terminal=false
Categories=AudioVideo;Audio;Player;
MimeType=audio/mpeg;audio/flac;audio/ogg;
StartupNotify=true
X-GNOME-UsesNotifications=true
EOF

# Update desktop database
update-desktop-database ~/.local/share/applications/
```

**Systemd User Service (for automatic startup):**
```bash
# Create systemd service for XFB
mkdir -p ~/.config/systemd/user
cat > ~/.config/systemd/user/xfb-accessible.service << 'EOF'
[Unit]
Description=XFB Radio Broadcasting Software (Accessible Mode)
After=graphical-session.target
Wants=at-spi-dbus-bus.service

[Service]
Type=simple
Environment=DISPLAY=:0
Environment=ORCA_ENABLED=1
ExecStart=/usr/bin/xfb --accessibility-mode --minimize-to-tray
Restart=on-failure
RestartSec=5

[Install]
WantedBy=default.target
EOF

# Enable and start service
systemctl --user enable xfb-accessible.service
systemctl --user start xfb-accessible.service
```

### AT-SPI Configuration

**System-wide AT-SPI Setup:**
```bash
# Configure AT-SPI for all users
sudo cat > /etc/at-spi2/accessibility.conf << 'EOF'
[D-BUS Service]
Name=org.a11y.atspi.Registry
Exec=/usr/libexec/at-spi2-registryd --use-gnome-session

[org.a11y.atspi.Registry]
toolkit-accessibility=true
EOF

# Enable AT-SPI for XFB specifically
sudo cat > /etc/xfb/at-spi.conf << 'EOF'
# AT-SPI Configuration for XFB
export GTK_MODULES=gail:atk-bridge
export OOO_FORCE_DESKTOP=gnome
export GNOME_ACCESSIBILITY=1
export QT_ACCESSIBILITY=1
export QT_LINUX_ACCESSIBILITY_ALWAYS_ON=1
EOF
```

### Braille Display Integration

**BrlTTY Configuration:**
```bash
# Configure BrlTTY for XFB
sudo cat >> /etc/brltty.conf << 'EOF'
# XFB-specific braille settings
api-parameters Auth=none
speech-driver es
pcm-device hw:0,0

# Application-specific settings for XFB
begin-application-settings xfb
  skip-identical-lines off
  skip-blank-windows off
  sliding-window off
end-application-settings
EOF

# Restart BrlTTY
sudo systemctl restart brltty
```

## Network Deployment

### Multi-User Environment

**Shared Configuration:**
```bash
# Create shared XFB configuration
sudo mkdir -p /etc/xfb/shared
sudo cat > /etc/xfb/shared/accessibility.conf << 'EOF'
[SharedAccessibility]
DefaultVerbosity=Normal
AllowUserCustomization=true
RequireAccessibilityMode=false
SharedShortcuts=true

[NetworkSettings]
AllowRemoteAccess=false
AccessibilityAPIEnabled=true
RemoteAnnouncementsEnabled=false
EOF

# Set permissions
sudo chmod 644 /etc/xfb/shared/accessibility.conf
sudo chown root:xfb /etc/xfb/shared/accessibility.conf
```

**User Profile Management:**
```bash
# Create accessibility profile template
sudo mkdir -p /etc/skel/.config/xfb
sudo cp /usr/share/xfb/config/accessibility-template.conf /etc/skel/.config/xfb/accessibility.conf

# Script for new user setup
sudo cat > /usr/local/bin/xfb-setup-accessibility << 'EOF'
#!/bin/bash
# XFB Accessibility Setup Script for New Users

USER_HOME="$1"
if [ -z "$USER_HOME" ]; then
    echo "Usage: $0 <user_home_directory>"
    exit 1
fi

# Create XFB config directory
mkdir -p "$USER_HOME/.config/xfb"

# Copy accessibility configuration
cp /etc/skel/.config/xfb/accessibility.conf "$USER_HOME/.config/xfb/"

# Set ownership
chown -R $(basename "$USER_HOME"):$(basename "$USER_HOME") "$USER_HOME/.config/xfb"

# Enable accessibility for user
gsettings set org.gnome.desktop.interface toolkit-accessibility true

echo "XFB accessibility setup completed for user: $(basename "$USER_HOME")"
EOF

sudo chmod +x /usr/local/bin/xfb-setup-accessibility
```

### Remote Access Configuration

**SSH with X11 Forwarding:**
```bash
# Configure SSH for accessibility
cat >> ~/.ssh/config << 'EOF'
Host xfb-server
    HostName your-server.example.com
    User xfb-user
    ForwardX11 yes
    ForwardX11Trusted yes
    Compression yes
    
    # Accessibility-specific forwarding
    LocalForward 32768 localhost:32768  # AT-SPI
    LocalForward 32769 localhost:32769  # Speech Dispatcher
EOF

# Connect with accessibility support
ssh -X xfb-server "DISPLAY=:10.0 ORCA_ENABLED=1 xfb --accessibility-mode"
```

**VNC with Accessibility:**
```bash
# Install accessible VNC server
sudo apt install tigervnc-standalone-server

# Configure VNC with accessibility
cat > ~/.vnc/xstartup << 'EOF'
#!/bin/bash
export GNOME_ACCESSIBILITY=1
export QT_ACCESSIBILITY=1
export GTK_MODULES=gail:atk-bridge

# Start accessibility services
/usr/libexec/at-spi2-registryd &
orca --no-setup &

# Start desktop environment
gnome-session &
EOF

chmod +x ~/.vnc/xstartup

# Start VNC server
vncserver :1 -geometry 1920x1080 -depth 24
```

## Testing and Validation

### Automated Testing

**Accessibility Test Suite:**
```bash
# Run XFB accessibility tests
xfb --test-accessibility

# Specific test categories
xfb --test-accessibility --category=navigation
xfb --test-accessibility --category=audio-feedback
xfb --test-accessibility --category=keyboard-shortcuts
xfb --test-accessibility --category=orca-integration
```

**System Validation Script:**
```bash
#!/bin/bash
# XFB Accessibility Validation Script

echo "XFB Accessibility System Validation"
echo "=================================="

# Check ORCA
if pgrep -x "orca" > /dev/null; then
    echo "✓ ORCA is running"
else
    echo "✗ ORCA is not running"
fi

# Check AT-SPI
if systemctl --user is-active at-spi-dbus-bus.service > /dev/null; then
    echo "✓ AT-SPI service is active"
else
    echo "✗ AT-SPI service is not active"
fi

# Check XFB accessibility
if xfb --check-accessibility; then
    echo "✓ XFB accessibility is functional"
else
    echo "✗ XFB accessibility has issues"
fi

# Check audio system
if pulseaudio --check; then
    echo "✓ PulseAudio is working"
else
    echo "✗ PulseAudio has issues"
fi

# Check braille (if configured)
if systemctl is-active brltty > /dev/null; then
    echo "✓ BrlTTY is active"
else
    echo "- BrlTTY not configured (optional)"
fi

echo ""
echo "Validation complete. Check any ✗ items above."
```

### Manual Testing Procedures

**Basic Functionality Test:**
1. Launch XFB with ORCA running
2. Verify ORCA announces "XFB Radio Broadcasting Software loaded"
3. Test Tab navigation through interface
4. Test keyboard shortcuts (Ctrl+N, Ctrl+O, etc.)
5. Test audio feedback for button clicks
6. Test playlist navigation with arrow keys

**Advanced Feature Test:**
1. Test library scanning with progress announcements
2. Test playback with time announcements (Ctrl+T)
3. Test live region updates (status bar changes)
4. Test error handling (try to open invalid file)
5. Test settings persistence (change verbosity, restart)

## Maintenance

### Regular Maintenance Tasks

**Weekly:**
```bash
# Update accessibility cache
xfb --update-accessibility-cache

# Check for ORCA updates
sudo apt update && sudo apt list --upgradable | grep orca

# Verify AT-SPI functionality
systemctl --user status at-spi-dbus-bus.service
```

**Monthly:**
```bash
# Full accessibility system check
xfb --full-accessibility-check

# Update XFB and accessibility components
sudo apt update && sudo apt upgrade xfb xfb-accessibility-pack orca

# Clean accessibility logs
find ~/.config/xfb/logs/ -name "accessibility*.log" -mtime +30 -delete
```

**Quarterly:**
```bash
# Backup accessibility configuration
tar -czf ~/xfb-accessibility-backup-$(date +%Y%m%d).tar.gz ~/.config/xfb/

# Review and update accessibility settings
xfb --accessibility-settings-review

# Test with latest ORCA version
# (Follow ORCA update procedures)
```

### Log Management

**Configure Log Rotation:**
```bash
# Create logrotate configuration for XFB accessibility logs
sudo cat > /etc/logrotate.d/xfb-accessibility << 'EOF'
/home/*/.config/xfb/logs/accessibility*.log {
    daily
    missingok
    rotate 30
    compress
    delaycompress
    notifempty
    copytruncate
    su root root
}
EOF
```

### Performance Monitoring

**Accessibility Performance Script:**
```bash
#!/bin/bash
# Monitor XFB accessibility performance

LOG_FILE="$HOME/.config/xfb/logs/accessibility-performance.log"

while true; do
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    
    # Check XFB memory usage
    XFB_MEM=$(ps -o pid,vsz,rss,comm -p $(pgrep xfb) | tail -1 | awk '{print $2,$3}')
    
    # Check ORCA memory usage
    ORCA_MEM=$(ps -o pid,vsz,rss,comm -p $(pgrep orca) | tail -1 | awk '{print $2,$3}')
    
    # Check AT-SPI responsiveness
    ATSPI_RESPONSE=$(time timeout 5 dbus-send --session --dest=org.a11y.atspi.Registry --type=method_call /org/a11y/atspi/registry org.a11y.atspi.Registry.GetApplications 2>&1 | grep real | awk '{print $2}')
    
    echo "$TIMESTAMP XFB_MEM:$XFB_MEM ORCA_MEM:$ORCA_MEM ATSPI_RESPONSE:$ATSPI_RESPONSE" >> "$LOG_FILE"
    
    sleep 300  # Check every 5 minutes
done
```

## Troubleshooting

### Common Deployment Issues

**Issue: Package Installation Fails**
```bash
# Check dependencies
sudo apt install -f

# Clear package cache
sudo apt clean && sudo apt update

# Try manual dependency installation
sudo apt install qt6-base-dev qt6-multimedia-dev at-spi2-core orca
```

**Issue: AT-SPI Service Won't Start**
```bash
# Reset AT-SPI configuration
rm -rf ~/.cache/at-spi2
systemctl --user restart at-spi-dbus-bus.service

# Check for conflicts
ps aux | grep at-spi
```

**Issue: ORCA Doesn't Detect XFB**
```bash
# Verify Qt accessibility is enabled
export QT_ACCESSIBILITY=1
export QT_LINUX_ACCESSIBILITY_ALWAYS_ON=1

# Restart both ORCA and XFB
orca --quit
sleep 2
orca &
xfb --accessibility-mode
```

### Environment-Specific Issues

**Virtual Machines:**
- Ensure 3D acceleration is enabled
- Allocate sufficient RAM (8GB+ recommended)
- Enable audio passthrough
- Install guest additions/tools

**Container Deployment:**
```bash
# Docker container with accessibility support
docker run -it \
  --env DISPLAY=$DISPLAY \
  --env QT_ACCESSIBILITY=1 \
  --env GNOME_ACCESSIBILITY=1 \
  --volume /tmp/.X11-unix:/tmp/.X11-unix:rw \
  --volume /run/user/$(id -u)/at-spi:/run/user/1000/at-spi:rw \
  --device /dev/snd \
  xfb:latest
```

**Network File Systems:**
- Ensure proper permissions for ~/.config/xfb/
- Consider local caching for accessibility data
- Test with reduced network latency requirements

For additional deployment support, consult the XFB deployment team or community forums.