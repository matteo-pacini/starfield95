# Starfield95

Starfield95 is a recreation of the classic Windows 95 starfield screensaver, reimplemented in C using SDL2. 

This project brings back the nostalgic starfield effect with modern rendering techniques.

![Starfield95](img.png)


- Realistic star movement with perspective scaling
- Trail effects for nearby stars
- Adjustable star speed and count
- FPS counter and renderer information display
- Resizable window with automatic star repositioning

## Build Instructions (Nix)

This project uses Nix for building and running. To build and run the project, use the following command:

```bash
nix run path:.
```

## Build Instructions (Non-Nix)

To build the project without Nix, ensure you have the following dependencies installed:
- pkg-config
- SDL2
- SDL2_ttf

Follow these steps:
1. Replace `@fontPath@` in `starfield95.c` with the path to a valid TTF font file on your system.
2. Run `CC="yourCompilerOfChoice" make` to compile the project.
3. Execute `./starfield95` to run the program.

## Controls

- **ESC**: Quit the application

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
