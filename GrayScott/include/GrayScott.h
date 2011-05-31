#ifndef GRAY_SCOTT_H
#define GRAY_SCOTT_H

class GrayScott
{
	public:
		GrayScott(int width, int height);
		~GrayScott();

		void reset();
		void set_coefficients(float f, float k, float dU, float dV);

		void set_rect(int x, int y, int w, int h);
		void update(float t = 1.0);

		float *u, *v;

	protected:


		float *uu, *vv;
		int width, height;
		int size;

		float f, k;
		float dU, dV;

};

#endif

