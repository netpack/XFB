# XFB Accessibility Troubleshooting Guide

## Common Issues and Solutions

### ORCA Integration Problems

#### Issue: ORCA Not Announcing XFB Interface Elements

**Symptoms:**
- ORCA is silent when navigating XFB
- Tab key moves focus but no announcements
- Buttons and controls are not announced

**Diagnostic Steps:**
1. Test ORCA with other applications (like text editor)
2. Check if XFB accessibility is enabled in preferences
3. Verify AT-SPI service is running

**Solutions:**

**Solution 1: Restart ORCA**
```bash
# Quit ORCA
orca --quit
# Wait 2 seconds
sleep 2
# Restart ORCA
orca &
```

**Solution 2: Check AT-SPI Service**
```bash
# Check if AT-SPI is running
systemctl --user status at-spi-dbus-bus

# If not running, start it
systemctl --user start at-spi-dbus-bus
```

**Solution 3: Enable XFB Accessibility**
1. Press `Ctrl+,` in XFB to open preferences
2. Navigate to "Accessibility" section
3. Ensure "Enable Accessibility Support" is checked
4. Restart XFB

**Solution 4: Reset Accessibility Settings**
1. Close XFB completely
2. Remove accessibility cache: `rm -rf ~/.cache/xfb/accessibility/`
3. Restart XFB

#### Issue: Partial ORCA Announcements

**Symptoms:**
- Some controls are announced, others are not
- Inconsistent announcement behavior
- Missing state information (checked/unchecked, etc.)

**Solutions:**

**Solution 1: Update ORCA Settings**
1. Press `Insert+Space` to open ORCA preferences
2. Go to "Speech" tab
3. Set verbosity to "Verbose"
4. Enable "Speak object mnemonics"
5. Apply and test

**Solution 2: Check XFB Verbosity Settings**
1. In XFB, press `Ctrl+Alt+,` for accessibility settings
2. Set verbosity level to "Verbose"
3. Enable all announcement categories
4. Test navigation

### Keyboard Navigation Issues

#### Issue: Keyboard Shortcuts Not Working

**Symptoms:**
- Pressing shortcuts has no effect
- Some shortcuts work, others don't
- Shortcuts work in other applications but not XFB

**Diagnostic Steps:**
1. Check if correct widget has focus
2. Test shortcuts in different XFB areas
3. Verify shortcuts in preferences

**Solutions:**

**Solution 1: Check Focus**
1. Press `Ctrl+?` to hear current location
2. Use `Tab` to navigate to correct area
3. Try shortcut again

**Solution 2: Reset Keyboard Shortcuts**
1. Press `Ctrl+Shift+,` to open keyboard settings
2. Click "Reset to Defaults" button
3. Restart XFB and test

**Solution 3: Check for Conflicts**
1. In keyboard settings, look for conflict warnings
2. Change conflicting shortcuts to different keys
3. Test new shortcuts

#### Issue: Tab Navigation Skips Controls

**Symptoms:**
- Tab key skips over some controls
- Focus jumps unexpectedly
- Some areas cannot be reached with Tab

**Solutions:**

**Solution 1: Check Widget Focus Policy**
1. This is usually an XFB bug - report it
2. Try using arrow keys within the area
3. Use Alt+Letter shortcuts to jump to areas

**Solution 2: Use Alternative Navigation**
- `Alt+M` for menu bar
- `Alt+L` for library
- `Alt+P` for playlist
- `Alt+C` for controls

### Audio Feedback Problems

#### Issue: No Audio Feedback or Announcements

**Symptoms:**
- Actions are performed but no confirmation sounds
- No progress announcements
- Silent operation despite settings

**Diagnostic Steps:**
1. Test ORCA speech in other applications
2. Check system audio settings
3. Verify XFB audio configuration

**Solutions:**

**Solution 1: Check ORCA Speech**
1. Press `Insert+Space` for ORCA preferences
2. Go to "Speech" tab
3. Test speech with "Test" button
4. Adjust speech rate and volume if needed

