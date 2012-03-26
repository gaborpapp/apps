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
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/params/Params.h"

#include "NI.h"

#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DoFNIApp : public AppBasic
{
	public:
		DoFNIApp();

		void prepareSettings(Settings *settings);
		void setup();

		void keyDown(KeyEvent event);

		void update();
		void draw();

	private:
		params::InterfaceGl mParams;

		OpenNI mNI;
		gl::Texture mColorTxt;
		gl::Texture mDepthTxt;

		gl::GlslProg mShaderDof;

		float mDofAmount;
		float mFocus;
		float mFps;
};

DoFNIApp::DoFNIApp()
	: mFocus( 0.3 ),
	  mDofAmount( 70. )
{
}

void DoFNIApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize(640, 480);
}

void DoFNIApp::setup()
{
	gl::disableVerticalSync();

	mParams = params::InterfaceGl("Parameters", Vec2i(200, 300));

	mParams.addParam( "Dof amount", &mDofAmount, "min=1 max=300 step=.5" );
	mParams.addParam( "Focus", &mFocus, "min=0 max=1 step=.01" );
	mParams.addParam( "Fps", &mFps, "", true );

	// OpenNI
	try
	{
		//mNI = OpenNI( OpenNI::Device() );

		string path = getAppPath().string();
	#ifdef CINDER_MAC
		path += "/../";
	#endif
		path += "rec.oni";
		mNI = OpenNI( path );
	}
	catch (...)
	{
		console() << "Could not open Kinect" << endl;
		quit();
	}
	mNI.setMirrored( true );
	mNI.setDepthAligned();
	mNI.start();

	mShaderDof = gl::GlslProg( loadResource( RES_DOF_VERT ), loadResource( RES_DOF_FRAG ) );
	mShaderDof.bind();
	mShaderDof.uniform( "rgb", 0 );
	mShaderDof.uniform( "depth", 1 );
	mShaderDof.unbind();
}

void DoFNIApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());
	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void DoFNIApp::update()
{
	mFps = getAverageFps();

	if ( mNI.checkNewVideoFrame() )
		mColorTxt = mNI.getVideoImage();
	if ( mNI.checkNewDepthFrame() )
		mDepthTxt = mNI.getDepthImage();
}

void DoFNIApp::draw()
{
	gl::clear( Color::black() );

	if ( mDepthTxt && mColorTxt )
	{
		mColorTxt.bind( 0 );
		mDepthTxt.bind( 1 );

		mShaderDof.bind();
		mShaderDof.uniform( "amount", mDofAmount );
		mShaderDof.uniform( "focus", mFocus );
		mShaderDof.uniform( "isize", Vec2f( 1. / mColorTxt.getWidth(),
					1. / mColorTxt.getHeight() ) );

		gl::drawSolidRect( getWindowBounds() );

		mShaderDof.unbind();

		mColorTxt.unbind();
		mDepthTxt.unbind();
	}


	params::InterfaceGl::draw();
}

CINDER_APP_BASIC(DoFNIApp, RendererGl( RendererGl::AA_NONE ))

