# XFB AUR Submission Checklist

Use this checklist to ensure your AUR submission is complete and correct.

## Pre-Submission Checklist

### 1. Account Setup
- [ ] Created AUR account at https://aur.archlinux.org/register
- [ ] Added SSH public key to AUR account
- [ ] Tested SSH connection: `ssh -T aur@aur.archlinux.org`
- [ ] Configured Git with name and email

### 2. Package Files
- [ ] `PKGBUILD` file created and tested
- [ ] `.SRCINFO` generated: `makepkg --printsrcinfo > .SRCINFO`
- [ ] Package builds successfully: `makepkg -si`
- [ ] Package installs without errors
- [ ] All files are in correct locations after installation

### 3. PKGBUILD Validation
- [ ] Package name is lowercase: `xfb`
- [ ] Version matches latest release: `2.0.0`
- [ ] Architecture is correct: `('x86_64' 'aarch64')`
- [ ] License is correct: `GPL3`
- [ ] All dependencies are listed
- [ ] Source URL is correct and accessible
- [ ] Checksums are valid (or using `SKIP` for git tags)

### 4. Testing
- [ ] Tested on clean Arch Linux system
- [ ] Tested with `namcap PKGBUILD`
- [ ] Tested with `namcap *.pkg.tar.zst`
- [ ] Verified all dependencies install correctly
- [ ] Application launches without errors
- [ ] Basic functionality works (play audio, navigate UI)

### 5. Documentation
- [ ] README.md updated with AUR installation instructions
- [ ] Created `docs/AUR_PUBLISHING_GUIDE.md`
- [ ] Created `docs/ARCH_DEPENDENCIES.md`
- [ ] Created `docs/ARCH_QUICKSTART.md`
- [ ] Updated installation table in README

### 6. Optional Enhancements
- [ ] Created GitHub Actions workflow for testing
- [ ] Created test script: `test-arch-install.sh`
- [ ] Tested on multiple Arch-based distros (Manjaro, EndeavourOS, etc.)
- [ ] Verified accessibility features work on Arch

## Submission Steps

### Step 1: Clone AUR Repository

```bash
git clone ssh://aur@aur.archlinux.org/xfb.git aur-xfb
cd aur-xfb
```

### Step 2: Add Files

```bash
# Copy PKGBUILD from main repo
cp /path/to/XFB/PKGBUILD .

# Generate .SRCINFO
makepkg --printsrcinfo > .SRCINFO
```

### Step 3: Initial Commit

```bash
git add PKGBUILD .SRCINFO
git commit -m "Initial import: XFB 2.0.0 - Professional radio automation software"
```

### Step 4: Push to AUR

```bash
git push origin master
```

### Step 5: Verify Submission

- [ ] Visit https://aur.archlinux.org/packages/xfb
- [ ] Package appears in search results
- [ ] Package page displays correctly
- [ ] All metadata is accurate

## Post-Submission Tasks

### Immediate
- [ ] Test installation from AUR: `yay -S xfb`
- [ ] Monitor AUR comments for issues
- [ ] Update GitHub README with AUR badge
- [ ] Announce release on relevant channels

### Ongoing Maintenance
- [ ] Respond to user comments within 48 hours
- [ ] Update package when new versions are released
- [ ] Fix reported bugs promptly
- [ ] Keep dependencies up to date

## Common Issues and Solutions

### Issue: SSH Authentication Failed

**Solution:**
```bash
# Regenerate SSH key
ssh-keygen -t ed25519 -C "your.email@example.com"

# Add to AUR account
cat ~/.ssh/id_ed25519.pub
# Copy and paste to https://aur.archlinux.org/account/
```

### Issue: Package Already Exists

**Solution:**
- Check if package is orphaned
- Contact current maintainer
- Consider using different package name (e.g., `xfb-git`)

### Issue: Build Fails

**Solution:**
```bash
# Clean build directory
rm -rf src pkg *.pkg.tar.zst

# Try again
makepkg -si

# Check logs
cat src/XFB/build/CMakeFiles/CMakeError.log
```

### Issue: Missing Dependencies

**Solution:**
```bash
# Update PKGBUILD depends array
depends=(
    'qt6-base'
    'qt6-multimedia'
    # Add missing dependencies here
)

# Regenerate .SRCINFO
makepkg --printsrcinfo > .SRCINFO
```

## Version Update Checklist

When releasing a new version:

- [ ] Update `pkgver` in PKGBUILD
- [ ] Reset `pkgrel` to 1
- [ ] Update source URL if needed
- [ ] Update checksums: `updpkgsums`
- [ ] Regenerate .SRCINFO: `makepkg --printsrcinfo > .SRCINFO`
- [ ] Test build: `makepkg -si`
- [ ] Commit: `git commit -am "Update to version X.Y.Z"`
- [ ] Push: `git push`
- [ ] Verify on AUR website

## Quality Assurance

### namcap Checks

```bash
# Check PKGBUILD
namcap PKGBUILD

# Check built package
namcap xfb-*.pkg.tar.zst
```

### Manual Verification

```bash
# List package contents
tar -tzf xfb-*.pkg.tar.zst

# Check file permissions
tar -tvzf xfb-*.pkg.tar.zst

# Verify dependencies
pactree xfb
```

### Installation Test

```bash
# Install
sudo pacman -U xfb-*.pkg.tar.zst

# Verify files
pacman -Ql xfb

# Test execution
xfb --version

# Remove
sudo pacman -R xfb
```

## Resources

- **AUR Guidelines**: https://wiki.archlinux.org/title/AUR_submission_guidelines
- **PKGBUILD Manual**: https://man.archlinux.org/man/PKGBUILD.5
- **namcap**: https://wiki.archlinux.org/title/Namcap
- **Arch Packaging Standards**: https://wiki.archlinux.org/title/Arch_package_guidelines

## Contact Information

- **Maintainer**: Netpack <info@netpack.pt>
- **GitHub**: https://github.com/netpack/XFB
- **Website**: https://netpack.pt
- **AUR Package**: https://aur.archlinux.org/packages/xfb

## Notes

- Keep this checklist updated as you learn more about AUR packaging
- Document any custom solutions or workarounds
- Share knowledge with other maintainers
- Follow Arch Linux packaging standards strictly

---

**Last Updated**: 2024-11-29
**Package Version**: 2.0.0
**Maintainer**: Netpack