**Solution 2: Check System Audio**
```bash
# Test system audio
speaker-test -t sine -f 1000 -l 1

# Check PulseAudio
pulseaudio --check -v

# Restart PulseAudio if needed
pulseaudio --kill
pulseaudio --start
```

**Solution 3: XFB Audio Settings**
1. Press `Ctrl+,` to open XFB preferences
2. Go to "Audio" section
3. Select correct output device
4. Test audio with built-in test

**Solution 4: Check Accessibility Audio Settings**
1. Press `Ctrl+Alt+,` for accessibility settings
2. Go to "Audio Feedback" section
3. Ensure feedback is enabled
4. Test different feedback types

#### Issue: Delayed or Interrupted Announcements

**Symptoms:**
- Announcements come several seconds late
- Announcements are cut off
- Multiple announcements overlap

**Solutions:**

**Solution 1: Adjust Announcement Timing**
1. In accessibility settings, go to "Timing" section
2. Reduce announcement delay
3. Enable announcement queuing
4. Test with different settings

**Solution 2: Check System Performance**
```bash
# Check CPU usage
top

# Check memory usage
free -h

# Check for high I/O
iotop
```

**Solution 3: Optimize XFB Performance**
1. Close unnecessary applications
2. Reduce music library size if very large
3. Disable visual effects in desktop environment
4. Consider upgrading hardware

### Library and Database Issues

#### Issue: Music Library Not Scanning

**Symptoms:**
- Library appears empty
- Scan progress never completes
- Error messages during scan

**Solutions:**

**Solution 1: Check File Permissions**
```bash
# Check if XFB can read music directory
ls -la /path/to/music/directory

# Fix permissions if needed
chmod -R 755 /path/to/music/directory
```

**Solution 2: Check Supported Formats**
- Verify files are in supported formats (MP3, FLAC, OGG, etc.)
- Check for corrupted files
- Test with a small directory first

**Solution 3: Manual Library Refresh**
1. Press `Ctrl+Shift+L` to open library management
2. Remove problematic directories
3. Add directories one at a time
4. Monitor scan progress

**Solution 4: Reset Library Database**
1. Close XFB completely
2. Backup library: `cp ~/.config/xfb/library.db ~/.config/xfb/library.db.backup`
3. Remove database: `rm ~/.config/xfb/library.db`
4. Restart XFB and re-scan library

#### Issue: Search Not Working

**Symptoms:**
- Search returns no results
- Search dialog not accessible
- Search results not announced

**Solutions:**

**Solution 1: Check Search Syntax**
- Use simple terms first
- Try partial matches
- Check spelling

**Solution 2: Rebuild Search Index**
1. In library management, find "Rebuild Index" option
2. Wait for indexing to complete
3. Test search again

### Playlist Management Issues

#### Issue: Cannot Add Tracks to Playlist

**Symptoms:**
- Drag and drop doesn't work
- Keyboard shortcuts don't add tracks
- No confirmation of additions

**Solutions:**

**Solution 1: Use Keyboard Alternatives**
1. Select tracks in library with arrow keys
2. Press `Ctrl+Shift+A` to add to current playlist
3. Listen for confirmation announcement

**Solution 2: Check Playlist State**
1. Ensure a playlist is selected/active
2. Create new playlist if needed (`Ctrl+N`)
3. Try adding tracks again

**Solution 3: Use Context Menu**
1. Select tracks in library
2. Press `Menu` key or `Shift+F10`
3. Choose "Add to Playlist" option

### Performance Issues

#### Issue: Slow Response or Lag

**Symptoms:**
- Delayed response to keyboard input
- Slow navigation between interface elements
- Long pauses before announcements

**Diagnostic Steps:**
1. Check system resources (CPU, memory, disk)
2. Monitor XFB process usage
3. Test with smaller music library

**Solutions:**

**Solution 1: Optimize System Performance**
```bash
# Close unnecessary processes
# Check running processes
ps aux | grep -v "\[.*\]" | sort -k3 -nr | head -10

# Free up memory
sudo sysctl vm.drop_caches=3
```

