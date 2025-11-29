# XFB Accessibility User Guide

## Welcome to XFB Radio Broadcasting Software

This guide will help you set up and use XFB Radio Broadcasting Software with the ORCA screen reader. XFB provides comprehensive accessibility features designed specifically for visually impaired radio broadcasters and content creators.

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Initial Setup](#initial-setup)
3. [ORCA Configuration](#orca-configuration)
4. [Getting Started](#getting-started)
5. [Keyboard Navigation](#keyboard-navigation)
6. [Audio Feedback](#audio-feedback)
7. [Broadcasting Workflow](#broadcasting-workflow)
8. [Accessibility Settings](#accessibility-settings)
9. [Troubleshooting](#troubleshooting)
10. [Support](#support)

## System Requirements

### Minimum Requirements

- **Operating System**: Ubuntu 20.04 LTS or newer, or compatible Linux distribution
- **Screen Reader**: ORCA 3.36 or newer
- **Qt Version**: Qt 6.0 or newer (automatically installed with XFB)
- **Audio System**: PulseAudio or ALSA
- **Memory**: 4 GB RAM minimum, 8 GB recommended
- **Storage**: 2 GB available space

### Recommended Setup

- **Operating System**: Ubuntu 22.04 LTS or newer
- **Screen Reader**: ORCA 40.0 or newer
- **Desktop Environment**: GNOME (for best ORCA integration)
- **Audio Interface**: Professional audio interface for broadcasting
- **Memory**: 16 GB RAM for large music libraries
- **Storage**: SSD for improved performance

## Initial Setup

### Installing XFB

1. **Download XFB**: Obtain the latest XFB package from the official website
2. **Install Package**: 
   ```bash
   sudo dpkg -i xfb-*.deb
   sudo apt-get install -f  # Install any missing dependencies
   ```
3. **Verify Installation**: Launch XFB from the applications menu or run `xfb` in terminal

### First Launch

When you first launch XFB, the accessibility system will automatically initialize:

1. **Accessibility Detection**: XFB automatically detects if ORCA is running
2. **Initial Configuration**: The system sets up default accessibility preferences
3. **Welcome Message**: ORCA will announce "XFB Radio Broadcasting Software loaded with accessibility support"

## ORCA Configuration

### Basic ORCA Setup

1. **Enable ORCA**: Press `Alt+F2`, type `orca`, and press Enter
2. **ORCA Preferences**: Press `Insert+Space` to open ORCA preferences
3. **Speech Settings**: 
   - Set speech rate to comfortable level (recommended: 50-70%)
   - Enable punctuation level: "Some" or "Most"
   - Enable progress bar updates

### XFB-Specific ORCA Settings

1. **Application-Specific Settings**:
   - In ORCA preferences, go to "Application" tab
   - Add XFB to application list if not automatically detected
   - Enable "Speak object mnemonics" for keyboard shortcuts

2. **Recommended Speech Settings for XFB**:
   - **Verbosity**: Verbose (for detailed information)
   - **Progress Bar Updates**: Every 10% (for file operations)
   - **System Messages**: Enabled (for important alerts)

### Testing ORCA Integration

1. **Launch XFB**: Start XFB Radio Broadcasting Software
2. **Navigation Test**: Press `Tab` to navigate through interface elements
3. **Announcement Test**: ORCA should announce each control with its name and type
4. **Feedback Test**: Click a button and listen for confirmation announcement

## Getting Started

### Main Interface Overview

The XFB main window consists of several key areas:

1. **Menu Bar** (Alt+M): File, Edit, View, Tools, Help menus
2. **Toolbar** (Alt+T): Quick access to common functions
3. **Music Library** (Alt+L): Browse and manage your music collection
4. **Playlist Area** (Alt+P): Current playlist and queue management
5. **Player Controls** (Alt+C): Playback, volume, and transport controls
6. **Status Bar** (Alt+S): Current status and live information

### Navigation Basics

- **Tab Navigation**: Use `Tab` and `Shift+Tab` to move between interface sections
- **Arrow Navigation**: Use arrow keys within lists, grids, and menus
- **Quick Access**: Use `Alt+Letter` combinations to jump to specific areas
- **Context Menus**: Press `Menu` key or `Shift+F10` for context-sensitive options

### Essential First Steps

1. **Set Up Music Library**:
   - Press `Ctrl+Shift+L` to open library management
   - Add your music directories using `Ctrl+A`
   - Wait for library scan to complete (progress announced)

2. **Configure Audio Settings**:
   - Press `Ctrl+,` to open preferences
   - Navigate to "Audio" section
   - Select your audio output device
   - Test audio output with `Ctrl+T`

3. **Create First Playlist**:
   - Press `Ctrl+N` to create new playlist
   - Name your playlist and press Enter
   - Add tracks using `Ctrl+A` or drag-and-drop alternatives

## Keyboard Navigation

### Global Shortcuts

| Shortcut | Function | Description |
|----------|----------|-------------|
| `Ctrl+O` | Open File | Open audio file or playlist |
| `Ctrl+S` | Save | Save current playlist or configuration |
| `Ctrl+N` | New Playlist | Create new playlist |
| `Ctrl+,` | Preferences | Open settings dialog |
| `F1` | Help | Open help system |
| `Alt+F4` | Exit | Close XFB application |

### Player Controls

| Shortcut | Function | Description |
|----------|----------|-------------|
| `Space` | Play/Pause | Toggle playback |
| `Ctrl+Space` | Stop | Stop playback |
| `Ctrl+Right` | Next Track | Skip to next track |
| `Ctrl+Left` | Previous Track | Go to previous track |
| `Ctrl+Up` | Volume Up | Increase volume by 5% |
| `Ctrl+Down` | Volume Down | Decrease volume by 5% |
| `Ctrl+M` | Mute | Toggle mute |

### Library Navigation

| Shortcut | Function | Description |
|----------|----------|-------------|
| `Ctrl+F` | Find | Search music library |
| `F3` | Find Next | Continue search |
| `Shift+F3` | Find Previous | Search backwards |
| `Ctrl+A` | Select All | Select all items in current view |
| `Delete` | Remove | Remove selected items |
| `F2` | Rename | Rename selected item |
| `Enter` | Play | Play selected track |

### Playlist Management

| Shortcut | Function | Description |
|----------|----------|-------------|
| `Ctrl+Shift+A` | Add to Playlist | Add selected tracks to playlist |
| `Ctrl+X` | Cut | Cut selected items |
| `Ctrl+C` | Copy | Copy selected items |
| `Ctrl+V` | Paste | Paste items to current location |
| `Ctrl+Z` | Undo | Undo last action |
| `Ctrl+Y` | Redo | Redo last undone action |

### Grid Navigation

When navigating music library grids:

- **Arrow Keys**: Move between cells
- **Home**: Go to first column in current row
- **End**: Go to last column in current row
- **Ctrl+Home**: Go to first cell in grid
- **Ctrl+End**: Go to last cell in grid
- **Page Up/Down**: Navigate by page
- **F2**: Edit current cell (if editable)
- **Escape**: Cancel edit operation

### Time Announcements

| Shortcut | Function | Description |
|----------|----------|-------------|
| `Ctrl+T` | Current Time | Announce current playback time |
| `Ctrl+Shift+T` | Remaining Time | Announce time remaining in track |
| `Ctrl+Alt+T` | Total Duration | Announce total track duration |

## Audio Feedback

### Feedback Types

XFB provides several types of audio feedback:

1. **Action Confirmations**: Immediate feedback for button clicks and menu selections
2. **Status Updates**: Announcements for playback state changes
3. **Progress Reports**: Updates during long operations like library scans
4. **Error Notifications**: Clear announcements of any errors or issues
5. **Navigation Feedback**: Confirmation of focus changes and selections

### Customizing Feedback

Access feedback settings through `Ctrl+,` → Accessibility:

1. **Verbosity Level**:
   - **Terse**: Minimal announcements (control names only)
   - **Normal**: Standard announcements (control names and states)
   - **Verbose**: Detailed announcements (includes hints and descriptions)

2. **Announcement Timing**:
   - **Immediate**: Instant feedback for all actions
   - **Delayed**: Brief pause before announcements
   - **On Request**: Announcements only when specifically requested

3. **Feedback Categories**:
   - Enable/disable specific types of announcements
   - Adjust volume levels for different feedback types
   - Configure interruption behavior for urgent messages

## Broadcasting Workflow

### Preparing for a Show

1. **Library Preparation**:
   - Ensure music library is up to date (`Ctrl+Shift+R` to refresh)
   - Create playlists for your show segments
   - Test audio levels and equipment

2. **Playlist Setup**:
   - Create main show playlist (`Ctrl+N`)
   - Add intro/outro music and jingles
   - Arrange tracks in broadcast order
   - Set crossfade and transition settings

3. **Audio Configuration**:
   - Verify audio output device (`Ctrl+,` → Audio)
   - Test microphone input levels
   - Configure streaming settings if broadcasting live

### During Live Broadcasting

1. **Playback Control**:
   - Use `Space` for quick play/pause
   - Monitor time remaining with `Ctrl+Shift+T`
   - Queue next tracks using playlist navigation

2. **Live Announcements**:
   - XFB announces track changes automatically
   - Use `Ctrl+T` to check current time during speech
   - Monitor recording status in status bar

3. **Emergency Procedures**:
   - `Ctrl+Space` for immediate stop
   - `Ctrl+M` for quick mute
   - `F12` for emergency backup playlist

### Post-Show Tasks

1. **Save Session**:
   - Save playlists with `Ctrl+S`
   - Export show log if required
   - Backup important configurations

2. **Library Maintenance**:
   - Add new music to library
   - Update track metadata as needed
   - Clean up temporary playlists

## Accessibility Settings

### Opening Accessibility Preferences

1. Press `Ctrl+,` to open main preferences
2. Navigate to "Accessibility" section using arrow keys
3. Press `Tab` to move between settings categories

### Verbosity Settings

**Terse Mode**:
- Announces only essential information
- Suitable for experienced users
- Minimal interruption during broadcasting

**Normal Mode** (Default):
- Balanced level of information
- Includes control states and context
- Recommended for most users

**Verbose Mode**:
- Detailed descriptions and usage hints
- Helpful for learning the interface
- May be too detailed for experienced users

### Keyboard Customization

1. **Accessing Shortcut Settings**:
   - In Accessibility preferences, select "Keyboard" tab
   - Browse available actions using arrow keys
   - Press `Enter` to modify a shortcut

2. **Creating Custom Shortcuts**:
   - Select action to customize
   - Press new key combination
   - Confirm with `Enter` or cancel with `Escape`

3. **Conflict Resolution**:
   - XFB warns about conflicting shortcuts
   - Choose to override or select different combination
   - Test new shortcuts before saving

### Braille Display Support

If you use a braille display:

1. **Enable Braille Output**:
   - Check "Enable Braille Support" in Accessibility preferences
   - Select your braille display model
   - Configure connection settings (USB/Bluetooth)

2. **Braille Formatting**:
   - Choose between computer braille and literary braille
   - Set line length for your display
   - Configure cursor routing behavior

3. **Braille-Specific Features**:
   - Track information displayed on braille line
   - Time remaining shown in braille
   - Status indicators for recording/streaming

## Troubleshooting

### ORCA Not Announcing XFB Elements

**Symptoms**: ORCA is silent when navigating XFB interface

**Solutions**:
1. **Restart ORCA**: Press `Insert+Q` to quit ORCA, then restart it
2. **Check AT-SPI**: Ensure AT-SPI service is running:
   ```bash
   systemctl --user status at-spi-dbus-bus
   ```
3. **Verify XFB Accessibility**: In XFB preferences, ensure accessibility is enabled
4. **Update Software**: Ensure both XFB and ORCA are up to date

### Keyboard Shortcuts Not Working

**Symptoms**: Keyboard shortcuts don't trigger expected actions

**Solutions**:
1. **Check Focus**: Ensure correct widget has keyboard focus
2. **Verify Shortcuts**: Check Accessibility preferences for shortcut conflicts
3. **Reset Shortcuts**: Use "Reset to Defaults" in keyboard settings
4. **Desktop Conflicts**: Check for conflicts with desktop environment shortcuts

### Audio Feedback Issues

**Symptoms**: No audio feedback or announcements

**Solutions**:
1. **Check Audio System**: Verify PulseAudio/ALSA is working
2. **ORCA Speech Settings**: Ensure ORCA speech is enabled and audible
3. **XFB Audio Settings**: Verify audio output device in XFB preferences
4. **Volume Levels**: Check system volume and ORCA volume settings

### Performance Issues

**Symptoms**: Slow response or delayed announcements

**Solutions**:
1. **System Resources**: Check available RAM and CPU usage
2. **Library Size**: Large music libraries may cause delays
3. **Accessibility Cache**: Clear accessibility cache in preferences
4. **Background Processes**: Close unnecessary applications

### Library Scanning Problems

**Symptoms**: Music library not updating or scanning fails

**Solutions**:
1. **File Permissions**: Ensure XFB can read music directories
2. **Supported Formats**: Verify audio files are in supported formats
3. **Manual Refresh**: Use `Ctrl+Shift+R` to force library refresh
4. **Database Reset**: Reset library database in preferences if needed

### Braille Display Issues

**Symptoms**: Braille display not showing XFB information

**Solutions**:
1. **Connection**: Verify braille display is connected and recognized
2. **ORCA Braille Settings**: Check ORCA braille configuration
3. **XFB Braille Support**: Ensure braille support is enabled in XFB
4. **Driver Updates**: Update braille display drivers if available

## Support

### Getting Help

1. **Built-in Help**: Press `F1` for context-sensitive help
2. **User Manual**: Access complete manual through Help menu
3. **Keyboard Reference**: Press `Ctrl+F1` for shortcut reference
4. **Accessibility Guide**: This guide is available in Help → Accessibility

### Community Support

- **User Forums**: Join the XFB accessibility community forum
- **Mailing List**: Subscribe to accessibility-focused mailing list
- **Video Tutorials**: Watch accessibility-focused video tutorials
- **User Groups**: Connect with other visually impaired broadcasters

### Reporting Issues

When reporting accessibility issues:

1. **Describe the Problem**: Clearly explain what's not working
2. **Steps to Reproduce**: Provide exact steps to recreate the issue
3. **System Information**: Include OS version, ORCA version, and XFB version
4. **Error Messages**: Include any error messages or unexpected behavior
5. **Workarounds**: Mention any temporary solutions you've found

### Contact Information

- **Email Support**: accessibility@xfb.com
- **Bug Reports**: Use the built-in bug reporter (`Ctrl+Shift+B`)
- **Feature Requests**: Submit through Help → Request Feature
- **Emergency Support**: For critical broadcasting issues, call support hotline

## Advanced Features

### Custom Scripts and Automation

XFB supports custom scripts for advanced users:

1. **Script Location**: Place scripts in `~/.config/xfb/scripts/`
2. **Accessibility Hooks**: Scripts can trigger accessibility announcements
3. **Keyboard Triggers**: Assign scripts to custom keyboard shortcuts
4. **Voice Commands**: Integration with speech recognition systems

### Integration with Other Tools

- **Streaming Software**: Configure XFB with OBS, Icecast, etc.
- **Automation Systems**: Connect with radio automation systems
- **Remote Control**: Use XFB with remote broadcasting setups
- **Mobile Apps**: Control XFB from accessible mobile applications

### Professional Broadcasting Features

- **Live Assist Mode**: Enhanced accessibility for live broadcasting
- **Countdown Timers**: Accessible countdown announcements
- **Emergency Systems**: Quick access to emergency broadcasts
- **Logging**: Comprehensive accessible logging for compliance

Remember: XFB is designed to be fully accessible from the start. If you encounter any barriers to using the software effectively, please don't hesitate to reach out for support. Your feedback helps make XFB better for all users.