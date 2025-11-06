#!/bin/bash

################################################################################
# PhysX Ecosystem Build Script
#
# This script builds PhysX, BLAST, and FLOW libraries with support for:
# - Selective building (individual libraries or all)
# - Multiple configurations (Debug, Release, Checked)
# - Incremental builds (skip if already built)
# - Clean builds
#
# Usage: ./build.sh [OPTIONS]
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
BUILD_PHYSX=false
BUILD_BLAST=false
BUILD_FLOW=false
BUILD_ALL=false
CLEAN=false
CONFIG="release"  # release, debug, or checked
FORCE=false
JOBS=$(nproc)

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
PhysX Ecosystem Build Script

USAGE:
    ./build.sh [OPTIONS]

OPTIONS:
    --physx             Build PhysX only
    --blast             Build BLAST only
    --flow              Build FLOW only
    --all               Build all libraries (default if no library specified)

    --config CONFIG     Build configuration: release, debug, checked
                        (default: release)

    --clean             Clean build directories before building
    --force             Force rebuild even if already built

    -j, --jobs N        Number of parallel jobs (default: $(nproc))

    -h, --help          Show this help message

EXAMPLES:
    # Build all libraries in release mode
    ./build.sh --all

    # Build only PhysX in debug mode
    ./build.sh --physx --config debug

    # Build BLAST and FLOW with clean
    ./build.sh --blast --flow --clean

    # Force rebuild PhysX with 8 jobs
    ./build.sh --physx --force -j 8

LIBRARY LOCATIONS:
    PhysX:  physx/
    BLAST:  blast/
    FLOW:   flow/

OUTPUT LOCATIONS:
    PhysX:  physx/bin/linux.clang/{config}/
    BLAST:  blast/bin/linux64_{config}/
    FLOW:   flow/bin/linux64_{config}/

EOF
}

check_dependencies() {
    print_info "Checking dependencies..."

    local missing_deps=()

    # Check for required tools
    command -v cmake >/dev/null 2>&1 || missing_deps+=("cmake")
    command -v make >/dev/null 2>&1 || missing_deps+=("make")
    command -v python3 >/dev/null 2>&1 || missing_deps+=("python3")

    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        echo "Install with: sudo apt-get install ${missing_deps[*]}"
        exit 1
    fi

    print_success "All dependencies found"
}

is_library_built() {
    local lib=$1
    local config=$2

    case $lib in
        physx)
            local physx_lib="physx/bin/linux.clang/${config}/libPhysX_64.so"
            [ -f "$physx_lib" ] && return 0 || return 1
            ;;
        blast)
            local blast_lib="blast/bin/linux64_${config}/libNvBlast.so"
            [ -f "$blast_lib" ] && return 0 || return 1
            ;;
        flow)
            local flow_lib="flow/bin/linux64_${config}/libNvFlow.so"
            [ -f "$flow_lib" ] && return 0 || return 1
            ;;
    esac
    return 1
}

################################################################################
# Build Functions
################################################################################

build_physx() {
    local config=$1

    print_header "Building PhysX ($config)"

    if ! $FORCE && is_library_built "physx" "$config"; then
        print_warning "PhysX already built for $config configuration. Use --force to rebuild."
        return 0
    fi

    cd "$SCRIPT_DIR/physx"

    # Set PHYSX_ROOT_DIR environment variable (required by PhysX build system)
    export PHYSX_ROOT_DIR="$SCRIPT_DIR/physx"

    # Use simplified preset (no GPU, no snippets) to avoid packman dependencies
    local preset="linux-clang-cpu-only"

    print_info "Using PhysX preset: $preset"
    print_info "Setting PHYSX_ROOT_DIR=$PHYSX_ROOT_DIR"

    # Call the PhysX CMake generation script
    print_info "Generating PhysX build files..."

    # Use Python to generate CMake files
    if [ -f "buildtools/cmake_generate_projects.py" ]; then
        python3 buildtools/cmake_generate_projects.py "$preset"
    else
        print_error "cmake_generate_projects.py not found"
        cd "$SCRIPT_DIR"
        return 1
    fi

    if [ $? -ne 0 ]; then
        print_error "Project generation failed"
        print_info "This might be due to missing Python dependencies or preset issues"
        cd "$SCRIPT_DIR"
        return 1
    fi

    # Determine the compiler directory based on preset and config
    local compiler_dir="compiler/linux-clang-cpu-only-${config}"

    if [ ! -d "$compiler_dir" ]; then
        print_error "Compiler directory not found: $compiler_dir"
        print_info "Available directories:"
        ls -la compiler/ 2>/dev/null || echo "No compiler directory found"
        cd "$SCRIPT_DIR"
        return 1
    fi

    cd "$compiler_dir"

    # Build the specific configuration
    print_info "Compiling PhysX ($config) with $JOBS jobs..."

    # For linux-clang-cpu-only preset, each config has its own directory
    # Just use 'make' without specific target
    make -j${JOBS}

    if [ $? -ne 0 ]; then
        print_error "Compilation failed"
        cd "$SCRIPT_DIR"
        return 1
    fi

    cd "$SCRIPT_DIR"
    print_success "PhysX ($config) built successfully"
    print_info "Libraries location: physx/bin/linux.clang/${config}/"
}

