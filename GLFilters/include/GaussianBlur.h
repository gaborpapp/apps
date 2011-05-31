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

#ifndef GAUSSIAN_BLUR_H
#define GAUSSIAN_BLUR_H

#include "cinder/Cinder.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"

namespace cinder { namespace gl { namespace ip {

class GaussianBlur
{
	public:
		GaussianBlur(int w, int h);
		GaussianBlur() {}

		ci::gl::Texture & process(const ci::gl::Texture & source, int n = 1);

	protected:
		struct Obj
		{
			Obj() {}
			Obj(int w, int h);

			ci::gl::Fbo fbo;
			ci::gl::GlslProg shader_vpass, shader_hpass;
		};
		std::shared_ptr<Obj> mObj;

};

}}} // namespace cinder::gl::ip

#endif

