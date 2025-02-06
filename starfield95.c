/*
 * MIT License
 * 
 * Copyright (c) 2025 Matteo Pacini
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

/* Nuklear implementation */
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include "nuklear.h"
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear_sdl_renderer.h"

/* Window dimensions */
#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720

/* Star count and speed control */
static int starCount = 500;
static float starSlider = 0.1f;  /* 0.1 = 500 stars, 1.0 = 5000 stars */
static float speedSlider = 0.5f;  /* Controls star movement speed: 0=stop, 0.5=normal, 1.0=2x */

/* Reset a star if it gets too close to the viewer */
#define MIN_Z 0.05f

/* Avoid placing a star too close to the center to prevent big streaks */
#define MIN_RADIUS 0.1f

/* Perspective calculation for dramatic starfield effect */
#define PERSPECTIVE_SCALE 150.0f

/* Draw trails for stars that are close enough */
#define NEAR_THRESHOLD 0.3f

SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;

struct nk_context* ctx = NULL;

#define BASE_SPEED 0.001f
#define SPEED_RANGE 0.009f

typedef struct {
    float x, y, z;    /* 3D position in [-1..1] for x,y, and (0..1] for z */
    float oldX, oldY; /* Last frame's 2D screen position in pixels */
    float speed;      /* Speed at which z decreases */
} Star;

static int gWidth  = WINDOW_WIDTH;
static int gHeight = WINDOW_HEIGHT;

static float gFPS    = 0.0f;
static int   gFrames = 0;
static Uint32 gLastTime = 0;

static SDL_RendererInfo gRendererInfo;

static float randFloat() {
    return (float)rand() / (float)RAND_MAX;
}

static Star* stars = NULL;

static int allocateStars(int count) {
    Star* newStars = (Star*)realloc(stars, count * sizeof(Star));
    if (!newStars) {
        return 0;
    }
    
    /* Initialize new stars if array grew */
    if (count > starCount) {
        for (int i = starCount; i < count; i++) {
            float r2 = 0.0f;
            float z = 0.1f + 0.9f * randFloat();
            float x, y;
            do {
                x = 2.0f * (randFloat() - 0.5f);
                y = 2.0f * (randFloat() - 0.5f);
                r2 = x*x + y*y;
            } while (r2 < (MIN_RADIUS * MIN_RADIUS));

            newStars[i].x = x;
            newStars[i].y = y;
            newStars[i].z = z;
            newStars[i].speed = (BASE_SPEED + SPEED_RANGE * randFloat()) * (speedSlider * 2.0f);

            float factor = PERSPECTIVE_SCALE / z;
            newStars[i].oldX = (gWidth  / 2.0f) + (x * factor);
            newStars[i].oldY = (gHeight / 2.0f) + (y * factor);
        }
    }
    
    stars = newStars;
    starCount = count;
    return 1;
}

static void initStar(int i) {
    float r2 = 0.0f;

    /* Random z in [0.1..1.0], i.e. "distance." */
    float z = 0.1f + 0.9f * randFloat();

    /*
     * Keep picking (x,y) in [-1..1] until 
     * we get one that is outside the MIN_RADIUS.
     */
    float x, y;
    do {
        x = 2.0f * (randFloat() - 0.5f);  /* in [-1..1] */
        y = 2.0f * (randFloat() - 0.5f);
        r2 = x*x + y*y;
    } while (r2 < (MIN_RADIUS * MIN_RADIUS));

    stars[i].x = x;
    stars[i].y = y;
    stars[i].z = z;

    /* Speed controlled by slider: 0=stop, 0.5=normal, 1.0=2x */
    stars[i].speed = (BASE_SPEED + SPEED_RANGE * randFloat()) * (speedSlider * 2.0f);

    /* 
     * Immediately compute the star's new screen position
     * and set oldX, oldY to that same position,
     * so the star doesn't produce a big line in the first frame.
     */
    {
        float factor = PERSPECTIVE_SCALE / stars[i].z;
        float newX = (gWidth  / 2.0f) + (stars[i].x * factor);
        float newY = (gHeight / 2.0f) + (stars[i].y * factor);

        stars[i].oldX = newX;
        stars[i].oldY = newY;
    }
}

