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

#include <string>
#include <vector>

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"
#include "cinder/Ray.h"
#include "cinder/Rect.h"

#include "boost/polygon/voronoi.hpp"

#include "GridManager.h"
#include "HomogeneousRectangularGridTorsion.h"
#include "HomogeneousTriangleGridTorsion.h"
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
		bool mVerticalSyncEnabled = true;

		PanZoomCamUI mPanZoomCam;

		int mGridManagerId = 0;
		vector< GridManagerRef > mGridManagers;

		bool calcCurrentGridBounds( Rectf *result );

		typedef double CoordinateType;
		typedef boost::polygon::point_data< CoordinateType > PointType;
		typedef boost::polygon::voronoi_diagram< CoordinateType > VoronoiDiagram;
		VoronoiDiagram mVd;
};

// register Vec2d with boost polygon
namespace boost { namespace polygon {
	template <>
	struct geometry_concept< ci::Vec2d > { typedef point_concept type; };

	template <>
	struct point_traits< ci::Vec2d >
	{
		typedef double coordinate_type;

		static inline coordinate_type get( const ci::Vec2d &point, orientation_2d orient )
		{
			return ( orient == HORIZONTAL ) ? point.x : point.y;
		}
	};
} } // namespace boost::polygon

void VoronoiGridApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
}

void VoronoiGridApp::setup()
{
	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addParam( "Vertical sync", &mVerticalSyncEnabled );
	mParams.addSeparator();

	mGridManagers.push_back( GridManager::create( GridTorsionRef( new HomogeneousRectangularGridTorsion() ) ) );
	mGridManagers.push_back( GridManager::create( GridTorsionRef( new HomogeneousTriangleGridTorsion() ) ) );

	vector< string > gridNames { "Rectangular", "Triangle" };
	mParams.addParam( "Grid layout", gridNames, &mGridManagerId );

	CameraPersp cam;
	cam.setPerspective( 60.f, getWindowAspectRatio(), 0.1f, 1000.0f );
	cam.setEyePoint( Vec3f( 0, 0, 50 ) );
	cam.setCenterOfInterestPoint( Vec3f::zero() );
	mPanZoomCam.setCurrentCam( cam );

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
	// FIXME: margin should be calculated from zoom distance and cell size
	const float margin = .5f;
	Ray rayTopLeft = cam.generateRay( -margin , 1.f + margin, cam.getAspectRatio() );
	Ray rayBottomRight = cam.generateRay( 1.f + margin , -margin, cam.getAspectRatio() );

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

	gl::drawColorCube( Vec3f::zero(), Vec3f( 3, 3, 3 ) );

	gl::color( Color::white() );

	Rectf gridRect;
	calcCurrentGridBounds( &gridRect );

	mGridManagers[ mGridManagerId ]->calcCurrentGridPoints( gridRect );

	const vector< Vec2f > &points = mGridManagers[ mGridManagerId ]->getPoints();
	for ( auto p : points )
	{
		gl::drawStrokedCircle( p, .5f, 10 );
	}

	vector< Vec2d > points2d;
	for ( auto p: points )
		points2d.push_back( Vec2d( p ) );

	mVd.clear();
	construct_voronoi( points2d.begin(), points2d.end(),
					   &mVd );

	for ( auto edge : mVd.edges() )
	{
		if ( edge.is_secondary() )
			continue;

		if ( edge.is_linear() )
		{
			auto *v0 = edge.vertex0();
			auto *v1 = edge.vertex1();

			Vec2f p0, p1;
			if ( edge.is_finite() )
			{
				p0 = Vec2f( v0->x(), v0->y() );
				p1 = Vec2f( v1->x(), v1->y() );
			}

			gl::drawLine( p0, p1 );
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

