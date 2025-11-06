#!/bin/bash

################################################################################
# PhysX Ecosystem Build Script (System Libraries Version)
#
# This script builds PhysX, BLAST, and FLOW libraries using system libraries:
# - Uses system compilers (gcc/clang) instead of packman
# - Uses system CMake and build tools
# - Minimizes external dependencies
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
COMPILER="gcc"  # gcc or clang

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
PhysX Ecosystem Build Script (System Libraries Version)

USAGE:
    ./build.sh [OPTIONS]

OPTIONS:
    --physx             Build PhysX only
    --blast             Build BLAST only
    --flow              Build FLOW only
    --all               Build all libraries (default if no library specified)

    --config CONFIG     Build configuration: release, debug, checked
                        (default: release)

    --compiler COMP     Compiler to use: gcc or clang (default: gcc)

    --clean             Clean build directories before building
    --force             Force rebuild even if already built

    -j, --jobs N        Number of parallel jobs (default: $(nproc))

    -h, --help          Show this help message

EXAMPLES:
    # Build all libraries in release mode with system GCC
    ./build.sh --all

    # Build only PhysX in debug mode with Clang
    ./build.sh --physx --config debug --compiler clang

    # Build BLAST and FLOW with clean
    ./build.sh --blast --flow --clean

    # Force rebuild PhysX with 8 jobs
    ./build.sh --physx --force -j 8

LIBRARY LOCATIONS:
    PhysX:  physx/
    BLAST:  blast/
    FLOW:   flow/

OUTPUT LOCATIONS:
    PhysX:  physx/bin/linux.${COMPILER}/<config>/
    BLAST:  blast/_build/linux-x86_64/<config>/
    FLOW:   flow/_build/linux-x86_64/<config>/

SYSTEM REQUIREMENTS:
    - gcc or clang compiler
    - cmake >= 3.15
    - make or ninja
    - python3
    - premake5 (for BLAST and FLOW)

EOF
}

check_dependencies() {
    print_info "Checking dependencies..."

    local missing_deps=()

    # Check for required tools
    command -v cmake >/dev/null 2>&1 || missing_deps+=("cmake")
    command -v make >/dev/null 2>&1 || missing_deps+=("make")
    command -v python3 >/dev/null 2>&1 || missing_deps+=("python3")

    # Check for compiler
    if [ "$COMPILER" = "gcc" ]; then
        command -v gcc >/dev/null 2>&1 || missing_deps+=("gcc")
        command -v g++ >/dev/null 2>&1 || missing_deps+=("g++")
    elif [ "$COMPILER" = "clang" ]; then
        command -v clang >/dev/null 2>&1 || missing_deps+=("clang")
        command -v clang++ >/dev/null 2>&1 || missing_deps+=("clang++")
    fi

    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        echo "Install with: sudo apt-get install ${missing_deps[*]}"
        exit 1
    fi

    # Check premake5 for BLAST/FLOW
    if ($BUILD_BLAST || $BUILD_FLOW) && ! command -v premake5 >/dev/null 2>&1; then
        print_warning "premake5 not found - will attempt to download it"
        install_premake5
    fi

    print_success "All dependencies found"
}

install_premake5() {
    print_info "Installing premake5..."

    local premake_dir="$SCRIPT_DIR/.tools/premake5"
    mkdir -p "$premake_dir"

    if [ ! -f "$premake_dir/premake5" ]; then
        cd "$premake_dir"
        curl -L -o premake5.tar.gz https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-linux.tar.gz
        tar -xzf premake5.tar.gz
        rm premake5.tar.gz
        cd "$SCRIPT_DIR"
        print_success "premake5 installed to $premake_dir"
    fi

    export PATH="$premake_dir:$PATH"
}

