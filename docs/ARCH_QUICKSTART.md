# XFB Quick Start Guide for Arch Linux

Get XFB up and running on Arch Linux in minutes.

## Installation Methods

### Method 1: Install from AUR (Recommended)

Using an AUR helper like `yay` or `paru`:

```bash
# Using yay
yay -S xfb

# Using paru
paru -S xfb

# Using pamac (Manjaro)
pamac build xfb
```

### Method 2: Manual Installation from AUR

```bash
# Clone the AUR repository
git clone https://aur.archlinux.org/xfb.git
cd xfb

# Build and install
makepkg -si
```

### Method 3: Build from Source

```bash
# Clone the main repository
git clone https://github.com/netpack/XFB.git
cd XFB

# Use the provided PKGBUILD
makepkg -si
```

## First-Time Setup

### 1. Install Dependencies

The package manager will automatically install required dependencies, but you may want optional tools:

```bash
# Audio editing and processing
sudo pacman -S audacity ffmpeg sox

# Accessibility (for visually impaired users)
sudo pacman -S orca brltty espeak-ng

# Audio format support
sudo pacman -S flac vorbis-tools opus-tools wavpack
```

### 2. Enable Accessibility (Optional)

If you need screen reader support:

```bash
# Start AT-SPI service
systemctl --user start at-spi-dbus-bus
systemctl --user enable at-spi-dbus-bus

# Start Speech Dispatcher
systemctl --user start speech-dispatcher
systemctl --user enable speech-dispatcher

# Launch ORCA
orca &
```

### 3. Configure Audio

Ensure your audio system is working:

```bash
# Check audio devices
aplay -l

# Test audio output
speaker-test -c 2 -t wav

# If using PulseAudio
pactl list sinks

# If using PipeWire
pw-cli list-objects | grep node.name
```

## Launching XFB

### From Terminal

```bash
xfb
```

### From Application Menu

Look for "XFB" in your application launcher or desktop menu under "Sound & Video" or "Multimedia".

### From Desktop File

```bash
gtk-launch xfb
```

## Quick Configuration

### First Launch Checklist

1. **Set Audio Output Device**
   - Go to Settings → Audio
   - Select your preferred output device

2. **Configure Database Location**
   - Go to Settings → Database
   - Choose where to store your music library

3. **Add Music**
   - Click "Add Music" or press `Ctrl+M`
   - Browse to your music folder
   - Wait for scanning to complete

4. **Test Playback**
   - Select a track from the library
   - Press `Space` to play/pause
   - Use arrow keys to navigate

## Keyboard Shortcuts

Essential shortcuts for quick navigation:

| Action | Shortcut |
|--------|----------|
| Play/Pause | `Space` |
| Stop | `S` |
| Next Track | `N` or `→` |
| Previous Track | `P` or `←` |
| Volume Up | `+` or `↑` |
| Volume Down | `-` or `↓` |
| Add Music | `Ctrl+M` |
| Search | `Ctrl+F` |
| Quit | `Ctrl+Q` |

For complete keyboard reference, see [Keyboard Reference Guide](accessibility/user-guide/keyboard-reference.md).

## Troubleshooting

### XFB Won't Start

```bash
# Check if dependencies are installed
pacman -Qi xfb

# Run from terminal to see errors
xfb

# Check logs
journalctl --user -xe
```

### No Audio Output

```bash
# Verify audio system
systemctl --user status pulseaudio
# or for PipeWire
systemctl --user status pipewire

# Restart audio system
systemctl --user restart pulseaudio
# or
systemctl --user restart pipewire
```

### Database Errors

```bash
# Reset database (WARNING: This deletes your library)
rm -rf ~/.config/XFB/database.db

# Restart XFB
xfb
```

### ORCA Not Announcing

```bash
# Ensure AT-SPI is running
systemctl --user status at-spi-dbus-bus

# Restart ORCA
killall orca
orca &

# Test speech
spd-say "Testing speech"
```

### Missing Dependencies

```bash
# Reinstall with dependencies
yay -S xfb --needed

# Or manually install missing packages
sudo pacman -S qt6-base qt6-multimedia qt6-webengine
```

## Updating XFB

### Using AUR Helper

```bash
# Update all AUR packages
yay -Syu

# Update only XFB
yay -S xfb
```

### Manual Update

```bash
cd xfb  # Your AUR clone directory
git pull
makepkg -si
```

## Uninstalling

```bash
# Remove package
sudo pacman -R xfb

# Remove with dependencies (careful!)
sudo pacman -Rns xfb

# Remove configuration (optional)
rm -rf ~/.config/XFB
```

## Getting Help

### Documentation

- **User Guide**: `docs/accessibility/user-guide/README.md`
- **Keyboard Reference**: `docs/accessibility/user-guide/keyboard-reference.md`
- **Troubleshooting**: `docs/accessibility/user-guide/troubleshooting.md`

### Online Resources

- **GitHub Issues**: https://github.com/netpack/XFB/issues
- **AUR Comments**: https://aur.archlinux.org/packages/xfb
- **Website**: https://netpack.pt

### Community Support

- Report bugs on GitHub Issues
- Ask questions in AUR comments
- Check existing documentation first

## Performance Tips

### For Low-End Systems

```bash
# Disable web engine if not needed
# Edit ~/.config/XFB/config.ini
[UI]
EnableWebEngine=false
```

### For High-End Systems

```bash
# Enable hardware acceleration
# Edit ~/.config/XFB/config.ini
[Performance]
HardwareAcceleration=true
UseGPU=true
```

## Advanced Configuration

### Custom Audio Backend

```bash
# Force ALSA
QT_MULTIMEDIA_PREFERRED_PLUGINS=alsa xfb

# Force PulseAudio
QT_MULTIMEDIA_PREFERRED_PLUGINS=pulseaudio xfb
```

### Debug Mode

```bash
# Run with debug output
QT_LOGGING_RULES="*.debug=true" xfb

# Save debug log
xfb 2>&1 | tee xfb-debug.log
```

### Custom Theme

```bash
# Use system theme
xfb --style=fusion

# Use specific Qt style
xfb --style=breeze
```

## Next Steps

1. **Import Your Music Library**: Add your music collection
2. **Create Playlists**: Organize your tracks
3. **Configure Automation**: Set up scheduled playback
4. **Explore Accessibility**: Try keyboard navigation and screen reader
5. **Read Full Documentation**: Check `docs/` folder for detailed guides

## Quick Reference Card

```
╔══════════════════════════════════════════════════════════╗
║                    XFB Quick Reference                    ║
╠══════════════════════════════════════════════════════════╣
║ Install:        yay -S xfb                               ║
║ Launch:         xfb                                      ║
║ Update:         yay -Syu                                 ║
║ Uninstall:      sudo pacman -R xfb                       ║
║                                                          ║
║ Play/Pause:     Space                                    ║
║ Stop:           S                                        ║
║ Next:           N or →                                   ║
║ Previous:       P or ←                                   ║
║ Volume:         +/- or ↑/↓                               ║
║                                                          ║
║ Add Music:      Ctrl+M                                   ║
║ Search:         Ctrl+F                                   ║
║ Help:           F1                                       ║
║ Quit:           Ctrl+Q                                   ║
╚══════════════════════════════════════════════════════════╝
```

---

**Welcome to XFB!** 🎵

For detailed information, see the [full documentation](README.md) or visit https://netpack.pt
