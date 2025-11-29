#!/bin/bash
# Safe script to push XFB to GitHub
# Checks for sensitive information before pushing

set -e

echo "=========================================="
echo "XFB GitHub Push Safety Check"
echo "=========================================="
echo ""

# Function to check for sensitive patterns
check_sensitive_info() {
    local file=$1
    local pattern=$2
    local description=$3
    
    if grep -q "$pattern" "$file" 2>/dev/null; then
        echo "⚠️  Warning: Possible $description found in $file"
        grep -n "$pattern" "$file" | head -5
        return 1
    fi
    return 0
}

# Files and directories to exclude from push
EXCLUDE_PATTERNS=(
    "*.key"
    "*.pem"
    "*.p12"
    "*.pfx"
    "*.jks"
    "*.keystore"
    ".env"
    ".env.*"
    "secrets.yml"
    "credentials.json"
    "*.log"
    "*.sqlite"
    "*.db"
    "build/"
    "tmp/"
    "tmp_*/"
    "*.pkg.tar.zst"
    "src/"
    "pkg/"
    "debian-repo/"
    "aur-xfb/"
)

echo "Step 1: Checking for sensitive information..."
echo ""

ISSUES_FOUND=0

# Check for actual passwords (not just the word "password")
echo "Checking for hardcoded credentials..."
if git grep -i "password\s*=\s*['\"][^'\"]*['\"]" -- '*.cpp' '*.h' '*.sh' 2>/dev/null | grep -v "Password:" | grep -v "password.*\[" | grep -v "//"; then
    echo "⚠️  Warning: Possible hardcoded password found"
    ISSUES_FOUND=1
fi

# Check for API keys
echo "Checking for API keys..."
if git grep -iE "(api[_-]?key|apikey)\s*=\s*['\"][^'\"]{20,}['\"]" 2>/dev/null; then
    echo "⚠️  Warning: Possible API key found"
    ISSUES_FOUND=1
fi

# Check for private keys
echo "Checking for private keys..."
if git grep -l "BEGIN.*PRIVATE KEY" 2>/dev/null; then
    echo "⚠️  Warning: Private key found"
    ISSUES_FOUND=1
fi

# Check for tokens
echo "Checking for tokens..."
if git grep -iE "token\s*=\s*['\"][^'\"]{20,}['\"]" 2>/dev/null | grep -v "GITHUB_TOKEN" | grep -v "secrets."; then
    echo "⚠️  Warning: Possible token found"
    ISSUES_FOUND=1
fi

# Check for real email addresses in code (not documentation)
echo "Checking for personal email addresses..."
if git grep -E "[a-zA-Z0-9._%+-]+@(?!example\.com|netpack\.pt|xfb\.com)[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}" -- '*.cpp' '*.h' 2>/dev/null; then
    echo "⚠️  Warning: Personal email address found in code"
    ISSUES_FOUND=1
fi

# Check .netrc for real credentials
if [ -f "config/.netrc" ]; then
    echo "Checking .netrc file..."
    if grep -v "\[" config/.netrc | grep -qE "password\s+[^[]"; then
        echo "⚠️  Warning: Real credentials in .netrc file"
        ISSUES_FOUND=1
    else
        echo "✓ .netrc contains only placeholders"
    fi
fi

if [ $ISSUES_FOUND -eq 0 ]; then
    echo "✓ No sensitive information detected"
else
    echo ""
    echo "❌ Potential sensitive information found!"
    echo "Please review the warnings above before pushing."
    echo ""
    read -p "Continue anyway? (y/N) " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Push cancelled"
        exit 1
    fi
fi

echo ""
echo "Step 2: Checking .gitignore..."
echo ""

# Verify important patterns are in .gitignore
REQUIRED_IGNORES=(
    "*.key"
    "*.pem"
    ".env"
    "*.log"
    "build/"
    "*.pkg.tar.zst"
)

