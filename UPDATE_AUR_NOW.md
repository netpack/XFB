# Update AUR Package to Version 2.0.0

The AUR package currently shows version 1.23 because the AUR repository hasn't been updated yet. Here's how to update it.

## Problem

- ✅ GitHub has v2.0.0
- ❌ AUR still shows v1.23
- **Reason**: AUR is a separate repository that needs manual update

## Solution Options

### Option 1: Update from Arch Linux System (Recommended)

If you have access to an Arch Linux system (VM, container, or physical machine):

```bash
# 1. Clone the AUR repository
git clone ssh://aur@aur.archlinux.org/xfb.git aur-xfb
cd aur-xfb

# 2. Copy the new PKGBUILD from your XFB repository
cp /path/to/XFB/PKGBUILD .

# 3. Generate .SRCINFO
makepkg --printsrcinfo > .SRCINFO

# 4. Review changes
git diff

# 5. Commit
git add PKGBUILD .SRCINFO
git commit -m "Update to version 2.0.0

- Updated to XFB 2.0.0
- Enhanced accessibility features with ORCA integration
- Improved keyboard navigation
- Added braille display support
- Performance optimizations
- Updated dependencies
- New CMake-based build system
"

# 6. Push to AUR
git push
```

**Time**: 5 minutes  
**Result**: AUR will show v2.0.0 within minutes

### Option 2: Use Docker with Arch Linux

If you don't have Arch Linux but have Docker:

```bash
# 1. Build Arch Linux container
docker build -t arch-aur -f Dockerfile.arch-test .

# 2. Run container with SSH agent forwarding
docker run -it --rm \
    -v $(pwd):/xfb \
    -v $SSH_AUTH_SOCK:/ssh-agent \
    -e SSH_AUTH_SOCK=/ssh-agent \
    arch-aur bash

# 3. Inside container, configure git
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

# 4. Clone AUR repo
git clone ssh://aur@aur.archlinux.org/xfb.git
cd xfb

# 5. Copy PKGBUILD
cp /xfb/PKGBUILD .

# 6. Generate .SRCINFO
makepkg --printsrcinfo > .SRCINFO

# 7. Commit and push
git add PKGBUILD .SRCINFO
git commit -m "Update to version 2.0.0"
git push
```

### Option 3: Ask Someone with Arch Linux

Send them:

1. **The PKGBUILD file** (from your repository)
2. **These instructions**:

```
Hi! Can you help update the XFB AUR package to version 2.0.0?

Steps:
1. git clone ssh://aur@aur.archlinux.org/xfb.git
2. cd xfb
3. Replace PKGBUILD with the attached file
4. makepkg --printsrcinfo > .SRCINFO
5. git add PKGBUILD .SRCINFO
6. git commit -m "Update to version 2.0.0"
7. git push

The PKGBUILD is attached. Thanks!
```

### Option 4: Use GitHub Codespaces (If Available)

If you have GitHub Codespaces:

1. Create a codespace from your repository
2. Install Arch Linux in a container
3. Follow Option 1 steps

## What Needs to Be Updated

Only 2 files in the AUR repository:

1. **PKGBUILD** - Already updated in your GitHub repo
2. **.SRCINFO** - Generated from PKGBUILD

## Verification

After updating, verify at:
- https://aur.archlinux.org/packages/xfb

It should show:
- Version: 2.0.0-1
- Last Updated: (today's date)

Users can then install with:
```bash
yay -S xfb
```

## Why This Happens

AUR is not like GitHub. It's a separate Git repository hosted by Arch Linux:
- **GitHub**: https://github.com/netpack/XFB (source code)
- **AUR**: ssh://aur@aur.archlinux.org/xfb.git (package metadata)

When you update GitHub, AUR doesn't automatically update. You must manually push to the AUR repository.

## Quick Check

To see what's currently in AUR:

```bash
# View AUR package info
curl https://aur.archlinux.org/rpc/?v=5&type=info&arg=xfb | jq
```

## SSH Key Setup (If Needed)

If you get "Permission denied" when pushing to AUR:

1. Generate SSH key:
   ```bash
   ssh-keygen -t ed25519 -C "your.email@example.com"
   ```

2. Add to AUR account:
   - Go to: https://aur.archlinux.org/account/
   - Paste your public key (~/.ssh/id_ed25519.pub)

3. Test connection:
   ```bash
   ssh -T aur@aur.archlinux.org
   ```

## Automated Script

If you're on Arch Linux, use the provided script:

```bash
./update-aur.sh
```

This will:
- Clone AUR repository
- Copy PKGBUILD
- Generate .SRCINFO
- Commit and push

## Timeline

Once you push to AUR:
- **Immediate**: AUR website updates
- **Within 5 minutes**: `yay -Ss xfb` shows v2.0.0
- **Within 10 minutes**: All Arch users can install v2.0.0

## Current Status

- ✅ GitHub: v2.0.0 (DONE)
- ✅ PKGBUILD: v2.0.0 (READY)
- ❌ AUR: v1.23 (NEEDS UPDATE)

## Next Steps

1. **Choose an option above** (Option 1 is fastest if you have Arch)
2. **Update AUR** (5 minutes)
3. **Verify** at https://aur.archlinux.org/packages/xfb
4. **Test**: `yay -S xfb` on Arch Linux

## Need Help?

- **AUR Guidelines**: https://wiki.archlinux.org/title/AUR_submission_guidelines
- **AUR Account**: https://aur.archlinux.org/account/
- **SSH Setup**: https://wiki.archlinux.org/title/AUR_submission_guidelines#Authentication

## Alternative: Wait for Community

If you can't update it yourself, you can:
1. Post on the AUR package comments asking for help
2. Someone from the community might update it
3. But it's better to maintain it yourself as the package maintainer

---

**Bottom Line**: You need access to an Arch Linux system (or Docker) to push the update to AUR. The PKGBUILD is ready, you just need to push it to the AUR Git repository.
