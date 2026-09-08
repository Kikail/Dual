//
// Created by killian on 8/24/26.
//
#include "../../include/DUAL_Graphics/particle_system.h"

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include "DUAL_Core/dual_core.h"

Particle Particle_Create(DUAL_Vec3 position, DUAL_Vec3 velocity, DUAL_Vec3 color, DUAL_Vec3 size, float life) {
    Particle particle;
    particle.position = position;
    particle.velocity = velocity;
    particle.color = color;
    particle.lifespan = life;
    particle.size = size;
    return particle;
}

void ParticleSystem_Create(unsigned int nb_particles, Particle* particles, ParticleEmitter* emitter, ParticleSystem* particleSystem) {
    particleSystem->particles = malloc(sizeof(Particle) * nb_particles);
    particleSystem->nb_particles = nb_particles;
    if (particles == NULL) {
        DUAL_Log(DUAL_LOG_ERROR, "ParticleEmitter: particleSystem is NULL");
        return;
    }
    particleSystem->particles = particles;
    if (emitter == NULL) {
        DUAL_Log(DUAL_LOG_ERROR, "ParticleEmitter: particles is NULL");
        return;
    }
    particleSystem->emitter = emitter;
}
void ParticleSystem_Update(ParticleSystem* particles, float dt) {
    if (particles->emitter != NULL) {
        ParticleEmitter_Update(particles->emitter, particles->particles, particles->nb_particles, dt);
    }
}

void ParticleSystem_Clean(ParticleSystem* particles) {
    free(particles->particles);
    free(particles->emitter);
}
ParticleEmitter* ParticleEmitter_Init(
    DUAL_Vec3 (*init_position_function)(void),
    DUAL_Vec3 (*init_scale)(void),
    DUAL_Vec3 (*init_color)(void),
    DUAL_Vec3 (*init_velocity)(void),
    float (*init_lifetime)(void),
    void (*size_over_time)(struct ParticleEmitter* emitter, Particle* particles, unsigned int nbParticles, float deltaTime),
    void (*color_over_time)(struct ParticleEmitter* emitter, Particle* particles, unsigned int nbParticles, float deltaTime),
    void (*velocity_over_time)(struct ParticleEmitter* emitter, Particle* particles, unsigned int nbParticles, float deltaTime),
    Particle* particles,
    unsigned int nb_particles
){
    ParticleEmitter* emitter = malloc(sizeof(ParticleEmitter));

    emitter->init_position = init_position_function;
    if (emitter->init_position == NULL) {
        DUAL_Log(DUAL_LOG_ERROR, "ParticleEmitter: init_position_function is NULL");
        free(emitter);
        return NULL;
    }
    emitter->init_velocity = init_velocity;
    if (emitter->init_velocity == NULL) {
        DUAL_Log(DUAL_LOG_ERROR, "ParticleEmitter: init_velocity_function is NULL");
        free(emitter);
        return NULL;
    }
    emitter->init_scale = init_scale;
    if (emitter->init_scale == NULL) {
        DUAL_Log(DUAL_LOG_ERROR, "ParticleEmitter: init_scale_function is NULL");
        free(emitter);
        return NULL;
    }
    emitter->init_color = init_color;
    if (emitter->init_color == NULL) {
        DUAL_Log(DUAL_LOG_ERROR, "ParticleEmitter: init_color_function is NULL");
        free(emitter);
        return NULL;
    }
    emitter->init_lifetime = init_lifetime;
    if (emitter->init_lifetime == NULL) {
        DUAL_Log(DUAL_LOG_ERROR, "ParticleEmitter: init_lifetime is NULL");
        free(emitter);
        return NULL;
    }

    // Ici c'est pas important de savoir si c'est un pointeur vide car nous executons pendant l'update
    emitter->color_over_time = color_over_time;
    emitter->size_over_time = size_over_time;
    emitter->velocity_over_time = velocity_over_time;

    if (particles == NULL) {
        DUAL_Log(DUAL_LOG_ERROR, "ParticleEmitter: particles is NULL");
        free(emitter);
        return NULL;
    }

    for (unsigned int i = 0; i < nb_particles; i++) {
        Particle* particle = &particles[i];
        particle->position = emitter->init_position();
        particle->size = emitter->init_scale();
        particle->color = emitter->init_color();
        particle->velocity = emitter->init_velocity();
        particle->lifespan = emitter->init_lifetime();
    }

    return emitter;
}
void ParticleEmitter_Update(ParticleEmitter* emitter, Particle* particles, unsigned int nbParticles, float deltaTime) {
    if (emitter->color_over_time != NULL) emitter->color_over_time(emitter, particles, nbParticles, deltaTime);
    if (emitter->size_over_time != NULL) emitter->size_over_time(emitter, particles, nbParticles, deltaTime);
    if (emitter->velocity_over_time != NULL) emitter->velocity_over_time(emitter, particles, nbParticles, deltaTime);

    for (unsigned int i = 0; i < nbParticles; i++) {
        Particle* particle = &particles[i];
        particle->lifespan -= deltaTime;
        if (particle->lifespan <= 0.0f) {
            particle->position = emitter->init_position();
            particle->size = emitter->init_scale();
            particle->color = emitter->init_color();
            particle->velocity = emitter->init_velocity();
            particle->lifespan = emitter->init_lifetime();
        }
    }
}