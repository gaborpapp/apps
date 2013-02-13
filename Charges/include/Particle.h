#ifndef __PARTICLE_H__
#define __PARTICLE_H__

#include "EffectCharge.h"
#include "AdarKutta.h"

#define ODE_ACCURACY .01

//#define	PARTICLE_COLOR1 0x205e95
//#define	PARTICLE_COLOR2 0xc65029

//#define		PARTICLE_COLOR1 0x251e16
//#define		PARTICLE_COLOR2 0x0f151a

class Particle
{
	public:
		void init(float x, float y, int sign, float chargex, float chargey,
				int color_index);

		int update(DERIVFUNC derivfunc);

	private:
		float x, y;
		int alive;
		int sign;
		float oldx, oldy;
		float htry;
		int update_count;

		int color_index; //< index in color table
};

#endif

