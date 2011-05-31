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

#ifndef GS_APP_H
#define GS_APP_H

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/params/Params.h"

#include "GrayScott.h"

class GSApp : public ci::app::AppBasic
{
	public:
		void prepareSettings(Settings *settings);
		void setup();
		void shutdown();

		void keyDown(ci::app::KeyEvent event);
		void mouseDown(ci::app::MouseEvent event);
		void mouseDrag(ci::app::MouseEvent event);

		void update();
		void draw();

	private:
		GrayScott *gs;

		static const int WIDTH = 256;
		static const int HEIGHT = 256;

		ci::params::InterfaceGl	params;
		float mReactionU;
		float mReactionV;
		float mReactionK;
		float mReactionF;
};

#endif

