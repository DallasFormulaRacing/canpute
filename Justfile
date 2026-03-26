# Build firmware: just build debug | just build release
build config:
    cmake --preset {{capitalize(config)}}
    cmake --build --preset {{capitalize(config)}}

# Start probe-rs DAP server, then run "debugger: start" in Zed
debug:
    probe-rs dap-server --port 50000

# Clean all build artifacts
clean:
    rm -rf build/Debug build/Release
