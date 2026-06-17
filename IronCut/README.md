# IronCut - Professional Video Editor

A high-performance, professional-grade video editing engine built with modern C++ and FFmpeg. Designed to provide Adobe Premiere Pro-level functionality in a lightweight, command-line driven interface with the potential for GUI integration.

## Features

### Core Capabilities
- **Multi-track Timeline**: Unlimited video and audio tracks with non-destructive editing
- **Precision Editing**: Frame-accurate cutting, trimming, and splitting
- **Professional Export**: H.264/H.265 encoding with customizable bitrate and quality settings
- **Real-time Playback**: Smooth preview with frame-accurate seeking
- **Thread-Safe Architecture**: All timeline operations are thread-safe for concurrent access

### Technical Highlights
- **FFmpeg Integration**: Full support for virtually all video/audio formats
- **Hardware Acceleration Ready**: Optimized for native CPU instructions (AVX, SSE)
- **Memory Efficient**: Smart buffer management and zero-copy operations where possible
- **Progress Tracking**: Real-time export progress with callback support
- **Rational Time System**: Precise time handling for variable frame rates

## System Requirements

### Minimum Requirements
- **OS**: Linux (Ubuntu 20.04+ recommended), macOS, or Windows with WSL
- **Compiler**: GCC 9+ or Clang 10+ with C++17 support
- **RAM**: 4GB minimum, 8GB+ recommended
- **Storage**: 100MB for source + dependencies

### Dependencies
- **CMake**: 3.10 or higher
- **FFmpeg**: Development libraries (libavcodec, libavformat, libavutil, libswscale, libswresample)
- **pkg-config**: For dependency discovery
- **pthread**: Threading support (included in most Unix systems)

## Installation Guide for WSL Ubuntu

### Step 1: Update System Packages

```bash
sudo apt update && sudo apt upgrade -y
```

### Step 2: Install Build Tools

```bash
sudo apt install -y build-essential cmake git pkg-config
```

### Step 3: Install FFmpeg Development Libraries

```bash
sudo apt install -y ffmpeg libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev
```

**Note**: If you need the latest FFmpeg version with additional codecs, you can build from source:

```bash
# Optional: Build FFmpeg from source for latest features
cd ~
git clone https://github.com/FFmpeg/FFmpeg.git ffmpeg-source
cd ffmpeg-source
./configure --enable-gpl --enable-libx264 --enable-libx265 --enable-libvpx --enable-libmp3lame --enable-libopus --enable-libvorbis --enable-libtheora --enable-libfreetype --enable-libass --enable-libfontconfig --enable-nonfree
make -j$(nproc)
sudo make install
sudo ldconfig
```

### Step 4: Verify FFmpeg Installation

```bash
pkg-config --modversion libavcodec
pkg-config --modversion libavformat
```

You should see version numbers (e.g., 58.91.100). If you get errors, ensure the dev packages are installed.

### Step 5: Navigate to Project Directory

```bash
cd /workspace/IronCut
```

### Step 6: Create Build Directory

```bash
mkdir -p build
cd build
```

### Step 7: Configure with CMake

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

**Important**: If CMake cannot find FFmpeg, you may need to specify the path:

```bash
# If FFmpeg was installed manually
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/usr/local
```

### Step 8: Build the Project

```bash
# Build with all available CPU cores
make -j$(nproc)
```

### Step 9: Install (Optional)

```bash
sudo make install
```

This installs the `ironcut` binary to `/usr/local/bin`.

### Step 10: Run IronCut

```bash
# From build directory
./ironcut

# Or if installed system-wide
ironcut
```

## Usage

### Command Line Options

```bash
# Show help
./ironcut --help

# Show version
./ironcut --version

# Start interactive mode
./ironcut
```

### Interactive Commands

Once in interactive mode:

```
ironcut> help                    # Show available commands
ironcut> add-video /path/to/video.mp4    # Add video clip
ironcut> add-audio /path/to/audio.mp3    # Add audio clip
ironcut> play                    # Start playback
ironcut> pause                   # Pause playback
ironcut> stop                    # Stop playback
ironcut> seek 100                # Seek to frame 100
ironcut> status                  # Show timeline status
ironcut> export output.mp4       # Export to file
ironcut> quit                    # Exit application
```

### Example Session

