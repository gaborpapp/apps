#ifndef __EFFECTCHARGE_H__
#define __EFFECTCHARGE_H__

#include "Effect.h"
#include "Charge.h"

/* maximum particle updates per frame, the bigger the fancier (and slower) */
#define MAX_PART_UPDATES 150

/* maximum number of colors */
#define MAX_COLORS 5

class EffectCharge : public Effect
{
	public:
		void setup(void);
		void draw(void);

		void instantiate(void);
		void deinstantiate(void);

		void addCursor(int id, float x, float y, float r);
		void updateCursor(int id, float x, float y, float r);
		void removeCursor(int id);

	private:
		//ofSoundPlayer sound[SHAPEMAX];
};

extern int within_bounds(float *x0, float *y0, float oldx, float oldy, int sign);
extern float colorShadeTab[MAX_COLORS][MAX_PART_UPDATES][4];

#endif

