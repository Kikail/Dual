//
// Created by killian on 8/24/26.
//
#include "../../include/DUAL_Graphics/particle_system.h"

#include <stdlib.h>
#include <time.h>

Particle Particle_Create(DUAL_Vec3 position, DUAL_Vec3 velocity, DUAL_Vec3 color, float life) {
    Particle particle;
    particle.position = position;
    particle.velocity = velocity;
    particle.color = color;
    particle.lifespan = life;
    return particle;
}

void Particle_Update(Particle* particle, float dt) {
    if (particle->lifespan > 0) {
        particle->lifespan -= dt;
        particle->position.x += particle->velocity.x * dt;
        particle->position.y += particle->velocity.y * dt;
        particle->position.z += particle->velocity.z * dt;
    }
}

void ParticleSystem_Create(unsigned int nb_particles, ParticleSystem* particles) {
    particles->particles = malloc(sizeof(Particle) * nb_particles);
    particles->nb_particles = nb_particles;

    float min = 0.0f;
    float max = 5.0f;

    for (unsigned int i = 0; i < nb_particles; i++) {

        float x = min + (float)rand() / ((float)RAND_MAX / (max - min));
        float y = min + (float)rand() / ((float)RAND_MAX / (max - min));
        float z = min + (float)rand() / ((float)RAND_MAX / (max - min));

        particles->particles[i] = Particle_Create(
            (DUAL_Vec3){ 0.0, 0.0, 0.0 },
            ((DUAL_Vec3){ x, y, z }),
            (DUAL_Vec3){ 1.0, 1.0, 1.0 },
            100.0
        );
    }
}
void ParticleSystem_Update(ParticleSystem* particles, float dt) {
    for (unsigned int i = 0; i < particles->nb_particles; i++) {
        Particle_Update(&particles->particles[i], dt);
    }
}

void ParticleSystem_Clean(ParticleSystem* particles) {
    free(particles->particles);
}