build_blast() {
    local config=$1

    print_header "Building BLAST ($config)"

    if ! $FORCE && is_library_built "blast" "$config"; then
        print_warning "BLAST already built for $config configuration. Use --force to rebuild."
        return 0
    fi

    if [ ! -d "$SCRIPT_DIR/blast" ]; then
        print_error "BLAST directory not found. Please ensure blast/ exists."
        return 1
    fi

    cd "$SCRIPT_DIR/blast"

    # Generate project files
    print_info "Generating BLAST build files..."
    local preset="linux-clang"
    if [ -f "generate_projects.sh" ]; then
        ./generate_projects.sh "$preset"
    else
        print_error "BLAST generate_projects.sh not found"
        return 1
    fi

    # Build
    print_info "Compiling BLAST ($config)..."
    local build_dir="compiler/linux-clang-${config}"

    if [ -d "$build_dir" ]; then
        cd "$build_dir"
        make -j${JOBS}
    else
        print_warning "BLAST build directory not found, trying alternative method..."
        # Try cmake directly
        mkdir -p "build_${config}"
        cd "build_${config}"
        cmake .. -DCMAKE_BUILD_TYPE=$(echo $config | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')
        make -j${JOBS}
    fi

    cd "$SCRIPT_DIR"
    print_success "BLAST ($config) built successfully"
}

build_flow() {
    local config=$1

    print_header "Building FLOW ($config)"

    if ! $FORCE && is_library_built "flow" "$config"; then
        print_warning "FLOW already built for $config configuration. Use --force to rebuild."
        return 0
    fi

    if [ ! -d "$SCRIPT_DIR/flow" ]; then
        print_error "FLOW directory not found. Please ensure flow/ exists."
        return 1
    fi

    cd "$SCRIPT_DIR/flow"

    # Generate project files
    print_info "Generating FLOW build files..."
    if [ -f "generate_projects.sh" ]; then
        ./generate_projects.sh linux
    else
        print_warning "FLOW generate_projects.sh not found, trying CMake..."
        mkdir -p "build_${config}"
        cd "build_${config}"
        cmake .. -DCMAKE_BUILD_TYPE=$(echo $config | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')
        make -j${JOBS}
        cd "$SCRIPT_DIR"
        print_success "FLOW ($config) built successfully"
        return 0
    fi

    # Build
    print_info "Compiling FLOW ($config)..."
    local build_dir="compiler/linux-${config}"

    if [ -d "$build_dir" ]; then
        cd "$build_dir"
        make -j${JOBS}
    fi

    cd "$SCRIPT_DIR"
    print_success "FLOW ($config) built successfully"
}

clean_library() {
    local lib=$1

    print_info "Cleaning $lib..."

    case $lib in
        physx)
            rm -rf physx/compiler/linux-*
            rm -rf physx/bin/
            ;;
        blast)
            rm -rf blast/compiler/
            rm -rf blast/bin/
            rm -rf blast/build_*
            ;;
        flow)
            rm -rf flow/compiler/
            rm -rf flow/bin/
            rm -rf flow/build_*
            ;;
    esac

    print_success "$lib cleaned"
}

################################################################################
# Main Script
################################################################################

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --physx)
            BUILD_PHYSX=true
            shift
            ;;
        --blast)
            BUILD_BLAST=true
            shift
            ;;
        --flow)
            BUILD_FLOW=true
            shift
            ;;
        --all)
            BUILD_ALL=true
            shift
            ;;
        --config)
            CONFIG="$2"
            shift 2
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --force)
            FORCE=true
            shift
            ;;
        -j|--jobs)
            JOBS="$2"
            shift 2
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

# If no library specified, build all
if ! $BUILD_PHYSX && ! $BUILD_BLAST && ! $BUILD_FLOW && ! $BUILD_ALL; then
    BUILD_ALL=true
fi

if $BUILD_ALL; then
    BUILD_PHYSX=true
    BUILD_BLAST=true
    BUILD_FLOW=true
fi

# Validate config
case $CONFIG in
    release|debug|checked)
        ;;
    *)
        print_error "Invalid config: $CONFIG"
        echo "Valid configs: release, debug, checked"
        exit 1
        ;;
esac

# Print build configuration
print_header "Build Configuration"
echo "Libraries:"
$BUILD_PHYSX && echo "  - PhysX"
$BUILD_BLAST && echo "  - BLAST"
$BUILD_FLOW && echo "  - FLOW"
echo "Config: $CONFIG"
echo "Jobs: $JOBS"
$CLEAN && echo "Clean: yes"
$FORCE && echo "Force rebuild: yes"
echo ""

# Check dependencies
check_dependencies

# Clean if requested
if $CLEAN; then
    $BUILD_PHYSX && clean_library "physx"
    $BUILD_BLAST && clean_library "blast"
    $BUILD_FLOW && clean_library "flow"
    echo ""
fi

# Build libraries
BUILD_FAILED=false

if $BUILD_PHYSX; then
    if ! build_physx "$CONFIG"; then
        BUILD_FAILED=true
        print_error "PhysX build failed"
    fi
    echo ""
fi

if $BUILD_BLAST; then
    if ! build_blast "$CONFIG"; then
        BUILD_FAILED=true
        print_error "BLAST build failed"
    fi
    echo ""
fi

if $BUILD_FLOW; then
    if ! build_flow "$CONFIG"; then
        BUILD_FAILED=true
        print_error "FLOW build failed"
    fi
    echo ""
fi

# Summary
print_header "Build Summary"
if $BUILD_FAILED; then
    print_error "Some builds failed. Check the output above for details."
    exit 1
else
    print_success "All builds completed successfully!"
    echo ""
    echo "Library locations:"
    $BUILD_PHYSX && echo "  PhysX: physx/bin/linux.clang/${CONFIG}/"
    $BUILD_BLAST && echo "  BLAST: blast/bin/linux64_${CONFIG}/"
    $BUILD_FLOW && echo "  FLOW: flow/bin/linux64_${CONFIG}/"
fi

exit 0
