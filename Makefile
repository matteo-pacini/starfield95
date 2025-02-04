CFLAGS = -O2 $(shell pkg-config --cflags sdl2 SDL2_ttf)
LDFLAGS = $(shell pkg-config --libs sdl2 SDL2_ttf)

starfield95: starfield95.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f starfield95
