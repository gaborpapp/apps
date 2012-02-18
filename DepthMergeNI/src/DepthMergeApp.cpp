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
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"

#include "NI.h"
#include "BoxBlur.h"
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

		//gl::Fbo mDepthTextures[TEXTURE_COUNT];
		gl::Texture mDepthTextures[TEXTURE_COUNT];
		gl::Texture mColorTextures[TEXTURE_COUNT];
		int mCurrentIndex;

		gl::GlslProg mShader;
		gl::ip::BoxBlur mBoxBlur;

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

	mShader = gl::GlslProg(loadResource(RES_PASSTHROUGH_VERT),
						   loadResource(RES_DEPTHMERGE_FRAG));
	mShader.bind();
	int depthUnits[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	mShader.uniform("dtex", depthUnits, 8);
	int colorUnits[] = { 8, 9, 10, 11, 12, 13, 14, 15 };
	mShader.uniform("ctex", colorUnits, 8);
	mShader.unbind();

	mCurrentIndex = 0;

	try
	{
		mNI = OpenNI( OpenNI::Device() );
	}
	catch (...)
	{
		console() << "Could not open Kinect" << endl;
		quit();
	}
	mNI.calibrateDepthToRGB( true );
	mNI.setMirror( true );

	/*
	gl::Fbo::Format fboFormat;
	fboFormat.enableDepthBuffer(false);
	fboFormat.setMagFilter(GL_NEAREST);
	for (int i = 0; i < TEXTURE_COUNT; i++)
	{
		mDepthTextures[i] = gl::Fbo( 640, 480, fboFormat );
		mDepthTextures[i].bindFramebuffer();
		gl::clear();
		mDepthTextures[i].unbindFramebuffer();
	}
	*/

	mBoxBlur = gl::ip::BoxBlur( 640, 480 );

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
	if (mNI.checkNewDepthFrame() && mNI.checkNewVideoFrame())
	{
		mCurrentIndex = (mCurrentIndex + 1) & (TEXTURE_COUNT - 1);

		/*
		gl::Texture::Format format;
		format.setMagFilter(GL_LINEAR);

		gl::Texture blurredDepth = mBoxBlur.process( gl::Texture( mNI.getDepthImage(), format ), .0);

		mDepthTextures[mCurrentIndex].bindFramebuffer();
		gl::setMatricesWindow(mDepthTextures[mCurrentIndex].getSize(), false);
		gl::setViewport(mDepthTextures[mCurrentIndex].getBounds());

		gl::draw( blurredDepth );

		mDepthTextures[mCurrentIndex].unbindFramebuffer();
		*/

		gl::Texture::Format format;
		format.setMagFilter(GL_NEAREST);
		mDepthTextures[mCurrentIndex] = gl::Texture( mNI.getDepthImage(), format );

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
		/*
		if (mDepthTextures[idx])
			mDepthTextures[idx].getTexture().bind(i);
		*/
		if (mDepthTextures[idx])
			mDepthTextures[idx].bind(i);
		//*
		if (mColorTextures[idx])
			mColorTextures[idx].bind(i + 8);
		//*/
		/*
		if (mDepthTextures[idx])
			mDepthTextures[idx].bind(i + 8);
		*/
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

