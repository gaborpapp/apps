#pragma once
#include "cinder/CinderResources.h"

#define RES_KAWASE_BLOOM_VERT	CINDER_RESOURCE(../resources/, shaders/KawaseBloom.vert, 128, GLSL)
#define RES_KAWASE_BLOOM_FRAG	CINDER_RESOURCE(../resources/, shaders/KawaseBloom.frag, 129, GLSL)

#define RES_PASSTHROUGH_VERT	CINDER_RESOURCE(../resources/, shaders/PassThrough.vert, 131, GLSL)
#define RES_MIXER_FRAG			CINDER_RESOURCE(../resources/, shaders/Mixer.frag, 133, GLSL)
#define RES_DOF_FRAG			CINDER_RESOURCE(../resources/, shaders/DoF.frag, 134, GLSL)

#define RES_GALLERY_FRAG		CINDER_RESOURCE(../resources/, shaders/Gallery.frag, 148, GLSL)

#define RES_SHUTTER				CINDER_RESOURCE(../resources/, audio/72714__horsthorstensen__shutter-photo.mp3, 135, MP3)

#define RES_TIMER_GAME_BOTTOM_LEFT		CINDER_RESOURCE(../resources/, gfx/game/game-bottom-left.png, 136, PNG)
#define RES_TIMER_GAME_BOTTOM_MIDDLE	CINDER_RESOURCE(../resources/, gfx/game/game-bottom-middle.png, 137, PNG)
#define RES_TIMER_GAME_BOTTOM_RIGHT		CINDER_RESOURCE(../resources/, gfx/game/game-bottom-right.png, 138, PNG)
#define RES_TIMER_GAME_DOT_0			CINDER_RESOURCE(../resources/, gfx/game/game-dot-0.png, 139, PNG)
#define RES_TIMER_GAME_DOT_1			CINDER_RESOURCE(../resources/, gfx/game/game-dot-1.png, 140, PNG)

#define RES_TIMER_POSE_BOTTOM_LEFT		CINDER_RESOURCE(../resources/, gfx/pose/pose-bottom-left.png, 141, PNG)
#define RES_TIMER_POSE_BOTTOM_MIDDLE	CINDER_RESOURCE(../resources/, gfx/pose/pose-bottom-middle.png, 142, PNG)
#define RES_TIMER_POSE_BOTTOM_RIGHT		CINDER_RESOURCE(../resources/, gfx/pose/pose-bottom-right.png, 143, PNG)
#define RES_TIMER_POSE_DOT_0			CINDER_RESOURCE(../resources/, gfx/pose/pose-dot-0.png, 144, PNG)
#define RES_TIMER_POSE_DOT_1			CINDER_RESOURCE(../resources/, gfx/pose/pose-dot-1.png, 145, PNG)

#define RES_CURSOR_LEFT		CINDER_RESOURCE(../resources/, gfx/cursors/lefthand.png, 146, PNG)
#define RES_CURSOR_RIGHT	CINDER_RESOURCE(../resources/, gfx/cursors/righthand.png, 147, PNG)

#define RES_WATERMARK		CINDER_RESOURCE(../resources/, gfx/ilovetelekom.png, 149, PNG)

