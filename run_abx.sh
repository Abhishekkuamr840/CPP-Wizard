#!/bin/bash

# ABX Mock Exchange Client Automation Script
# Usage: ./run_abx.sh
# This script:
# 1. Checks Node.js version
# 2. Starts the ABX server
# 3. Sets up the client
# 4. Runs the client
# 5. Verifies the output

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to check Node.js version
check_node_version() {
    if ! command_exists node; then
        echo "Error: Node.js is not installed"
        exit 1
    fi

    # Get Node.js version
    NODE_VERSION=$(node -v | cut -d'v' -f2)
    REQUIRED_VERSION="16.17.0"

    # Compare versions
    if [ "$(printf '%s\n' "$REQUIRED_VERSION" "$NODE_VERSION" | sort -V | head -n1)" != "$REQUIRED_VERSION" ]; then
        echo "Error: Node.js version must be >= 16.17.0 (found $NODE_VERSION)"
        exit 1
    fi

    echo "Node.js version $NODE_VERSION is compatible"
}

# Function to start the ABX server
start_server() {
    echo "Starting ABX server..."
    if [ ! -d "server" ]; then
        echo "Error: server directory not found"
        exit 1
    fi

    # Start server in background
    cd server
    node main.js > server.log 2>&1 &
    SERVER_PID=$!
    cd ..

    # Wait for server to start
    sleep 2
    if ! kill -0 $SERVER_PID 2>/dev/null; then
        echo "Error: Failed to start ABX server"
        cat server/server.log
        exit 1
    fi

    echo "ABX server started (PID: $SERVER_PID)"
}

# Function to setup client
setup_client() {
    echo "Setting up ABX client..."
    if [ ! -d "abx-client" ]; then
        echo "Cloning abx-client repository..."
        if ! git clone https://github.com/your-username/abx-client.git; then
            echo "Error: Failed to clone repository"
            exit 1
        fi
    fi

    cd abx-client
    echo "Compiling abx_client.cpp..."
    if ! g++ -std=c++17 -o abx_client abx_client.cpp; then
        echo "Error: Failed to compile abx_client.cpp"
        exit 1
    fi
    cd ..
}

# Function to run client
run_client() {
    echo "Running ABX client..."
    cd abx-client

    # Get host from user
    read -p "Enter server host [127.0.0.1]: " HOST
    HOST=${HOST:-127.0.0.1}

    # Run client
    if ! ./abx_client "$HOST" 3000; then
        echo "Error: Client execution failed"
        exit 1
    fi

    # Verify packets.json
    if [ ! -f "packets.json" ]; then
        echo "Error: packets.json not found"
        exit 1
    fi

    echo "Success! packets.json created at: $(pwd)/packets.json"
    cd ..
}

# Function to cleanup
cleanup() {
    if [ -n "$SERVER_PID" ]; then
        echo "Stopping ABX server..."
        kill $SERVER_PID 2>/dev/null
    fi
}

# Set up cleanup on script exit
trap cleanup EXIT

# Main execution
echo "=== ABX Mock Exchange Client Automation ==="
check_node_version
start_server
setup_client
run_client
echo "=== Script completed successfully ===" 