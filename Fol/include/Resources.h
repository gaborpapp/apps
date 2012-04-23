#pragma once
#include "cinder/CinderResources.h"

#define RES_PASSTHROUGH_VERT	CINDER_RESOURCE(../resources/, PassThrough.vert, 128, GLSL)
#define RES_BLUR_FRAG	CINDER_RESOURCE(../resources/, Blur.frag, 129, GLSL)
#define RES_BLUR_KERNEL	CINDER_RESOURCE(../resources/, kernel.png, 130, PNG)

#define RES_WAVE_VERT	CINDER_RESOURCE(../resources/, Wave.vert, 131, GLSL)
#define RES_WAVE_FRAG	CINDER_RESOURCE(../resources/, Wave.frag, 132, GLSL)

#define RES_KAWASE_BLOOM_VERT   CINDER_RESOURCE(../resources/, KawaseBloom.vert, 133, GLSL)
#define RES_KAWASE_BLOOM_FRAG   CINDER_RESOURCE(../resources/, KawaseBloom.frag, 134, GLSL)

#define RES_MIXER_FRAG   CINDER_RESOURCE(../resources/, Mixer.frag, 135, GLSL)