```bash
$ ./ironcut

=================================
  IronCut Video Editor v1.0.0
=================================

Type 'help' for available commands

ironcut> add-video ~/videos/intro.mp4
Video added to track 0

ironcut> add-audio ~/music/background.mp3
Audio added to track 0

ironcut> status

Timeline Status:
  Resolution: 1920x1080
  FPS: 30
  Duration: 45.5 seconds
  Current Frame: 0
  Playing: No

ironcut> export ~/output/final_video.mp4
Starting export to ~/output/final_video.mp4...
Export Progress: [==================================================>] 100% - Export complete!
Export completed successfully!

ironcut> quit

Goodbye!
```

## Building for Different Configurations

### Debug Build

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### Optimized Release Build (Recommended)

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native"
make -j$(nproc)
```

### Static Linking (For Portable Binary)

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF
make -j$(nproc)
```

## Troubleshooting

### Common Issues

#### 1. "FFmpeg not found" Error

**Solution**: Ensure FFmpeg development packages are installed:
```bash
sudo apt install -y libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libswresample-dev
```

#### 2. "pkg-config: command not found"

**Solution**: Install pkg-config:
```bash
sudo apt install -y pkg-config
```

#### 3. Compilation Errors Related to FFmpeg API

FFmpeg APIs change between versions. If you encounter compilation errors:
- Check the FFmpeg version: `ffmpeg -version`
- The code is tested with FFmpeg 4.x and 5.x
- For FFmpeg 6.x+, you may need to update deprecated API calls

#### 4. "Permission denied" when running

**Solution**: Make the binary executable:
```bash
chmod +x ironcut
```

#### 5. Slow Export Performance

**Solutions**:
- Ensure you're building in Release mode
- Use hardware acceleration if available (requires additional FFmpeg flags)
- Reduce bitrate or resolution for faster encoding

### Performance Optimization Tips

1. **Use Native Optimizations**:
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native"
   ```

2. **Enable Parallel Compilation**:
   ```bash
   make -j$(nproc)
   ```

3. **Use Fast Storage**: SSD storage significantly improves I/O performance

4. **Allocate More RAM**: Video editing is memory-intensive

## Architecture Overview

```
IronCut/
├── include/
│   ├── clip.h          # Clip data structure and operations
│   ├── timeline.h      # Multi-track timeline management
│   └── engine.h        # Rendering and export engine
├── src/
│   ├── main.cpp        # Application entry point and CLI
│   ├── clip.cpp        # Clip implementation
│   ├── timeline.cpp    # Timeline implementation
│   └── engine.cpp      # FFmpeg-based rendering engine
├── CMakeLists.txt      # Build configuration
└── README.md           # This file
```

### Key Components

1. **Clip**: Represents a media segment with timing information
2. **Timeline**: Manages multiple tracks of clips with composition logic
3. **Engine**: Handles decoding, processing, and encoding via FFmpeg
4. **RationalTime**: Precise time representation for variable frame rates

## Extending IronCut

### Adding Effects

To add video effects, extend the `Engine` class:

```cpp
// In engine.h
void applyGrayscale(uint8_t* frame, int width, int height);

// In engine.cpp
void Engine::applyGrayscale(uint8_t* frame, int width, int height) {
    for (int i = 0; i < width * height * 3; i += 3) {
        uint8_t gray = (frame[i] + frame[i+1] + frame[i+2]) / 3;
        frame[i] = frame[i+1] = frame[i+2] = gray;
    }
}
```

### Adding GUI Support

IronCut is designed to be backend-agnostic. You can integrate with:
- **Qt**: For cross-platform desktop GUI
- **Dear ImGui**: For lightweight immediate-mode GUI
- **Custom OpenGL**: For GPU-accelerated preview

Example Qt integration:
```cpp
// Create QOpenGLWidget that calls engine->getFrameRGB()
// Display frames at timeline->getFPS()
```

## License

This project is provided as-is for educational and commercial use. 

## Contributing

Contributions are welcome! Areas for improvement:
- GPU acceleration (CUDA/Vulkan)
- Additional video effects and transitions
- Audio waveform visualization
- Proxy editing for 4K+ footage
- XML/EDL import/export for interchange with other editors

## Acknowledgments

Built with:
- [FFmpeg](https://ffmpeg.org/) - Multimedia framework
- [CMake](https://cmake.org/) - Build system
- Modern C++17 Standard Library

---

**Happy Editing!** 🎬

For issues or questions, please check the troubleshooting section or review the source code comments for detailed implementation notes.
