# Build for ST-Link/SWD debugging (standalone, no bootloader)
debug:
    cmake --preset Debug
    cmake --build --preset Debug

# Build for bootloader deployment
release:
    cmake --preset Release
    cmake --build --preset Release

# Clean all build artifacts
clean:
    rm -rf build/Debug build/Release
