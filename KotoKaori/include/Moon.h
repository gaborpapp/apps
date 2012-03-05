#pragma once

#include "cinder/Cinder.h"
#include "cinder/gl/Texture.h"
#include "cinder/qtime/QuickTime.h"

#include "PParams.h"
#include "Effect.h"

class Moon : public Effect
{
	public:
		Moon( ci::app::App *app );

		void setup();

		void deinstantiate();

		void update();
		void draw();

	private:
		ci::gl::Texture mFrameTexture;
		ci::qtime::MovieGl mMovie;

		float mRate;

		void startMovie();
};

