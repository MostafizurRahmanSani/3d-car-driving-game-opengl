#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif


GLuint menuTextureID;



#define PI 3.1415926535f


#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720


#define ROAD_WIDTH 14.0f
#define NUM_LANES 3
#define LANE_WIDTH (ROAD_WIDTH / NUM_LANES)

#define SCENE_SLOTS 100

int sceneType[SCENE_SLOTS];
float sceneScale[SCENE_SLOTS];

float treeOffset = 0.0f;
float houseOffset = 0.0f;

#define NUM_SCENE_OBJECTS 80

typedef struct {
    float z;
    int typeLeft;
    int typeRight;
    float scaleLeft;
    float scaleRight;
    float offsetLeft;
    float offsetRight;
} SceneObj;

SceneObj scene[NUM_SCENE_OBJECTS];

#define TREE_LEFT_X -9.0f
#define TREE_RIGHT_X 9.0f

#define HOUSE_LEFT_X -11.5f
#define HOUSE_RIGHT_X 11.5f



static const float LANE_CENTERS[NUM_LANES] = {
    -LANE_WIDTH,
     0.0f,
    +LANE_WIDTH
};



#define PLAYER_Z 18.0f
#define CAM_Y 0.8f
#define CAM_LOOK_Y 0.7f
#define CAM_LOOK_Z -30.0f
#define LANE_SMOOTH 0.15f


#define NUM_OBSTACLES 10
#define OBS_SPAWN_Z -150.0f
#define OBS_CULL_Z 40.0f
#define OBS_PASS_Z 20.0f
#define OBS_SCORE_VALUE 5
#define OBS_INIT_SPACING 50.0f


#define COL_THRESH_X 1.4f
#define COL_THRESH_Z 2.5f


#define VTYPE_CAR 0
#define VTYPE_BUS 1
#define VTYPE_TRUCK 2
#define NUM_VTYPES 3


#define CAR_SCALE 0.7f
#define BUS_SCALE 0.9f
#define TRUCK_SCALE 0.9f

int lanePatterns[][NUM_LANES] = {
    {1, 0, 1},
    {0, 1, 0},
    {1, 1, 0},
    {0, 1, 1},
    {1, 0, 0},
    {0, 0, 1}
};
static float nextSpawnZ = OBS_SPAWN_Z;

#define NUM_PATTERNS 6


#define SCENERY_SPACING 8.0f
#define SCENERY_EXTENT 220.0f

#define HOUSE_EVERY 2


#define NUM_CLOUDS 50
#define CLOUD_Y 28.0f
#define CLOUD_DRIFT 0.02f
#define CLOUD_EXTENT 120.0f


#define SUN_RADIUS 45.0f
#define SUN_SCREEN_X 0.82f
#define SUN_SCREEN_Y 0.78f
float sunY = 80.0f;


#define INITIAL_SPEED 0.4f
#define TIMER_MS 16
#define BLINK_DURATION 40


#define DASH_SPACING 15.0f
#define ROAD_EXTENT 200.0f


#define HUD_SCORE_Y_OFFSET 160
#define HUD_HEALTH_Y_OFFSET 190

float steeringAngle = 0.0f;
float targetSteeringAngle = 0.0f;





typedef struct {
    float x, z;
    int passed;
    int vtype;
} Obstacle;

typedef struct {
    float x, z;
    float drift;
} Cloud;

typedef struct {
    float playerX, targetX;
    float roadOffset;
    float gameSpeed;
    int score, health;
    int gameStarted, gameOver;
    int blink, blinkTimer;
    int invincible;
    float screenShake;
    int winWidth, winHeight;
} GameState;





static GameState gs;
static Obstacle obstacles[NUM_OBSTACLES];
static Cloud clouds[NUM_CLOUDS];


GLuint DL_CAR, DL_BUS, DL_TRUCK, DL_TREE, DL_HOUSE, DL_CLOUD, DL_SUN, DL_MOUNTAINS;
# 196 "test.c"
static void bresenhamLineFloat(float x0, float y0, float x1, float y1) {
    int GRID = 1000;
    int ix0 = (int)(x0 * GRID), iy0 = (int)(y0 * GRID);
    int ix1 = (int)(x1 * GRID), iy1 = (int)(y1 * GRID);

    int dx = abs(ix1 - ix0), sx = ix0 < ix1 ? 1 : -1;
    int dy = abs(iy1 - iy0), sy = iy0 < iy1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;

    glBegin(GL_POINTS);
    for (;;) {
        glVertex2f((float)ix0 / GRID, (float)iy0 / GRID);
        if (ix0 == ix1 && iy0 == iy1) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; ix0 += sx; }
        if (e2 < dy) { err += dx; iy0 += sy; }
    }
    glEnd();
}


static void bresenhamCircleFloat(float cx, float cy, float r) {
    int GRID = 1000;
    int xc = (int)(cx * GRID), yc = (int)(cy * GRID);
    int r_int = (int)(r * GRID);

    int x = 0, y = r_int;
    int d = 3 - 2 * r_int;

    glBegin(GL_POINTS);
    while (y >= x) {
        glVertex2f((float)(xc + x) / GRID, (float)(yc + y) / GRID);
        glVertex2f((float)(xc - x) / GRID, (float)(yc + y) / GRID);
        glVertex2f((float)(xc + x) / GRID, (float)(yc - y) / GRID);
        glVertex2f((float)(xc - x) / GRID, (float)(yc - y) / GRID);
        glVertex2f((float)(xc + y) / GRID, (float)(yc + x) / GRID);
        glVertex2f((float)(xc - y) / GRID, (float)(yc + x) / GRID);
        glVertex2f((float)(xc + y) / GRID, (float)(yc - x) / GRID);
        glVertex2f((float)(xc - y) / GRID, (float)(yc - x) / GRID);

        x++;
        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }
    }
    glEnd();
}

static void drawBoxRaw(float x1, float y1, float z1,
                       float x2, float y2, float z2) {
    glBegin(GL_QUADS);

    glVertex3f(x1,y1,z2); glVertex3f(x2,y1,z2);
    glVertex3f(x2,y2,z2); glVertex3f(x1,y2,z2);

    glVertex3f(x1,y1,z1); glVertex3f(x1,y2,z1);
    glVertex3f(x2,y2,z1); glVertex3f(x2,y1,z1);

    glVertex3f(x1,y2,z1); glVertex3f(x1,y2,z2);
    glVertex3f(x2,y2,z2); glVertex3f(x2,y2,z1);

    glVertex3f(x1,y1,z1); glVertex3f(x2,y1,z1);
    glVertex3f(x2,y1,z2); glVertex3f(x1,y1,z2);

    glVertex3f(x2,y1,z1); glVertex3f(x2,y2,z1);
    glVertex3f(x2,y2,z2); glVertex3f(x2,y1,z2);

    glVertex3f(x1,y1,z1); glVertex3f(x1,y1,z2);
    glVertex3f(x1,y2,z2); glVertex3f(x1,y2,z1);
    glEnd();
}


static void drawBox(float w, float h, float d) {
    glPushMatrix();
        glScalef(w, h, d);
        glutSolidCube(1.0f);
    glPopMatrix();
}


static void drawWheel3D(float x, float y, float z) {
    glPushMatrix();
        glTranslatef(x, y, z);
        glColor3f(0.1f, 0.1f, 0.1f);
        GLUquadric *q = gluNewQuadric();
        gluCylinder(q, 0.5, 0.5, 0.30, 20, 1);
        gluDisk(q, 0, 0.5, 20, 1);
        glTranslatef(0, 0, 0.30f);
        gluDisk(q, 0, 0.5, 20, 1);
        gluDeleteQuadric(q);
    glPopMatrix();
}


static void drawTrunk(float base, float top, float height) {
    GLUquadric *q = gluNewQuadric();
    glPushMatrix();
        glRotatef(-90, 1, 0, 0);
        gluCylinder(q, base, top, height, 20, 1);
        glPushMatrix();
            glRotatef(180, 1, 0, 0);
            gluDisk(q, 0, base, 20, 1);
        glPopMatrix();
    gluDeleteQuadric(q);
    glPopMatrix();
}


