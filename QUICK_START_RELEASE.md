# Quick Start: Releasing XFB 2.0.0

This is your quick reference for releasing XFB right now.

## ⚡ Super Quick Release (3 Commands)

```bash
# 1. Safety check and push to GitHub
./push-to-github.sh

# 2. Update AUR
./update-aur.sh

# 3. Create GitHub Release manually
# Go to: https://github.com/netpack/XFB/releases/new
# Upload: xfb_2.0.0-1_amd64.deb
```

Done! ✅

## 📋 What You Have Ready

✅ **PKGBUILD** - Ready for AUR  
✅ **xfb_2.0.0-1_amd64.deb** - Ready for GitHub Release  
✅ **XFB-2.0.0-macOS.dmg** - Ready for GitHub Release (if built)  
✅ **Documentation** - Complete and ready  
✅ **Safety checks** - Scripts will verify no sensitive data  

## 🚀 Step-by-Step Release

### Step 1: Push to GitHub (2 minutes)

```bash
./push-to-github.sh
```

This will:
- ✅ Check for passwords, keys, tokens
- ✅ Verify .gitignore
- ✅ Show what will be pushed
- ✅ Push to https://github.com/netpack/XFB

### Step 2: Update AUR (3 minutes)

```bash
./update-aur.sh
```

This will:
- ✅ Clone/update AUR repo
- ✅ Copy PKGBUILD
- ✅ Generate .SRCINFO
- ✅ Commit and push to AUR

Verify at: https://aur.archlinux.org/packages/xfb

### Step 3: Create GitHub Release (5 minutes)

1. Go to: https://github.com/netpack/XFB/releases/new

2. Fill in:
   - **Tag**: `v2.0.0`
   - **Title**: `XFB 2.0.0 - Accessibility Enhanced`
   - **Description**: Copy from `RELEASE_GUIDE.md` (section "Create GitHub Release")

3. Upload files:
   - `xfb_2.0.0-1_amd64.deb`
   - `XFB-2.0.0-macOS.dmg` (if available)

4. Click **Publish release**

### Step 4: Announce (Optional)

Update README.md badges and post on social media.

## 🔍 Pre-Flight Checklist

Before running the scripts, verify:

- [ ] You're in the XFB directory
- [ ] Git is configured: `git config user.name` and `git config user.email`
- [ ] AUR SSH key is set up: `ssh -T aur@aur.archlinux.org`
- [ ] .deb file exists: `ls xfb_2.0.0-1_amd64.deb`
- [ ] All changes are committed: `git status`

## 🛡️ Safety Checks

The scripts automatically check for:

- ❌ Hardcoded passwords
- ❌ Private keys
- ❌ API tokens
- ❌ Personal email addresses
- ❌ Real credentials in .netrc

**Current status**: ✅ All clear! Only placeholders and public info found.

## 📦 What Users Will Get

### Arch Linux Users

```bash
yay -S xfb
```

One command, automatic dependencies, done in 5-10 minutes.

### Debian/Ubuntu/Mint Users

```bash
wget https://github.com/netpack/XFB/releases/download/v2.0.0/xfb_2.0.0-1_amd64.deb
sudo apt install ./xfb_2.0.0-1_amd64.deb
```

Two commands, may need to install dependencies first.

### macOS Users

1. Download XFB-2.0.0-macOS.dmg
2. Open and drag to Applications
3. Done!

## 🔧 If Something Goes Wrong

### Push to GitHub fails

```bash
# Pull latest changes
git pull --rebase origin main

# Try again
./push-to-github.sh
```

### AUR update fails

```bash
# Check SSH connection
ssh -T aur@aur.archlinux.org

# If connection works, try again
./update-aur.sh
```

### Need to undo a release

```bash
# Delete GitHub release
# Go to releases page and delete

# Revert AUR
cd aur-xfb
git revert HEAD
git push
```

## 📞 Quick Links

- **GitHub Repo**: https://github.com/netpack/XFB
- **AUR Package**: https://aur.archlinux.org/packages/xfb
- **Your Website**: https://netpack.pt

## 🎯 After Release

1. Monitor GitHub issues for bug reports
2. Respond to AUR comments
3. Update website with new version
4. Post on social media

## 💡 Tips

- Run `./release.sh` for an interactive menu
- Use `./release.sh` option 6 for safety check only
- Keep `RELEASE_GUIDE.md` handy for detailed steps
- Check `DISTRIBUTION_SUMMARY.md` for overview

## ⏱️ Time Estimate

- **Minimum**: 10 minutes (if everything goes smoothly)
- **Typical**: 20-30 minutes (with verification)
- **Maximum**: 1 hour (if issues arise)

## ✅ Success Indicators

After release, verify:

- [ ] GitHub shows new release: https://github.com/netpack/XFB/releases
- [ ] AUR shows updated version: https://aur.archlinux.org/packages/xfb
- [ ] Users can install: `yay -S xfb` works
- [ ] .deb file downloads from GitHub Releases

## 🎉 You're Ready!

Everything is prepared. Just run:

```bash
./push-to-github.sh
./update-aur.sh
```

Then create the GitHub Release manually.

Good luck! 🚀

---

**Questions?** Check `RELEASE_GUIDE.md` or email info@netpack.pt