is_library_built() {
    local lib=$1
    local config=$2

    case $lib in
        physx)
            local physx_lib="physx/bin/linux.${COMPILER}/${config}/libPhysX_64.so"
            [ -f "$physx_lib" ] && return 0 || return 1
            ;;
        blast)
            local blast_lib="blast/_build/linux-x86_64/${config}/libNvBlast.so"
            [ -f "$blast_lib" ] && return 0 || return 1
            ;;
        flow)
            local flow_lib="flow/_build/linux-x86_64/${config}/libNvFlow.so"
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

    print_header "Building PhysX ($config) with system $COMPILER"

    if ! $FORCE && is_library_built "physx" "$config"; then
        print_warning "PhysX already built for $config configuration. Use --force to rebuild."
        return 0
    fi

    cd "$SCRIPT_DIR/physx"

    # Set environment variables
    export PHYSX_ROOT_DIR="$SCRIPT_DIR/physx"

    # Map config names: release->release, debug->debug, checked->checked
    local cmake_build_type
    case $config in
        release) cmake_build_type="release" ;;
        debug) cmake_build_type="debug" ;;
        checked) cmake_build_type="checked" ;;
        *) cmake_build_type="release" ;;
    esac

    print_info "Using system $COMPILER compiler"
    print_info "Build configuration: $cmake_build_type"

    # Determine compiler executables
    local cc_compiler
    local cxx_compiler
    if [ "$COMPILER" = "gcc" ]; then
        cc_compiler=$(which gcc)
        cxx_compiler=$(which g++)
    else
        cc_compiler=$(which clang)
        cxx_compiler=$(which clang++)
    fi

    print_info "C Compiler: $cc_compiler"
    print_info "C++ Compiler: $cxx_compiler"

    # Create build directory
    local build_dir="compiler/linux-${COMPILER}-system-${config}"
    mkdir -p "$build_dir"
    cd "$build_dir"

    # Run CMake with system compilers
    print_info "Generating PhysX build files with CMake..."

    # Set module path for PhysX CMake modules
    local cmake_module_path="$SCRIPT_DIR/physx/source/compiler/cmake/modules"
    local output_bin_dir="$SCRIPT_DIR/physx/bin/linux.${COMPILER}"
    local output_lib_dir="$SCRIPT_DIR/physx/bin/linux.${COMPILER}"

    cmake \
        -DCMAKE_C_COMPILER="$cc_compiler" \
        -DCMAKE_CXX_COMPILER="$cxx_compiler" \
        -DCMAKE_BUILD_TYPE="$cmake_build_type" \
        -DCMAKE_MODULE_PATH="$cmake_module_path" \
        -DTARGET_BUILD_PLATFORM="linux" \
        -DPX_OUTPUT_LIB_DIR="$output_lib_dir" \
        -DPX_OUTPUT_BIN_DIR="$output_bin_dir" \
        -DPX_OUTPUT_EXE_DIR="$output_bin_dir" \
        -DPX_OUTPUT_DLL_DIR="$output_bin_dir" \
        -DPX_BUILDSNIPPETS=OFF \
        -DPX_BUILDPVDRUNTIME=ON \
        -DPX_GENERATE_STATIC_LIBRARIES=OFF \
        -DPX_GENERATE_GPU_PROJECTS=OFF \
        -DPX_GENERATE_GPU_PROJECTS_ONLY=OFF \
        -DPUBLIC_RELEASE=ON \
        -DCMAKE_INSTALL_PREFIX="$SCRIPT_DIR/physx/install/linux-${COMPILER}-system" \
        "$SCRIPT_DIR/physx/source/compiler/cmake"

    if [ $? -ne 0 ]; then
        print_error "CMake generation failed"
        cd "$SCRIPT_DIR"
        return 1
    fi

    # Build
    print_info "Compiling PhysX ($config) with $JOBS jobs..."
    make -j${JOBS}

    if [ $? -ne 0 ]; then
        print_error "Compilation failed"
        cd "$SCRIPT_DIR"
        return 1
    fi

    # Create convenience symlink to match expected output directory
    cd "$SCRIPT_DIR/physx"
    mkdir -p "bin/linux.${COMPILER}/${config}"

    # Copy built libraries to expected location
    if [ -d "$build_dir/bin" ]; then
        cp -r "$build_dir/bin/"* "bin/linux.${COMPILER}/${config}/" 2>/dev/null || true
    fi

    cd "$SCRIPT_DIR"
    print_success "PhysX ($config) built successfully with system $COMPILER"
    print_info "Libraries location: physx/bin/linux.${COMPILER}/${config}/"
}

