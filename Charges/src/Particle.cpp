#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "cinder/gl/gl.h"

#include "EffectCharge.h"
#include "AdarKutta.h"
#include "Particle.h"

void Particle::init(float x, float y, int sign, float chargex, float chargey,
		int color_index)
{
    this->x = x;
	this->y = y;
    this->sign = sign;

    oldx = chargex;
	oldy = chargey;

    alive = 1;
    htry = 1;
    update_count = 1;

	this->color_index = color_index;
}

// returns 1 if alive
int Particle::update(DERIVFUNC derivfunc)
{
	if (!alive)
		return 0;

	rkqs(x, y, derivfunc, &x, &y, htry, ODE_ACCURACY, &htry, sign);

	if (!within_bounds(&x, &y, oldx, oldy, sign))
	{
		alive = 0;
	}

	float *color0 = colorShadeTab[color_index][update_count - 1];
	float *color1 = colorShadeTab[color_index][update_count];

	glColor4fv(color0);
	glVertex2f(oldx, oldy);
	glColor4fv(color1);
	glVertex2f(x, y);

	oldx = x;
	oldy = y;
	update_count++;

	return alive;
}

