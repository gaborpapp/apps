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

#include "GSApp.h"

using namespace ci;
using namespace ci::app;
using namespace std;

void GSApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize(512, 512);
}

void GSApp::setup()
{
	gs = new GrayScott(WIDTH, HEIGHT);

	mReactionU = 0.16f;
	mReactionV = 0.08f;
	mReactionK = 0.077f;
	mReactionF = 0.023f;

	params = params::InterfaceGl( "Parameters", Vec2i( 175, 100 ) );
	params.addParam( "Reaction u", &mReactionU, "min=0.0 max=0.4 step=0.01 keyIncr=u keyDecr=U" );
	params.addParam( "Reaction v", &mReactionV, "min=0.0 max=0.4 step=0.01 keyIncr=v keyDecr=V" );
	params.addParam( "Reaction k", &mReactionK, "min=0.0 max=1.0 step=0.001 keyIncr=k keyDecr=K" );
	params.addParam( "Reaction f", &mReactionF, "min=0.0 max=1.0 step=0.001 keyIncr=f keyDecr=F" );
}

void GSApp::shutdown()
{
	delete gs;
}

void GSApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());
	else
	if (event.getChar() == 'r')
		gs->reset();
	else
	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void GSApp::mouseDown(MouseEvent event)
{
	RectMapping mapping(getWindowBounds(), Area(0, 0, WIDTH, HEIGHT));
	Vec2f p = mapping.map(event.getPos());
	gs->set_rect(p.x, p.y, 10, 10);
}

void GSApp::mouseDrag(MouseEvent event)
{
	RectMapping mapping(getWindowBounds(), Area(0, 0, WIDTH, HEIGHT));
	Vec2f p = mapping.map(event.getPos());
	gs->set_rect(p.x, p.y, 10, 10);
}

void GSApp::update()
{
	gs->set_coefficients(mReactionF, mReactionK, mReactionU, mReactionV);
	gs->update();
}

void GSApp::draw()
{
	gl::clear(Color(0, 1, 0));

	/*
	Channel32f ch(WIDTH, HEIGHT);
	console() << int(ch.getWidth()) << endl;
	float *data = ch.getData();
	for (int i = 0; i < WIDTH * HEIGHT; i++)
	{
		*data = gs->u[i];
		data++;
	}
	*/
	Channel8u ch(WIDTH, HEIGHT);
	uint8_t *data = ch.getData();
	for (int i = 0; i < WIDTH * HEIGHT; i++)
	{
		*data = gs->u[i] * 255;
		data++;
	}

	/*
	gl::color(Color::white());
	gl::Texture::Format format;
	format.setInternalFormat(GL_LUMINANCE);
	gl::Texture gstxt(WIDTH, HEIGHT, format);
	gstxt.update(ch, ch.getBounds());
	*/
	gl::draw(ch, getWindowBounds());

	params::InterfaceGl::draw();
}


CINDER_APP_BASIC(GSApp, RendererGl)

