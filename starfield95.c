#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

/* Window dimensions */
#define WINDOW_WIDTH  1280
#define WINDOW_HEIGHT 720

/* How many stars to draw */
#define STAR_COUNT 500

/* 
 * We'll reset (re-init) a star if it gets too close
 * to the viewer, i.e. z < MIN_Z.
 */
#define MIN_Z 0.05f

/* 
 * We avoid placing a star within this radius
 * of the origin in the (x,y) plane on reinit,
 * to reduce the chance of "big rays" from near (0,0).
 */
#define MIN_RADIUS 0.1f

/*
 * For perspective, we do:
 *   screen_x = center_x + (x * (PERSPECTIVE_SCALE / z))
 * The bigger this is, the more dramatic the outward "explosion."
 */
#define PERSPECTIVE_SCALE 150.0f

/* 
 * If you only want the trail when it's very close, 
 * you can set a threshold, for example 0.3. 
 * If star.z < NEAR_THRESHOLD => draw line. 
 * Otherwise, just draw a point. 
 * (Optional feature, can set NEAR_THRESHOLD=999 if you want trails always.)
 */
#define NEAR_THRESHOLD 0.3f

/* SDL2 context handles */
SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;
TTF_Font* gFont = NULL;

/* 
 * Define the base speed and speed range for stars.
 * Adjust these values to change the speed of the stars.
 */
#define BASE_SPEED 0.001f
#define SPEED_RANGE 0.009f

typedef struct {
    float x, y, z;    /* 3D position in [-1..1] for x,y, and (0..1] for z */
    float oldX, oldY; /* Last frame's 2D screen position in pixels */
    float speed;      /* Speed at which z decreases */
} Star;

/* Global array of stars */
static Star stars[STAR_COUNT];

/* Track current window size so we can recenter stars after resize. */
static int gWidth  = WINDOW_WIDTH;
static int gHeight = WINDOW_HEIGHT;

/* Globals for FPS calculation */
static float gFPS    = 0.0f;
static int   gFrames = 0;
static Uint32 gLastTime = 0;

/* Store the renderer info */
static SDL_RendererInfo gRendererInfo;

/* Return a random float in [0,1]. */
static float randFloat() {
    return (float)rand() / (float)RAND_MAX;
}

/*
 * Initialize (or reinitialize) one star at a random position
 * that is at least MIN_RADIUS away from the origin in x,y.
 * Then set the star's old screen position = new screen position
 * so no immediate "big line" is drawn on first frame.
 */
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

    /* Speeds in [0.002..0.02], adjust as you like. */
    stars[i].speed = BASE_SPEED + SPEED_RANGE * randFloat();

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

/* Initialize all stars once at program start. */
static void initStars() {
    for (int i = 0; i < STAR_COUNT; i++) {
        initStar(i);
    }
}

/*
 * Update each star per frame:
 *  1) Record the star's current screen position as oldX/oldY.
 *  2) Move the star closer by subtracting 'speed' from z.
 *  3) If z < MIN_Z, re-initialize the star.
 */
static void updateStars() {
    for (int i = 0; i < STAR_COUNT; i++) {
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

/* 
 * Draw text using SDL_ttf at position (x, y).
 */
static void drawText(const char* text, int x, int y, SDL_Color color) {
    if (!gFont) return;
    
    SDL_Surface* surface = TTF_RenderText_Solid(gFont, text, color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(gRenderer, surface);
        if (texture) {
            SDL_Rect dstRect = {x, y, surface->w, surface->h};
            SDL_RenderCopy(gRenderer, texture, NULL, &dstRect);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
}

/*
 * Draw the starfield:
 *  - "Far" stars (z >= NEAR_THRESHOLD) as points
 *  - "Near" stars (z < NEAR_THRESHOLD) as short lines from old -> new
 * Then draw the FPS counter and renderer info.
 */
static void render() {
    /* Clear screen to black */
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
    SDL_RenderClear(gRenderer);

    /* Set draw color to white for stars */
    SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);

    /* 1. Draw FAR STARS as points */
    for (int i = 0; i < STAR_COUNT; i++) {
        if (stars[i].z >= NEAR_THRESHOLD) {
            float factor = PERSPECTIVE_SCALE / stars[i].z;
            int x = (int)((gWidth  / 2.0f) + (stars[i].x * factor));
            int y = (int)((gHeight / 2.0f) + (stars[i].y * factor));
            SDL_RenderDrawPoint(gRenderer, x, y);
        }
    }

    /* 2. Draw NEAR STARS as short lines (trails) */
    for (int i = 0; i < STAR_COUNT; i++) {
        if (stars[i].z < NEAR_THRESHOLD) {
            float factor = PERSPECTIVE_SCALE / stars[i].z;
            int newX = (int)((gWidth  / 2.0f) + (stars[i].x * factor));
            int newY = (int)((gHeight / 2.0f) + (stars[i].y * factor));

            SDL_RenderDrawLine(gRenderer, 
                (int)stars[i].oldX, (int)stars[i].oldY,
                newX, newY);
        }
    }

    /* 3. Draw the FPS counter and renderer info */
    SDL_Color green = {0, 255, 0, 255};
    char buf[64];
    sprintf(buf, "FPS: %.2f", gFPS);
    drawText(buf, 10, gHeight - 30, green);
    
    /* Print renderer name */
    drawText(gRendererInfo.name, 10, gHeight - 60, green);

    SDL_RenderPresent(gRenderer);
}

/*
 * Handle window resize event
 */
static void handleResize(int width, int height) {
    if (height == 0) height = 1;

    gWidth = width;
    gHeight = height;

    /*
     * Re-sync the 'oldX/oldY' for each star to its 
     * "current" position in the *new* window size.
     */
    for (int i = 0; i < STAR_COUNT; i++) {
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

    /* Initialize SDL_ttf */
    if (TTF_Init() < 0) {
        printf("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    /* Create window */
    gWindow = SDL_CreateWindow("Starfield95",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    
    if (!gWindow) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    /* Create renderer */
    gRenderer = SDL_CreateRenderer(gWindow, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (!gRenderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(gWindow);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    /* Get renderer info */
    SDL_GetRendererInfo(gRenderer, &gRendererInfo);

    /* Load font */
    gFont = TTF_OpenFont("@fontPath@", 18);
    if (!gFont) {
        printf("Failed to load font! TTF_Error: %s\n", TTF_GetError());
        /* Continue without font, text won't be rendered */
    }

    /* Seed RNG and init stars */
    srand((unsigned)time(NULL));
    initStars();

    /* Initialize the time marker for FPS */
    gLastTime = SDL_GetTicks();

    /* Main loop flag */
    int quit = 0;

    /* Event handler */
    SDL_Event e;

    /* Main loop */
    while (!quit) {
        /* Handle events */
        while (SDL_PollEvent(&e) != 0) {
            switch (e.type) {
                case SDL_QUIT:
                    quit = 1;
                    break;
                case SDL_WINDOWEVENT:
                    if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                        handleResize(e.window.data1, e.window.data2);
                    }
                    break;
                case SDL_KEYDOWN:
                    if (e.key.keysym.sym == SDLK_ESCAPE) {
                        quit = 1;
                    }
                    break;
            }
        }

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
    if (gFont) TTF_CloseFont(gFont);
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
