#ifdef __APPLE__
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
#endif

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

/* Star data */
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
static int   gLastTime = 0;

/* Store the OpenGL vendor & renderer strings */
static const char *gVendorStr   = NULL;
static const char *gRendererStr = NULL;

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
    stars[i].speed = 0.002f + 0.018f * randFloat();

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
 * Simple function to draw a string in 2D at (x, y).
 * Uses a GLUT bitmap font for simplicity.
 */
static void drawBitmapString(float x, float y, const char *string)
{
    glRasterPos2f(x, y);
    while (*string) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *string);
        ++string;
    }
}

/*
 * Draw the starfield:
 *  - "Far" stars (z >= NEAR_THRESHOLD) as points
 *  - "Near" stars (z < NEAR_THRESHOLD) as short lines from old -> new
 * Then draw the green FPS counter and vendor/renderer strings.
 */
static void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /* 1. Draw FAR STARS as points. */
    glColor3f(1.0f, 1.0f, 1.0f);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < STAR_COUNT; i++) {
        if (stars[i].z >= NEAR_THRESHOLD) {
            float factor = PERSPECTIVE_SCALE / stars[i].z;
            float x = (gWidth  / 2.0f) + (stars[i].x * factor);
            float y = (gHeight / 2.0f) + (stars[i].y * factor);
            glVertex2f(x, y);
        }
    }
    glEnd();

    /* 2. Draw NEAR STARS as short lines (trails). */
    glBegin(GL_LINES);
    for (int i = 0; i < STAR_COUNT; i++) {
        if (stars[i].z < NEAR_THRESHOLD) {
            float factor = PERSPECTIVE_SCALE / stars[i].z;
            float newX = (gWidth  / 2.0f) + (stars[i].x * factor);
            float newY = (gHeight / 2.0f) + (stars[i].y * factor);

            glVertex2f(stars[i].oldX, stars[i].oldY);
            glVertex2f(newX, newY);
        }
    }
    glEnd();

    /* 3. Draw the green FPS counter and provider info near the bottom-left. */
    glColor3f(0.0f, 1.0f, 0.0f);

    char buf[64];
    sprintf(buf, "FPS: %.2f", gFPS);
    /* 
     * Because y=0 is the top in this coordinate system (gluOrtho2D(0, w, h, 0)),
     * a larger y-value is lower on the screen. So use gHeight - offset.
     */
    drawBitmapString(10.0f, gHeight - 10.0f, buf);

    /* Print out vendor and renderer strings below the FPS counter. */
    if (gVendorStr) {
        drawBitmapString(10.0f, gHeight - 30.0f, gVendorStr);
    }
    if (gRendererStr) {
        drawBitmapString(10.0f, gHeight - 50.0f, gRendererStr);
    }

    glutSwapBuffers();
}

/*
 * GLUT idle callback: keep the animation going.
 * Also measure FPS once per second.
 */
static void idle() {
    updateStars();

    /* Measure FPS */
    int currentTime = glutGet(GLUT_ELAPSED_TIME); /* in milliseconds */
    gFrames++;
    if (currentTime - gLastTime >= 1000) {
        /* Compute frames per second over the last ~1 second */
        gFPS = (gFrames * 1000.0f) / (float)(currentTime - gLastTime);
        gLastTime = currentTime;
        gFrames = 0;
    }

    glutPostRedisplay();
}

/*
 * On window resize, update the viewport and projection.
 * Also re-sync the oldX/oldY for each star to avoid "jumping" lines.
 */
static void reshape(int width, int height) {
    if (height == 0) height = 1;

    gWidth  = width;
    gHeight = height;

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    /* 
     * With gluOrtho2D(0, width, height, 0), 
     *  (0,0) is the top-left corner, 
     *  (width, height) is the bottom-right corner.
     */
    gluOrtho2D(0.0, (GLdouble)width, (GLdouble)height, 0.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    /*
     * Re-sync the 'oldX/oldY' for each star to its 
     * "current" position in the *new* window size.
     */
    for (int i = 0; i < STAR_COUNT; i++) {
        float factor = PERSPECTIVE_SCALE / stars[i].z;
        float newX   = (gWidth  / 2.0f) + (stars[i].x * factor);
        float newY   = (gHeight / 2.0f) + (stars[i].y * factor);
        stars[i].oldX = newX;
        stars[i].oldY = newY;
    }
}

/*
 * Keyboard callback: ESC quits.
 */
static void handleKeypress(unsigned char key, int x, int y) {
    if (key == 27) {
        exit(0);
    }
}

int main(int argc, char** argv) {
    /* Seed RNG for star init */
    srand((unsigned)time(NULL));
    initStars();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Starfield95");

    /* 
     * Query and store the OpenGL driver/vendor info 
     * (e.g., "Mesa/X.org", "NVIDIA", etc.).
     */
    gVendorStr   = (const char*) glGetString(GL_VENDOR);
    gRendererStr = (const char*) glGetString(GL_RENDERER);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(handleKeypress);

    /* Initialize the time marker for FPS. */
    gLastTime = glutGet(GLUT_ELAPSED_TIME);

    glutMainLoop();
    return 0;
}
