#pragma once

#include "cinder/Cinder.h"
#include "cinder/gl/Texture.h"
#include "cinder/qtime/QuickTime.h"
#include "cinder/Timeline.h"

#include "PParams.h"
#include "Effect.h"

class Moon : public Effect
{
	public:
		Moon( ci::app::App *app );

		void setup();

		void instantiate();
		void deinstantiate();

		void update();
		void draw();

	private:
		ci::gl::Texture mFrameTexture;
		ci::qtime::MovieGl mMovie;

		uint32_t mStartFrame;

		float mRate;
		float mFadeDuration;

		void startMovie();
		void fadeOut();

		ci::Anim<float> mFade;
};

