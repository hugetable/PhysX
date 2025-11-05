#!/bin/bash

################################################################################
# PhysX Ecosystem Update Script
#
# This script synchronizes with upstream repositories while preserving
# local modifications. It handles:
# - Fetching upstream changes
# - Merging or rebasing changes
# - Resolving conflicts
# - Updating submodules
#
# Usage: ./update.sh [OPTIONS]
################################################################################

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Default settings
STRATEGY="merge"  # merge or rebase
DRY_RUN=false
STASH_CHANGES=true
UPDATE_SUBMODULES=true

################################################################################
# Helper Functions
################################################################################

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

show_help() {
    cat << EOF
PhysX Ecosystem Update Script

USAGE:
    ./update.sh [OPTIONS]

DESCRIPTION:
    Synchronizes the local repository with upstream changes while preserving
    local modifications. This script is designed for forked repositories.

OPTIONS:
    --merge             Use merge strategy (default)
    --rebase            Use rebase strategy to keep linear history

    --no-stash          Don't stash local changes (will fail if there are
                        uncommitted changes)

    --no-submodules     Don't update git submodules

    --dry-run           Show what would be done without making changes

    -h, --help          Show this help message

WORKFLOW:
    1. Check for uncommitted changes
    2. Stash local changes (if any)
    3. Fetch upstream changes
    4. Merge/rebase upstream changes
    5. Update submodules
    6. Restore stashed changes

UPSTREAM CONFIGURATION:
    This script expects an 'upstream' remote to be configured:

    git remote add upstream https://github.com/NVIDIA-Omniverse/PhysX.git

    If not configured, the script will prompt to add it.

EXAMPLES:
    # Update with merge strategy (default)
    ./update.sh

    # Update with rebase strategy
    ./update.sh --rebase

    # Dry run to see what would happen
    ./update.sh --dry-run

    # Update without stashing (requires clean working directory)
    ./update.sh --no-stash

EOF
}

check_git_repo() {
    if ! git rev-parse --git-dir > /dev/null 2>&1; then
        print_error "Not a git repository"
        exit 1
    fi
    print_success "Git repository found"
}

check_upstream_remote() {
    print_info "Checking upstream remote..."

    if ! git remote | grep -q "^upstream$"; then
        print_warning "Upstream remote not configured"
        echo ""
        echo "The upstream remote should point to the original NVIDIA PhysX repository."
        echo "Suggested upstream URL: https://github.com/NVIDIA-Omniverse/PhysX.git"
        echo ""
        read -p "Add upstream remote? (y/n): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            read -p "Enter upstream URL: " upstream_url
            if [ -z "$upstream_url" ]; then
                upstream_url="https://github.com/NVIDIA-Omniverse/PhysX.git"
            fi
            git remote add upstream "$upstream_url"
            print_success "Upstream remote added: $upstream_url"
        else
            print_error "Cannot proceed without upstream remote"
            exit 1
        fi
    else
        local upstream_url=$(git remote get-url upstream)
        print_success "Upstream remote found: $upstream_url"
    fi
}

check_working_directory() {
    print_info "Checking working directory status..."

    if ! git diff-index --quiet HEAD -- 2>/dev/null; then
        print_warning "You have uncommitted changes"
        git status --short
        echo ""

        if ! $STASH_CHANGES; then
            print_error "Uncommitted changes found and --no-stash specified"
            echo "Please commit or stash your changes first"
            exit 1
        fi

        return 1  # Has changes
    else
        print_success "Working directory is clean"
        return 0  # Clean
    fi
}

stash_changes() {
    print_info "Stashing local changes..."
    git stash push -m "update.sh auto-stash $(date +%Y-%m-%d_%H:%M:%S)"
    print_success "Changes stashed"
}

pop_stash() {
    if git stash list | head -1 | grep -q "update.sh auto-stash"; then
        print_info "Restoring stashed changes..."
        if git stash pop; then
            print_success "Stashed changes restored"
        else
            print_warning "Conflicts detected while restoring stash"
            echo "Please resolve conflicts manually and run: git stash drop"
        fi
    fi
}

get_current_branch() {
    git symbolic-ref --short HEAD 2>/dev/null || echo "HEAD"
}

