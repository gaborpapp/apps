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

#ifndef GSRD_APP_H
#define GSRD_APP_H

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/params/Params.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Font.h"

#include "Kinect.h"

#include "GaussianBlur.h"

class KinectGSRDApp : public ci::app::AppBasic
{
	public:
		KinectGSRDApp();

		void prepareSettings(Settings *settings);

		void setup();
		void shutdown();

		void keyDown(ci::app::KeyEvent event);
		void keyUp(ci::app::KeyEvent event);

		void mouseDown(ci::app::MouseEvent event);
		void mouseUp(ci::app::MouseEvent event);
		void mouseMove(ci::app::MouseEvent event);
		void mouseDrag(ci::app::MouseEvent event);

		void update();
		void draw();

	private:
		bool mouse_left;
		ci::Vec2i mouse_pos;

		ci::params::InterfaceGl params;

		float dU;
		float dV;
		float k;
		float f;

		ci::Kinect kinect;
		ci::gl::Texture kinect_depth;

		float depth_threshold;

		int blur_n;
		bool show_seed;
		bool show_fps;
		bool show_greyscale;

		ci::gl::Fbo fbo;
		static const int FBO_WIDTH = 320;
		static const int FBO_HEIGHT = 240;
		int ping_pong;

		ci::gl::GlslProg shader_gsrd;
		ci::gl::GlslProg shader_redlum;

		void reset();

		ci::Font font;

		void enableVSync(bool vs);

		ci::gl::ip::GaussianBlur blur;
		ci::gl::GlslProg shader_seed;

		// envmap
		ci::gl::GlslProg shader_envmap;
		ci::gl::Texture envmap_tex;
		float xoffset, yoffset;
};

#endif

