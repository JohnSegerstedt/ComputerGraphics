#pragma once
#include <GL/glew.h>
#include <vector>
#include <glm/detail/type_vec3.hpp>
#include <glm/mat4x4.hpp>

struct Particle {
	float lifetime;
	float life_length;
	glm::vec3 velocity;
	glm::vec3 pos;
};

class ParticleSystem {
 public:
	// Members
	std::vector<Particle> particles;
	int max_size;
	// Ctor/Dtor
	ParticleSystem() : max_size(0) {}
	explicit ParticleSystem(int size) : max_size(size) {}
	~ParticleSystem() {}
	// Methods
	void kill(int id) {
		particles.erase(particles.begin() + id);
	}
	void spawn(Particle particle) {
		particles.push_back(particle);
	}
	void process_particles(float dt) {
		for (int i = 0; i < particles.size(); i++) {
			particles[i].life_length++;
			if (particles[i].life_length >= particles[i].lifetime) kill(i);
		}
	}
};