fetch_upstream() {
    print_info "Fetching upstream changes..."
    git fetch upstream

    local current_branch=$(get_current_branch)
    print_info "Current branch: $current_branch"

    # Try to find corresponding upstream branch
    local upstream_branch="upstream/main"
    if git rev-parse --verify upstream/master >/dev/null 2>&1; then
        upstream_branch="upstream/master"
    fi

    print_info "Upstream branch: $upstream_branch"
    echo "$upstream_branch"
}

merge_upstream() {
    local upstream_branch=$1

    print_info "Merging upstream changes..."
    if git merge "$upstream_branch" --no-edit; then
        print_success "Merge completed successfully"
        return 0
    else
        print_error "Merge conflicts detected"
        echo ""
        echo "Conflicts in:"
        git diff --name-only --diff-filter=U
        echo ""
        echo "Please resolve conflicts manually:"
        echo "  1. Edit conflicting files"
        echo "  2. git add <resolved-files>"
        echo "  3. git commit"
        echo ""
        echo "Or abort the merge:"
        echo "  git merge --abort"
        return 1
    fi
}

rebase_upstream() {
    local upstream_branch=$1

    print_info "Rebasing onto upstream..."
    if git rebase "$upstream_branch"; then
        print_success "Rebase completed successfully"
        return 0
    else
        print_error "Rebase conflicts detected"
        echo ""
        echo "Conflicts in:"
        git diff --name-only --diff-filter=U
        echo ""
        echo "Please resolve conflicts manually:"
        echo "  1. Edit conflicting files"
        echo "  2. git add <resolved-files>"
        echo "  3. git rebase --continue"
        echo ""
        echo "Or abort the rebase:"
        echo "  git rebase --abort"
        return 1
    fi
}

update_submodules() {
    if ! $UPDATE_SUBMODULES; then
        return 0
    fi

    print_info "Updating submodules..."
    if git submodule update --init --recursive; then
        print_success "Submodules updated"
    else
        print_warning "Failed to update submodules"
    fi
}

################################################################################
# Main Script
################################################################################

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --merge)
            STRATEGY="merge"
            shift
            ;;
        --rebase)
            STRATEGY="rebase"
            shift
            ;;
        --no-stash)
            STASH_CHANGES=false
            shift
            ;;
        --no-submodules)
            UPDATE_SUBMODULES=false
            shift
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Print configuration
print_header "Update Configuration"
echo "Strategy: $STRATEGY"
echo "Stash changes: $STASH_CHANGES"
echo "Update submodules: $UPDATE_SUBMODULES"
$DRY_RUN && echo "Mode: DRY RUN (no changes will be made)"
echo ""

# Dry run mode
if $DRY_RUN; then
    print_info "DRY RUN MODE - Showing what would be done..."
    check_git_repo
    check_upstream_remote
    check_working_directory && HAS_CHANGES=false || HAS_CHANGES=true

    echo ""
    print_info "Would perform:"
    $HAS_CHANGES && $STASH_CHANGES && echo "  1. Stash local changes"
    echo "  2. Fetch from upstream"
    echo "  3. $STRATEGY with upstream"
    $UPDATE_SUBMODULES && echo "  4. Update submodules"
    $HAS_CHANGES && $STASH_CHANGES && echo "  5. Restore stashed changes"

    print_success "Dry run completed"
    exit 0
fi

# Actual update process
print_header "Starting Update Process"

check_git_repo
check_upstream_remote

STASHED=false
if ! check_working_directory; then
    if $STASH_CHANGES; then
        stash_changes
        STASHED=true
    fi
fi

# Fetch and merge/rebase
upstream_branch=$(fetch_upstream)

if [ "$STRATEGY" == "merge" ]; then
    if ! merge_upstream "$upstream_branch"; then
        print_error "Update failed during merge"
        exit 1
    fi
elif [ "$STRATEGY" == "rebase" ]; then
    if ! rebase_upstream "$upstream_branch"; then
        print_error "Update failed during rebase"
        exit 1
    fi
fi

# Update submodules
update_submodules

# Restore stashed changes
if $STASHED; then
    pop_stash
fi

# Summary
print_header "Update Summary"
print_success "Repository updated successfully!"
echo ""
echo "Current status:"
git log --oneline -3
echo ""
print_info "You may want to rebuild libraries after updating:"
echo "  ./build.sh --all"

exit 0
