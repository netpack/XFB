# XFB Release Guide

Complete guide for releasing XFB across all platforms.

## Quick Release

For a full release to all platforms:

```bash
./release.sh
```

Select option 1 (Full Release) and follow the prompts.

## Manual Release Process

### Prerequisites

- [ ] Git configured with GitHub credentials
- [ ] AUR account with SSH key configured
- [ ] All changes committed
- [ ] Version numbers updated in:
  - [ ] CMakeLists.txt
  - [ ] PKGBUILD
  - [ ] debian/changelog
  - [ ] README.md

### Step 1: Safety Check

Run the safety check to ensure no sensitive information:

```bash
./push-to-github.sh
```

Or just check without pushing:

```bash
./release.sh
# Select option 6 (Safety Check Only)
```

### Step 2: Build Packages

#### Debian Package

```bash
./build-deb-no-tests.sh
```

This creates: `xfb_2.0.0-1_amd64.deb`

#### Arch Package (on Arch Linux)

```bash
makepkg -sf
```

This creates: `xfb-2.0.0-1-x86_64.pkg.tar.zst`

#### macOS Package (on macOS)

```bash
./build-macos.sh
```

This creates: `XFB-2.0.0-macOS.dmg`

### Step 3: Test Packages

#### Test Debian Package

```bash
# On Debian/Ubuntu system
sudo apt install ./xfb_2.0.0-1_amd64.deb

# Test
xfb

# Uninstall
sudo apt remove xfb
```

#### Test Arch Package

```bash
# On Arch Linux
sudo pacman -U xfb-2.0.0-1-x86_64.pkg.tar.zst

# Test
xfb

# Uninstall
sudo pacman -R xfb
```

### Step 4: Push to GitHub

```bash
./push-to-github.sh
```

This will:
- Check for sensitive information
- Verify .gitignore
- Show what will be pushed
- Push to GitHub

### Step 5: Create GitHub Release

1. Go to https://github.com/netpack/XFB/releases/new
2. Click "Draft a new release"
3. Fill in:
   - **Tag**: `v2.0.0`
   - **Title**: `XFB 2.0.0 - Accessibility Enhanced`
   - **Description**:

```markdown
# XFB 2.0.0 - Accessibility Enhanced

Major release with comprehensive accessibility features and ORCA screen reader integration.

## 🎯 Highlights

- **Full ORCA Integration**: Complete screen reader support for visually impaired users
- **Enhanced Keyboard Navigation**: Navigate entire application without mouse
- **Audio Feedback**: Audible confirmation for all user actions
- **Braille Display Support**: BrlTTY integration for braille displays
- **Performance Improvements**: Optimized database operations and memory usage

## 📦 Installation

### Debian/Ubuntu/Mint

```bash
# Download and install
wget https://github.com/netpack/XFB/releases/download/v2.0.0/xfb_2.0.0-1_amd64.deb
sudo apt install ./xfb_2.0.0-1_amd64.deb
```

### Arch Linux

```bash
yay -S xfb
```

### macOS

Download `XFB-2.0.0-macOS.dmg` and drag to Applications folder.

## 🔧 What's New

### Accessibility Features
- ORCA screen reader integration with live region updates
- Complete keyboard navigation system
- Audio feedback service for all actions
- Braille display support via BrlTTY
- Customizable accessibility preferences
- Context-sensitive help system

### Performance
- Optimized database queries
- Improved memory management
- Faster playlist loading
- Reduced CPU usage during playback

### Bug Fixes
- Fixed audio glitches during crossfade
- Resolved database locking issues
- Fixed memory leaks in audio processing
- Improved error handling

## 📚 Documentation

- [User Guide](docs/accessibility/user-guide/README.md)
- [Keyboard Reference](docs/accessibility/user-guide/keyboard-reference.md)
- [Installation Guide](docs/accessibility/deployment/installation-guide.md)
- [Troubleshooting](docs/accessibility/user-guide/troubleshooting.md)

## 🐛 Known Issues

None at this time.

## 💬 Support

- **GitHub Issues**: https://github.com/netpack/XFB/issues
- **Email**: info@netpack.pt
- **Website**: https://netpack.pt

---

**Full Changelog**: https://github.com/netpack/XFB/compare/v1.0.0...v2.0.0
```

4. Upload files:
   - `xfb_2.0.0-1_amd64.deb`
   - `XFB-2.0.0-macOS.dmg` (if available)
   - `CHANGELOG.md` (if exists)

5. Click "Publish release"

### Step 6: Update AUR

```bash
./update-aur.sh
```

This will:
- Clone/update AUR repository
- Copy PKGBUILD
- Generate .SRCINFO
- Commit and push to AUR

Verify at: https://aur.archlinux.org/packages/xfb

### Step 7: Update Debian Repository (Optional)

For `apt install xfb` support:

```bash
./update-debian-repo.sh
```

Then push to GitHub Pages:

