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

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"

#include "Kinect.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DepthMergeApp : public ci::app::AppBasic
{
	public:
		void prepareSettings(Settings *settings);
		void setup();
		void enableVSync(bool vs);
		void shutdown();

		void keyDown(ci::app::KeyEvent event);
		void keyUp(ci::app::KeyEvent event);

		void update();
		void draw();

	private:
		Kinect mKinect;
		gl::Texture mDepthTexture;
};

void DepthMergeApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize(640, 480);
}

void DepthMergeApp::setup()
{
	try
	{
		mKinect = Kinect(Kinect::Device());
	}
	catch (Kinect::ExcFailedOpenDevice)
	{
		console() << "Could not open Kinect" << endl;
		quit();
	}
	mKinect.setVideoInfrared(true);

	enableVSync(false);
}

void DepthMergeApp::enableVSync(bool vs)
{
#if defined(CINDER_MAC)
	GLint vsync = vs;
	CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &vsync);
#endif
}

void DepthMergeApp::shutdown()
{
}

void DepthMergeApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());
	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void DepthMergeApp::keyUp(KeyEvent event)
{
}

void DepthMergeApp::update()
{
	if (mKinect.checkNewDepthFrame())
		mDepthTexture = mKinect.getDepthImage();
}

void DepthMergeApp::draw()
{
	gl::clear(Color(0, 0, 0));
	gl::setMatricesWindow(getWindowSize());
	if (mDepthTexture)
		gl::draw(mDepthTexture);
}

CINDER_APP_BASIC(DepthMergeApp, RendererGl)

