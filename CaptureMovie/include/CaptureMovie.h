#ifndef CAPTURE_APP_H
#define CAPTURE_APP_H

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"
#include "cinder/Capture.h"
#include "cinder/qtime/MovieWriter.h"

class CaptureMovie : public ci::app::AppBasic
{
	public:
		void setup();
		void keyDown(ci::app::KeyEvent event);

		void update();
		void draw();

	private:
		ci::Capture mCapture;
		ci::gl::Texture mTexture;
		ci::qtime::MovieWriter mMovieWriter;

		std::string timeStamp();
};

#endif

