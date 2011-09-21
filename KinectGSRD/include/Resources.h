#pragma once
#include "cinder/CinderResources.h"

#define RES_PASS_THRU_VERT	CINDER_RESOURCE(../resources/, passthru_vert.glsl, 128, GLSL)
#define RES_GSRD_FRAG		CINDER_RESOURCE(../resources/, gsrd_frag.glsl, 129, GLSL)
#define RES_REDLUM_FRAG		CINDER_RESOURCE(../resources/, redlum_frag.glsl, 131, GLSL)

#define RES_BLURV11_VERT  CINDER_RESOURCE(../resources/, blurv11_vert.glsl, 132, GLSL)
#define RES_BLURH11_VERT  CINDER_RESOURCE(../resources/, blurh11_vert.glsl, 133, GLSL)
#define RES_BLURV11_FRAG  CINDER_RESOURCE(../resources/, blurv11_frag.glsl, 134, GLSL)
#define RES_BLURH11_FRAG  CINDER_RESOURCE(../resources/, blurh11_frag.glsl, 135, GLSL)
#define RES_FREICHEN_FRAG  CINDER_RESOURCE(../resources/, freichen_frag.glsl, 136, GLSL)
#define RES_SOBEL_FRAG  CINDER_RESOURCE(../resources/, sobel_frag.glsl, 137, GLSL)

#define RES_SEED_FRAG  CINDER_RESOURCE(../resources/, seed_frag.glsl, 138, GLSL)

#define RES_ENVMAP_FRAG  CINDER_RESOURCE(../resources/, envmap_frag.glsl, 139, GLSL)
#define RES_ENVMAP_TEX  CINDER_RESOURCE(../resources/, envmap.jpg, 140, JPG)