```bash
git checkout gh-pages || git checkout --orphan gh-pages
git rm -rf . 2>/dev/null || true
cp -r debian-repo/* .
git add .
git commit -m "Update Debian repository to version 2.0.0"
git push origin gh-pages
```

Enable GitHub Pages:
1. Go to Settings → Pages
2. Source: gh-pages branch
3. Save

Users can then install with:

```bash
echo "deb [trusted=yes] https://netpack.github.io/XFB stable main" | sudo tee /etc/apt/sources.list.d/xfb.list
sudo apt update
sudo apt install xfb
```

### Step 8: Announce Release

#### Update README.md

Ensure installation instructions are current.

#### Social Media

Post on:
- Twitter/X
- LinkedIn
- Reddit (r/linux, r/archlinux, r/debian)
- Hacker News

Example post:

```
🎉 XFB 2.0.0 Released!

Professional radio automation software with comprehensive accessibility support.

✨ New in 2.0:
- Full ORCA screen reader integration
- Complete keyboard navigation
- Audio feedback system
- Braille display support

📦 Install:
- Debian/Ubuntu: apt install
- Arch Linux: yay -S xfb
- macOS: Download DMG

🔗 https://github.com/netpack/XFB

#OpenSource #Accessibility #RadioAutomation #Linux
```

#### Email Newsletter

If you have a mailing list, send release announcement.

#### Website Update

Update https://netpack.pt with new version information.

## Version Numbering

XFB follows Semantic Versioning (SemVer):

- **Major** (2.0.0): Breaking changes, major new features
- **Minor** (2.1.0): New features, backwards compatible
- **Patch** (2.0.1): Bug fixes, backwards compatible

## Release Checklist

### Pre-Release

- [ ] All tests passing
- [ ] Documentation updated
- [ ] CHANGELOG.md updated
- [ ] Version numbers updated everywhere
- [ ] No sensitive information in code
- [ ] .gitignore up to date

### Build

- [ ] Debian package built and tested
- [ ] Arch package built and tested (if on Arch)
- [ ] macOS package built and tested (if on macOS)
- [ ] All packages install correctly
- [ ] Application launches and works

### Release

- [ ] Code pushed to GitHub
- [ ] GitHub release created
- [ ] Packages uploaded to GitHub release
- [ ] AUR package updated
- [ ] Debian repository updated (optional)
- [ ] Release announced

### Post-Release

- [ ] Monitor GitHub issues for bug reports
- [ ] Respond to AUR comments
- [ ] Update documentation if needed
- [ ] Plan next release

## Hotfix Release

For urgent bug fixes:

1. Create hotfix branch:
   ```bash
   git checkout -b hotfix/2.0.1
   ```

2. Fix the bug

3. Update version to 2.0.1

4. Test thoroughly

5. Merge to main:
   ```bash
   git checkout main
   git merge hotfix/2.0.1
   ```

6. Follow normal release process

7. Delete hotfix branch:
   ```bash
   git branch -d hotfix/2.0.1
   ```

## Rollback

If a release has critical issues:

1. **GitHub**: Delete the release and tag
2. **AUR**: Revert PKGBUILD to previous version
3. **Debian Repo**: Remove broken package from repository
4. **Announce**: Post about the issue and rollback

## Automation

Consider setting up GitHub Actions for automated releases:

```yaml
# .github/workflows/release.yml
name: Release

on:
  push:
    tags:
      - 'v*'

jobs:
  build-deb:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build .deb
        run: ./build-deb-no-tests.sh
      - name: Upload to release
        uses: actions/upload-release-asset@v1
        with:
          upload_url: ${{ github.event.release.upload_url }}
          asset_path: ./xfb_*.deb
          asset_name: xfb_${{ github.ref_name }}_amd64.deb
          asset_content_type: application/vnd.debian.binary-package
```

## Troubleshooting

### Push Rejected

```bash
# Pull latest changes
git pull --rebase origin main

# Push again
git push origin main
```

### AUR Update Failed

```bash
# Check SSH connection
ssh -T aur@aur.archlinux.org

# Verify PKGBUILD
namcap PKGBUILD

# Regenerate .SRCINFO
makepkg --printsrcinfo > .SRCINFO
```

### Package Build Failed

```bash
# Clean build directory
rm -rf build/ src/ pkg/

# Try again
./build-deb-no-tests.sh
```

## Resources

- **Semantic Versioning**: https://semver.org/
- **GitHub Releases**: https://docs.github.com/en/repositories/releasing-projects-on-github
- **AUR Guidelines**: https://wiki.archlinux.org/title/AUR_submission_guidelines
- **Debian Packaging**: https://www.debian.org/doc/manuals/maint-guide/

## Support

For questions about the release process:
- **GitHub Issues**: https://github.com/netpack/XFB/issues
- **Email**: info@netpack.pt

---

**Last Updated**: 2024-11-29
**Current Version**: 2.0.0
