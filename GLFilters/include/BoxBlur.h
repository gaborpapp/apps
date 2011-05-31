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

#ifndef BOX_BLUR_H
#define BOX_BLUR_H

#include "cinder/Cinder.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"

namespace cinder { namespace gl { namespace ip {

class BoxBlur
{
	public:
		BoxBlur(int w, int h);
		BoxBlur() {}

		ci::gl::Texture & process(const ci::gl::Texture & source, float radius = 1.);

	protected:
		struct Obj
		{
			Obj() {}
			Obj(int w, int h);

			ci::gl::Fbo fbo;
			ci::gl::GlslProg shader;
		};
		std::shared_ptr<Obj> mObj;

};

}}} // namespace cinder::gl::ip

#endif