static int initStars() {
    if (!allocateStars(starCount)) {
        return 0;
    }
    for (int i = 0; i < starCount; i++) {
        initStar(i);
    }
    return 1;
}

static void updateStars() {
    for (int i = 0; i < starCount; i++) {
        /* 1) Store old projected position. */
        float factorOld = PERSPECTIVE_SCALE / stars[i].z;
        stars[i].oldX = (gWidth  / 2.0f) + (stars[i].x * factorOld);
        stars[i].oldY = (gHeight / 2.0f) + (stars[i].y * factorOld);

        /* 2) Move star forward (decrease z). */
        stars[i].z -= stars[i].speed;

        /* 3) If star is too close, reinit it far away. */
        if (stars[i].z < MIN_Z) {
            initStar(i);
        }
    }
}

static void render() {
    /* Clear screen to black */
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
    SDL_RenderClear(gRenderer);

    /* Set draw color to white for stars */
    SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);

    /* 1. Draw FAR STARS as points */
    for (int i = 0; i < starCount; i++) {
        if (stars[i].z >= NEAR_THRESHOLD) {
            float factor = PERSPECTIVE_SCALE / stars[i].z;
            int x = (int)((gWidth  / 2.0f) + (stars[i].x * factor));
            int y = (int)((gHeight / 2.0f) + (stars[i].y * factor));
            SDL_RenderDrawPoint(gRenderer, x, y);
        }
    }

    /* 2. Draw NEAR STARS as short lines (trails) */
    for (int i = 0; i < starCount; i++) {
        if (stars[i].z < NEAR_THRESHOLD) {
            float factor = PERSPECTIVE_SCALE / stars[i].z;
            int newX = (int)((gWidth  / 2.0f) + (stars[i].x * factor));
            int newY = (int)((gHeight / 2.0f) + (stars[i].y * factor));

            SDL_RenderDrawLine(gRenderer, 
                (int)stars[i].oldX, (int)stars[i].oldY,
                newX, newY);
        }
    }

    /* 3. Draw the UI windows using Nuklear */
    struct nk_style *style = &ctx->style;
    struct nk_color bg = nk_rgba(0, 0, 0, 200);
    style->window.background = nk_rgba(0, 0, 0, 200);
    style->window.fixed_background = nk_style_item_color(bg);
    style->window.padding = nk_vec2(8, 8);
    
    /* Info window (bottom left) */
    if (nk_begin(ctx, "Info", nk_rect(10, gHeight - 75, 180, 60),
        NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_NO_INPUT)) {
        
        char buf[64];
        sprintf(buf, "FPS: %.2f", gFPS);
        
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label_colored(ctx, gRendererInfo.name, NK_TEXT_LEFT, nk_rgb(0, 255, 0));
        nk_label_colored(ctx, buf, NK_TEXT_LEFT, nk_rgb(0, 255, 0));
    }
    nk_end(ctx);

    /* Settings window (bottom right) */
    if (nk_begin(ctx, "Settings", nk_rect(gWidth - 220, gHeight - 110, 200, 105),
        NK_WINDOW_NO_SCROLLBAR)) {
        
        nk_layout_row_dynamic(ctx, 20, 1);
        
        char buf[32];
        sprintf(buf, "Stars: %d", starCount);
        nk_label_colored(ctx, buf, NK_TEXT_LEFT, nk_rgb(0, 255, 0));
        
        float oldStarValue = starSlider;
        float oldSpeedValue = speedSlider;
        
        nk_slider_float(ctx, 0, &starSlider, 1.0f, 0.01f);
        
        nk_label_colored(ctx, "Speed:", NK_TEXT_LEFT, nk_rgb(0, 255, 0));
        nk_slider_float(ctx, 0, &speedSlider, 1.0f, 0.01f);
        
        /* Handle star count changes */
        if (oldStarValue != starSlider) {
            int newCount = (int)(starSlider * 5000.0f);
            if (newCount < 1) newCount = 1;  /* Ensure at least 1 star */
            allocateStars(newCount);
        }
        
        /* Handle speed changes */
        if (oldSpeedValue != speedSlider) {
            /* Update all existing stars with new speed */
            for (int i = 0; i < starCount; i++) {
                stars[i].speed = (BASE_SPEED + SPEED_RANGE * randFloat()) * (speedSlider * 2.0f);
            }
        }
    }
    nk_end(ctx);

    /* Render Nuklear */
    nk_sdl_render(NK_ANTI_ALIASING_ON);

    /* Present the final frame */
    SDL_RenderPresent(gRenderer);
}

