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

#include "cinder/Area.h"

#include "DepthMerge.h"

#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

DepthMerge::DepthMerge( App *app )
	: Effect( app )
{
}

void DepthMerge::setup()
{
	mShader = gl::GlslProg(loadResource(RES_PASSTHROUGH_VERT),
						   loadResource(RES_DEPTHMERGE_FRAG));
	mShader.bind();
	int depthUnits[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
	mShader.uniform("dtex", depthUnits, 8);
	int colorUnits[] = { 8, 9, 10, 11, 12, 13, 14, 15 };
	mShader.uniform("ctex", colorUnits, 8);
	mShader.unbind();

	mCurrentIndex = 0;

	// params
	mParams = params::PInterfaceGl("KezekLabak", Vec2i(200, 300));
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

	/*
	mParams.addPersistentParam( "Mirror", &mMirror, true, "key=m" );

	mParams.addSeparator();
	mParams.addButton("Screenshot", std::bind(&DepthMergeApp::saveScreenshot, this), "key=s");
	mParams.addButton("Start recording", std::bind(&DepthMergeApp::toggleRecording, this));

	// start OpenNI
	try
	{
		//mNI = OpenNI( OpenNI::Device() );

		string path = getAppPath().string();
#ifdef CINDER_MAC
		path += "/../";
#endif
		path += "rec-12022610062000.oni";
		mNI = OpenNI( path );
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
	*/
}

void DepthMerge::update()
{
	/*
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
	*/

	if (mNI.checkNewDepthFrame() && mNI.checkNewVideoFrame())
	{
		mCurrentIndex = (mCurrentIndex + 1) & (TEXTURE_COUNT - 1);

		gl::Texture::Format format;
		format.setMagFilter(GL_NEAREST);
		mDepthTextures[mCurrentIndex] = gl::Texture( mNI.getDepthImage(), format );

		mColorTextures[mCurrentIndex] = mNI.getVideoImage();
	}
}

void DepthMerge::draw()
{
	gl::clear(Color(0, 0, 0));
	// FIXME
	gl::setMatricesWindow( Vec2i( 1024, 768 ) ); //getWindowSize());

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

	// FIXME
	//gl::drawSolidRect(getWindowBounds());
	gl::drawSolidRect( Area(0, 0, 1024, 768) );

	// unbind textures
	for (int i = 0; i < 16; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	glActiveTexture(GL_TEXTURE0);

	mShader.unbind();
}

