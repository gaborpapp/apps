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

#include "GaussianBlur.h"

using namespace ci;
using namespace ci::app;
using namespace ci::gl::ip;

GaussianBlur::GaussianBlur(int w, int h) :
	mObj(new Obj(w, h))
{
}

GaussianBlur::Obj::Obj(int w, int h)
{
	gl::Fbo::Format format;
	format.enableColorBuffer(true, 3);
	fbo = gl::Fbo(w, h, format);

	shader_vpass = gl::GlslProg(loadResource(RES_BLURV11_VERT), loadResource(RES_BLURV11_FRAG));
	shader_hpass = gl::GlslProg(loadResource(RES_BLURH11_VERT), loadResource(RES_BLURH11_FRAG));

	shader_vpass.bind();
	shader_vpass.uniform("tex", 0);
	shader_vpass.uniform("width", float(fbo.getWidth()));
	shader_vpass.unbind();

	shader_hpass.bind();
	shader_hpass.uniform("tex", 0);
	shader_hpass.uniform("width", float(fbo.getWidth()));
	shader_hpass.unbind();
}

gl::Texture & GaussianBlur::process(const gl::Texture & source, int n /* = 1 */)
{
	gl::pushMatrices();
	Area viewport = gl::getViewport();

	// set orthographic projection with lower-left origin
	gl::setMatricesWindow(mObj->fbo.getSize(), false);
	gl::setViewport(mObj->fbo.getBounds());

	// draw
	mObj->fbo.bindFramebuffer();
	glDrawBuffer(GL_COLOR_ATTACHMENT2_EXT);
	gl::color(Color::white());
	gl::draw(source, mObj->fbo.getBounds());

	// blur
	for (int i = 0; i < n; i++)
	{
		// vertical
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
		mObj->fbo.bindTexture(0, (i == 0) ? 2 : 1);
		mObj->shader_vpass.bind();
		gl::drawSolidRect(mObj->fbo.getBounds());
		mObj->shader_vpass.unbind();

		// horizontal
		glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
		mObj->fbo.bindTexture(0, 0);
		mObj->shader_hpass.bind();
		gl::drawSolidRect(mObj->fbo.getBounds());
		mObj->shader_hpass.unbind();
	}
	mObj->fbo.unbindFramebuffer();

	gl::popMatrices();
	gl::setViewport(viewport);
	return mObj->fbo.getTexture((n == 0) ? 2 : 1);
}