build_blast() {
    local config=$1

    print_header "Building BLAST ($config) with system tools"

    if ! $FORCE && is_library_built "blast" "$config"; then
        print_warning "BLAST already built for $config configuration. Use --force to rebuild."
        return 0
    fi

    if [ ! -d "$SCRIPT_DIR/blast" ]; then
        print_error "BLAST directory not found. Please ensure blast/ exists."
        return 1
    fi

    cd "$SCRIPT_DIR/blast"

    # Map config for premake
    local premake_config
    case $config in
        release) premake_config="release_x86_64" ;;
        debug) premake_config="debug_x86_64" ;;
        checked) premake_config="checked_x86_64" ;;
        *) premake_config="release_x86_64" ;;
    esac

    print_info "Using premake5 to generate build files..."

    # Set compiler in environment for premake
    if [ "$COMPILER" = "gcc" ]; then
        export CC=$(which gcc)
        export CXX=$(which g++)
    else
        export CC=$(which clang)
        export CXX=$(which clang++)
    fi

    # Generate makefiles with premake5
    premake5 gmake2

    if [ $? -ne 0 ]; then
        print_error "premake5 generation failed"
        print_info "Trying alternative: direct CMake approach..."

        # Fallback to CMake if available
        if [ -f "CMakeLists.txt" ]; then
            mkdir -p "_build/${config}"
            cd "_build/${config}"
            cmake ../.. -DCMAKE_BUILD_TYPE=$(echo $config | awk '{print toupper(substr($0,1,1)) tolower(substr($0,2))}')
            make -j${JOBS}
            cd "$SCRIPT_DIR"
            print_success "BLAST ($config) built successfully"
            return 0
        else
            cd "$SCRIPT_DIR"
            return 1
        fi
    fi

    # Build with make
    print_info "Compiling BLAST ($config)..."
    cd _build/gmake2
    make config=${premake_config} -j${JOBS}

    if [ $? -ne 0 ]; then
        print_error "BLAST compilation failed"
        cd "$SCRIPT_DIR"
        return 1
    fi

    cd "$SCRIPT_DIR"
    print_success "BLAST ($config) built successfully"
    print_info "Libraries location: blast/_build/linux-x86_64/${config}/"
}

build_flow() {
    local config=$1

    print_header "Building FLOW ($config) with system tools"

    if ! $FORCE && is_library_built "flow" "$config"; then
        print_warning "FLOW already built for $config configuration. Use --force to rebuild."
        return 0
    fi

    if [ ! -d "$SCRIPT_DIR/flow" ]; then
        print_error "FLOW directory not found. Please ensure flow/ exists."
        return 1
    fi

    cd "$SCRIPT_DIR/flow"

    # Map config for premake
    local premake_config
    case $config in
        release) premake_config="release_x86_64" ;;
        debug) premake_config="debug_x86_64" ;;
        *) premake_config="release_x86_64" ;;
    esac

    print_info "Using premake5 to generate build files..."

    # Set compiler in environment for premake
    if [ "$COMPILER" = "gcc" ]; then
        export CC=$(which gcc)
        export CXX=$(which g++)
    else
        export CC=$(which clang)
        export CXX=$(which clang++)
    fi

    # Generate makefiles with premake5
    premake5 gmake2

    if [ $? -ne 0 ]; then
        print_error "premake5 generation failed for FLOW"
        print_warning "FLOW requires premake5 and may need additional dependencies"
        cd "$SCRIPT_DIR"
        return 1
    fi

    # Build with make
    print_info "Compiling FLOW ($config)..."
    cd _compiler/gmake2
    make config=${premake_config} -j${JOBS}

    if [ $? -ne 0 ]; then
        print_error "FLOW compilation failed"
        cd "$SCRIPT_DIR"
        return 1
    fi

    cd "$SCRIPT_DIR"
    print_success "FLOW ($config) built successfully"
    print_info "Libraries location: flow/_build/linux-x86_64/${config}/"
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
            rm -rf blast/_build/
            rm -rf blast/_compiler/
            ;;
        flow)
            rm -rf flow/_build/
            rm -rf flow/_compiler/
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
        --compiler)
            COMPILER="$2"
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

# Validate compiler
case $COMPILER in
    gcc|clang)
        ;;
    *)
        print_error "Invalid compiler: $COMPILER"
        echo "Valid compilers: gcc, clang"
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
echo "Compiler: $COMPILER"
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
        print_error "BLAST build failed (this is expected if dependencies are missing)"
    fi
    echo ""
fi

if $BUILD_FLOW; then
    if ! build_flow "$CONFIG"; then
        BUILD_FAILED=true
        print_error "FLOW build failed (this is expected if dependencies are missing)"
    fi
    echo ""
fi

# Summary
print_header "Build Summary"
if $BUILD_FAILED; then
    print_warning "Some builds failed. Check the output above for details."
    echo ""
    echo "Note: BLAST and FLOW may require additional setup."
    echo "PhysX should build successfully with system libraries."
    exit 1
else
    print_success "All builds completed successfully!"
    echo ""
    echo "Library locations:"
    $BUILD_PHYSX && echo "  PhysX: physx/bin/linux.${COMPILER}/${CONFIG}/"
    $BUILD_BLAST && echo "  BLAST: blast/_build/linux-x86_64/${CONFIG}/"
    $BUILD_FLOW && echo "  FLOW: flow/_build/linux-x86_64/${CONFIG}/"
fi

exit 0
