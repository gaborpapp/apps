/*
 Copyright (C) 2012 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "cinder/Cinder.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Material.h"

#include "AssimpLoader.h"

#include "GlobalData.h"
#include "fsExp.h"

namespace mndl
{

class Devil: public fsExp
{
	public:
		Devil() : fsExp( "Devil" ) {}

		void draw();

		void setup();
		void setupParams();

		void update();

	private:
		ci::gl::GlslProg mShader;
		ci::gl::Material mMaterial;

		float mDevilness;

		mndl::assimp::AssimpLoader mAiHorns;
};

} // namespace mndl
