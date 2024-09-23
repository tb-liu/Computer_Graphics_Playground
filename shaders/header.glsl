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
const int THREADS_PER_GROUP = 256;
// SPH parameters
const float particleMass = 1.0;           // Mass of each particle
const float smoothingLength = 1.0;        // Smoothing length (h)
const float stiffness = 1000.0;           // Gas stiffness constant (k)
const float restDensity = 1000.0;         // Rest density of the fluid (ρ₀)
const float viscosity = 0.1;              // Viscosity coefficient

#endif