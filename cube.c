#include <stdio.h>
#include <math.h>
#include <string.h>

#ifndef _WIN32
  #include <unistd.h>
  #define sleep_ms(ms) usleep((ms) * 1000)
#else
  #include <windows.h>
  #define sleep_ms(ms) Sleep(ms)
#endif 

#define WIDTH 160
#define HEIGHT 44

static const struct {
  float k1;
  float camDist;
  float dA, dB, dC; // rotation speeds
  float step; // density of surface mesh
  float frame_ms; // ~ we want 60 fps
} config = { 40, 100, 0.05f, 0.05f, 0.01f, 0.6f, 16.0f};

float zBuf[WIDTH * HEIGHT];
char buf[WIDTH * HEIGHT];


float rotateX(float x, float y, float z, float A, float B, float C){
  return x * cos(B) * cos(C) + y * (cos(A) * sin(C) + sin(A) * sin(B) * cos(C)) + z * (sin(A) * sin(C) - cos(A) * sin(B) * cos(C));
}

float rotateY(float x, float y, float z, float A, float B, float C){
  return -x * cos(B) * sin(C) + y * (cos(A) * cos(C) - sin(A) * sin(B) * sin(C)) + z * (sin(A) * cos(C) + cos(A) * sin(B) * sin(C));
}

float rotateZ(float x, float y, float z, float A, float B, float C){
  return x * sin(B) - y * sin(A) * cos(B) + z * cos(A) * cos(B);
}

// project and depth test
static void projectSurface(float worldX, float worldY, float worldZ, char ch, float A, float B, float C, float hOffset){
  float x = rotateX(worldX, worldY, worldZ, A, B, C);
  float y = rotateY(worldX, worldY, worldZ, A, B, C);
  float z = rotateZ(worldX, worldY, worldZ, A, B, C) + config.camDist;

  // calculate one over z for depth perception
  float ooz = 1.0 / z;

  int xProj = (int)(WIDTH / 2 /*+ hOffset */ + config.k1 * ooz * x * 2.0f);
  int yProj = (int)(HEIGHT / 2 - config.k1 * ooz * y);
  int index = xProj + yProj * WIDTH;

  // unsigned cast basically checks both ends, i.e. (xp => 0) && (xp < WIDTH)
  if ((unsigned)xProj < WIDTH /*&& (unsigned)yProj < HEIGHT */) {
    if (ooz > zBuf[index]){
      zBuf[index] = ooz;
      buf [index] = ch;
    }
  }
}

int main() {
  puts("\x1b[2J"); // clear terminal screen
                   
  float A = 0.0f, B = 0.0f, C = 0.0f;

  while(1){
    memset(buf, ' ', sizeof buf );
    memset(zBuf, 0, sizeof zBuf);

    int cubeHalfWidth = 20;
    for (float i = -cubeHalfWidth; i < cubeHalfWidth; i += config.step){
      for (float j = -cubeHalfWidth; j < cubeHalfWidth; j += config.step){
        // project and z-test surfaces one by one, order doesn't matter as buffers are flushed after
        projectSurface(i, j, -cubeHalfWidth, '@', A, B, C, -2 * cubeHalfWidth);
        projectSurface(cubeHalfWidth, j, i,  '$', A, B, C, -2 * cubeHalfWidth);
        projectSurface(-cubeHalfWidth, j, -i,'~', A, B, C, -2 * cubeHalfWidth);
        projectSurface(-i, j, cubeHalfWidth, '#', A, B, C, -2 * cubeHalfWidth);
        projectSurface(i, -cubeHalfWidth, -j,';', A, B, C, -2 * cubeHalfWidth);
        projectSurface(i, cubeHalfWidth, j,  '+', A, B, C, -2 * cubeHalfWidth);
      }
    }
    // flush to terminal
    puts("\x1b[H"); // return cursor to start
    for (int k = 0; k < WIDTH * HEIGHT; k++) putchar(k % WIDTH ? buf[k] : '\n');

    A += config.dA;
    B += config.dB;
    C += config.dC;
    
    sleep_ms(config.frame_ms);
  } 
}                  
