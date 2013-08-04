/*
 Copyright (C) 2013 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"
#include "cinder/Ray.h"
#include "cinder/Rect.h"

#include "GridManager.h"
#include "HomogeneousRectangularGridTorsion.h"
#include "PanZoomCamUI.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class VoronoiGridApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void resize();

		void mouseDown( MouseEvent event );
		void mouseDrag( MouseEvent event );
		void keyDown( KeyEvent event );

		void update();
		void draw();

	private:
		params::InterfaceGl mParams;

		float mFps;
		bool mVerticalSyncEnabled = false;

		PanZoomCamUI mPanZoomCam;

		GridManagerRef mGridManagerRef;
		bool calcCurrentGridBounds( Rectf *result );
};

void VoronoiGridApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
}

void VoronoiGridApp::setup()
{
	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addParam( "Vertical sync", &mVerticalSyncEnabled );

	CameraPersp cam;
	cam.setPerspective( 60.f, getWindowAspectRatio(), 0.1f, 1000.0f );
	cam.setEyePoint( Vec3f( 0, 0, 10 ) );
	cam.setCenterOfInterestPoint( Vec3f::zero() );
	mPanZoomCam.setCurrentCam( cam );

	mGridManagerRef = GridManager::create( GridTorsionRef( new HomogeneousRectangularGridTorsion() ) );

	gl::enableDepthRead();
	gl::enableDepthWrite();
}

void VoronoiGridApp::update()
{
	mFps = getAverageFps();

	if ( mVerticalSyncEnabled != gl::isVerticalSyncEnabled() )
		gl::enableVerticalSync( mVerticalSyncEnabled );
}

bool VoronoiGridApp::calcCurrentGridBounds( Rectf *result )
{
	CameraPersp cam = mPanZoomCam.getCamera();
	Ray rayTopLeft = cam.generateRay( -.1f , 1.1f, cam.getAspectRatio() );
	Ray rayBottomRight = cam.generateRay( 1.1f , -.1f, cam.getAspectRatio() );

	bool intersectTL, intersectBR;
	float resTL, resBR;
	intersectTL = rayTopLeft.calcPlaneIntersection( Vec3f::zero(), Vec3f( 0, 0, 1 ), &resTL );
	intersectBR = rayBottomRight.calcPlaneIntersection( Vec3f::zero(), Vec3f( 0, 0, 1 ), &resBR );

	if ( intersectTL && intersectBR )
	{
		Vec3f topLeft = rayTopLeft.calcPosition( resTL );
		Vec3f bottomRight = rayBottomRight.calcPosition( resBR );
		*result = Rectf( topLeft.xy(), bottomRight.xy() );
		result->canonicalize();
		return true;
	}
	else
	{
		return false;
	}
}

void VoronoiGridApp::draw()
{
	gl::setViewport( getWindowBounds() );
	gl::setMatrices( mPanZoomCam.getCamera() );

	gl::clear( Color::black() );

	gl::drawColorCube( Vec3f::zero(), Vec3f::one() );

	gl::color( Color::white() );
	Rectf gridRect;
	if ( calcCurrentGridBounds( &gridRect ) )
	{
		//gl::drawSolidRect( gridRect );
		mGridManagerRef->calcCurrentGridPoints( gridRect );

		const vector< Vec2f > &points = mGridManagerRef->getPoints();
		app::console() << points.size() << endl;
		for ( auto p : points )
		{
			gl::drawStrokedCircle( p, .05f, 10 );
		}
	}

	mParams.draw();
}

void VoronoiGridApp::resize()
{
	CameraPersp cam = mPanZoomCam.getCamera();
	cam.setAspectRatio( getWindowAspectRatio() );
	mPanZoomCam.setCurrentCam( cam );
}

void VoronoiGridApp::mouseDown( MouseEvent event )
{
	mPanZoomCam.mouseDown( event.getPos() );
}

void VoronoiGridApp::mouseDrag( MouseEvent event )
{
	mPanZoomCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void VoronoiGridApp::keyDown( KeyEvent event )
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

		case KeyEvent::KEY_v:
			 mVerticalSyncEnabled = !mVerticalSyncEnabled;
			 break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

CINDER_APP_BASIC( VoronoiGridApp, RendererGl )

