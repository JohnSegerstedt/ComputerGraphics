#include "..\lab5-rendertotexture\ParticleSystem.h"
#include <GL/glew.h>
#include <vector>
#include <glm/detail/type_vec3.hpp>
#include <glm/mat4x4.hpp>




Particle::Particle(float maxLife, float currentLife, glm::vec3 vel, glm::vec3 pos) {
	lifetime = maxLife;
	life_length = currentLife;
	velocity = vel;
	position = pos;
}

void ParticleSystem::kill(int id) {
	particles.erase(particles.begin() + id);
}

void ParticleSystem::spawn(Particle particle) {
	particles.push_back(particle);
}

void ParticleSystem::process_particles(float dt) {
	for (int i = 0; i < particles.size(); i++) {
		particles[i].life_length++;
		if (particles[i].life_length >= particles[i].lifetime) kill(i);
	}
}





/*

1. Eventually spawn particle(s) by calling spawn()
2. Process particles by calling process_particles()
	a. For each particle, remove if dead by calling kill()
	b. For each particle, update the state (see below)
3. Update the vector of reduced states, from the full states
4. Upload the data to the gpu
5. Render the particles as point sprites



*/