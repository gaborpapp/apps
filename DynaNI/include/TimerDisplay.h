#pragma once

#include <string>

#include "cinder/Datasource.h"
#include "cinder/gl/Texture.h"

class TimerDisplay
{
	public:
		TimerDisplay() {}
		TimerDisplay( const std::string &bottomLeft,
					  const std::string &bottomMiddle,
					  const std::string &bottomRight,
					  const std::string &dot0,
					  const std::string &dot1 );

		void draw( float u );

	private:
		ci::gl::Texture mBottomLeft;
		ci::gl::Texture mBottomMiddle;
		ci::gl::Texture mBottomRight;
		ci::gl::Texture mDot0;
		ci::gl::Texture mDot1;
};

