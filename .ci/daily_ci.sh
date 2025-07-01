#!/bin/bash

# Daily CI Script for Mooncake
# Runs at 00:00 each day

set -e  # Exit on any error

# Configuration
REPO_URL="git@github.com:kvcache-ai/Mooncake.git"
WORKSPACE_DIR="$HOME/daily-ci"
REPO_NAME="Mooncake"
BUILD_DIR="build"

# set cron job:
# crontab -e
# 0 16 * * * /home/ubuntu/daily-ci/daily_ci.sh > /home/ubuntu/daily-ci/ci.out 2>/home/ubuntu/daily-ci/ci.err

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging function
log() {
    echo -e "${BLUE}[$(date '+%Y-%m-%d %H:%M:%S')]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Main CI function
run_daily_ci() {
    log "Starting daily CI process..."
    export MC_METADATA_SERVER=http://127.0.0.1:8080/metadata
    export PATH=$PATH:/usr/local/etcd:/usr/local/go/bin
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
    export GOPROXY=https://goproxy.io,direct
    
    # Create workspace directory if it doesn't exist
    if [ ! -d "$WORKSPACE_DIR" ]; then
        log "Creating workspace directory: $WORKSPACE_DIR"
        mkdir -p "$WORKSPACE_DIR"
    fi
    
    # Navigate to workspace directory
    cd "$WORKSPACE_DIR"
    log "Working directory: $(pwd)"
    
    # Remove existing repo if it exists
    if [ -d "$REPO_NAME" ]; then
        log "Removing existing repository..."
        rm -rf "$REPO_NAME"
    fi
    
    # Clone the repository
    log "Cloning repository: $REPO_URL"
    if git clone "$REPO_URL" "$REPO_NAME"; then
        success "Repository cloned successfully"
    else
        error "Failed to clone repository"
        exit 1
    fi
    
    # Navigate to repository
    cd "$REPO_NAME"
    log "Entered repository directory: $(pwd)"

    # Git clone submodules
    mkdir -p extern
    cd extern
    rm -rf ./pybind11
    git clone -b stable git@github.com:pybind/pybind11.git
    cd ..
    
    # Create build directory
    if [ -d "$BUILD_DIR" ]; then
        log "Removing existing build directory..."
        rm -rf "$BUILD_DIR"
    fi
    
    log "Creating build directory..."
    mkdir -p "$BUILD_DIR"
    
    # Navigate to build directory
    cd "$BUILD_DIR"
    log "Entered build directory: $(pwd)"
    
    # Run cmake to configure the project
    log "Running cmake configuration..."
    cmake .. -DUSE_HTTP=ON -DUSE_ETCD=ON -DSTORE_USE_ETCD=ON
    
    # Compile the project
    log "Compiling the project..."
    make -j$(nproc);

    # Stop etcd servers and python server
    if pgrep -x "etcd" > /dev/null; then
        log "Stopping existing etcd processes..."
        pkill -f etcd
        sleep 2  # Give processes time to terminate
    fi
    if pgrep -x "bootstrap_server.py" > /dev/null; then
        log "Stopping existing python server..."
        pkill -f bootstrap_server.py
        sleep 2  # Give processes time to terminate
    fi
    
    # Start etcd servers
    nohup etcd \
  --name etcd1 \
  --data-dir ./etcd/data1 \
  --listen-client-urls http://localhost:3379 \
  --advertise-client-urls http://localhost:3379 \
  --listen-peer-urls http://localhost:3380 \
  --initial-advertise-peer-urls http://localhost:3380 \
  --initial-cluster etcd1=http://localhost:3380,etcd2=http://localhost:3480,etcd3=http://localhost:3580 \
  --initial-cluster-state new &
  
    nohup etcd \
  --name etcd2 \
  --data-dir ./etcd/data2 \
  --listen-client-urls http://localhost:3479 \
  --advertise-client-urls http://localhost:3479 \
  --listen-peer-urls http://localhost:3480 \
  --initial-advertise-peer-urls http://localhost:3480 \
  --initial-cluster etcd1=http://localhost:3380,etcd2=http://localhost:3480,etcd3=http://localhost:3580 \
  --initial-cluster-state new &

    nohup etcd \
  --name etcd3 \
  --data-dir ./etcd/data3 \
  --listen-client-urls http://localhost:3579 \
  --advertise-client-urls http://localhost:3579 \
  --listen-peer-urls http://localhost:3580 \
  --initial-advertise-peer-urls http://localhost:3580 \
  --initial-cluster etcd1=http://localhost:3380,etcd2=http://localhost:3480,etcd3=http://localhost:3580 \
  --initial-cluster-state new &

    # Start transfer-engine's meta server
    nohup python3 ../mooncake-transfer-engine/example/http-metadata-server-python/bootstrap_server.py &

    # Run unit tests
    log "Running unit tests..."
    ./mooncake-store/tests/e2e/chaos_test --etcd_endpoints="127.0.0.1:3379;127.0.0.1:3479;127.0.0.1:3579"
    log "Daily CI process completed successfully!"

    # Stop etcd servers and python server
    pkill -f etcd
    pkill -f bootstrap_server.py
}

# Check if script is being run directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    run_daily_ci
fi
