#include <math.h>
#include "AdarKutta.h"

#define SAFETY	 0.9
#define PGROW	-0.2
#define PSHRINK -0.25
#define ERRCON 1.89e-4	// equals (5/SAFETY) raised to the power (1/PGROW)

typedef void (*DERIVFUNC)(float x0, float y0, float *rx, float *ry);

// adaptive runge-kutta-fehlberg cash-karp version
// ordinary differential equation integrator with cash-karp runge-kutta method
//
// h - suggested step size
// x0, y0 - coords
// derivfunc - function calculating derivatives
// xout, your - new coordinates at step h
// xerr, yerr - errors
// sign - 1, -1  - in the electric field some particles follows the field gradients
//				   while others move in the opposite direction depending on their charge
static void rkck(float h, float x0, float y0, DERIVFUNC derivfunc,
		  float *xout, float *yout, float *xerr, float *yerr, int sign)
{
	const float b21=0.2, b31=3.0/40.0, b32=9.0/40.0, b41=0.3, b42=-0.9, b43=1.2,
		b51=-11.0/54.0, b52=2.5, b53=-70.0/27.0, b54=35.0/27.0,
		b61=1631.0/55296.0, b62=175.0/512.0, b63=575.0/13824.0,
		b64=44275.0/110592.0, b65=253.0/4096.0;
	const float	c1=37.0/378.0, c3=250.0/621.0, c4=125.0/594.0, c6=512.0/1771.0;
	const float dc1=c1-2825.0/27648.0, dc3=c3-18575.0/48384.0, dc4=c4-13525.0/55296.0,
		dc5=-277.0/14336.0, dc6=c6-0.25;

	float k1x, k2x, k3x, k4x, k5x, k6x;
	float k1y, k2y, k3y, k4y, k5y, k6y;

	if (sign>0)
	{
		derivfunc(x0, y0, &k1x, &k1y);						// 1st
		k1x*=h;
		k1y*=h;

		derivfunc(x0+k1x*b21, y0+k1y*b21, &k2x, &k2y);		// 2nd
		k2x*=h;
		k2y*=h;

		derivfunc(x0+k1x*b31+k2x*b32,						//3rd
			y0+k1y*b31+k2y*b32, &k3x, &k3y);
		k3x*=h;
		k3y*=h;

		derivfunc(x0+k1x*b41+k2x*b42+k3x*b43,				// 4th
				  y0+k1y*b41+k2y*b42+k3y*b43, &k4x, &k4y);
		k4x*=h;
		k4y*=h;

		derivfunc(x0+k1x*b51+k2x*b52+k3x*b53+k4x*b54,		// 5th
				  y0+k1y*b51+k2y*b52+k3y*b53+k4y*b54, &k5x, &k5y);
		k5x*=h;
		k5y*=h;

		derivfunc(x0+k1x*b61+k2x*b62+k3x*b63+k4x*b64+k5x*b65, // 6th
				  y0+k1y*b61+k2y*b62+k3y*b63+k4y*b64+k5y*b65, &k6x, &k6y);
		k6x*=h;
		k6y*=h;

		*xout = x0 + (c1*k1x + c3*k3x + c4*k4x + c6*k6x);	// output
		*yout = y0 + (c1*k1y + c3*k3y + c4*k4y + c6*k6y);
	}
	else
	{
		derivfunc(x0, y0, &k1x, &k1y);						// 1st
		k1x*=h;
		k1y*=h;

		derivfunc(x0-k1x*b21, y0-k1y*b21, &k2x, &k2y);		// 2nd
		k2x*=h;
		k2y*=h;

		derivfunc(x0-(k1x*b31+k2x*b32),						// 3rd
				  y0-(k1y*b31+k2y*b32), &k3x, &k3y);
		k3x*=h;
		k3y*=h;

		derivfunc(x0-(k1x*b41+k2x*b42+k3x*b43),				// 4th
				  y0-(k1y*b41+k2y*b42+k3y*b43), &k4x, &k4y);
		k4x*=h;
		k4y*=h;

		derivfunc(x0-(k1x*b51+k2x*b52+k3x*b53+k4x*b54),		// 5th
				  y0-(k1y*b51+k2y*b52+k3y*b53+k4y*b54), &k5x, &k5y);
		k5x*=h;
		k5y*=h;

		derivfunc(x0-(k1x*b61+k2x*b62+k3x*b63+k4x*b64+k5x*b65), // 6th
				  y0-(k1y*b61+k2y*b62+k3y*b63+k4y*b64+k5y*b65), &k6x, &k6y);
		k6x*=h;
		k6y*=h;

		*xout = x0 - (c1*k1x + c3*k3x + c4*k4x + c6*k6x);	// output
		*yout = y0 - (c1*k1y + c3*k3y + c4*k4y + c6*k6y);
	}

	*xerr = dc1*k1x + dc3*k3x + dc4*k4x + dc5*k5x + dc6*k6x; // errors
	*yerr = dc1*k1y + dc3*k3y + dc4*k4y + dc5*k5y + dc6*k6y;

}

// driver for adaptive runge-kutta ode solver
// most parameters are the same as above
// htry  - step size to try
// hnext - new step size
// eps	 - required accuracy
void rkqs(float x0, float y0, DERIVFUNC derivfunc,
	float *xout, float *yout, float htry, float eps,
	float *hnext, int sign)
{
	float xerr, yerr, errmax;
	float h = htry;
	int k=0;
	while(k<MAX_RK_STEPS)
	{
		rkck(h, x0, y0, derivfunc, xout, yout, &xerr, &yerr, sign);

		errmax = fabs(xerr) + fabs(yerr);
		if (errmax <= eps)	break;

		float htemp=SAFETY*h*pow(errmax, PSHRINK);
		h = (htemp < h) ? htemp : 0.1*h;
		k++;
	}

	if (errmax>ERRCON)
		*hnext = SAFETY*h*pow(errmax, PGROW);
	else
		*hnext = 5.0*h;
}

