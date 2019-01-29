#include "ParticleSystem.h"


/*

1. Eventually spawn particle(s) by calling spawn()
2. Process particles by calling process_particles()
	a. For each particle, remove if dead by calling kill()
	b. For each particle, update the state (see below)
3. Update the vector of reduced states, from the full states
4. Upload the data to the gpu
5. Render the particles as point sprites



*/