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

#include "KinectCalibrationApp.h"

using namespace ci;
using namespace ci::app;
using namespace std;

void KinectCalibrationApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize(640, 480);
}

void KinectCalibrationApp::setup()
{
	enableVSync(false);
}

void KinectCalibrationApp::enableVSync(bool vs)
{
#if defined(CINDER_MAC)
	GLint vsync = vs;
	CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &vsync);
#endif
}

void KinectCalibrationApp::shutdown()
{
}

void KinectCalibrationApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());
	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void KinectCalibrationApp::keyUp(KeyEvent event)
{
}

void KinectCalibrationApp::update()
{
}

void KinectCalibrationApp::draw()
{
	gl::clear(Color(0, 0, 0));
}

CINDER_APP_BASIC(KinectCalibrationApp, RendererGl)

