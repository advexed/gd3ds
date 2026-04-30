// Adapted from OpenGD

#ifndef MOTION_TRAIL_H
#define MOTION_TRAIL_H

#include <citro2d.h>
#include "objects.h"
#include "color_channels.h"

#define MAX_TRAIL_POINTS 128
#define TRAIL_CLEAR_DISTANCE 30.f

typedef struct {
    float x, y;
} Vec2;

typedef struct {
    float u, v;
} Tex2F;

typedef struct {
    bool appendNewPoints;
    bool startingPositionInitialized;
    bool waveTrail;

    int nuPoints;
    int previousNuPoints;
    int actualNuPoints;
    int maxPoints;
    int offscreenCount;

    float opacity;
    float fadeDelta;
    float minSeg;
    float stroke;

    Vec2 positionR;
    Color displayedColor;

    C2D_Image image;

    float pointState[MAX_TRAIL_POINTS];
    Vec2 pointVertexes[MAX_TRAIL_POINTS];
    Vec2 vertices[MAX_TRAIL_POINTS * 2];
    u8 colorPointer[MAX_TRAIL_POINTS * 8]; // RGBA * 2 per point
    Tex2F texCoords[MAX_TRAIL_POINTS * 2];
    
    Vec2 centerVertices[MAX_TRAIL_POINTS * 2];

    Vec2 lastStopPosition;
    bool wasStopped;

} MotionTrail;

void MotionTrail_Init(MotionTrail* trail, float fade, float minSeg, float stroke, bool waveTrail, Color color, C2D_Image tex);
void MotionTrail_UpdateWaveTrail(MotionTrail *trail, float delta);
void MotionTrail_Update(MotionTrail* trail, float delta);
void MotionTrail_ResumeStroke(MotionTrail* trail);
void MotionTrail_Clear(MotionTrail *trail);
void MotionTrail_StopStroke(MotionTrail* trail);
void MotionTrail_Draw(MotionTrail* trail);
void MotionTrail_DrawWaveTrail(MotionTrail *trail);

void MotionTrail_CopyTrail(MotionTrail *dst, MotionTrail *src);

void MotionTrail_AddWavePoint(MotionTrail* trail);

#endif