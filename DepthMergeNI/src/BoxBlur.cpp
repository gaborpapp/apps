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
#include "cinder/app/AppBasic.h"
#include "Resources.h"

#include "BoxBlur.h"

using namespace ci;
using namespace ci::app;
using namespace ci::gl::ip;

BoxBlur::BoxBlur(int w, int h) :
	mObj(new Obj(w, h))
{
}

BoxBlur::Obj::Obj(int w, int h)
{
	gl::Fbo::Format format;
	format.enableColorBuffer(true, 2);
	fbo = gl::Fbo(w, h, format);

	shader = gl::GlslProg(loadResource(RES_PASSTHROUGH_VERT), loadResource(RES_BOXBLUR_FRAG));

	shader.bind();
	shader.uniform("tex", 0);
	shader.unbind();
}

gl::Texture & BoxBlur::process(const gl::Texture & source, float radius /* = 1. */)
{
	gl::pushMatrices();
	Area viewport = gl::getViewport();

	// set orthographic projection with lower-left origin
	gl::setMatricesWindow(mObj->fbo.getSize(), false);
	gl::setViewport(mObj->fbo.getBounds());

	mObj->fbo.bindFramebuffer();
	gl::color(Color::white());

	Vec2f voffset(0.0, 1.0 / float(mObj->fbo.getHeight()));
	Vec2f hoffset(1.0 / float(mObj->fbo.getWidth()), 0.0);

	// vertical
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	source.bind();
	mObj->shader.bind();
	mObj->shader.uniform("radius", radius);
	mObj->shader.uniform("offset_step", voffset);
	gl::drawSolidRect(mObj->fbo.getBounds());
	mObj->shader.unbind();

	// horizontal
	glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
	mObj->fbo.bindTexture(0, 0);
	mObj->shader.bind();
	mObj->shader.uniform("offset_step", hoffset);
	gl::drawSolidRect(mObj->fbo.getBounds());
	mObj->shader.unbind();

	mObj->fbo.unbindFramebuffer();

	gl::popMatrices();
	gl::setViewport(viewport);
	return mObj->fbo.getTexture(1);
}

