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
#include <cmath>

#include "cinder/app/AppBasic.h"
#include "cinder/Surface.h"
#include "Resources.h"

#include "Sobel.h"

using namespace ci;
using namespace ci::app;
using namespace ci::gl::ip;

Sobel::Sobel(int w, int h) :
	mObj(new Obj(w, h))
{
}

Sobel::Obj::Obj(int w, int h)
{
	fbo = gl::Fbo(w, h);
	shader = gl::GlslProg(loadResource(RES_PASS_THRU_VERT), loadResource(RES_SOBEL_FRAG));

	float fw = 1.0 / (float)(fbo.getWidth());
	float fh = 1.0 / (float)(fbo.getHeight());
	Vec2f offset[9];

	offset[0] = Vec2f( -fw, -fh);
	offset[1] = Vec2f(0.0, -fh);
	offset[2] = Vec2f(  fw, -fh);

	offset[3] = Vec2f( -fw, 0.0);
	offset[4] = Vec2f(0.0, 0.0);
	offset[5] = Vec2f(  fw, 0.0);

	offset[6] = Vec2f( -fw, fh);
	offset[7] = Vec2f(0.0, fh);
	offset[8] = Vec2f(  fw, fh);

	float G[] = {1.0, 2.0, 1.0, 0.0, 0.0, 0.0, -1.0, -2.0, -1.0,
				 1.0, 0.0, -1.0, 2.0, 0.0, -2.0, 1.0, 0.0, -1.0};

	shader.bind();
	shader.uniform("tex", 0);
	shader.uniform("offset", offset, 9);

	GLint loc = shader.getUniformLocation("G");
	glUniformMatrix3fv(loc, 2, GL_FALSE, G);
	shader.unbind();
}

gl::Texture & Sobel::process(const gl::Texture & source)
{
	Area viewport = gl::getViewport();
	gl::pushMatrices();

	// set orthographic projection with lower-left origin
	gl::setMatricesWindow(mObj->fbo.getSize(), false);
	gl::setViewport(mObj->fbo.getBounds());

	// draw
	mObj->fbo.bindFramebuffer();
	gl::color(Color::white());
	mObj->shader.bind();
	gl::draw(source, mObj->fbo.getBounds());
	mObj->shader.unbind();
	mObj->fbo.unbindFramebuffer();

	gl::popMatrices();
	gl::setViewport(viewport);
	return mObj->fbo.getTexture();
}

