#ifndef __CHARGE_H__
#define __CHARGE_H__

#include "AdarKutta.h"
#include "Particle.h"

#define MAX_PARTICLES 107	// maximum number of particles per charge
#define GRAD_SCALE 200		// scale factor for gradient vectors

class Charge
{
	public:
		Charge(int id, float x, float y, float c, int color_index);

		float x, y;
		float c;
		int id;
		int color_index;

		Particle particles[MAX_PARTICLES];

		void field(float x0, float y0, float *rx, float *ry);
		int update_particles(DERIVFUNC derivfunc);
};

#endif
