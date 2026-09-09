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
    DUAL_Vec3 size;
    float lifespan;
    float lifetime;
}Particle;
Particle Particle_Create(DUAL_Vec3 position, DUAL_Vec3 velocity, DUAL_Vec3 color, DUAL_Vec3 size, float life);

// Structure qui s'occupe de gerer la modification des particules
typedef struct ParticleEmitter {
    float lifeTime;
    float lifespan;
    DUAL_Vec3 (*init_position)(void);
    DUAL_Vec3 (*init_scale)(void);
    DUAL_Vec3 (*init_color)(void);
    DUAL_Vec3 (*init_velocity)(void);
    float (*init_lifetime)(void);
    void (*particle_over_time)(struct ParticleEmitter* emitter, Particle* particles, unsigned int nbParticles, float deltaTime);
}ParticleEmitter;
ParticleEmitter* ParticleEmitter_Init(
    DUAL_Vec3 (*init_position_function)(void),
    DUAL_Vec3 (*init_scale)(void),
    DUAL_Vec3 (*init_color)(void),
    DUAL_Vec3 (*init_velocity)(void),
    float (*init_lifetime)(void),
    void (*particle_over_time)(struct ParticleEmitter* emitter, Particle* particles, unsigned int nbParticles, float deltaTime),
    Particle* particles,
    unsigned int nb_particles
);
void ParticleEmitter_Update(ParticleEmitter* emitter, Particle* particles, unsigned int nbParticles, float deltaTime);

typedef struct ParticleSystem {
    Particle* particles;
    ParticleEmitter* emitter;
    unsigned int nb_particles;
}ParticleSystem;
void ParticleSystem_Create(unsigned int nb_particles, Particle* particles, ParticleEmitter* emitter, ParticleSystem* particleSystem);
void ParticleSystem_Update(ParticleSystem* particles, float dt);
void ParticleSystem_Clean(ParticleSystem* particles);

#endif //DUAL_PARTICLE_SYSTEM_H