**Solution 2: Optimize XFB Settings**
1. Reduce library scan frequency
2. Disable visual effects in preferences
3. Limit playlist size for better performance
4. Use terse verbosity mode

**Solution 3: Hardware Considerations**
- Ensure adequate RAM (8GB+ recommended)
- Use SSD for better I/O performance
- Close other resource-intensive applications

### Braille Display Issues

#### Issue: Braille Display Not Working

**Symptoms:**
- No output on braille display
- Incorrect or garbled braille text
- Display not recognized

**Solutions:**

**Solution 1: Check ORCA Braille Settings**
1. Press `Insert+Space` for ORCA preferences
2. Go to "Braille" tab
3. Verify display is detected and configured
4. Test braille output

**Solution 2: Check XFB Braille Support**
1. In XFB accessibility settings, enable braille support
2. Select correct braille display model
3. Configure braille formatting options
4. Test with simple navigation

**Solution 3: Hardware and Drivers**
```bash
# Check USB connection (if USB display)
lsusb

# Check Bluetooth connection (if Bluetooth display)
bluetoothctl devices

# Restart braille driver
sudo systemctl restart brltty
```

### Configuration and Settings Issues

#### Issue: Settings Not Saving

**Symptoms:**
- Preferences reset after restart
- Custom shortcuts don't persist
- Accessibility settings revert to defaults

**Solutions:**

**Solution 1: Check File Permissions**
```bash
# Check XFB config directory permissions
ls -la ~/.config/xfb/

# Fix permissions if needed
chmod -R 755 ~/.config/xfb/
```

**Solution 2: Check Disk Space**
```bash
# Check available disk space
df -h ~/.config/
```

**Solution 3: Reset Configuration**
1. Backup current settings: `cp -r ~/.config/xfb ~/.config/xfb.backup`
2. Remove config: `rm -rf ~/.config/xfb`
3. Restart XFB to create fresh configuration
4. Reconfigure settings

### Emergency Procedures

#### Complete Accessibility Failure

If accessibility stops working completely:

1. **Immediate Actions:**
   - Press `Ctrl+S` to save current work
   - Press `Alt+F4` to close XFB safely
   - Restart ORCA: `orca --quit && orca &`

2. **Recovery Steps:**
   - Restart XFB
   - Check system accessibility services
   - Reset XFB accessibility settings
   - Contact support if issue persists

#### Audio System Failure During Broadcasting

If audio fails during live broadcasting:

1. **Emergency Stop:** Press `F9` for emergency stop
2. **Switch Audio:** Press `Ctrl+,` → Audio → Select backup device
3. **Test Audio:** Press `Ctrl+T` to test
4. **Resume:** Press `Space` to resume playback

## Getting Additional Help

### Built-in Diagnostics

XFB includes diagnostic tools:

1. **Accessibility Validator:** `Ctrl+Shift+V` to run accessibility check
2. **Performance Monitor:** `Ctrl+Shift+P` to check performance
3. **Audio Test:** `Ctrl+Shift+A` to test audio system
4. **System Info:** `Ctrl+Shift+I` to view system information

### Log Files

Check log files for detailed error information:

```bash
# XFB main log
tail -f ~/.config/xfb/logs/xfb.log

# Accessibility log
tail -f ~/.config/xfb/logs/accessibility.log

# Audio system log
tail -f ~/.config/xfb/logs/audio.log
```

### Reporting Bugs

When reporting accessibility issues:

1. **Describe the problem clearly**
2. **Include steps to reproduce**
3. **Attach relevant log files**
4. **Specify your system configuration:**
   - OS version and distribution
   - ORCA version
   - XFB version
   - Hardware details (audio interface, braille display, etc.)

### Contact Support

- **Email:** accessibility@xfb.com
- **Bug Tracker:** Use `Ctrl+Shift+B` in XFB to report bugs
- **Community Forum:** Join the XFB accessibility forum
- **Emergency Support:** For critical broadcasting issues, use emergency contact

Remember: Most accessibility issues can be resolved by restarting ORCA and XFB. If problems persist, don't hesitate to seek help from the community or support team.