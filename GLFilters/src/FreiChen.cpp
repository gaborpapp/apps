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

#include "FreiChen.h"

using namespace ci;
using namespace ci::app;
using namespace ci::gl::ip;

FreiChen::FreiChen(int w, int h) :
	mObj(new Obj(w, h))
{
}

FreiChen::Obj::Obj(int w, int h)
{
	fbo = gl::Fbo(w, h);
	shader = gl::GlslProg(loadResource(RES_PASSTHROUGH_VERT), loadResource(RES_FREICHEN_FRAG));

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

	// FIXME: math expressions instead of numbers
	// mat3 instead of vec3
	Vec3f G[27];
	G[0] = Vec3f(0.353553390593, 0.5, 0.353553390593);
	G[1] = Vec3f(0.0, 0.0, 0.0);
	G[2] = Vec3f(-0.353553390593, -0.5, -0.353553390593);
	G[3] = Vec3f(0.353553390593, 0.0, -0.353553390593);
	G[4] = Vec3f(0.5, 0.0, -0.5);
	G[5] = Vec3f(0.353553390593, 0.0, -0.353553390593);
	G[6] = Vec3f(0.0, -0.353553390593, 0.5);
	G[7] = Vec3f(0.353553390593, 0.0, -0.353553390593);
	G[8] = Vec3f(-0.5, 0.353553390593, 0.0);
	G[9] = Vec3f(0.5, -0.353553390593, 0.0);
	G[10] = Vec3f(-0.353553390593, 0.0, 0.353553390593);
	G[11] = Vec3f(0.0, 0.353553390593, -0.5);
	G[12] = Vec3f(0.0, 0.5, 0.0);
	G[13] = Vec3f(-0.5, 0.0, -0.5);
	G[14] = Vec3f(0.0, 0.5, 0.0);
	G[15] = Vec3f(-0.5, 0.0, 0.5);
	G[16] = Vec3f(0.0, 0.0, 0.0);
	G[17] = Vec3f(0.5, 0.0, -0.5);
	G[18] = Vec3f(0.166666666667, -0.333333333333, 0.166666666667);
	G[19] = Vec3f(-0.333333333333, 0.666666666667, -0.333333333333);
	G[20] = Vec3f(0.166666666667, -0.333333333333, 0.166666666667);
	G[21] = Vec3f(-0.333333333333, 0.166666666667, -0.333333333333);
	G[22] = Vec3f(0.166666666667, 0.666666666667, 0.166666666667);
	G[23] = Vec3f(-0.333333333333, 0.166666666667, -0.333333333333);
	G[24] = Vec3f(0.333333333333, 0.333333333333, 0.333333333333);
	G[25] = Vec3f(0.333333333333, 0.333333333333, 0.333333333333);
	G[26] = Vec3f(0.333333333333, 0.333333333333, 0.333333333333);

	shader.bind();
	shader.uniform("tex", 0);
	shader.uniform("offset", offset, 9);
	shader.uniform("G", G, 27);
	shader.unbind();
}

gl::Texture & FreiChen::process(const gl::Texture & source)
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

