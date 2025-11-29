# ⚠️ AUR Package Needs Update

## Current Situation

- ✅ **GitHub**: v2.0.0 (LIVE) - https://github.com/netpack/XFB
- ✅ **PKGBUILD**: v2.0.0 (READY)
- ❌ **AUR**: v1.23 (OUTDATED) - https://aur.archlinux.org/packages/xfb

## Why AUR Shows Old Version

AUR (Arch User Repository) is a **separate Git repository** from GitHub:

- **GitHub repo**: https://github.com/netpack/XFB (source code)
- **AUR repo**: ssh://aur@aur.archlinux.org/xfb.git (package metadata)

Updating GitHub does NOT automatically update AUR. You must manually push to the AUR repository.

## Quick Fix (3 Options)

### Option 1: On Arch Linux System (5 minutes)

If you have access to Arch Linux:

```bash
./update-aur-simple.sh
```

Or manually:

```bash
git clone ssh://aur@aur.archlinux.org/xfb.git aur-xfb
cd aur-xfb
cp ../PKGBUILD .
makepkg --printsrcinfo > .SRCINFO
git add PKGBUILD .SRCINFO
git commit -m "Update to version 2.0.0"
git push
```

### Option 2: Using Docker from macOS (10 minutes)

```bash
./update-aur-docker.sh
```

This creates an Arch Linux container where you can update AUR.

### Option 3: Ask Someone with Arch Linux (1 minute)

Send them the `PKGBUILD` file and ask them to update the AUR package.

## What Needs to Happen

Only 2 files need to be updated in the AUR repository:

1. **PKGBUILD** (already ready in your repo)
2. **.SRCINFO** (generated from PKGBUILD)

## Detailed Instructions

See: `UPDATE_AUR_NOW.md`

## After Update

Once AUR is updated:
- ✅ `yay -Ss xfb` will show v2.0.0
- ✅ Users can install with `yay -S xfb`
- ✅ AUR website will show v2.0.0

## Verification

Check AUR package status:
```bash
curl https://aur.archlinux.org/rpc/?v=5&type=info&arg=xfb | jq '.results[0].Version'
```

Should return: `"2.0.0-1"`

## Timeline

- **Now**: AUR shows 1.23
- **After update**: AUR shows 2.0.0 (within 5 minutes)
- **User impact**: All Arch users can install v2.0.0

## Need Help?

1. **Read**: `UPDATE_AUR_NOW.md` (detailed guide)
2. **Try**: `./update-aur-simple.sh` (on Arch Linux)
3. **Or**: `./update-aur-docker.sh` (using Docker)
4. **Or**: Ask on AUR package comments

## Bottom Line

**You need to push the PKGBUILD to the AUR Git repository.**

The PKGBUILD is ready, GitHub is updated, but AUR is a separate system that requires a manual push.

---

**Quick Links:**
- AUR Package: https://aur.archlinux.org/packages/xfb
- Detailed Guide: `UPDATE_AUR_NOW.md`
- Simple Script: `./update-aur-simple.sh`
- Docker Script: `./update-aur-docker.sh`
