# Starfield95

Starfield95 is a recreation of the classic Windows 95 starfield screensaver, reimplemented in C using SDL2. 

This project brings back the nostalgic starfield effect with modern rendering techniques.

![Starfield95](img.png)

## Features
- Realistic star movement with perspective scaling
- Trail effects for nearby stars
- Adjustable star speed and count
- Modern UI using Nuklear immediate mode GUI
- Clean, styled overlay showing FPS and renderer information
- Resizable window with automatic star repositioning

## Build Instructions (Nix)

This project uses Nix for building and running. To build and run the project, use the following command:

```bash
nix run path:.
```

## Build Instructions (Non-Nix)

### Prerequisites

The following dependencies are required:
- SDL2
- SDL2_ttf
- pkg-config
- GCC (default) or Clang compiler

### Ubuntu/Debian
```bash
# Install dependencies
sudo apt update
sudo apt install build-essential pkg-config libsdl2-dev libsdl2-ttf-dev
```

### macOS
```bash
# Using Homebrew
brew install sdl2 sdl2_ttf pkg-config
```

### Fedora
```bash
# Install dependencies
sudo dnf install gcc make pkgconfig SDL2-devel SDL2_ttf-devel
```

### Arch Linux
```bash
# Install dependencies
sudo pacman -S base-devel pkgconf sdl2 sdl2_ttf
```

### Building the Project

1. Clone the repository and navigate to the project directory
2. Compile the project (uses gcc by default):
   ```bash
   make
   ```
   Or specify a different compiler:
   ```bash
   CC=clang make
   ```
3. Run the program:
   ```bash
   ./starfield95
   ```

### Troubleshooting

1. **SDL2 libraries not found**:
   - Ensure development packages are installed
   - Ubuntu/Debian: Check for `libsdl2-dev` and `libsdl2-ttf-dev`
   - Fedora: Check for `SDL2-devel` and `SDL2_ttf-devel`
   - Arch: Verify `sdl2` and `sdl2_ttf` are installed

2. **Compilation errors**:
   - Make sure all development tools are installed
   - Ubuntu/Debian: `sudo apt install build-essential`
   - Fedora: `sudo dnf group install "Development Tools"`
   - Arch: `sudo pacman -S base-devel`

## Controls

- **ESC**: Quit the application

## Third-Party Libraries

This project uses the following third-party libraries:

- **SDL2**: Simple DirectMedia Layer (https://www.libsdl.org)
- **Nuklear**: Immediate mode GUI library (https://github.com/Immediate-Mode-UI/Nuklear)
  - Used for rendering the UI overlay
  - Available under MIT License
  - Included in repository (nuklear.h, nuklear_sdl_renderer.h)

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

### Third-Party Licenses

Nuklear is licensed under MIT License:
```
Copyright (c) 2017 Micha Mettke

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
