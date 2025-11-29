# Testing XFB on Arch Linux

This guide covers various methods for testing XFB on Arch-based systems before publishing to AUR.

## Testing Methods

### Method 1: Local Testing (Recommended)

If you have Arch Linux installed:

```bash
# Navigate to XFB directory
cd /path/to/XFB

# Build and install
makepkg -si

# Test the application
xfb

# Uninstall when done
sudo pacman -R xfb
```

### Method 2: Docker Container Testing

Test in an isolated Arch Linux environment:

```bash
# Build the test container
docker build -t xfb-arch-test -f Dockerfile.arch-test .

# Run the container
docker run -it --rm -v $(pwd):/xfb xfb-arch-test

# Inside container, build and test
makepkg -si
```

### Method 3: Virtual Machine Testing

Use QEMU/KVM or VirtualBox:

1. **Download Arch ISO**: https://archlinux.org/download/
2. **Create VM** with at least:
   - 2 GB RAM
   - 20 GB disk
   - 2 CPU cores
3. **Install Arch Linux**
4. **Clone XFB repository**
5. **Run test script**: `./test-arch-install.sh`

### Method 4: Automated Testing Script

Use the provided test script:

```bash
# Make executable
chmod +x test-arch-install.sh

# Run (requires Arch Linux)
./test-arch-install.sh
```

## Testing Checklist

### Build Testing

- [ ] PKGBUILD syntax is valid
- [ ] Package builds without errors
- [ ] All dependencies are resolved
- [ ] Build completes in reasonable time
- [ ] No warnings from makepkg

### Installation Testing

- [ ] Package installs successfully
- [ ] All files are in correct locations
- [ ] Desktop file is installed
- [ ] Icon is installed
- [ ] Executable is in PATH
- [ ] No file conflicts

### Functionality Testing

- [ ] Application launches
- [ ] Main window appears
- [ ] Audio playback works
- [ ] Database operations work
- [ ] Playlist management works
- [ ] Settings can be changed
- [ ] Application closes cleanly

### Accessibility Testing

- [ ] AT-SPI service is detected
- [ ] ORCA announces UI elements
- [ ] Keyboard navigation works
- [ ] Speech Dispatcher responds
- [ ] Audio feedback plays
- [ ] Braille display works (if available)

### Dependency Testing

- [ ] All required packages are installed
- [ ] Optional dependencies are suggested
- [ ] No missing library errors
- [ ] Qt6 modules load correctly
- [ ] GStreamer plugins work

### Removal Testing

- [ ] Package uninstalls cleanly
- [ ] No orphaned files remain
- [ ] Configuration is preserved (optional)
- [ ] No broken dependencies

## Detailed Testing Procedures

### 1. PKGBUILD Validation

```bash
# Check PKGBUILD syntax
namcap PKGBUILD

# Expected output: No errors, possibly some warnings
```

Common warnings you can ignore:
- `W: Dependency included and not needed ('base-devel')`
- `W: File (...) is a symbolic link`

### 2. Build Process Testing

```bash
# Clean previous builds
rm -rf src pkg *.pkg.tar.zst

# Build package
makepkg -s

# Check for errors in output
# Build should complete with: "Finished making: xfb ..."
```

### 3. Package Validation

```bash
# Check built package
namcap xfb-*.pkg.tar.zst

# List package contents
tar -tzf xfb-*.pkg.tar.zst

# Verify file structure
tar -tzf xfb-*.pkg.tar.zst | grep -E "(usr/bin|usr/share)"
```

Expected files:
```
usr/bin/xfb
usr/share/applications/xfb.desktop
usr/share/pixmaps/xfb.png
usr/share/doc/xfb/README.md
usr/share/doc/xfb/accessibility/...
```

### 4. Installation Testing

```bash
# Install package
sudo pacman -U xfb-*.pkg.tar.zst

# Verify installation
pacman -Qi xfb
pacman -Ql xfb

# Check executable
which xfb
xfb --version
```

### 5. Runtime Testing

```bash
# Launch application
xfb &

# Check process
ps aux | grep xfb

# Check logs
journalctl --user -f | grep -i xfb

# Test basic operations
# - Add music
# - Play track
# - Create playlist
# - Change settings
```

### 6. Accessibility Testing

```bash
# Start AT-SPI
systemctl --user start at-spi-dbus-bus

# Start Speech Dispatcher
systemctl --user start speech-dispatcher

# Test speech
spd-say "Testing speech"

# Launch ORCA
orca &

# Launch XFB and test with ORCA
xfb

# Test keyboard navigation
# - Tab through elements
# - Use arrow keys
# - Test shortcuts
```

### 7. Dependency Testing

```bash
# Check dependencies
pactree xfb

# Verify all dependencies are installed
pacman -Qi $(pactree -u xfb)

# Check for missing libraries
ldd /usr/bin/xfb

# All libraries should be found (not "not found")
```

