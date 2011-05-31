#include <cstring>

#include "cinder/CinderMath.h"

#include "GrayScott.h"

using namespace cinder;

/* based on toxiclibs GrayScott.java by Karsten Schmidt */

GrayScott::GrayScott(int width, int height)
{
	this->width = width;
	this->height = height;

	size = width * height;
	u = new float[size];
	v = new float[size];
	uu = new float[size];
	vv = new float[size];

	reset();

	set_coefficients(0.023f, 0.077f, 0.16f, 0.08f);
}

GrayScott::~GrayScott()
{
	delete [] u;
	delete [] v;
	delete [] uu;
	delete [] vv;
}

void GrayScott::reset()
{
	for (unsigned i = 0; i < size; i++)
	{
		uu[i] = 1.0f;
		vv[i] = 0.0f;
	}
}

void GrayScott::set_coefficients(float f, float k, float dU, float dV)
{
	this->f = f;
	this->k = k;
	this->dU = dU;
	this->dV = dV;
}

void GrayScott::set_rect(int x, int y, int w, int h)
{
	int mix = math<int>::clamp(x - w / 2, 0, width);
	int max = math<int>::clamp(x + w / 2, 0, width);
	int miy = math<int>::clamp(y - h / 2, 0, height);
	int may = math<int>::clamp(y + h / 2, 0, height);
	for (int yy = miy; yy < may; yy++)
	{
		for (int xx = mix; xx < max; xx++)
		{
			int idx = yy * width + xx;
			uu[idx] = 0.5f;
			vv[idx] = 0.25f;
		}
	}
}

void GrayScott::update(float t /* = 1.0 */)
{
	t = math<float>::clamp(t, 0, 1.f);
	int w1 = width - 1;
	int h1 = height - 1;
	for (int y = 1; y < h1; y++)
	{
		for (int x = 1; x < w1; x++)
		{
			int idx = y * width + x;
			int top = idx - width;
			int bottom = idx + width;
			int left = idx - 1;
			int right = idx + 1;
			float currF = f;
			float currK = k;
			float currU = uu[idx];
			float currV = vv[idx];
			float d2 = currU * currV * currV;
			u[idx] = math<float>::max(0,
						currU
						+ t
						* ((dU
								* ((uu[right] + uu[left]
									+ uu[bottom] + uu[top]) - 4 * currU) - d2) + currF
							* (1.0f - currU)));
			v[idx] = math<float>::max(0,
						currV
						+ t
						* ((dV
								* ((vv[right] + vv[left]
									+ vv[bottom] + vv[top]) - 4 * currV) + d2) - currK
							* currV));
		}
	}

	memcpy(uu, u, size * sizeof(float));
	memcpy(vv, v, size * sizeof(float));
}
