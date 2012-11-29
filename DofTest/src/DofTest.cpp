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

 Depth of Field test based on by http://www.ro.me/tech/depth-of-field
*/

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/DisplayList.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/params/Params.h"
#include "cinder/Cinder.h"
#include "cinder/CinderMath.h"
#include "cinder/ImageIo.h"
#include "cinder/MayaCamUI.h"
#include "cinder/ObjLoader.h"
#include "cinder/TriMesh.h"

#include "CubeMap.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DofTest : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void keyDown( KeyEvent event );

		void mouseDown( MouseEvent event );
		void mouseDrag( MouseEvent event );

		void resize( ResizeEvent event );

		void update();
		void draw();

	private:
		gl::Fbo mFbo;
		void renderSceneToFbo();

		mndl::gl::CubeMap mCubeMap;

		TriMesh mTriMesh;
		gl::DisplayList mSpheres;
		gl::GlslProg mShader;

		MayaCamUI mMayaCam;

		params::InterfaceGl mParams;

		float mFps;

		float mFocus;
		float mAperture;
		float mMaxBlur;
};

void DofTest::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 640, 480 );
}

void DofTest::setup()
{
	gl::disableVerticalSync();

	ObjLoader loader( loadAsset( "sphere.obj" ) );
	loader.load( &mTriMesh );

	mSpheres = gl::DisplayList( GL_COMPILE );
	mSpheres.newList();

	const int xGrid = 19,
			  yGrid = 17,
			  zGrid = 19;
	int numObjects = xGrid * yGrid * zGrid;
	float hStep = 544.37f * 1.f / numObjects;
	float h = 0.f;

	for ( int z = 0; z < zGrid; z++ )
	{
		for ( int y = 0; y < yGrid; y++ )
		{
			for ( int x = 0; x < xGrid; x++ )
			{
				h += hStep;
				gl::pushModelView();
				gl::color( Color( CM_HSV, math< float>::fmod( h, 1.f ), 1.f, 1.f ) );
				gl::translate( 4 * Vec3f( ( x - xGrid / 2.f ),
										  ( y - yGrid / 2.f ),
										  ( z - zGrid / 2.f ) ) );
				gl::draw( mTriMesh );
				gl::popModelView();
			}
		}
	}
	gl::color( Color::white() );
	mSpheres.endList();

	mFbo = gl::Fbo( 1024, 768 );
	try
	{
		mShader = gl::GlslProg( loadAsset( "shaders/PassThroughVert.glsl" ),
								loadAsset( "shaders/DofFrag.glsl" ) );
	}
	catch ( const std::exception &exc )
	{
		console() << exc.what() << endl;
	}

	mShader.bind();
	mShader.uniform( "tColor", 0 );
	mShader.uniform( "tDepth", 1 );
	mShader.uniform( "size", Vec2f( mFbo.getSize() ) );

	mCubeMap = mndl::gl::CubeMap( loadImage( loadAsset( "cubemap/px.jpg" ) ),
								loadImage( loadAsset( "cubemap/py.jpg" ) ),
								loadImage( loadAsset( "cubemap/pz.jpg" ) ),
								loadImage( loadAsset( "cubemap/nx.jpg" ) ),
								loadImage( loadAsset( "cubemap/ny.jpg" ) ),
								loadImage( loadAsset( "cubemap/nz.jpg" ) ) );

	CameraPersp cam;
	cam.setPerspective( 60.f, getWindowAspectRatio(), 0.1f, 100.0f );
	mMayaCam.setCurrentCam( cam );

	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addSeparator();

	mFocus = 0.15f;
	mParams.addParam( "Focus", &mFocus, "min=0 max=1 step=.01" );
	mAperture = 0.035f;
	mParams.addParam( "Aperture", &mAperture, "min=0 max=.2 step=.001" );
	mMaxBlur = 1.f;
	mParams.addParam( "Max blur", &mMaxBlur, "min=0 max=3. step=.025" );
}

void DofTest::update()
{
	mFps = getAverageFps();
}

void DofTest::draw()
{
	gl::enableDepthRead();
	gl::enableDepthWrite();
	renderSceneToFbo();

	gl::disableDepthRead();
	gl::disableDepthWrite();

	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	gl::clear( Color::black() );
	mShader.bind();
	mFbo.getTexture().bind( 0 );
	mFbo.getDepthTexture().bind( 1 );

	mShader.uniform( "maxblur", mMaxBlur );
	mShader.uniform( "aperture", mAperture );
	mShader.uniform( "focus", mFocus );

	gl::drawSolidRect( getWindowBounds() );
	mShader.unbind();

	params::InterfaceGl::draw();
}

void DofTest::renderSceneToFbo()
{
	gl::SaveFramebufferBinding bindingSaver;

	mFbo.bindFramebuffer();

	gl::clear( Color::black() );

	gl::setViewport( mFbo.getBounds() );
	gl::setMatrices( mMayaCam.getCamera() );

	gl::enable( GL_NORMALIZE );
	mndl::gl::enableReflectionMapping();
	mCubeMap.bind();
	gl::pushMatrices();
	mSpheres.draw();
	gl::popMatrices();
	mCubeMap.unbind();
	mndl::gl::disableReflectionMapping();
}


void DofTest::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_f:
			if ( !isFullScreen() )
			{
				setFullScreen( true );
				if ( mParams.isVisible() )
					showCursor();
				else
					hideCursor();
			}
			else
			{
				setFullScreen( false );
				showCursor();
			}
			break;

		case KeyEvent::KEY_s:
			mParams.show( !mParams.isVisible() );
			if ( isFullScreen() )
			{
				if ( mParams.isVisible() )
					showCursor();
				else
					hideCursor();
			}
			break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

void DofTest::mouseDown( MouseEvent event )
{
	mMayaCam.mouseDown( event.getPos() );
}

void DofTest::mouseDrag( MouseEvent event )
{
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void DofTest::resize( ResizeEvent event )
{
	CameraPersp cam = mMayaCam.getCamera();
	cam.setAspectRatio( getWindowAspectRatio() );
	mMayaCam.setCurrentCam( cam );
}


CINDER_APP_BASIC( DofTest, RendererGl() )

