#include <stdio.h>
#include <math.h>
#include <vector>

#include "cinder/Cinder.h"
#include "cinder/gl/gl.h"

#include "EffectCharge.h"

#define R2 100
#define R 10

using namespace ci;

static int width;
static int height;

std::vector<Charge> charges;

static float colors[MAX_COLORS][3] = {
	{43, 103., 222},
	{240, 207, 19},
	{240, 19, 38},
	{18, 103, 13},
	{176, 25, 193}
};

float colorShadeTab[MAX_COLORS][MAX_PART_UPDATES][4];

int within_bounds(float *x0, float *y0, float oldx, float oldy, int sign)
{
    float x = *x0;
    float y = *y0;

    if ((x < 0) || (y < 0) || ( x >= width) || (y >= height))
		return 0;

    for (unsigned i = 0; i < charges.size(); i++)
	{
		float dx0 = x - charges[i].x;
		float dy0 = y - charges[i].y;

		if (charges[i].c * sign < 0.0)
		{
			float d0 = dx0 * dx0 + dy0 * dy0;
			if (d0 < R2)
				return 0;

			// circle - line segment intersection
			// FIXME: speed
			float x1 = x, y1 = y;
			float x2 = oldx, y2 = oldy;
			float xc = charges[i].x, yc = charges[i].y;

			float d = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);

			if (d > 2)
			{
				float r = ((xc - x1)*(x2 - x1) + (yc - y1) * (y2 - y1));
				if (r < 0.0 || r > d)
					continue;
				float ap2 = r * r / d;
				float ac2 = (x1 - xc) * (x1 - xc) + (y1 - yc) * (y1 - yc);
				float pc2 = ac2 - ap2;
				if (pc2 < R2)
				{
					*x0 = charges[i].x;
					*y0 = charges[i].y;
					return 0;
				}
			}
		}
	}
    return 1;
}

static void calc_gradients(float x0, float y0, float *rx, float *ry)
{
	float ex, ey, gx, gy;
	ex = ey = 0;
	for (unsigned i = 0; i < charges.size(); i++)
	{
		charges[i].field(x0, y0, &gx, &gy);
		ex += gx;
		ey += gy;
	}
	*rx = ex;
	*ry = ey;
}

#define max(a, b) (((a) > (b)) ? (a) : (b))

void EffectCharge::setup(void)
{
	/* precalculate color shades */
	for (int i = 0; i < MAX_COLORS; i++)
	{
		float r = colors[i][0] / 255.0;
		float g = colors[i][1] / 255.0;
		float b = colors[i][2] / 255.0;

		for (int j = 0; j < MAX_PART_UPDATES; j++)
		{
			float m = max(0, (MAX_PART_UPDATES - (float)j*15) / MAX_PART_UPDATES);
			colorShadeTab[i][j][0] = r;
			colorShadeTab[i][j][1] = g;
			colorShadeTab[i][j][2] = b;
			colorShadeTab[i][j][3] = m;
		}
	}

#if 0
	/* load sounds */
	for (int i = 0; i < SHAPEMAX; i++)
	{
		sound[i].loadSound("charge.wav");
		sound[i].setLoop(true);
		sound[i].setVolume(0.3);
	}
#endif
}

void EffectCharge::instantiate(void)
{
	/* delete all charges */
	charges.clear();
}

void EffectCharge::deinstantiate(void)
{
#if 0
	for (int i = 0; i < SHAPEMAX; i++)
	{
		sound[i].stop();
	}
#endif
}

void EffectCharge::draw(void)
{
	width = mBounds.getWidth();
	height = mBounds.getHeight();

	gl::clear( Color::black() );
	gl::enableAlphaBlending();

    if (charges.size())
	{
		/* generate particles from charges */
		for (unsigned j = 0; j < charges.size(); j++)
		{
			float a = 0.0;
			for (int i = 0; i < MAX_PARTICLES; i++)
			{
				charges[j].particles[i].init(charges[j].x + 5 * cos(a),
						charges[j].y + 5 * sin(a),
						charges[j].c < 0 ? -1 : 1,
						charges[j].x, charges[j].y,
						charges[j].color_index);
				a += 2*M_PI / MAX_PARTICLES;
			}
		}

		/* trace path of particles */
		int particles_alive, max_updates = 0;
		do
		{
			particles_alive = 0;
			for (unsigned j = 0; j < charges.size(); j++)
			{
				particles_alive |= charges[j].update_particles(calc_gradients);
			}
			max_updates++;
		} while (particles_alive && (max_updates < MAX_PART_UPDATES));
	}

	gl::disableAlphaBlending();
}

void EffectCharge::addCursor(int id, float x, float y, float r)
{
	int color = rand() % MAX_COLORS;
	int sign = (rand() % 2) ? 1 : -1;
	charges.push_back(Charge(id, x, y, sign * r, color));

#if 0
	sound[id].play();
#endif
}

void EffectCharge::updateCursor(int id, float x, float y, float r)
{
	for (unsigned i = 0; i < charges.size(); i++)
	{
		if (id == charges[i].id)
		{
			charges[i].x = x;
			charges[i].y = y;
			if (charges[i].c < 0)
				charges[i].c = -r;
			else
				charges[i].c = r;
			break;
		}
	}
}

void EffectCharge::removeCursor(int id)
{
	std::vector<Charge>::iterator it;
	for (it = charges.begin(); it < charges.end(); it++)
	{
		if ((*it).id == id)
		{
			charges.erase(it);
			break;
		}
	}

#if 0
	sound[id].stop();
#endif
}

