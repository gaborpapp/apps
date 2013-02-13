#ifndef __ADARKUTTA_H__
#define __ADARKUTTA_H__

#define MAX_RK_STEPS 50	// solver would repeat ode integrating with various
		// step sizes until the desired accuracy is reached, this variable
		// limits the number of tries, higher values results better, but
		// slower output

typedef void (*DERIVFUNC)(float x0, float y0, float *rx, float *ry);

#ifdef __cplusplus
extern "C" {
#endif

void rkqs(float x0, float y0, DERIVFUNC derivfunc, float *xout, float *yout,
	float htry, float eps, float *hnext, int sign);

#ifdef __cplusplus
}
#endif
#endif

