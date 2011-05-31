/*
 Copyright (C) 2011 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef BLUR_APP_H
#define BLUR_APP_H

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/Capture.h"
#include "cinder/params/Params.h"
#include "cinder/Font.h"

#include "GaussianBlur.h"
#include "Sobel.h"
#include "FreiChen.h"
#include "BoxBlur.h"

class FilterApp : public ci::app::AppBasic
{
	public:
		void prepareSettings(Settings *settings);
		void setup();
		void shutdown();
		void enableVSync(bool vs);

		void update();
		void draw();

		void keyDown(ci::app::KeyEvent event);

	private:
		ci::Capture capture;

		ci::gl::ip::BoxBlur boxblur;
		ci::gl::ip::GaussianBlur blur;
		ci::gl::ip::Sobel sobel;
		ci::gl::ip::FreiChen frei_chen;

		ci::params::InterfaceGl	params;
		bool apply_boxblur;
		float boxblur_radius;
		int blur_n;
		bool apply_blur;
		bool apply_sobel;
		bool apply_frei_chen;

		ci::Font font;

		static const int WIDTH = 640;
		static const int HEIGHT = 480;
};

#endif