static void begin2D(int w, int h) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
        glLoadIdentity();
        gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void end2D(void) {
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static void drawText(float x, float y, const char *str) {
    glRasterPos2f(x, y);
    for (const char *c = str; *c != '\0'; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
}







static void drawCar(void) {

    glColor3f(0.15f, 0.15f, 0.15f);
    drawBoxRaw(-2.6f, 0.15f, -0.95f, 2.5f, 0.30f, 0.95f);






    glColor3f(0.2f, 0.2f, 0.2f);
    drawBoxRaw(-2.61f, 0.30f, -1.01f, 2.51f, 0.55f, 1.01f);


    glColor3f(0.5f, 0.1f, 0.1f);
    drawBoxRaw(-2.6f, 0.55f, -1.0f, 2.5f, 1.15f, 1.0f);


    glColor3f(0.8f, 0.8f, 0.85f);
    drawBoxRaw(-2.62f, 1.15f, -1.02f, 2.52f, 1.25f, 1.02f);


    glColor3f(0.15f, 0.15f, 0.15f);
    drawBoxRaw(-2.5f, 1.25f, -0.9f, 0.6f, 2.15f, 0.9f);




    glColor3f(0.05f, 0.05f, 0.05f);
    drawBoxRaw(-2.2f, 2.15f, 0.6f, 0.3f, 2.3f, 0.7f);
    drawBoxRaw(-2.2f, 2.15f, -0.7f, 0.3f, 2.3f, -0.6f);


    glColor3f(0.7f, 0.7f, 0.7f);
    drawBoxRaw(-1.2f, 0.2f, 0.95f, 1.2f, 0.25f, 1.15f);
    drawBoxRaw(-1.2f, 0.2f, -1.15f, 1.2f, 0.25f, -0.95f);


    glColor3f(0.05f, 0.05f, 0.05f);
    drawBoxRaw( 2.45f, 0.35f, -0.6f, 2.55f, 1.05f, 0.6f);


    glColor3f(0.3f, 0.3f, 0.3f);
    drawBoxRaw( 2.52f, 0.2f, -0.4f, 2.65f, 0.8f, -0.3f);
    drawBoxRaw( 2.52f, 0.2f, 0.3f, 2.65f, 0.8f, 0.4f);
    drawBoxRaw( 2.6f, 0.5f, -0.4f, 2.65f, 0.6f, 0.4f);


    glColor3f(0.4f, 0.6f, 0.8f);

    drawBoxRaw( 0.55f, 1.35f, -0.85f, 0.61f, 2.05f, 0.85f);
    drawBoxRaw(-2.51f, 1.35f, -0.85f, -2.45f, 2.05f, 0.85f);


    drawBoxRaw(-2.4f, 1.35f, -0.91f, -1.5f, 2.05f, -0.91f);
    drawBoxRaw(-1.4f, 1.35f, -0.91f, -0.5f, 2.05f, -0.91f);
    drawBoxRaw(-0.4f, 1.35f, -0.91f, 0.5f, 2.05f, -0.91f);


    drawBoxRaw(-2.4f, 1.35f, 0.91f, -1.5f, 2.05f, 0.91f);
    drawBoxRaw(-1.4f, 1.35f, 0.91f, -0.5f, 2.05f, 0.91f);
    drawBoxRaw(-0.4f, 1.35f, 0.91f, 0.5f, 2.05f, 0.91f);


    glColor3f(0.8f, 0.8f, 0.8f);
    drawBoxRaw( 0.1f, 1.05f, 1.0f, 0.3f, 1.1f, 1.05f);
    drawBoxRaw(-0.9f, 1.05f, 1.0f, -0.7f, 1.1f, 1.05f);
    drawBoxRaw( 0.1f, 1.05f, -1.05f, 0.3f, 1.1f, -1.0f);
    drawBoxRaw(-0.9f, 1.05f, -1.05f, -0.7f, 1.1f, -1.0f);


    glColor3f(0.9f, 1.0f, 1.0f);
    drawBoxRaw( 2.45f, 0.75f, 0.5f, 2.55f, 1.05f, 0.9f);
    drawBoxRaw( 2.45f, 0.75f, -0.9f, 2.55f, 1.05f, -0.5f);

    glColor3f(1.0f, 0.2f, 0.2f);
    drawBoxRaw(-2.65f, 0.75f, 0.6f, -2.55f, 1.15f, 0.9f);
    drawBoxRaw(-2.65f, 0.75f, -0.9f, -2.55f, 1.15f, -0.6f);



    glColor3f(0.5f, 0.1f, 0.1f);
    drawBoxRaw( 0.35f, 1.25f, 1.0f, 0.5f, 1.45f, 1.2f);
    drawBoxRaw( 0.35f, 1.25f, -1.2f, 0.5f, 1.45f, -1.0f);

    glColor3f(0.5f, 0.7f, 0.9f);
    drawBoxRaw( 0.30f, 1.28f, 1.02f, 0.35f, 1.42f, 1.18f);
    drawBoxRaw( 0.30f, 1.28f, -1.18f, 0.35f, 1.42f, -1.02f);


    glColor3f(0.9f, 0.9f, 0.9f);
    drawBoxRaw( 2.51f, 0.2f, -0.2f, 2.52f, 0.3f, 0.2f);
    drawBoxRaw(-2.61f, 0.35f, -0.2f, -2.60f, 0.45f, 0.2f);


    drawWheel3D( 1.5f, 0.2f, 0.75f);
    drawWheel3D(-1.5f, 0.2f, 0.75f);
    drawWheel3D( 1.5f, 0.2f, -1.05f);
    drawWheel3D(-1.5f, 0.2f, -1.05f);
}


static void drawBus(void) {

    glColor3f(0.15f, 0.15f, 0.15f);
    drawBoxRaw(-5.1f, 0.15f, -1.15f, 5.1f, 0.5f, 1.15f);


    glColor3f(0.9f, 0.9f, 0.9f);
    drawBoxRaw(-5.0f, 0.5f, -1.2f, 5.0f, 3.0f, 1.2f);


    glColor3f(0.7f, 0.7f, 0.7f);
    drawBoxRaw(-2.0f, 3.0f, -0.8f, 1.5f, 3.3f, 0.8f);
    drawBoxRaw(-4.0f, 3.0f, -0.6f, -3.0f, 3.15f, 0.6f);


    glColor3f(0.1f, 0.4f, 0.8f);
    drawBoxRaw(-5.02f, 0.8f, -1.22f, 5.02f, 1.1f, 1.22f);


    glColor3f(0.1f, 0.1f, 0.15f);


    drawBoxRaw( 5.01f, 1.2f, -1.1f, 5.01f, 2.9f, 1.1f);

    drawBoxRaw(-5.01f, 1.5f, -1.0f, -5.01f, 2.5f, 1.0f);


    for (float i = -4.5f; i < 3.5f; i += 1.8f) {
        drawBoxRaw(i, 1.3f, 1.21f, i+1.6f, 2.7f, 1.21f);
        drawBoxRaw(i, 1.3f, -1.21f, i+1.6f, 2.7f, -1.21f);
    }


    glColor3f(0.05f, 0.05f, 0.1f);
    drawBoxRaw( 3.2f, 0.5f, -1.23f, 4.5f, 2.7f, -1.23f);
    drawBoxRaw(-1.0f, 0.5f, -1.23f, 0.5f, 2.7f, -1.23f);


    glColor3f(1.0f, 0.6f, 0.0f);
    drawBoxRaw( 5.02f, 2.6f, -0.8f, 5.02f, 2.85f, 0.8f);



    glColor3f(0.1f, 0.1f, 0.1f);
    drawBoxRaw(4.8f, 2.8f, 1.2f, 5.1f, 2.9f, 1.7f);
    drawBoxRaw(4.8f, 2.8f, -1.7f, 5.1f, 2.9f, -1.2f);

    glColor3f(0.3f, 0.3f, 0.3f);
    drawBoxRaw(4.9f, 1.9f, 1.6f, 5.1f, 2.9f, 1.8f);
    drawBoxRaw(4.9f, 1.9f, -1.8f, 5.1f, 2.9f, -1.6f);

    glColor3f(0.8f, 0.95f, 1.0f);
    drawBoxRaw(4.88f, 1.95f, 1.62f, 4.9f, 2.85f, 1.78f);
    drawBoxRaw(4.88f, 1.95f, -1.78f, 4.9f, 2.85f, -1.62f);


    glColor3f(0.9f, 1.0f, 1.0f);
    drawBoxRaw(5.01f, 0.55f, 0.6f, 5.11f, 0.75f, 1.0f);
    drawBoxRaw(5.01f, 0.55f, -1.0f, 5.11f, 0.75f, -0.6f);

    glColor3f(1.0f, 0.2f, 0.2f);
    drawBoxRaw(-5.11f, 0.6f, 0.6f, -5.01f, 1.2f, 1.0f);
    drawBoxRaw(-5.11f, 0.6f, -1.0f, -5.01f, 1.2f, -0.6f);


    drawWheel3D( 3.5f, 0.2f, 1.0f);
    drawWheel3D(-2.5f, 0.2f, 1.0f);
    drawWheel3D(-3.8f, 0.2f, 1.0f);
    drawWheel3D( 3.5f, 0.2f, -1.25f);
    drawWheel3D(-2.5f, 0.2f, -1.25f);
    drawWheel3D(-3.8f, 0.2f, -1.25f);
}

static void drawTruck(void) {

    glColor3f(0.15f, 0.15f, 0.15f);
    drawBoxRaw(-5.0f, 0.2f, -1.2f, 5.0f, 0.6f, 1.2f);
    drawBoxRaw( 4.9f, 0.2f, -1.25f, 5.2f, 0.65f, 1.25f);



    glColor3f(0.75f, 0.75f, 0.75f);
    drawBoxRaw(-1.0f, 0.1f, 1.2f, 1.5f, 0.5f, 1.4f);
    drawBoxRaw(-1.0f, 0.1f, -1.5f, 1.5f, 0.5f, -1.2f);


    glColor3f(0.9f, 0.7f, 0.1f);
    drawBoxRaw( 2.0f, 0.6f, -1.2f, 5.0f, 2.8f, 1.2f);



    drawBoxRaw( 3.8f, 2.8f, -1.2f, 4.8f, 3.1f, 1.2f);
    drawBoxRaw( 2.0f, 2.8f, -1.2f, 3.8f, 3.5f, 1.2f);


    glColor3f(0.8f, 0.6f, 0.0f);
    drawBoxRaw( 5.0f, 2.6f, -1.25f, 5.15f, 2.85f, 1.25f);


    glColor3f(0.7f, 0.7f, 0.7f);
    drawBoxRaw( 5.01f, 0.8f, -0.7f, 5.01f, 1.8f, 0.7f);
    glColor3f(0.05f, 0.05f, 0.05f);
    drawBoxRaw( 5.02f, 0.9f, -0.6f, 5.02f, 1.7f, 0.6f);


    glColor3f(0.4f, 0.4f, 0.4f);
    drawBoxRaw(-5.0f, 0.6f, -1.2f, 2.0f, 0.8f, 1.2f);
    drawBoxRaw(-5.0f, 0.8f, 1.1f, 2.0f, 1.5f, 1.2f);
    drawBoxRaw(-5.0f, 0.8f, -1.2f, 2.0f, 1.5f, -1.1f);
    drawBoxRaw( 1.9f, 0.8f, -1.2f, 2.0f, 1.6f, 1.2f);
    drawBoxRaw(-5.0f, 0.8f, -1.2f, -4.9f, 1.5f, 1.2f);


    glColor3f(0.45f, 0.3f, 0.15f);
    drawBoxRaw(-4.5f, 0.8f, -0.8f, -2.5f, 2.1f, 0.8f);
    drawBoxRaw(-2.0f, 0.8f, -0.9f, 0.5f, 1.6f, 0.9f);


    glColor3f(0.8f, 0.8f, 0.8f);
    drawBoxRaw( 1.6f, 0.6f, 1.21f, 1.8f, 3.8f, 1.4f);
    drawBoxRaw( 1.6f, 0.6f, -1.4f, 1.8f, 3.8f, -1.21f);


    glColor3f(0.1f, 0.15f, 0.2f);
    drawBoxRaw( 5.01f, 1.9f, -1.1f, 5.01f, 2.6f, 1.1f);
    drawBoxRaw( 3.0f, 1.8f, 1.21f, 4.5f, 2.6f, 1.21f);
    drawBoxRaw( 3.0f, 1.8f, -1.21f, 4.5f, 2.6f, -1.21f);
    drawBoxRaw( 2.01f, 1.8f, -0.8f, 2.01f, 2.5f, 0.8f);


    glColor3f(0.1f, 0.1f, 0.1f);
    drawBoxRaw(4.5f, 2.5f, 1.2f, 5.4f, 2.6f, 1.6f);
    drawBoxRaw(4.5f, 2.5f, -1.6f, 5.4f, 2.6f, -1.2f);
    glColor3f(0.9f, 0.7f, 0.1f);
    drawBoxRaw(5.2f, 1.8f, 1.5f, 5.4f, 2.6f, 1.9f);
    drawBoxRaw(5.2f, 1.8f, -1.9f, 5.4f, 2.6f, -1.5f);
    glColor3f(0.8f, 0.95f, 1.0f);
    drawBoxRaw(5.18f, 1.85f, 1.52f, 5.2f, 2.55f, 1.88f);
    drawBoxRaw(5.18f, 1.85f, -1.88f, 5.2f, 2.55f, -1.52f);


    glColor3f(0.9f, 1.0f, 1.0f);
    drawBoxRaw( 5.21f, 0.75f, 0.7f, 5.22f, 1.0f, 1.1f);
    drawBoxRaw( 5.21f, 0.75f, -1.1f, 5.22f, 1.0f, -0.7f);

    glColor3f(1.0f, 0.2f, 0.2f);
    drawBoxRaw(-5.01f, 0.35f, 0.7f, -5.0f, 0.65f, 1.1f);
    drawBoxRaw(-5.01f, 0.35f, -1.1f, -5.0f, 0.65f, -0.7f);


    glColor3f(0.05f, 0.05f, 0.05f);
    drawBoxRaw(-4.7f, 0.1f, 0.8f, -4.6f, 0.6f, 1.2f);
    drawBoxRaw(-4.7f, 0.1f, -1.3f, -4.6f, 0.6f, -0.9f);


    drawWheel3D( 3.5f, 0.2f, 1.0f);
    drawWheel3D(-2.5f, 0.2f, 1.0f);
    drawWheel3D(-3.8f, 0.2f, 1.0f);
    drawWheel3D( 3.5f, 0.2f, -1.30f);
    drawWheel3D(-2.5f, 0.2f, -1.30f);
    drawWheel3D(-3.8f, 0.2f, -1.30f);
}





void buildSunGeometry() {
    int i;
    float angle;
    int segments = 64;


    glBegin(GL_TRIANGLE_FAN);
        glColor4f(1.0f, 0.7f, 0.2f, 0.8f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glColor4f(0.8f, 0.1f, 0.0f, 0.0f);
        for (i = 0; i <= segments; i++) {
            angle = i * 2.0f * PI / segments;
            glVertex3f(cos(angle) * 120.0f, sin(angle) * 75.0f, 0.0f);
        }
    glEnd();


    glBegin(GL_TRIANGLE_FAN);
        glColor4f(1.0f, 0.6f, 0.1f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.01f);
        glColor4f(0.9f, 0.2f, 0.0f, 0.9f);
        for (i = 0; i <= segments; i++) {
            angle = i * 2.0f * PI / segments;
            glVertex3f(cos(angle) * 35.0f, sin(angle) * 22.0f, 0.01f);
        }
    glEnd();
}

void buildMountainsGeometry() {
    static const float px[] = {
        -1000,-800,-600,-450,-300,-200,-100, 0, 100, 200, 300, 450, 600, 800,1000,
         -900,-700,-520,-380,-250,-120, -60, 0, 60, 120, 250, 380, 520, 700, 900
    };
    static const float py[] = {
        -20,-15,-10, -5, -8, -5, -5, -5, -5, -5,-10, -5,-10,-15,-20,
         60,120, 70,110, 60,100, 70, 20, 70,100,100, 80,120, 70, 60
    };
    static const float pz[] = {
        -320,-300,-280,-260,-240,-230,-220,-210,-220,-230,-240,-260,-280,-300,-320,
        -330,-310,-290,-270,-250,-240,-230,-220,-230,-240,-250,-270,-290,-310,-330
    };
    static const int ORDER[] = {
         0,15, 1,16, 2,17, 3,18, 4,19, 5,20, 6,21, 7,22,
         8,23, 9,24,10,25,11,26,12,27,13,28,14,29
    };
    static const int SUNLIT[] = {
        0,1, 0,1, 0,1, 0,1, 0,1, 0,1, 0,1, 0,0,
        0,1, 0,1, 0,1, 0,1, 0,1, 0,1, 0,1
    };
    static const int NORDER = 30;

    static const float layers[3][7] = {
        { 1.15f, 0.65f, -80.0f, 0.62f, 0.67f, 0.80f, 0.82f },
        { 1.00f, 1.00f, 0.0f, 0.48f, 0.52f, 0.64f, 0.28f },
        { 0.70f, 0.50f, 60.0f, 0.22f, 0.24f, 0.28f, 0.06f }
    };

    int li, i;
    for (li = 0; li < 3; li++) {
        float sx = layers[li][0]; float sy = layers[li][1]; float dz = layers[li][2];
        float fr = layers[li][3]; float fg = layers[li][4]; float fb = layers[li][5];
        float ft = layers[li][6]; float ft2 = ft * ft;

#define FOG(r,g,b) glColor3f((r)+(fr-(r))*ft2, (g)+(fg-(g))*ft2, (b)+(fb-(b))*ft2)

        glBegin(GL_TRIANGLE_STRIP);
        for (i = 0; i < NORDER; i++) {
            int idx = ORDER[i];
            int sun = SUNLIT[i];
            float r, g, b;
            if (sun) { r = 0.70f; g = 0.62f; b = 0.48f; }
            else { r = 0.28f; g = 0.30f; b = 0.40f; }
            FOG(r, g, b);
            glVertex3f(px[idx]*sx, py[idx]*sy, pz[idx]+dz);
        }
        glEnd();
#undef FOG
    }
}


static void drawVehicle(int vtype) {
    switch (vtype) {
        case VTYPE_CAR: glCallList(DL_CAR); break;
        case VTYPE_BUS: glCallList(DL_BUS); break;
        case VTYPE_TRUCK: glCallList(DL_TRUCK); break;
    }
}


static float vehicleScale(int vtype) {
    switch (vtype) {
        case VTYPE_CAR: return CAR_SCALE;
        case VTYPE_BUS: return BUS_SCALE;
        case VTYPE_TRUCK: return TRUCK_SCALE;
        default: return CAR_SCALE;
    }
}





static void drawBigCloudTree(void) {




    glColor3f(0.28f, 0.13f, 0.05f);
    static const float roots[][3] = {
        { 0.55f, 0.0f, 0.20f },
        { -0.50f, 0.0f, -0.25f },
        { 0.15f, 0.0f, 0.60f },
        { -0.20f, 0.0f, -0.58f },
        { 0.48f, 0.0f, -0.38f },
    };
    for (int i = 0; i < 5; i++) {
        glPushMatrix();
            glTranslatef(roots[i][0], roots[i][1], roots[i][2]);
            glScalef(0.55f, 0.22f, 0.45f);
            glutSolidSphere(0.8f, 10, 6);
        glPopMatrix();
    }




    glColor3f(0.30f, 0.14f, 0.05f);
    drawTrunk(0.52f, 0.28f, 4.2f);


    glColor3f(0.20f, 0.09f, 0.03f);
    glPushMatrix();
        glTranslatef(0.18f, 0.0f, 0.0f);
        drawTrunk(0.22f, 0.10f, 4.0f);
    glPopMatrix();


    glColor3f(0.48f, 0.26f, 0.11f);
    glPushMatrix();
        glTranslatef(-0.15f, 0.0f, 0.10f);
        drawTrunk(0.18f, 0.08f, 3.6f);
    glPopMatrix();





    glColor3f(0.05f, 0.30f, 0.07f);
    static const float skirt[][4] = {
        { 0.0f, 3.6f, 0.0f, 2.60f },
        { 2.2f, 3.2f, 0.8f, 1.50f },
        { -2.2f, 3.2f, -0.8f, 1.50f },
        { 1.0f, 3.0f, -2.0f, 1.40f },
        { -1.0f, 3.0f, 2.0f, 1.40f },
        { 2.4f, 3.4f, -1.0f, 1.20f },
        { -2.4f, 3.4f, 1.0f, 1.20f },
    };
    for (int i = 0; i < 7; i++) {
        glPushMatrix();
            glTranslatef(skirt[i][0], skirt[i][1] + 0.10f, skirt[i][2]);
            glutSolidSphere(skirt[i][3], 20, 16);
        glPopMatrix();
    }





    glColor3f(0.08f, 0.40f, 0.10f);
    static const float mid[][4] = {
        { 0.0f, 5.0f, 0.0f, 2.80f },
        { 1.8f, 4.5f, 1.2f, 1.80f },
        { -1.8f, 4.5f, -1.2f, 1.80f },
        { 1.5f, 4.8f, -1.6f, 1.70f },
        { -1.5f, 4.8f, 1.6f, 1.70f },
        { 2.3f, 5.0f, 0.0f, 1.55f },
        { -2.3f, 5.0f, 0.0f, 1.55f },
        { 0.0f, 4.4f, 2.2f, 1.60f },
        { 0.0f, 4.4f, -2.2f, 1.60f },
        { 1.0f, 5.5f, 1.0f, 1.40f },
        { -1.0f, 5.5f, -1.0f, 1.40f },
    };
    for (int i = 0; i < 11; i++) {
        glPushMatrix();
            glTranslatef(mid[i][0], mid[i][1]+ 0.10f, mid[i][2]);
            glutSolidSphere(mid[i][3], 22, 18);
        glPopMatrix();
    }




    glColor3f(0.16f, 0.56f, 0.15f);
    static const float upper[][4] = {
        { 0.0f, 6.6f, 0.0f, 1.90f },
        { 1.4f, 6.2f, 1.0f, 1.30f },
        { -1.4f, 6.2f, -1.0f, 1.30f },
        { 1.2f, 6.5f, -1.3f, 1.20f },
        { -1.2f, 6.5f, 1.3f, 1.20f },
        { 0.0f, 6.0f, 1.8f, 1.15f },
        { 0.0f, 6.0f, -1.8f, 1.15f },
        { 1.8f, 6.8f, 0.0f, 1.00f },
        { -1.8f, 6.8f, 0.0f, 1.00f },
    };
    for (int i = 0; i < 9; i++) {
        glPushMatrix();
            glTranslatef(upper[i][0], upper[i][1]+ 0.10f, upper[i][2]);
            glutSolidSphere(upper[i][3], 20, 16);
        glPopMatrix();
    }




    glColor3f(0.26f, 0.68f, 0.20f);
    static const float peak[][4] = {
        { 0.0f, 7.8f, 0.0f, 1.30f },
        { 0.9f, 7.5f, 0.7f, 0.90f },
        { -0.9f, 7.5f, -0.7f, 0.90f },
        { 0.8f, 7.8f, -0.8f, 0.80f },
        { -0.8f, 7.8f, 0.8f, 0.80f },
        { 0.0f, 8.4f, 0.0f, 0.85f },
    };
    for (int i = 0; i < 6; i++) {
        glPushMatrix();
            glTranslatef(peak[i][0], peak[i][1]+ 0.10f, peak[i][2]);
            glutSolidSphere(peak[i][3], 16, 14);
        glPopMatrix();
    }




    glColor3f(0.38f, 0.80f, 0.28f);
    glPushMatrix();
        glTranslatef(0.0f, 9.1f, 0.0f);
        glutSolidSphere(0.55f, 12, 10);
    glPopMatrix();
}

static void drawHouse(void) {




    glColor3f(0.52f, 0.48f, 0.42f);
    drawBoxRaw(-2.15f, -0.18f, -2.15f, 2.15f, 0.0f, 2.15f);




    glColor3f(0.91f, 0.88f, 0.80f);
    drawBoxRaw(-2.0f, 0.0f, -2.0f, 2.0f, 4.0f, 2.0f);


    glColor3f(0.78f, 0.72f, 0.62f);
    for (float cy = 0.22f; cy < 4.0f; cy += 0.32f) {

        drawBoxRaw(-2.01f, cy, 2.00f, 2.01f, cy+0.06f, 2.02f);

        drawBoxRaw(-2.01f, cy, -2.02f, 2.01f, cy+0.06f, -2.00f);

        drawBoxRaw(-2.02f, cy, -2.00f, -2.00f, cy+0.06f, 2.00f);

        drawBoxRaw( 2.00f, cy, -2.00f, 2.02f, cy+0.06f, 2.00f);
    }


    glColor3f(0.82f, 0.76f, 0.66f);
    for (float qy = 0.0f; qy < 4.0f; qy += 0.48f) {
        drawBoxRaw(-2.04f, qy, 1.80f, -1.80f, qy+0.22f, 2.04f);
        drawBoxRaw(-2.04f, qy, -2.04f, -1.80f, qy+0.22f, -1.80f);
        drawBoxRaw( 1.80f, qy, 1.80f, 2.04f, qy+0.22f, 2.04f);
        drawBoxRaw( 1.80f, qy, -2.04f, 2.04f, qy+0.22f, -1.80f);
    }






    glColor3f(0.88f, 0.85f, 0.78f);
    glBegin(GL_QUADS);

        glVertex3f(-2.45f, 4.0f, 2.45f);
        glVertex3f( 2.45f, 4.0f, 2.45f);
        glVertex3f( 2.00f, 4.0f, 2.00f);
        glVertex3f(-2.00f, 4.0f, 2.00f);

        glVertex3f(-2.45f, 4.0f, -2.45f);
        glVertex3f(-2.00f, 4.0f, -2.00f);
        glVertex3f( 2.00f, 4.0f, -2.00f);
        glVertex3f( 2.45f, 4.0f, -2.45f);

        glVertex3f(-2.45f, 4.0f, -2.45f);
        glVertex3f(-2.45f, 4.0f, 2.45f);
        glVertex3f(-2.00f, 4.0f, 2.00f);
        glVertex3f(-2.00f, 4.0f, -2.00f);

        glVertex3f( 2.45f, 4.0f, -2.45f);
        glVertex3f( 2.00f, 4.0f, -2.00f);
        glVertex3f( 2.00f, 4.0f, 2.00f);
        glVertex3f( 2.45f, 4.0f, 2.45f);
    glEnd();


    glColor3f(0.30f, 0.28f, 0.25f);
    drawBoxRaw(-2.46f, 3.82f, -2.46f, 2.46f, 4.02f, -2.44f);
    drawBoxRaw(-2.46f, 3.82f, 2.44f, 2.46f, 4.02f, 2.46f);
    drawBoxRaw(-2.46f, 3.82f, -2.46f, -2.44f, 4.02f, 2.46f);
    drawBoxRaw( 2.44f, 3.82f, -2.46f, 2.46f, 4.02f, 2.46f);


    glColor3f(0.58f, 0.18f, 0.10f);
    glBegin(GL_TRIANGLES);

        glVertex3f(-2.45f, 4.0f, 2.45f);
        glVertex3f( 2.45f, 4.0f, 2.45f);
        glVertex3f( 0.00f, 6.4f, 0.00f);

        glVertex3f(-2.45f, 4.0f, -2.45f);
        glVertex3f( 0.00f, 6.4f, 0.00f);
        glVertex3f( 2.45f, 4.0f, -2.45f);

        glVertex3f(-2.45f, 4.0f, 2.45f);
        glVertex3f( 0.00f, 6.4f, 0.00f);
        glVertex3f(-2.45f, 4.0f, -2.45f);

        glVertex3f( 2.45f, 4.0f, 2.45f);
        glVertex3f( 2.45f, 4.0f, -2.45f);
        glVertex3f( 0.00f, 6.4f, 0.00f);
    glEnd();


    glColor3f(0.42f, 0.12f, 0.06f);
    for (float t = 0.12f; t < 1.0f; t += 0.12f) {

        float y1 = 4.0f + t * (6.4f - 4.0f);
        float y2 = 4.0f + (t+0.025f) * (6.4f - 4.0f);
        float z1 = 2.45f - t * 2.45f;
        float z2 = 2.45f - (t+0.025f) * 2.45f;
        float x1 = 2.45f - t * 2.45f;
        float x2 = 2.45f - (t+0.025f) * 2.45f;


        glBegin(GL_QUADS);
            glVertex3f(-x1, y1, z1);
            glVertex3f( x1, y1, z1);
            glVertex3f( x2, y2, z2);
            glVertex3f(-x2, y2, z2);
        glEnd();

        glBegin(GL_QUADS);
            glVertex3f(-x1, y1, -z1);
            glVertex3f(-x2, y2, -z2);
            glVertex3f( x2, y2, -z2);
            glVertex3f( x1, y1, -z1);
        glEnd();
    }


    glColor3f(0.35f, 0.10f, 0.05f);
    drawBoxRaw(-0.18f, 6.28f, -0.40f, 0.18f, 6.50f, 0.40f);





    glColor3f(0.68f, 0.30f, 0.16f);
    drawBoxRaw( 1.05f, 4.20f, -0.70f, 1.60f, 7.20f, -0.15f);


    glColor3f(0.52f, 0.22f, 0.10f);
    for (float cy = 4.40f; cy < 7.20f; cy += 0.30f)
        drawBoxRaw( 1.04f, cy, -0.71f, 1.61f, cy+0.06f, -0.14f);


    glColor3f(0.42f, 0.42f, 0.45f);
    drawBoxRaw( 1.02f, 5.40f, -0.72f, 1.62f, 5.56f, -0.13f);


    glColor3f(0.28f, 0.12f, 0.06f);
    drawBoxRaw( 0.98f, 7.18f, -0.74f, 1.64f, 7.30f, -0.11f);


    glColor3f(0.58f, 0.26f, 0.14f);
    drawBoxRaw( 1.18f, 7.30f, -0.55f, 1.46f, 7.75f, -0.30f);





    glColor3f(0.58f, 0.54f, 0.48f);
    drawBoxRaw(-0.90f, -0.02f, 2.00f, 0.90f, 0.06f, 3.10f);


    glColor3f(0.62f, 0.58f, 0.52f);
    drawBoxRaw(-0.80f, -0.10f, 3.00f, 0.80f, 0.00f, 3.40f);
    glColor3f(0.66f, 0.62f, 0.56f);
    drawBoxRaw(-0.70f, -0.20f, 3.30f, 0.70f,-0.10f, 3.70f);


    glColor3f(0.92f, 0.90f, 0.85f);
    drawBoxRaw(-0.76f, 0.05f, 2.92f, -0.62f, 2.60f, 3.06f);
    drawBoxRaw( 0.62f, 0.05f, 2.92f, 0.76f, 2.60f, 3.06f);


    glColor3f(0.75f, 0.72f, 0.68f);
    drawBoxRaw(-0.82f, 0.05f, 2.88f, -0.56f, 0.18f, 3.10f);
    drawBoxRaw( 0.56f, 0.05f, 2.88f, 0.82f, 0.18f, 3.10f);
    drawBoxRaw(-0.82f, 2.48f, 2.88f, -0.56f, 2.62f, 3.10f);
    drawBoxRaw( 0.56f, 2.48f, 2.88f, 0.82f, 2.62f, 3.10f);


    glColor3f(0.35f, 0.32f, 0.28f);
    drawBoxRaw(-0.90f, 2.60f, 1.98f, 0.90f, 2.72f, 3.10f);

    glColor3f(0.25f, 0.23f, 0.20f);
    drawBoxRaw(-0.92f, 2.50f, 3.08f, 0.92f, 2.62f, 3.12f);





    glColor3f(0.88f, 0.86f, 0.80f);
    drawBoxRaw(-0.52f, 0.0f, 2.01f, 0.52f, 1.72f, 2.03f);


    glColor3f(0.32f, 0.16f, 0.06f);
    drawBoxRaw(-0.44f, 0.04f, 2.03f, 0.44f, 1.48f, 2.05f);


    glColor3f(0.40f, 0.20f, 0.08f);
    drawBoxRaw(-0.38f, 0.10f, 2.05f, 0.38f, 0.70f, 2.06f);
    drawBoxRaw(-0.38f, 0.78f, 2.05f, 0.38f, 1.42f, 2.06f);


    glColor3f(0.22f, 0.10f, 0.04f);
    drawBoxRaw(-0.36f, 0.12f, 2.06f, -0.34f, 0.68f, 2.07f);
    drawBoxRaw(-0.36f, 0.80f, 2.06f, -0.34f, 1.40f, 2.07f);


    glColor3f(0.60f, 0.85f, 1.0f);
    drawBoxRaw(-0.44f, 1.48f, 2.03f, 0.44f, 1.68f, 2.05f);

    glColor3f(0.70f, 0.68f, 0.62f);
    drawBoxRaw(-0.44f, 1.56f, 2.04f, 0.44f, 1.60f, 2.06f);


    glColor3f(0.78f, 0.62f, 0.18f);
    drawBoxRaw( 0.30f, 0.72f, 2.05f, 0.38f, 0.82f, 2.08f);


    glColor3f(0.55f, 0.42f, 0.10f);
    drawBoxRaw(-0.18f, 0.38f, 2.05f, 0.18f, 0.44f, 2.08f);






    glColor3f(0.88f, 0.86f, 0.80f);
    drawBoxRaw(-1.62f, 0.48f, 2.01f, -0.62f, 1.62f, 2.03f);

    glColor3f(0.70f, 0.68f, 0.62f);
    drawBoxRaw(-1.66f, 0.44f, 2.01f, -0.58f, 0.52f, 2.06f);

    glColor3f(0.52f, 0.78f, 1.0f);
    drawBoxRaw(-1.58f, 0.54f, 2.03f, -0.66f, 1.56f, 2.04f);

    glColor3f(0.80f, 0.78f, 0.72f);
    drawBoxRaw(-1.58f, 1.02f, 2.04f, -0.66f, 1.07f, 2.05f);
    drawBoxRaw(-1.14f, 0.54f, 2.04f, -1.10f, 1.56f, 2.05f);

    glColor3f(0.18f, 0.36f, 0.16f);
    drawBoxRaw(-1.82f, 0.48f, 2.01f, -1.64f, 1.62f, 2.02f);
    drawBoxRaw(-0.60f, 0.48f, 2.01f, -0.42f, 1.62f, 2.02f);


    glColor3f(0.88f, 0.86f, 0.80f);
    drawBoxRaw( 0.62f, 0.48f, 2.01f, 1.62f, 1.62f, 2.03f);
    glColor3f(0.70f, 0.68f, 0.62f);
    drawBoxRaw( 0.58f, 0.44f, 2.01f, 1.66f, 0.52f, 2.06f);
    glColor3f(0.52f, 0.78f, 1.0f);
    drawBoxRaw( 0.66f, 0.54f, 2.03f, 1.58f, 1.56f, 2.04f);
    glColor3f(0.80f, 0.78f, 0.72f);
    drawBoxRaw( 0.66f, 1.02f, 2.04f, 1.58f, 1.07f, 2.05f);
    drawBoxRaw( 1.10f, 0.54f, 2.04f, 1.14f, 1.56f, 2.05f);
    glColor3f(0.18f, 0.36f, 0.16f);
    drawBoxRaw( 0.42f, 0.48f, 2.01f, 0.60f, 1.62f, 2.02f);
    drawBoxRaw( 1.64f, 0.48f, 2.01f, 1.82f, 1.62f, 2.02f);





    glColor3f(0.88f, 0.86f, 0.80f);
    drawBoxRaw(-1.62f, 2.40f, 2.01f, -0.62f, 3.55f, 2.03f);
    glColor3f(0.70f, 0.68f, 0.62f);
    drawBoxRaw(-1.66f, 2.36f, 2.01f, -0.58f, 2.44f, 2.06f);
    glColor3f(0.52f, 0.78f, 1.0f);
    drawBoxRaw(-1.58f, 2.46f, 2.03f, -0.66f, 3.49f, 2.04f);
    glColor3f(0.80f, 0.78f, 0.72f);
    drawBoxRaw(-1.58f, 2.95f, 2.04f, -0.66f, 3.00f, 2.05f);
    drawBoxRaw(-1.14f, 2.46f, 2.04f, -1.10f, 3.49f, 2.05f);
    glColor3f(0.18f, 0.36f, 0.16f);
    drawBoxRaw(-1.82f, 2.40f, 2.01f, -1.64f, 3.55f, 2.02f);
    drawBoxRaw(-0.60f, 2.40f, 2.01f, -0.42f, 3.55f, 2.02f);


    glColor3f(0.88f, 0.86f, 0.80f);
    drawBoxRaw( 0.62f, 2.40f, 2.01f, 1.62f, 3.55f, 2.03f);
    glColor3f(0.70f, 0.68f, 0.62f);
    drawBoxRaw( 0.58f, 2.36f, 2.01f, 1.66f, 2.44f, 2.06f);
    glColor3f(0.52f, 0.78f, 1.0f);
    drawBoxRaw( 0.66f, 2.46f, 2.03f, 1.58f, 3.49f, 2.04f);
    glColor3f(0.80f, 0.78f, 0.72f);
    drawBoxRaw( 0.66f, 2.95f, 2.04f, 1.58f, 3.00f, 2.05f);
    drawBoxRaw( 1.10f, 2.46f, 2.04f, 1.14f, 3.49f, 2.05f);
    glColor3f(0.18f, 0.36f, 0.16f);
    drawBoxRaw( 0.42f, 2.40f, 2.01f, 0.60f, 3.55f, 2.02f);
    drawBoxRaw( 1.64f, 2.40f, 2.01f, 1.82f, 3.55f, 2.02f);


    glColor3f(0.88f, 0.86f, 0.80f);
    drawBoxRaw(-0.42f, 2.40f, 2.01f, 0.42f, 3.55f, 2.03f);
    glColor3f(0.70f, 0.68f, 0.62f);
    drawBoxRaw(-0.46f, 2.36f, 2.01f, 0.46f, 2.44f, 2.06f);
    glColor3f(0.52f, 0.78f, 1.0f);
    drawBoxRaw(-0.36f, 2.46f, 2.03f, 0.36f, 3.49f, 2.04f);

    glColor3f(0.52f, 0.78f, 1.0f);
    drawBoxRaw(-0.28f, 3.30f, 2.03f, 0.28f, 3.49f, 2.04f);
    glColor3f(0.80f, 0.78f, 0.72f);
    drawBoxRaw(-0.36f, 2.95f, 2.04f, 0.36f, 3.00f, 2.05f);




    glColor3f(0.88f, 0.86f, 0.80f);
    drawBoxRaw( 2.01f, 1.20f, -0.60f, 2.03f, 2.40f, 0.60f);
    glColor3f(0.70f, 0.68f, 0.62f);
    drawBoxRaw( 2.01f, 1.16f, -0.64f, 2.06f, 1.24f, 0.64f);
    glColor3f(0.52f, 0.78f, 1.0f);
    drawBoxRaw( 2.03f, 1.26f, -0.54f, 2.04f, 2.34f, 0.54f);
    glColor3f(0.80f, 0.78f, 0.72f);
    drawBoxRaw( 2.03f, 1.80f, -0.54f, 2.05f, 1.84f, 0.54f);
    drawBoxRaw( 2.03f, 1.26f, -0.02f, 2.05f, 2.34f, 0.02f);





    glColor3f(0.68f, 0.64f, 0.56f);
    drawBoxRaw(-0.30f, -0.16f, 2.00f, 0.30f, -0.10f, 4.20f);

    glColor3f(0.55f, 0.52f, 0.46f);
    for (float pz = 2.40f; pz < 4.20f; pz += 0.55f)
        drawBoxRaw(-0.30f, -0.10f, pz, 0.30f, -0.08f, pz+0.04f);


    glColor3f(0.14f, 0.40f, 0.12f);
    glPushMatrix();
        glTranslatef(-0.75f, 0.10f, 2.80f);
        glScalef(1.0f, 0.70f, 1.0f);
        glutSolidSphere(0.40f, 14, 10);
    glPopMatrix();
    glPushMatrix();
        glTranslatef( 0.75f, 0.10f, 2.80f);
        glScalef(1.0f, 0.70f, 1.0f);
        glutSolidSphere(0.40f, 14, 10);
    glPopMatrix();

    glColor3f(0.10f, 0.30f, 0.09f);
    drawBoxRaw(-1.05f, -0.10f, 2.50f, -0.45f, 0.10f, 3.10f);
    drawBoxRaw( 0.45f, -0.10f, 2.50f, 1.05f, 0.10f, 3.10f);




    glColor3f(0.30f, 0.30f, 0.32f);

    drawBoxRaw(-2.48f, 3.82f, 2.44f, 2.48f, 3.92f, 2.50f);

    drawBoxRaw( 1.88f, 0.0f, 2.01f, 1.96f, 3.84f, 2.09f);

    drawBoxRaw( 1.88f, -0.10f, 2.01f, 1.96f, 0.08f, 2.30f);




    glColor3f(0.65f, 0.55f, 0.20f);
    drawBoxRaw(-0.06f, 2.12f, 2.02f, 0.06f, 2.28f, 2.14f);
    glColor3f(1.0f, 0.95f, 0.70f);
    drawBoxRaw(-0.04f, 2.00f, 2.14f, 0.04f, 2.12f, 2.20f);
}

static void drawCloud(void) {
    glColor3f(1.0f, 1.0f, 1.0f);

    glPushMatrix();
        glScalef(1.5f, 0.8f, 1.0f);
        glutSolidSphere(1.0, 20, 20);
    glPopMatrix();

    glPushMatrix();
        glTranslatef(-1.2f, -0.2f, 0.0f);
        glScalef(1.2f, 0.6f, 0.8f);
        glutSolidSphere(0.8, 20, 20);
    glPopMatrix();

    glPushMatrix();
        glTranslatef(1.2f, -0.2f, 0.0f);
        glScalef(1.2f, 0.6f, 0.8f);
        glutSolidSphere(0.8, 20, 20);
    glPopMatrix();

    glPushMatrix();
        glTranslatef(0.0f, 0.5f, 0.0f);
        glScalef(1.0f, 0.7f, 0.8f);
        glutSolidSphere(0.7, 20, 20);
    glPopMatrix();
}






static void drawFilledCircle2D(float cx, float cy, float r, int segments) {
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(0.02f, 0.02f, 0.02f);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * PI * (float)i / (float)segments;
        glVertex2f(cx + r * cosf(theta), cy + r * sinf(theta));
    }
    glEnd();
}



static void drawVent(float x, float y) {

    glColor3f(0.05f, 0.05f, 0.05f);
    glBegin(GL_QUADS);
        glVertex2f(x - 0.06f, y - 0.06f); glVertex2f(x + 0.06f, y - 0.06f);
        glVertex2f(x + 0.06f, y + 0.06f); glVertex2f(x - 0.06f, y + 0.06f);
    glEnd();


    glColor3f(0.3f, 0.3f, 0.3f);
    glLineWidth(2.0f);
    for(float vy = y - 0.04f; vy <= y + 0.04f; vy += 0.04f) {
        glBegin(GL_LINES);
            glVertex2f(x - 0.05f, vy); glVertex2f(x + 0.05f, vy);
        glEnd();
    }
    glLineWidth(1.0f);
}


static void drawSpoke(float cx, float cy, float angle, float innerW, float outerW, float length) {
    float cosA = cosf(angle), sinA = sinf(angle);
    float cosA90 = cosf(angle + PI / 2.0f), sinA90 = sinf(angle + PI / 2.0f);
    glBegin(GL_POLYGON);
        glColor3f(0.4f, 0.4f, 0.4f);
        glVertex2f(cx + (innerW/2.0f)*cosA90, cy + (innerW/2.0f)*sinA90);
        glVertex2f(cx - (innerW/2.0f)*cosA90, cy - (innerW/2.0f)*sinA90);
        glColor3f(0.7f, 0.7f, 0.7f);
        glVertex2f(cx + length*cosA - (outerW/2.0f)*cosA90, cy + length*sinA - (outerW/2.0f)*sinA90);
        glVertex2f(cx + length*cosA + (outerW/2.0f)*cosA90, cy + length*sinA + (outerW/2.0f)*sinA90);
    glEnd();
}

static void drawDashboard(void) {


    glDisable(GL_DEPTH_TEST);
    glLoadIdentity();






    glColor3f(0.08f, 0.08f, 0.08f);
    glBegin(GL_QUADS);
        glVertex2f(-0.015f, 1.0f); glVertex2f(0.015f, 1.0f);
        glVertex2f(0.01f, 0.86f); glVertex2f(-0.01f, 0.86f);
    glEnd();


    glColor3f(0.04f, 0.04f, 0.05f);
    glBegin(GL_POLYGON);
        glVertex2f(-0.26f, 0.89f); glVertex2f(0.26f, 0.89f);
        glVertex2f(0.24f, 0.71f); glVertex2f(-0.24f, 0.71f);
    glEnd();


    glBegin(GL_POLYGON);

        glColor3f(0.3f, 0.5f, 0.8f);
        glVertex2f(-0.24f, 0.86f); glVertex2f(0.24f, 0.86f);

        glColor3f(0.15f, 0.15f, 0.2f);
        glVertex2f(0.22f, 0.74f); glVertex2f(-0.22f, 0.74f);
    glEnd();


    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
        glColor3f(0.0f, 0.0f, 0.0f);
        glVertex2f(-0.24f, 0.86f); glVertex2f(0.24f, 0.86f);
        glVertex2f(0.22f, 0.74f); glVertex2f(-0.22f, 0.74f);
    glEnd();


    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_TRIANGLES);
        glColor4f(1.0f, 1.0f, 1.0f, 0.3f);
        glVertex2f(-0.23f, 0.85f);
        glVertex2f(-0.10f, 0.85f);
        glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
        glVertex2f(-0.23f, 0.78f);
    glEnd();
    glDisable(GL_BLEND);





    glBegin(GL_QUADS);
        glColor3f(0.1f, 0.1f, 0.12f);
        glVertex2f(-1.0f, -1.0f); glVertex2f( 1.0f, -1.0f);
        glVertex2f( 1.0f, -0.60f); glVertex2f(-1.0f, -0.60f);
    glEnd();


    glBegin(GL_QUADS);
        glColor3f(0.0f, 0.2f, 0.4f);
        glVertex2f(-1.0f, -0.60f); glVertex2f( 1.0f, -0.60f);
        glVertex2f( 0.55f, -0.35f); glVertex2f(-0.55f, -0.35f);
    glEnd();


    glBegin(GL_QUADS);
        glColor3f(0.0f, 0.6f, 1.0f);
        glVertex2f(-1.0f, -0.60f); glVertex2f( 1.0f, -0.60f);
        glVertex2f( 1.0f, -0.58f); glVertex2f(-1.0f, -0.58f);
    glEnd();




    glColor3f(0.08f, 0.08f, 0.1f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(0.4f, -0.9f); glVertex2f(0.9f, -0.9f);
        glVertex2f(0.85f, -0.65f); glVertex2f(0.45f, -0.65f);
    glEnd();
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_QUADS);
        glVertex2f(0.60f, -0.72f); glVertex2f(0.70f, -0.72f);
        glVertex2f(0.70f, -0.75f); glVertex2f(0.60f, -0.75f);
    glEnd();




    glColor3f(0.05f, 0.05f, 0.05f);
    glBegin(GL_QUADS);
        glVertex2f(-0.25f, -1.0f); glVertex2f(0.25f, -1.0f);
        glVertex2f(0.22f, -0.45f); glVertex2f(-0.22f, -0.45f);
    glEnd();

    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
        glVertex2f(-0.18f, -0.82f); glVertex2f(0.18f, -0.82f);
        glVertex2f(0.16f, -0.50f); glVertex2f(-0.16f, -0.50f);
    glEnd();


    glColor3f(0.0f, 0.15f, 0.3f);
    glBegin(GL_QUADS);
        glVertex2f(-0.16f, -0.80f); glVertex2f(0.16f, -0.80f);
        glVertex2f(0.14f, -0.52f); glVertex2f(-0.14f, -0.52f);
    glEnd();


    glColor3f(0.4f, 0.6f, 0.8f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);

        glVertex2f(-0.14f, -0.80f);
        glVertex2f(-0.06f, -0.52f);


        glVertex2f(-0.05f, -0.80f);
        glVertex2f(-0.02f, -0.52f);


        glVertex2f( 0.05f, -0.80f);
        glVertex2f( 0.02f, -0.52f);


        glVertex2f( 0.14f, -0.80f);
        glVertex2f( 0.06f, -0.52f);
    glEnd();
    glLineWidth(1.0f);


    float gpsCarX = (gs.playerX / LANE_WIDTH) * 0.09f;


    glColor3f(1.0f, 0.2f, 0.2f);
    glBegin(GL_TRIANGLES);

        glVertex2f(gpsCarX, -0.70f);
        glVertex2f(gpsCarX - 0.02f, -0.76f);
        glVertex2f(gpsCarX + 0.02f, -0.76f);
    glEnd();


    glPointSize(8.0f);
    glBegin(GL_POINTS);
        glColor3f(1.0f, 0.2f, 0.2f); glVertex2f(-0.10f, -0.90f);
        glColor3f(0.2f, 0.5f, 1.0f); glVertex2f( 0.10f, -0.90f);
    glEnd();
    glPointSize(1.0f);


    drawVent(-0.32f, -0.65f);
    drawVent( 0.32f, -0.65f);




    float wX = -0.6f, wY = -0.75f;
    float spX = wX, spY = wY + 0.1f;
    float fX = 0.65f, fY = -0.75f;

    drawFilledCircle2D(spX, spY, 0.22f, 32);
    drawFilledCircle2D(fX, fY, 0.18f, 32);


    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (int i = 0; i <= 10; i++) {
        float a = PI - (PI * i / 10.0f);
        glVertex2f(spX + 0.16f*cosf(a), spY + 0.16f*sinf(a));
        glVertex2f(spX + 0.20f*cosf(a), spY + 0.20f*sinf(a));
    }
    glEnd();
    glLineWidth(1.0f);

    float speedAngle = PI - (gs.gameSpeed * 1.2f);
    glColor3f(1.0f, 0.2f, 0.2f);



    glPointSize(4.0f);
    bresenhamLineFloat(spX, spY, spX + 0.18f*cosf(speedAngle), spY + 0.18f*sinf(speedAngle));
    glPointSize(1.0f);


    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    for (int i = 0; i <= 5; i++) {
        float a = PI - (PI * i / 5.0f);
        glVertex2f(fX + 0.12f*cosf(a), fY + 0.12f*sinf(a));
        glVertex2f(fX + 0.16f*cosf(a), fY + 0.16f*sinf(a));
    }
    glEnd();
    glLineWidth(1.0f);

    float fuelLevel = 0.6f;
    float fAngle = PI - (PI * fuelLevel);
    glColor3f(0.2f, 1.0f, 0.2f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
        glVertex2f(fX, fY);
        glVertex2f(fX + 0.14f*cosf(fAngle), fY + 0.14f*sinf(fAngle));
    glEnd();
    glLineWidth(1.0f);





    float fLetterX = fX - 0.010f;
    float fLetterY = fY - 0.08f;

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);

        glVertex2f(fLetterX, fLetterY);
        glVertex2f(fLetterX + 0.01f, fLetterY);
        glVertex2f(fLetterX + 0.01f, fLetterY + 0.05f);
        glVertex2f(fLetterX, fLetterY + 0.05f);


        glVertex2f(fLetterX, fLetterY + 0.04f);
        glVertex2f(fLetterX + 0.03f, fLetterY + 0.04f);
        glVertex2f(fLetterX + 0.03f, fLetterY + 0.05f);
        glVertex2f(fLetterX, fLetterY + 0.05f);


        glVertex2f(fLetterX, fLetterY + 0.02f);
        glVertex2f(fLetterX + 0.02f, fLetterY + 0.02f);
        glVertex2f(fLetterX + 0.02f, fLetterY + 0.03f);
        glVertex2f(fLetterX, fLetterY + 0.03f);
    glEnd();




    glPushMatrix();
        glTranslatef(wX, wY, 0);
        glRotatef(steeringAngle, 0, 0, 1);
        glTranslatef(-wX, -wY, 0);

        glBegin(GL_TRIANGLE_STRIP);
        for (int i = 0; i <= 80; i++) {
            float a = 2 * PI * i / 80;
            glColor3f(0.02f, 0.02f, 0.02f);
            glVertex2f(wX + 0.38f*cosf(a), wY + 0.38f*sinf(a));
            glColor3f(0.2f, 0.2f, 0.2f);
            glVertex2f(wX + 0.28f*cosf(a), wY + 0.28f*sinf(a));
        }
        glEnd();

        drawSpoke(wX, wY, PI, 0.12f, 0.06f, 0.28f);
        drawSpoke(wX, wY, 0.0f, 0.12f, 0.06f, 0.28f);
        drawSpoke(wX, wY, -PI/2.0f, 0.10f, 0.05f, 0.28f);

        glColor3f(0.1f, 0.1f, 0.1f);
        glBegin(GL_TRIANGLE_FAN);
            glVertex2f(wX, wY);
            for (int i = 0; i <= 30; i++) glVertex2f(wX + 0.15f * cosf(2.0f * PI * i / 30.0f), wY + 0.15f * sinf(2.0f * PI * i / 30.0f));
        glEnd();

        glColor3f(0.8f, 0.8f, 0.8f);


        glPointSize(2.0f);
        bresenhamCircleFloat(wX, wY, 0.05f);
        glPointSize(1.0f);

    glPopMatrix();
}





static void drawGround(void) {
    glColor3f(0.4f, 0.5f, 0.2f);
    glPushMatrix();
        glTranslatef(0.0f, -0.6f, 0.0f);
        glScalef(120.0f, 0.1f, 400.0f);
        glutSolidCube(1.0f);
    glPopMatrix();
}

static void drawRoad(void) {
    glColor3f(0.18f, 0.18f, 0.18f);
    glPushMatrix();
        glTranslatef(0.0f, -0.5f, 0.0f);
        glScalef(ROAD_WIDTH, 0.1f, 400.0f);
        glutSolidCube(1.0f);
    glPopMatrix();
}

static void drawRoadLines(void) {
    const float leftDiv = -LANE_WIDTH / 2.0f;
    const float rightDiv = LANE_WIDTH / 2.0f;
    glColor3f(1.0f, 1.0f, 0.0f);
    for (float z = -ROAD_EXTENT + gs.roadOffset; z < ROAD_EXTENT; z += DASH_SPACING) {
        glPushMatrix();
            glTranslatef(leftDiv, -0.44f, z);
            glScalef(0.2f, 0.05f, 5.0f);
            glutSolidCube(1.0f);
        glPopMatrix();
        glPushMatrix();
            glTranslatef(rightDiv, -0.44f, z);
            glScalef(0.2f, 0.05f, 5.0f);
            glutSolidCube(1.0f);
        glPopMatrix();
    }
}


static void drawTrafficVehicles(void) {
    for (int i = 0; i < NUM_OBSTACLES; i++) {

        if (gs.blink && (gs.blinkTimer / 5) % 2 == 0)
            glColor3f(1.0f, 1.0f, 1.0f);

        float sc = vehicleScale(obstacles[i].vtype);

        glPushMatrix();
            glTranslatef(obstacles[i].x, -0.2f, obstacles[i].z);

            glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
            glScalef(sc, sc, sc);
            drawVehicle(obstacles[i].vtype);
        glPopMatrix();
    }
}

static void drawRoadside(void) {
    for (int i = 0; i < NUM_SCENE_OBJECTS; i++) {
        float z = scene[i].z;


        if (z > 40.0f || z < -600.0f) continue;




        float xLeft = (scene[i].typeLeft == 0) ? TREE_LEFT_X : HOUSE_LEFT_X;
        glPushMatrix();
            glTranslatef(xLeft - scene[i].offsetLeft, -0.5f, z);

            float finalScaleL = scene[i].scaleLeft * ((scene[i].typeLeft == 0) ? 1.3f : 1.5f);
            glScalef(finalScaleL, finalScaleL, finalScaleL);

            if (scene[i].typeLeft == 0) {
                glCallList(DL_TREE);
            } else {
                glRotatef(90.0f, 0, 1, 0);
                glScalef(1.6f, 1.2f, 1.0f);
                glCallList(DL_HOUSE);
            }
        glPopMatrix();




        float xRight = (scene[i].typeRight == 0) ? TREE_RIGHT_X : HOUSE_RIGHT_X;
        glPushMatrix();
            glTranslatef(xRight + scene[i].offsetRight, -0.5f, z);

            float finalScaleR = scene[i].scaleRight * ((scene[i].typeRight == 0) ? 1.3f : 1.5f);
            glScalef(finalScaleR, finalScaleR, finalScaleR);

            if (scene[i].typeRight == 0) {
                glCallList(DL_TREE);
            } else {
                glRotatef(-90.0f, 0, 1, 0);

                glScalef(1.6f, 1.2f, 1.0f);
                glCallList(DL_HOUSE);
            }
        glPopMatrix();
    }
}

void drawCloudBlob(float x, float y, float z) {
    glPushMatrix();
        glTranslatef(x, y, z);
        glutSolidSphere(0.7f, 20, 20);
    glPopMatrix();
}

void drawDenseCloud() {

    drawCloudBlob(0.0f, 0.0f, 0.0f);
    drawCloudBlob(0.8f, 0.2f, 0.0f);
    drawCloudBlob(-0.8f, 0.2f, 0.0f);

    drawCloudBlob(1.5f, 0.3f, 0.3f);
    drawCloudBlob(-1.5f, 0.3f, 0.3f);
    drawCloudBlob(1.5f, 0.3f, -0.3f);
    drawCloudBlob(-1.5f, 0.3f, -0.3f);

    drawCloudBlob(0.0f, 0.9f, 0.0f);
    drawCloudBlob(0.7f, 1.0f, 0.2f);
    drawCloudBlob(-0.7f, 1.0f, 0.2f);

    drawCloudBlob(0.5f, 0.2f, 0.8f);
    drawCloudBlob(-0.5f, 0.2f, 0.8f);

    drawCloudBlob(0.5f, 0.2f, -0.8f);
    drawCloudBlob(-0.5f, 0.2f, -0.8f);
}

static void drawClouds(void) {

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);


    glColor4f(1.0f, 1.0f, 1.0f, 0.9f);

    for (int i = 0; i < NUM_CLOUDS; i++) {
        glPushMatrix();
            glTranslatef(clouds[i].x, CLOUD_Y, clouds[i].z);
            float scale = 2.0f + (i % 4) * 0.5f;
            glScalef(scale, scale * 0.8f, scale);

            glCallList(DL_CLOUD);

        glPopMatrix();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void drawSun3D() {
    glPushMatrix();
    glTranslatef(0.0f, sunY, -350.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glCallList(DL_SUN);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glPopMatrix();
}

void drawMountains() {
    glPushMatrix();
    glTranslatef(0.0f, -1.0f, 0.0f);

    glCallList(DL_MOUNTAINS);

    glPopMatrix();
}





void loadMenuTexture() {
    int width, height, channels;

    unsigned char* data = stbi_load("menu.png", &width, &height, &channels, 4);

    if (data) {
        glGenTextures(1, &menuTextureID);
        glBindTexture(GL_TEXTURE_2D, menuTextureID);


        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
        printf("Menu image loaded successfully!\n");
    } else {
        printf("Failed to load menu.png. Check bin/Debug folder!\n");
    }
}



float getTextWidth(void* font, const char* str) {
    return (float)glutBitmapLength(font, (const unsigned char*)str);
}

static void drawHUD(void) {
    begin2D(gs.winWidth, gs.winHeight);

    char buf[64];
    void* font = GLUT_BITMAP_HELVETICA_18;




    float sx1 = 10;
    float sy1 = gs.winHeight - 60;
    float sx2 = 180;
    float sy2 = gs.winHeight - 10;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
    glBegin(GL_QUADS);
        glVertex2f(sx1, sy1); glVertex2f(sx2, sy1);
        glVertex2f(sx2, sy2); glVertex2f(sx1, sy2);
    glEnd();


    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(sx1, sy1); glVertex2f(sx2, sy1);
        glVertex2f(sx2, sy2); glVertex2f(sx1, sy2);
    glEnd();


    sprintf(buf, "SCORE: %d", gs.score);
    drawText(sx1 + 15, sy2 - 32, buf);




    float maxHealth = 5.0f;
    float hpPercent = (float)gs.health / maxHealth;


    if (hpPercent < 0.0f) hpPercent = 0.0f;
    if (hpPercent > 1.0f) hpPercent = 1.0f;

    float hx2 = gs.winWidth - 20;
    float hx1 = hx2 - 200;
    float hy2 = gs.winHeight - 20;
    float hy1 = hy2 - 25;


    glColor4f(0.1f, 0.1f, 0.1f, 0.8f);
    glBegin(GL_QUADS);
        glVertex2f(hx1, hy1); glVertex2f(hx2, hy1);
        glVertex2f(hx2, hy2); glVertex2f(hx1, hy2);
    glEnd();


    glColor3f(1.0f - hpPercent, hpPercent, 0.0f);
    glBegin(GL_QUADS);
        glVertex2f(hx1, hy1);
        glVertex2f(hx1 + (200.0f * hpPercent), hy1);
        glVertex2f(hx1 + (200.0f * hpPercent), hy2);
        glVertex2f(hx1, hy2);
    glEnd();




    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(hx1, hy1); glVertex2f(hx2, hy1);
        glVertex2f(hx2, hy2); glVertex2f(hx1, hy2);
    glEnd();


    glColor3f(1.0f, 1.0f, 1.0f);
    sprintf(buf, "HEALTH: %d / 5", gs.health);



    drawText(hx1 + 45, hy1 - 20, buf);




    if (!gs.gameStarted && !gs.gameOver) {
        float midX = gs.winWidth / 2.0f;
        float midY = gs.winHeight / 2.0f;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, menuTextureID);

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 1.0f); glVertex2f(0, 0);
            glTexCoord2f(1.0f, 1.0f); glVertex2f(gs.winWidth, 0);
            glTexCoord2f(1.0f, 0.0f); glVertex2f(gs.winWidth, gs.winHeight);
            glTexCoord2f(0.0f, 0.0f); glVertex2f(0, gs.winHeight);
        glEnd();
        glDisable(GL_TEXTURE_2D);


        float panelW = 280.0f;
        float panelH = 260.0f;
        float px1 = midX - panelW / 2;
        float px2 = midX + panelW / 2;
        float py1 = midY - panelH / 2;
        float py2 = midY + panelH / 2;


        glColor4f(0.2f, 0.2f, 0.2f, 0.8f);
        glBegin(GL_QUADS);
            glVertex2f(px1, py1); glVertex2f(px2, py1);
            glVertex2f(px2, py2); glVertex2f(px1, py2);
        glEnd();


        glColor3f(1.0f, 0.5f, 0.0f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
            glVertex2f(px1, py1); glVertex2f(px2, py1);
            glVertex2f(px2, py2); glVertex2f(px1, py2);
        glEnd();
        glLineWidth(1.0f);


        glColor3f(0.5f, 0.8f, 1.0f);
        drawText(midX - 45, midY + 95, "CONTROLS");


        glColor3f(1.0f, 0.84f, 0.0f);
        drawText(midX - 90, midY + 65, "A / D  -  Switch Lanes");

        glColor3f(1.0f, 0.7f, 0.0f);
        drawText(midX - 90, midY + 40, "S      -  Start Engine");

        glColor3f(1.0f, 0.6f, 0.0f);
        drawText(midX - 90, midY + 15, "R      -  Restart Game");

        glColor3f(1.0f, 0.4f, 0.0f);
        drawText(midX - 90, midY - 10, "ESC    -  Exit Game");


        float btnW = 90.0f;


        glColor4f(0.0f, 0.3f, 0.3f, 0.6f);
        glBegin(GL_QUADS);
            glVertex2f(midX - btnW, midY - 65); glVertex2f(midX + btnW, midY - 65);
            glVertex2f(midX + btnW, midY - 35); glVertex2f(midX - btnW, midY - 35);
        glEnd();

        glColor3f(0.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
            glVertex2f(midX - btnW, midY - 65); glVertex2f(midX + btnW, midY - 65);
            glVertex2f(midX + btnW, midY - 35); glVertex2f(midX - btnW, midY - 35);
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(midX - 25, midY - 55, "START");


        glColor4f(0.3f, 0.0f, 0.1f, 0.6f);
        glBegin(GL_QUADS);
            glVertex2f(midX - btnW, midY - 105); glVertex2f(midX + btnW, midY - 105);
            glVertex2f(midX + btnW, midY - 75); glVertex2f(midX - btnW, midY - 75);
        glEnd();

        glColor3f(1.0f, 0.0f, 0.4f);
        glBegin(GL_LINE_LOOP);
            glVertex2f(midX - btnW, midY - 105); glVertex2f(midX + btnW, midY - 105);
            glVertex2f(midX + btnW, midY - 75); glVertex2f(midX - btnW, midY - 75);
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        drawText(midX - 20, midY - 95, "EXIT");

    }




    if (gs.gameOver) {
        float midX = gs.winWidth / 2.0f;
        float midY = gs.winHeight / 2.0f;


        glColor4f(0.2f, 0.0f, 0.0f, 0.6f);
        glBegin(GL_QUADS);
            glVertex2f(0, 0); glVertex2f(gs.winWidth, 0);
            glVertex2f(gs.winWidth, gs.winHeight); glVertex2f(0, gs.winHeight);
        glEnd();


        float boxW = 300.0f, boxH = 200.0f;
        float bx1 = midX - boxW/2, bx2 = midX + boxW/2;
        float by1 = midY - boxH/2, by2 = midY + boxH/2;

        glColor4f(0.05f, 0.05f, 0.05f, 0.95f);
        glBegin(GL_QUADS);
            glVertex2f(bx1, by1); glVertex2f(bx2, by1);
            glVertex2f(bx2, by2); glVertex2f(bx1, by2);
        glEnd();


        glColor3f(1.0f, 0.0f, 0.2f);
        glLineWidth(3.0f);
        glBegin(GL_LINE_LOOP);
            glVertex2f(bx1, by1); glVertex2f(bx2, by1);
            glVertex2f(bx2, by2); glVertex2f(bx1, by2);
        glEnd();
        glLineWidth(1.0f);


        glColor3f(1.0f, 0.2f, 0.2f);
        const char* goTxt = "CRASHED!";
        drawText(midX - getTextWidth(font, goTxt)/2, midY + 60, goTxt);

        glColor3f(1.0f, 1.0f, 1.0f);
        sprintf(buf, "FINAL SCORE: %d", gs.score);
        drawText(midX - getTextWidth(font, buf)/2, midY + 20, buf);


        glColor3f(0.0f, 1.0f, 1.0f);
        drawText(midX - 70, midY - 30, "PRESS [R] RETRY");

        glColor3f(1.0f, 0.5f, 0.0f);
        drawText(midX - 70, midY - 65, "PRESS [ESC] EXIT");
    }

    glDisable(GL_BLEND);
    end2D();
}
# 2023 "test.c"
void initScene() {
    for (int i = 0; i < NUM_SCENE_OBJECTS; i++) {

        scene[i].z = -i * SCENERY_SPACING;


        scene[i].typeLeft = rand() % 2;
        scene[i].typeRight = rand() % 2;


        scene[i].scaleLeft = (scene[i].typeLeft == 0) ? (0.8f + (rand()%3)*0.2f) : 1.0f;
        scene[i].scaleRight = (scene[i].typeRight == 0) ? (0.8f + (rand()%3)*0.2f) : 1.0f;


        scene[i].offsetLeft = ((rand() % 100) / 100.0f) * 2.0f;
        scene[i].offsetRight = ((rand() % 100) / 100.0f) * 2.0f;
    }
}

void generateScenePattern() {
    int i = 0;

    while (i < SCENE_SLOTS) {


        int numTrees = 3 + rand() % 2;

        for (int t = 0; t < numTrees && i < SCENE_SLOTS; t++, i++) {
            sceneType[i] = 0;


            int r = rand() % 3;
            if (r == 0) sceneScale[i] = 0.8f;
            else if (r == 1) sceneScale[i] = 1.0f;
            else sceneScale[i] = 1.3f;
        }


        int numHouses = 2 + rand() % 2;

        for (int h = 0; h < numHouses && i < SCENE_SLOTS; h++, i++) {
            sceneType[i] = 1;


            sceneScale[i] = (rand() % 2) ? 0.8f : 1.2f;
        }
    }
}


static void updateObstacles(void) {

    for (int i = 0; i < NUM_OBSTACLES; i++) {


        obstacles[i].z += gs.gameSpeed;


        if (!obstacles[i].passed && obstacles[i].z > OBS_PASS_Z) {
            gs.score += OBS_SCORE_VALUE;
            obstacles[i].passed = 1;
        }


        if (obstacles[i].z > OBS_CULL_Z) {


            obstacles[i].z = OBS_SPAWN_Z - (rand() % 50);


            int newLane = rand() % NUM_LANES;


            if (i > 0) {
                int prevLane = (int)((obstacles[i - 1].x + LANE_WIDTH) / LANE_WIDTH);
                if (newLane == prevLane) {
                    newLane = (newLane + 1) % NUM_LANES;
                }
            }

            obstacles[i].x = LANE_CENTERS[newLane];


            int r = rand() % 100;
            if (r < 60) obstacles[i].vtype = VTYPE_CAR;
            else if (r < 85) obstacles[i].vtype = VTYPE_BUS;
            else obstacles[i].vtype = VTYPE_TRUCK;

            obstacles[i].passed = 0;
        }
    }
}

static void checkCollision(void) {
    for (int i = 0; i < NUM_OBSTACLES; i++) {

        int hitX = fabsf(gs.playerX - obstacles[i].x) < COL_THRESH_X;
        int hitZ = fabsf(PLAYER_Z - obstacles[i].z) < COL_THRESH_Z;

        if (!gs.invincible && hitX && hitZ) {

            gs.health--;

            gs.blink = 1;
            gs.blinkTimer = 0;
            gs.invincible = 1;


            gs.screenShake = 0.8f + gs.gameSpeed * 2.0f;
            gs.gameSpeed *= 0.5f;


            obstacles[i].z = OBS_SPAWN_Z;
            obstacles[i].passed = 0;

            if (gs.health <= 0){
                gs.gameOver = 1;
                gs.screenShake = 0.0f;
            }

        }
    }
}

static void updateBlink(void) {
    if (gs.blink) {
        gs.blinkTimer++;
        if (gs.blinkTimer > BLINK_DURATION) gs.blink = 0;
    }
}


static void updateClouds(void) {
    for (int i = 0; i < NUM_CLOUDS; i++) {
        clouds[i].x += clouds[i].drift;
        if (clouds[i].x > CLOUD_EXTENT) clouds[i].x = -CLOUD_EXTENT;
        if (clouds[i].x < -CLOUD_EXTENT) clouds[i].x = CLOUD_EXTENT;
    }
}





void initDisplayLists(void) {
    DL_CAR = glGenLists(8);
    DL_BUS = DL_CAR + 1;
    DL_TRUCK = DL_CAR + 2;
    DL_TREE = DL_CAR + 3;
    DL_HOUSE = DL_CAR + 4;
    DL_CLOUD = DL_CAR + 5;
    DL_SUN = DL_CAR + 6;
    DL_MOUNTAINS = DL_CAR + 7;

    glNewList(DL_CAR, GL_COMPILE); drawCar(); glEndList();
    glNewList(DL_BUS, GL_COMPILE); drawBus(); glEndList();
    glNewList(DL_TRUCK, GL_COMPILE); drawTruck(); glEndList();
    glNewList(DL_TREE, GL_COMPILE); drawBigCloudTree(); glEndList();
    glNewList(DL_HOUSE, GL_COMPILE); drawHouse(); glEndList();
    glNewList(DL_CLOUD, GL_COMPILE); drawDenseCloud(); glEndList();
    glNewList(DL_SUN, GL_COMPILE); buildSunGeometry(); glEndList();
    glNewList(DL_MOUNTAINS, GL_COMPILE); buildMountainsGeometry(); glEndList();
}

static void resetGame(void) {
    gs.score = 0;
    gs.health = 5;
    gs.gameOver = 0;

    gs.playerX = 0.0f;
    gs.targetX = 0.0f;
    gs.roadOffset = 0.0f;

    gs.blink = 0;
    gs.blinkTimer = 0;
    gs.invincible = 0;
    gs.screenShake = 0.0f;

    sunY = 80.0f;
    gs.gameSpeed = INITIAL_SPEED;


    initScene();

    for (int i = 0; i < NUM_OBSTACLES; i++) {
        obstacles[i].x = LANE_CENTERS[rand() % NUM_LANES];
        obstacles[i].z = -(float)i * OBS_INIT_SPACING;
        obstacles[i].vtype = rand() % NUM_VTYPES;
        obstacles[i].passed = 0;
    }
}


void handleMouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {

        float glX = (float)x;
        float glY = (float)(gs.winHeight - y);
        float midX = gs.winWidth / 2.0f;
        float midY = gs.winHeight / 2.0f;

        if (gs.gameOver) {

            if (glX >= midX - 80 && glX <= midX + 80 &&
                glY >= midY - 40 && glY <= midY - 10) {
                resetGame();
                gs.gameStarted = 1;
            }

            if (glX >= midX - 80 && glX <= midX + 80 &&
                glY >= midY - 75 && glY <= midY - 50) {
                exit(0);
            }
        }

        if (!gs.gameStarted && !gs.gameOver) {
            float midX = gs.winWidth / 2.0f;
            float midY = gs.winHeight / 2.0f;
            float btnW = 90.0f;



            if (glX >= midX - btnW && glX <= midX + btnW &&
                glY >= midY - 65 && glY <= midY - 35) {

                gs.gameStarted = 1;
                gs.score = 0;
                gs.health = 5;
            }



            if (glX >= midX - btnW && glX <= midX + btnW &&
                glY >= midY - 105 && glY <= midY - 75) {

                exit(0);
            }
        }
    }
}

static void initClouds(void) {
    srand(30 );
    for (int i = 0; i < NUM_CLOUDS; i++) {
        clouds[i].x = ((float)rand()/RAND_MAX) * CLOUD_EXTENT * 2.0f - CLOUD_EXTENT;
        clouds[i].z = ((float)rand()/RAND_MAX) * 80.0f - 60.0f;

        clouds[i].drift = (rand() % 2 ? 1.0f : -1.0f) *
                          (CLOUD_DRIFT + ((float)rand()/RAND_MAX) * 0.01f);
    }
}

static void initGL(void) {
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gs.gameSpeed = INITIAL_SPEED;
    gs.winWidth = WINDOW_WIDTH;
    gs.winHeight = WINDOW_HEIGHT;
    gs.gameStarted = 0;

    resetGame();
    initClouds();


    initDisplayLists();
}

static void drawFadeOverlay(void) {



    float maxAlpha = 0.22f;
    float currentAlpha = (100.0f - sunY) / (100.0f - 40.0f) * maxAlpha;


    if (currentAlpha < 0.0f) currentAlpha = 0.0f;
    if (currentAlpha > maxAlpha) currentAlpha = maxAlpha;


    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBegin(GL_QUADS);

        glColor4f(1.0f, 0.45f, 0.10f, currentAlpha);

        glVertex2f(-1.0f, -1.0f);
        glVertex2f( 1.0f, -1.0f);
        glVertex2f( 1.0f, 1.0f);
        glVertex2f(-1.0f, 1.0f);
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}







void display(void) {


    float skyDay[3] = {0.4f, 0.7f, 1.0f};
    float skySunset[3] = {0.9f, 0.6f, 0.4f};

    float t = (sunY - 40.0f) / (60.0 - 40.0f);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    float r = skySunset[0] + (skyDay[0] - skySunset[0]) * t;
    float g = skySunset[1] + (skyDay[1] - skySunset[1]) * t;
    float b = skySunset[2] + (skyDay[2] - skySunset[2]) * t;

    glClearColor(r, g, b, 1.0f);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();


    float shakeX = 0.0f;
    float shakeY = 0.0f;

    if (gs.screenShake > 0.01f) {
        shakeX = ((rand() % 100) / 100.0f - 0.5f) * gs.screenShake;
        shakeY = ((rand() % 100) / 100.0f - 0.5f) * gs.screenShake;
    }

    gluLookAt(gs.playerX + shakeX, CAM_Y + shakeY, PLAYER_Z,
              gs.playerX, CAM_LOOK_Y, CAM_LOOK_Z,
              0.0f, 1.0f, 0.0f);



    drawGround();
    drawSun3D();
    drawMountains();
    drawRoad();
    drawRoadLines();
    drawRoadside();
    drawClouds();
    drawTrafficVehicles();




    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();


    glDisable(GL_DEPTH_TEST);



    drawFadeOverlay();
    drawDashboard();

    drawHUD();


    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();


    glutSwapBuffers();
}

void timerCallback(int value) {

    if (gs.gameStarted && !gs.gameOver) {


        for (int i = 0; i < NUM_SCENE_OBJECTS; i++) {
            scene[i].z += gs.gameSpeed * 3.0f;
            if (scene[i].z > 40.0f) {
                scene[i].z -= (NUM_SCENE_OBJECTS * SCENERY_SPACING);
                scene[i].typeLeft = rand() % 2;
                scene[i].typeRight = rand() % 2;
                scene[i].scaleLeft = (scene[i].typeLeft == 0) ? (0.8f + (rand() % 3) * 0.2f) : 1.0f;
                scene[i].scaleRight = (scene[i].typeRight == 0) ? (0.8f + (rand() % 3) * 0.2f) : 1.0f;
                scene[i].offsetLeft = ((rand() % 100) / 100.0f) * 2.0f;
                scene[i].offsetRight = ((rand() % 100) / 100.0f) * 2.0f;
            }
        }


        float laneDiff = gs.targetX - gs.playerX;
        gs.playerX += laneDiff * LANE_SMOOTH;
        targetSteeringAngle = laneDiff * -15.0f;
        steeringAngle += (targetSteeringAngle - steeringAngle) * 0.3f;


        gs.roadOffset += gs.gameSpeed * 5.0f;
        if (gs.roadOffset > DASH_SPACING) gs.roadOffset = 0.0f;


        gs.gameSpeed += 0.0005f;
        if (gs.gameSpeed > 0.8f) gs.gameSpeed = 0.8f;


        if (sunY > 40.0f) sunY -= 0.040f;


        updateObstacles();
        checkCollision();
        updateClouds();


        if (gs.blink) {
            gs.blinkTimer++;
            if (gs.blinkTimer > BLINK_DURATION) {
                gs.blink = 0;
                gs.invincible = 0;
            }
        }
    }


    if (gs.gameStarted && !gs.gameOver) {
        if (gs.screenShake > 0.0f) gs.screenShake *= 0.9f;
    }


    glutPostRedisplay();
    glutTimerFunc(TIMER_MS, timerCallback, 0);
}



void keyboardUp(unsigned char key, int x, int y) {}

void keyboardCallback(unsigned char key, int x, int y) {
    switch (key) {
        case 27:
            exit(0);
            break;
        case 's': case 'S':
            if(!gs.gameOver) gs.gameStarted = 1;
            break;
        case 'r': case 'R':
            resetGame();
            gs.gameStarted = 1;
            break;
    }

    if (!gs.gameStarted || gs.gameOver) return;


    switch (key) {
        case 'a': case 'A':
            if (gs.targetX > LANE_CENTERS[0]) gs.targetX -= LANE_WIDTH;
            break;
        case 'd': case 'D':
            if (gs.targetX < LANE_CENTERS[NUM_LANES - 1]) gs.targetX += LANE_WIDTH;
            break;
    }
}



void reshapeCallback(int width, int height) {
    if (height == 0) height = 1;
    gs.winWidth = width;
    gs.winHeight = height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0f, (float)width / height, 1.0f, 500.0f);
    glMatrixMode(GL_MODELVIEW);
}





int main(int argc, char **argv) {
    srand((unsigned int)time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("3D First-Person Car Game");
    glutMouseFunc(handleMouseClick);


    initGL();

    glutDisplayFunc(display);
    glutKeyboardUpFunc(keyboardUp);
    glutKeyboardFunc(keyboardCallback);
    glutReshapeFunc(reshapeCallback);
    glutTimerFunc(0, timerCallback, 0);
    loadMenuTexture();
    glutMainLoop();
    return 0;
}
