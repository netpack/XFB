#!/bin/bash
set -e # Exit immediately if a command fails

echo "--- STARTING FORENSIC DEBIAN PACKAGE BUILD ---"
echo

# --- Configuration ---
SOURCE_BINARY="src/build-compatible/XFB"
STAGED_BINARY="src/build/deb/xfb/usr/bin/XFB"
STAGING_DIR="src/build/deb/xfb"
OUTPUT_DEB="src/build/deb/xfb.deb"

# --- Forensic Step 1: Verify the Source ---
echo "1. MD5 of the SOURCE binary:"
md5sum "$SOURCE_BINARY"
echo "------------------------------------"
echo

# --- Forensic Step 2: Verify the Staged File BEFORE any action ---
echo "2. MD5 of the STAGED binary (before script runs):"
md5sum "$STAGED_BINARY"
echo "------------------------------------"
echo

# --- Forensic Step 3: Set Permissions ---
echo "3. Setting permissions..."
chmod 755 "$STAGED_BINARY"
echo "   Permissions set. Let's re-check the MD5 sum (should be unchanged):"
md5sum "$STAGED_BINARY"
echo "------------------------------------"
echo

# --- Forensic Step 4: Strip the Binary ---
echo "4. Stripping the binary..."
strip "$STAGED_BINARY"
echo "   Binary has been stripped. The MD5 sum MUST be DIFFERENT now."
echo "   MD5 of the STAGED binary (AFTER stripping):"
md5sum "$STAGED_BINARY"
echo "------------------------------------"
echo

# --- Forensic Step 5: Build the Package ---
echo "5. Building the .deb package..."
rm -f "$OUTPUT_DEB" # Delete old package to be sure
fakeroot dpkg-deb --build "$STAGING_DIR" "$OUTPUT_DEB"
echo "   Package built."
echo "------------------------------------"
echo

# --- Forensic Step 6: Extract and Verify the Packaged Binary ---
echo "6. Extracting the new .deb to verify its contents..."
TMP_DIR="tmp_verify"
rm -rf "$TMP_DIR"
mkdir "$TMP_DIR"
dpkg-deb -x "$OUTPUT_DEB" "$TMP_DIR"
echo "   Package extracted."
echo "   FINAL MD5 of the binary INSIDE the .deb file:"
md5sum "$TMP_DIR/usr/bin/XFB"
echo "------------------------------------"
echo

# --- Final Analysis ---
echo "ANALYSIS:"
echo " - The MD5 from step 1 and 2 should match."
echo " - The MD5 from step 4 should be DIFFERENT from step 2."
echo " - The MD5 from step 6 MUST MATCH the MD5 from step 4."
echo
echo "If the MD5 from step 6 matches the old, bad checksum, something extraordinary is happening."
echo "--- FORENSIC BUILD COMPLETE ---"
