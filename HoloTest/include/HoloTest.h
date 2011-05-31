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

#ifndef HOLO_TEST_H
#define HOLO_TEST_H

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/params/Params.h"
#include "cinder/ObjLoader.h"
#include "cinder/Camera.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Fbo.h"
#include "cinder/Font.h"

class HoloTest : public ci::app::AppBasic
{
	public:
		void prepareSettings(Settings *settings);
		void setup();
		void shutdown();

		void resize(ci::app::ResizeEvent event);

		void keyDown(ci::app::KeyEvent event);
		void keyUp(ci::app::KeyEvent event);
		void mouseDown(ci::app::MouseEvent event);
		void mouseDrag(ci::app::MouseEvent event);

		void update();
		void draw();

	private:
		static const int FBO_WIDTH = 512;
		static const int FBO_HEIGHT = 512;
		ci::gl::Fbo fbo;

		ci::CameraPersp cams[5];

		ci::TriMesh mesh;
		ci::gl::VboMesh vbo;
		ci::gl::GlslProg shader;

		ci::params::InterfaceGl params;

		float obj_size;
		ci::Quatf obj_rotation;
		ci::Quatf obj_auto_rotation;
		bool auto_rotate;
		ci::ColorA obj_color;
		float cam_fov;
		bool label_views;
		int display_mode;

		void draw_scene(int cam_index);
		enum CAMERA_VIEW {
			VIEW_FRONT = 0,
			VIEW_LEFT,
			VIEW_BACK,
			VIEW_RIGHT,
			VIEW_MAIN
		};
		enum DISPLAY_MODE {
			DISPLAY_MAIN = 0,
			DISPLAY_VIEWS,
			DISPLAY_PYRAMID
		};

		ci::Font font;
};

#endif

