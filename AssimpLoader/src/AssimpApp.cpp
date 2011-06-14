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

#include "cinder/ImageIo.h"
#include "cinder/gl/Light.h"

#include "AssimpLoader.h"
#include "Resources.h"

#include "AssimpApp.h"

using namespace ci;
using namespace ci::app;
using namespace std;

void AssimpApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize(640, 480);
}

void AssimpApp::setup()
{
	AssimpLoader loader((DataSourceRef)loadResource(RES_OBJ));
	loader.load(&mesh);

	txt = loadImage(loadResource(RES_TXT));

	//gl::enable(GL_LIGHTING);

	enableVSync(false);
}

void AssimpApp::enableVSync(bool vs)
{
#if defined(CINDER_MAC)
	GLint vsync = vs;
	CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &vsync);
#endif
}

void AssimpApp::resize(ResizeEvent event)
{
	camera.setPerspective(60., getWindowAspectRatio(), 1, 1000);
}

void AssimpApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());
	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void AssimpApp::update()
{
}

void AssimpApp::draw()
{
	gl::clear(Color(0, 0, 0));

	gl::setMatrices(camera);

	gl::enableDepthWrite();
	gl::enableDepthRead();

	gl::Light light(gl::Light::DIRECTIONAL, 0);
	light.setDirection(Vec3f(1, 1, 1).normalized());
	light.setAmbient(Color::black());
	light.update(camera);
	light.enable();

	//glEnable(GL_TEXTURE_2D);
	//txt.bind();
	gl::rotate(Vec3f(0, getElapsedSeconds() * 20., 0));
	//gl::scale(Vec3f(4., 4., 4.));
	gl::translate(Vec3f(0, -5, 0));
	gl::color(Color::white());
	gl::draw(mesh);
	//txt.unbind();
	//glDisable(GL_TEXTURE_2D);
	light.disable();
}

CINDER_APP_BASIC(AssimpApp, RendererGl)

