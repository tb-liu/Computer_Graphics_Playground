// header.glsl
#ifndef HEADER
#define HEADER

struct Particle
{
    vec4 pos;
    vec4 velocity;
    vec4 force;
    float density;
    float pad[3];
};

const int MAX_INSTANCE = 4096;

#endif