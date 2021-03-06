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
#include "Utils.h"
#include "PParams.h"
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

		void saveScreenshot();
		void toggleRecording();

	private:
		OpenNI mNI;

		enum { TYPE_COLOR = 0, TYPE_IR, TYPE_DEPTH };

		//gl::Fbo mDepthTextures[TEXTURE_COUNT];
		gl::Texture mDepthTextures[TEXTURE_COUNT];
		gl::Texture mColorTextures[TEXTURE_COUNT];
		int mCurrentIndex;

		gl::GlslProg mShader;
		//gl::ip::BoxBlur mBoxBlur;

		ci::params::PInterfaceGl mParams;
		int mType;
		int mStepLog2;
		float mMinDepth;
		float mMaxDepth;
		bool mMirror;
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

	//mBoxBlur = gl::ip::BoxBlur( 640, 480 );

	// params
	string paramsXml = getAppPath().string();
#ifdef CINDER_MAC
	paramsXml += "/Contents/Resources/";
#endif
	paramsXml += "params.xml";
	params::PInterfaceGl::load( paramsXml );

	mParams = params::PInterfaceGl("Parameters", Vec2i(200, 300));
	mParams.addPersistentSizeAndPosition();

	const string typeArr[] = { "Color", "Infrared", "Depth" };
	const int typeSize = sizeof( typeArr ) / sizeof( typeArr[0] );
	std::vector<string> typeNames(typeArr, typeArr + typeSize);
	mParams.addPersistentParam( "Type", typeNames, &mType, 0 );

	const string stepArr[] = { "1", "2", "4", "8", "16", "32", "64", "128" };
	const int stepSize = sizeof( stepArr ) / sizeof( stepArr[0] );
	std::vector<string> log2Names(stepArr, stepArr + stepSize);
	mParams.addPersistentParam( "Step", log2Names, &mStepLog2, 4 );

	mParams.addPersistentParam( "Min depth", &mMinDepth, 0, "min=0 max=1 step=0.001 keyIncr=x keyDecr=X" );
	mParams.addPersistentParam( "Max depth", &mMaxDepth, 1, "min=0 max=1 step=0.001 keyIncr=c keyDecr=C" );

	mParams.addPersistentParam( "Mirror", &mMirror, true, "key=m" );

	mParams.addSeparator();
	mParams.addButton("Screenshot", std::bind(&DepthMergeApp::saveScreenshot, this), "key=s");
	mParams.addButton("Start recording", std::bind(&DepthMergeApp::toggleRecording, this));

	// start OpenNI
	try
	{
		mNI = OpenNI( OpenNI::Device() );

		/*
		string path = getAppPath().string();
#ifdef CINDER_MAC
		path += "/../";
#endif
		//path += "rec-12022610062000.oni";
		path += "rec-12032014223600.oni";
		mNI = OpenNI( path );
		*/
	}
	catch (...)
	{
		console() << "Could not open Kinect" << endl;
		quit();
	}

	mNI.setMirrored( mMirror );
	if (mType == TYPE_COLOR)
		mNI.setDepthAligned( true );
	mNI.start();

	enableVSync(false);
}

void DepthMergeApp::enableVSync(bool vs)
{
#if defined(CINDER_MAC)
	GLint vsync = vs;
	CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &vsync);
#endif
}

void DepthMergeApp::saveScreenshot()
{
	string path = getAppPath().string();
#ifdef CINDER_MAC
	path += "/../";
#endif
	path += "snap-" + timeStamp() + ".png";
	fs::path pngPath(path);

	try
	{
		Surface srf = copyWindowSurface();

		if (!pngPath.empty())
		{
			writeImage( pngPath, srf );
		}
	}
	catch ( ... )
	{
		console() << "unable to save image file " << path << endl;
	}
}

void DepthMergeApp::toggleRecording()
{
	if ( mNI.isRecording() )
	{
		mNI.stopRecording();
		mParams.setOptions( "Stop recording", " label='Start recording' " );
	}
	else
	{
		string path = getAppPath().string();
#ifdef CINDER_MAC
		path += "/../";
#endif
		path += "rec-" + timeStamp() + ".oni";
		fs::path oniPath(path);

		mNI.startRecording( oniPath );
		mParams.setOptions( "Start recording", " label='Stop recording' " );
	}
}

void DepthMergeApp::shutdown()
{
	params::PInterfaceGl::save();
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
	if (mMirror != mNI.isMirrored())
		mNI.setMirrored( mMirror );

	if (mType == TYPE_COLOR)
	{
		if (!mNI.isDepthAligned())
			mNI.setDepthAligned();

		if (mNI.isVideoInfrared())
			mNI.setVideoInfrared( false );
	}
	else
	if (mType == TYPE_DEPTH)
	{
		if (mNI.isDepthAligned())
			mNI.setDepthAligned( false );
	}
	else
	if (mType == TYPE_IR)
	{
		if (mNI.isDepthAligned())
			mNI.setDepthAligned( false );

		if (!mNI.isVideoInfrared())
			mNI.setVideoInfrared();
	}

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

	mShader.uniform("dbottom", mMinDepth);
	mShader.uniform("dtop", mMaxDepth);

	// bind textures
	int step = 1 << mStepLog2;
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
		switch (mType)
		{
			case TYPE_COLOR:
			case TYPE_IR:
				if (mColorTextures[idx])
					mColorTextures[idx].bind(i + 8);
				break;

			case TYPE_DEPTH:
				if (mDepthTextures[idx])
					mDepthTextures[idx].bind(i + 8);
				break;
		}
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

	params::PInterfaceGl::draw();
}

CINDER_APP_BASIC(DepthMergeApp, RendererGl)