static void handleResize(int width, int height) {
    if (height == 0) height = 1;

    gWidth = width;
    gHeight = height;

    /*
     * Re-sync the 'oldX/oldY' for each star to its 
     * "current" position in the *new* window size.
     */
    for (int i = 0; i < starCount; i++) {
        float factor = PERSPECTIVE_SCALE / stars[i].z;
        float newX = (gWidth  / 2.0f) + (stars[i].x * factor);
        float newY = (gHeight / 2.0f) + (stars[i].y * factor);
        stars[i].oldX = newX;
        stars[i].oldY = newY;
    }
}

int main(int argc, char** argv) {
    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    /* Create window */
    gWindow = SDL_CreateWindow("Starfield95",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    
    if (!gWindow) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    /* Create renderer */
    gRenderer = SDL_CreateRenderer(gWindow, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (!gRenderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(gWindow);
        SDL_Quit();
        return 1;
    }

    /* Get renderer info */
    SDL_GetRendererInfo(gRenderer, &gRendererInfo);

    /* Initialize Nuklear */
    ctx = nk_sdl_init(gWindow, gRenderer);
    if (!ctx) {
        printf("Could not initialize Nuklear!\n");
        SDL_DestroyRenderer(gRenderer);
        SDL_DestroyWindow(gWindow);
        SDL_Quit();
        return 1;
    }

    /* Load Nuklear's default font */
    struct nk_font_atlas *atlas;
    nk_sdl_font_stash_begin(&atlas);
    nk_sdl_font_stash_end();

    /* Seed RNG and init stars */
    srand((unsigned)time(NULL));
    if (!initStars()) {
        printf("Could not allocate stars!\n");
        nk_sdl_shutdown();
        SDL_DestroyRenderer(gRenderer);
        SDL_DestroyWindow(gWindow);
        SDL_Quit();
        return 1;
    }

    /* Initialize the time marker for FPS */
    gLastTime = SDL_GetTicks();

    /* Main loop flag */
    int quit = 0;

    /* Event handler */
    SDL_Event e;

    /* Main loop */
    while (!quit) {
        /* Handle events */
        nk_input_begin(ctx);
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            } else if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                    handleResize(e.window.data1, e.window.data2);
                }
            } else if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    quit = 1;
                }
            }
            nk_sdl_handle_event(&e);
        }
        nk_input_end(ctx);

        /* Update stars */
        updateStars();

        /* Calculate FPS */
        Uint32 currentTime = SDL_GetTicks();
        gFrames++;
        if (currentTime - gLastTime >= 1000) {
            gFPS = (gFrames * 1000.0f) / (float)(currentTime - gLastTime);
            gLastTime = currentTime;
            gFrames = 0;
        }

        /* Render */
        render();
    }

    /* Cleanup */
    if (stars) {
        free(stars);
        stars = NULL;
    }
    nk_sdl_shutdown();
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    SDL_Quit();

    return 0;
}
