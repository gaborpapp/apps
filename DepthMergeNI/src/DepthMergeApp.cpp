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

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"

#include "NI.h"
#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#define TEXTURE_COUNT 128

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
		OpenNI mNI;

		gl::Texture mDepthTextures[TEXTURE_COUNT];
		gl::Texture mColorTextures[TEXTURE_COUNT];
		int mCurrentIndex;

		gl::GlslProg mShader;
};

void DepthMergeApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize(640, 480);
}

void DepthMergeApp::setup()
{
	int maxTextureUnits;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
	console() << "Max texture units: " << maxTextureUnits << endl;

	mShader = gl::GlslProg(loadResource(RES_DEPTHMERGE_VERT),
						   loadResource(RES_DEPTHMERGE_FRAG));
	mShader.bind();
	int depthUnits[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	mShader.uniform("dtex", depthUnits, 8);
	int colorUnits[] = { 8, 9, 10, 11, 12, 13, 14, 15 };
	mShader.uniform("ctex", colorUnits, 8);
	mShader.unbind();

	mCurrentIndex = 0;

	mNI = OpenNI( OpenNI::Device() );
	mNI.calibrateDepthToRGB( true );
	/*
	try
	{
		mKinect = Kinect(Kinect::Device());
	}
	catch (Kinect::ExcFailedOpenDevice)
	{
		console() << "Could not open Kinect" << endl;
		quit();
	}

	// without this setVideoInfrared() locks up the app
	while (!mKinect.checkNewVideoFrame()) ;

	mKinect.setVideoInfrared(true);
	*/

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
	/*
	if ( mNI.checkNewDepthFrame() )
		mDepthTextures[0] = mNI.getDepthImage();

	if ( mNI.checkNewVideoFrame() )
		mColorTextures[0] = mNI.getVideoImage();
	*/
	if (mNI.checkNewDepthFrame() && mNI.checkNewVideoFrame())
	{
		mCurrentIndex = (mCurrentIndex + 1) & (TEXTURE_COUNT - 1);
		mDepthTextures[mCurrentIndex] = mNI.getDepthImage();
		mColorTextures[mCurrentIndex] = mNI.getVideoImage();
	}
}

void DepthMergeApp::draw()
{
	gl::clear(Color(0, 0, 0));
	gl::setMatricesWindow(getWindowSize());

	/*
	gl::enableAlphaBlending();
	gl::disableDepthRead();
	gl::disableDepthWrite();

	if (mColorTextures[0])
	{
		gl::color(ColorA(1, 1, 1, 0.5));
		gl::draw(mColorTextures[0]);
	}
	if (mDepthTextures[0])
	{
		gl::color(ColorA(1, 1, 1, 0.5));
		gl::draw(mDepthTextures[0]);
	}
	*/

	mShader.bind();

	// bind textures
	int step = TEXTURE_COUNT / 8;
	int idx = mCurrentIndex;
	for (int i = 0; i < 8; i++)
	{
		if (mDepthTextures[idx])
			mDepthTextures[idx].bind(i);
		if (mColorTextures[idx])
			mColorTextures[idx].bind(i + 8);
		idx = (idx - step) & (TEXTURE_COUNT - 1);
	}

	gl::drawSolidRect(getWindowBounds());

	// unbind textures
	for (int i = 0; i < 16; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	glActiveTexture(GL_TEXTURE0);

	mShader.unbind();
}

CINDER_APP_BASIC(DepthMergeApp, RendererGl)

