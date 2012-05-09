/*
 Copyright (C) 2012 Gabor Papp

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

#include "Sharpen.h"

using namespace ci;
using namespace ci::app;
using namespace ci::gl::ip;

const char *Sharpen::sVertexShaderGlsl =
	"void main()\n"
	"{\n"
		"gl_Position = ftransform();\n"
		"gl_TexCoord[0] = gl_MultiTexCoord0;\n"
	"}\n";

const char *Sharpen::sFragmentShaderGlsl =
	"const int kernelSize = 9;\n"
	// array of offsets for accessing the base image
	"uniform vec2 offset[kernelSize];\n"
	// value for each location in the convolution kernel
	"uniform float kernel[kernelSize];\n"
	// scaling factor for edge image
	"uniform float strength;\n"
	"uniform sampler2D tex;\n"

	"void main()\n"
	"{\n"
		"vec4 sum = vec4( .0 );\n"
		"for ( int i = 0; i < kernelSize; i++ )\n"
		"{\n"
			"vec4 tmp = texture2D( tex, gl_TexCoord[0].st + offset[i] );\n"
			"sum += tmp * kernel[i];\n"
		"}\n"

		"vec4 baseColor = texture2D( tex, vec2(gl_TexCoord[0]) );\n"
		"gl_FragColor = strength * sum + baseColor;\n"
	"}\n";

Sharpen::Sharpen(int w, int h) :
	mObj(new Obj(w, h))
{
}

Sharpen::Obj::Obj(int w, int h)
{
	fbo = gl::Fbo(w, h);
	shader = gl::GlslProg( sVertexShaderGlsl, sFragmentShaderGlsl );

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

	float kernel[] = {	 0., -1, 0,
						-1., 4., -1.,
						 0, -1., 0. };

	shader.bind();
	shader.uniform("tex", 0);
	shader.uniform("offset", offset, 9);
	shader.uniform("kernel", kernel, 9);
	shader.unbind();
}

gl::Texture & Sharpen::process(const gl::Texture & source, float strength)
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
	mObj->shader.uniform("strength", strength);
	gl::draw(source, mObj->fbo.getBounds());
	mObj->shader.unbind();
	mObj->fbo.unbindFramebuffer();

	gl::popMatrices();
	gl::setViewport(viewport);
	return mObj->fbo.getTexture();
}

