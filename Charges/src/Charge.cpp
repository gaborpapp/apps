#include <stdio.h>
#include <math.h>

#include "cinder/gl/gl.h"

#include "Charge.h"

using namespace ci;

Charge::Charge(int id, float x, float y, float c, int color_index)
{
	this->id = id;
	this->x = x;
	this->y = y;
	this->c = c;
	this->color_index = color_index;
}

void Charge::field(float x0, float y0, float *rx, float *ry)
{
	float dx = x0 - x;
	float dy = y0 - y;
	float d2r = c * GRAD_SCALE / (dx * dx + dy * dy);
	*rx = dx * d2r;	// cut-off for speed
	*ry = dy * d2r;

	//printf("%5.2f %5.2f %5.2f %5.2f %5.2f %5.2f\n", x0, y0, x, c, *rx, *ry);
	/*	float d = sqrt(d2);			// real gradient, looks slightly better
	 *rx = GRAD_SCALE*dx*c/(d2*d);
	 *ry = GRAD_SCALE*dy*c/(d2*d); */
}

// returns 1 if there are particles alive
int Charge::update_particles(DERIVFUNC derivfunc)
{
	int r = 0;
	gl::enable( GL_LINE_SMOOTH );
	glBegin( GL_LINES );
	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		r |= particles[i].update(derivfunc);
	}
	glEnd();
	gl::disable( GL_LINE_SMOOTH );
	return r;
}

