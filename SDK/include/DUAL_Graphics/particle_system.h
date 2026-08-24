//
// Created by killian on 8/24/26.
//

#ifndef DUAL_PARTICLE_SYSTEM_H
#define DUAL_PARTICLE_SYSTEM_H

#include "../DUAL_Math/dual_math.h"

typedef struct Particle {
    DUAL_Vec3 position;
    DUAL_Vec3 velocity;
    DUAL_Vec3 color;
    float lifespan;
}Particle;
Particle Particle_Create(DUAL_Vec3 position, DUAL_Vec3 velocity, DUAL_Vec3 color, float life);
void Particle_Update(Particle* particle, float dt);

typedef struct ParticleSystem {
    Particle* particles;
    unsigned int nb_particles;
}ParticleSystem;
void ParticleSystem_Create(unsigned int nb_particles, ParticleSystem* particles);
void ParticleSystem_Update(ParticleSystem* particles, float dt);
void ParticleSystem_Clean(ParticleSystem* particles);

#endif //DUAL_PARTICLE_SYSTEM_H