MISSING_IGNORES=()
for pattern in "${REQUIRED_IGNORES[@]}"; do
    if ! grep -q "^$pattern" .gitignore 2>/dev/null; then
        MISSING_IGNORES+=("$pattern")
    fi
done

if [ ${#MISSING_IGNORES[@]} -gt 0 ]; then
    echo "⚠️  Warning: Some patterns missing from .gitignore:"
    printf '%s\n' "${MISSING_IGNORES[@]}"
    echo ""
    read -p "Add them now? (y/N) " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        for pattern in "${MISSING_IGNORES[@]}"; do
            echo "$pattern" >> .gitignore
        done
        echo "✓ Updated .gitignore"
    fi
else
    echo "✓ .gitignore looks good"
fi

echo ""
echo "Step 3: Checking Git status..."
echo ""

# Check if there are uncommitted changes
if ! git diff-index --quiet HEAD -- 2>/dev/null; then
    echo "Uncommitted changes found:"
    git status --short
    echo ""
    read -p "Commit these changes? (y/N) " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Enter commit message:"
        read -r COMMIT_MSG
        git add .
        git commit -m "$COMMIT_MSG"
        echo "✓ Changes committed"
    else
        echo "Please commit or stash changes before pushing"
        exit 1
    fi
else
    echo "✓ Working directory clean"
fi

echo ""
echo "Step 4: Checking remote repository..."
echo ""

# Check if remote exists
if ! git remote get-url origin &>/dev/null; then
    echo "❌ No remote 'origin' configured"
    echo "Add remote with:"
    echo "  git remote add origin https://github.com/netpack/XFB.git"
    exit 1
fi

REMOTE_URL=$(git remote get-url origin)
echo "Remote URL: $REMOTE_URL"

# Verify it's the correct repository
if [[ ! "$REMOTE_URL" =~ "netpack/XFB" ]]; then
    echo "⚠️  Warning: Remote URL doesn't match expected repository"
    read -p "Continue anyway? (y/N) " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Push cancelled"
        exit 1
    fi
fi

echo ""
echo "Step 5: Checking branch..."
echo ""

CURRENT_BRANCH=$(git branch --show-current)
echo "Current branch: $CURRENT_BRANCH"

if [ "$CURRENT_BRANCH" != "main" ] && [ "$CURRENT_BRANCH" != "master" ]; then
    echo "⚠️  Warning: Not on main/master branch"
    read -p "Continue pushing to $CURRENT_BRANCH? (y/N) " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Push cancelled"
        exit 1
    fi
fi

echo ""
echo "Step 6: Files to be pushed..."
echo ""

# Show what will be pushed
echo "Recent commits:"
git log --oneline -5

echo ""
echo "Files in repository:"
git ls-files | head -20
echo "... (showing first 20 files)"

echo ""
echo "=========================================="
echo "Ready to Push"
echo "=========================================="
echo ""
echo "Repository: $REMOTE_URL"
echo "Branch: $CURRENT_BRANCH"
echo ""

read -p "Push to GitHub? (y/N) " -n 1 -r
echo ""

if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo ""
    echo "Pushing to GitHub..."
    git push origin "$CURRENT_BRANCH"
    
    echo ""
    echo "=========================================="
    echo "✓ Successfully pushed to GitHub!"
    echo "=========================================="
    echo ""
    echo "View at: https://github.com/netpack/XFB"
    echo ""
    
    # Check if there are tags to push
    if git tag | grep -q .; then
        echo "Tags found in repository:"
        git tag | tail -5
        echo ""
        read -p "Push tags as well? (y/N) " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            git push origin --tags
            echo "✓ Tags pushed"
        fi
    fi
    
    echo ""
    echo "Next steps:"
    echo "1. Update AUR: ./update-aur.sh"
    echo "2. Create GitHub release with .deb file"
    echo "3. Update Debian repository: ./update-debian-repo.sh"
    
else
    echo "Push cancelled"
    exit 0
fi
