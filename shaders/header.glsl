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

// if only change this, remember manually recompile shaders

const int MAX_INSTANCE = 8192;
const int THREADS_PER_GROUP = 256;
// SPH parameters
const float particleMass = 0.2;           // Mass of each particle
const float smoothingLength = 1.0;        // Smoothing length (h)
const float stiffness = 980.0;           // Gas stiffness constant (k)
const float restDensity = 980.0;         // Rest density of the fluid (ρ₀)
const float viscosity = 0.1;              // Viscosity coefficient
const float gravity = -9.81;               // Gravity constant

#endif