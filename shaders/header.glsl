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
// add bounding box here
float boxSize = 6;
vec3 domainMax = vec3(boxSize);
vec3 domainMin = vec3(-boxSize);
// if only change this, remember manually recompile shaders

const int MAX_INSTANCE = 1024*32;
const int THREADS_PER_GROUP = 256;
// SPH parameters
const float particleMass = 1.2;           // Mass of each particle
const float smoothingLength = 0.98;        // Smoothing length (h)
const float stiffness = 120.0;           // Gas stiffness constant (k)
const float restDensity = 980.0;         // Rest density of the fluid (ρ₀)
const float viscosity = 0.7;              // Viscosity coefficient
const float gravity = -9.81;               // Gravity constant

#endif