### 8. Cleanup Testing

```bash
# Remove package
sudo pacman -R xfb

# Verify removal
which xfb  # Should return nothing
pacman -Qi xfb  # Should error

# Check for orphaned files
find /usr -name "*xfb*" 2>/dev/null

# Check configuration (should remain)
ls ~/.config/XFB
```

## Testing on Different Arch Variants

### Manjaro

```bash
# Update system
sudo pacman -Syu

# Install from PKGBUILD
makepkg -si

# Or use pamac
pamac build /path/to/PKGBUILD
```

### EndeavourOS

```bash
# Update system
sudo pacman -Syu

# Install from PKGBUILD
makepkg -si

# Test with yay
yay -S xfb
```

### Garuda Linux

```bash
# Update system
sudo pacman -Syu

# Install from PKGBUILD
makepkg -si

# Test with paru
paru -S xfb
```

### Artix Linux (OpenRC/runit)

```bash
# Update system
sudo pacman -Syu

# Install from PKGBUILD
makepkg -si

# Note: systemd services won't work
# Use manual service management
```

## Automated Testing with GitHub Actions

The repository includes a GitHub Actions workflow that automatically tests the PKGBUILD:

```yaml
# .github/workflows/test-arch-package.yml
# Runs on every push to PKGBUILD
```

To trigger manually:
1. Go to GitHub Actions tab
2. Select "Test Arch Linux Package"
3. Click "Run workflow"

## Performance Testing

### Build Time

```bash
# Measure build time
time makepkg -s

# Expected: < 5 minutes on modern hardware
```

### Package Size

```bash
# Check package size
ls -lh xfb-*.pkg.tar.zst

# Expected: 5-15 MB compressed
```

### Installation Time

```bash
# Measure installation time
time sudo pacman -U xfb-*.pkg.tar.zst

# Expected: < 30 seconds
```

### Startup Time

```bash
# Measure startup time
time xfb --version

# Expected: < 2 seconds
```

## Troubleshooting Test Failures

### Build Fails: Missing Dependencies

```bash
# Install missing build dependencies
sudo pacman -S base-devel cmake qt6-base

# Try again
makepkg -si
```

### Build Fails: CMake Errors

```bash
# Check CMake version
cmake --version

# Update CMake if needed
sudo pacman -S cmake

# Clean and rebuild
rm -rf src pkg
makepkg -si
```

### Installation Fails: File Conflicts

```bash
# Check conflicting files
pacman -Qo /path/to/conflicting/file

# Remove conflicting package
sudo pacman -R conflicting-package

# Try again
sudo pacman -U xfb-*.pkg.tar.zst
```

### Runtime Fails: Missing Libraries

```bash
# Check missing libraries
ldd /usr/bin/xfb | grep "not found"

# Install missing packages
sudo pacman -S missing-package

# Or reinstall with dependencies
sudo pacman -U xfb-*.pkg.tar.zst
```

### ORCA Not Working

```bash
# Check AT-SPI
systemctl --user status at-spi-dbus-bus

# Restart if needed
systemctl --user restart at-spi-dbus-bus

# Check ORCA
orca --version

# Restart ORCA
killall orca
orca &
```

## Test Reports

Document your test results:

```markdown
## Test Report: XFB 2.0.0 on Arch Linux

**Date**: YYYY-MM-DD
**Tester**: Your Name
**System**: Arch Linux (kernel version)

### Build Test
- [ ] PKGBUILD validates
- [ ] Package builds successfully
- [ ] Build time: X minutes

### Installation Test
- [ ] Package installs
- [ ] All files present
- [ ] No conflicts

### Functionality Test
- [ ] Application launches
- [ ] Audio playback works
- [ ] UI responsive

### Accessibility Test
- [ ] ORCA integration works
- [ ] Keyboard navigation works
- [ ] Speech output works

### Issues Found
- None / List issues here

### Conclusion
- [ ] Ready for AUR submission
- [ ] Needs fixes (describe)
```

## Continuous Testing

Set up regular testing:

```bash
# Create a cron job for weekly testing
crontab -e

# Add line:
0 0 * * 0 cd /path/to/XFB && ./test-arch-install.sh > test-report.log 2>&1
```

## Resources

- **Arch Wiki - Creating Packages**: https://wiki.archlinux.org/title/Creating_packages
- **Arch Wiki - PKGBUILD**: https://wiki.archlinux.org/title/PKGBUILD
- **namcap**: https://wiki.archlinux.org/title/Namcap
- **makepkg**: https://wiki.archlinux.org/title/Makepkg

## Next Steps

After successful testing:

1. Review [AUR Publishing Guide](AUR_PUBLISHING_GUIDE.md)
2. Complete [AUR Submission Checklist](../AUR_SUBMISSION_CHECKLIST.md)
3. Submit to AUR
4. Monitor for user feedback

---

**Happy Testing!** 🧪
