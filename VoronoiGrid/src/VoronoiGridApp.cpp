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
#include "cinder/Rand.h"
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
		void mouseUp( MouseEvent event );
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
		bool calcScreenRectBounds( const Vec2f &p0, const Vec2f &p1, Rectf *result );

		typedef double CoordinateType;
		typedef boost::polygon::point_data< CoordinateType > PointType;
		typedef boost::polygon::voronoi_diagram< CoordinateType > VoronoiDiagram;
		VoronoiDiagram mVd;

		Vec2i mMousePos0, mMousePos1;
		bool mIsDragging = false;
		vector< Rectf > mSlideRects;

		bool intersectLines( const Vec2f &p1, const Vec2f &p2,
							 const Vec2f &p3, const Vec2f &p4,
							 Vec2f *result );
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

	gl::disableDepthRead();
	gl::disableDepthWrite();
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

bool VoronoiGridApp::calcScreenRectBounds( const Vec2f &p0, const Vec2f &p1, Rectf *result )
{
	CameraPersp cam = mPanZoomCam.getCamera();
	Vec2f p0n = p0;
	p0n.x = p0.x / float( getWindowWidth() );
	p0n.y = 1.0 - ( p0.y / float( getWindowHeight() ) );
	Vec2f p1n = p0;
	p1n.x = p1.x / float( getWindowWidth() );
	p1n.y = 1.0 - ( p1.y / float( getWindowHeight() ) );

	Ray rayTopLeft = cam.generateRay( p0n.x , p0n.y, cam.getAspectRatio() );
	Ray rayBottomRight = cam.generateRay( p1n.x , p1n.y, cam.getAspectRatio() );

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

bool VoronoiGridApp::intersectLines( const Vec2f &p1, const Vec2f &p2,
									 const Vec2f &p3, const Vec2f &p4,
									 Vec2f *result )
{
	float bx = p2.x - p1.x;
	float by = p2.y - p1.y;
	float dx = p4.x - p3.x;
	float dy = p4.y - p3.y;
	float b_dot_d_perp = bx*dy - by*dx;
	if ( b_dot_d_perp == 0.f )
	{
		return false;
	}

	float cx = p3.x - p1.x;
	float cy = p3.y - p1.y;
	float t = ( cx * dy - cy * dx ) / b_dot_d_perp;

	*result = Vec2f( p1.x + t * bx, p1.y + t * by );
	return true;
}

void VoronoiGridApp::draw()
{
	gl::setViewport( getWindowBounds() );
	gl::setMatrices( mPanZoomCam.getCamera() );

	gl::clear( Color::black() );

	//gl::drawColorCube( Vec3f::zero(), Vec3f( 3, 3, 3 ) );

	gl::color( Color::white() );

	Rectf gridRect;
	calcCurrentGridBounds( &gridRect );

	mGridManagers[ mGridManagerId ]->calcCurrentGridPoints( gridRect );

	const vector< Vec2f > &points = mGridManagers[ mGridManagerId ]->getPoints();

	struct AveragePoint
	{
		AveragePoint() : p( Vec2d( 0., 0. ) ), n( 0 ) {}
		Vec2d p;
		int n;
	};

	map< size_t, AveragePoint > averagePoints;
	vector< Vec2d > points2d;
	for ( auto p: points )
	{
		bool found = false;
		for ( size_t s = 0; s < mSlideRects.size(); s++)
		{
			if ( mSlideRects[ s ].contains( p ) )
			{
				averagePoints[ s ].n++;
				averagePoints[ s ].p += p;
				found = true;
				break;
			}
		}
		if ( !found )
		{
			points2d.push_back( Vec2d( p ) );
		}
	}
	for ( auto &kv : averagePoints )
	{
		points2d.push_back( kv.second.p / kv.second.n );
	}

	/*
	for ( auto p : points2d )
	{
		gl::drawStrokedCircle( p, .5f, 10 );
	}
	*/

	mVd.clear();
	construct_voronoi( points2d.begin(), points2d.end(),
					   &mVd );

	/*
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
	*/
	gl::enableAdditiveBlending();
	Vec2f cornerVectors[ 4 ] = { Vec2f( 8.f, 4.5f ),
		Vec2f( 8.f, -4.5f ), Vec2f( -8.f, -4.5f ),
		Vec2f( -8.f, 4.5f ) };
	for ( int i = 0; i < 4; i++ )
	{
		cornerVectors[ i ].normalize();
	}

	for ( auto &cell : mVd.cells() )
	{
		auto *start_edge = cell.incident_edge();
		auto *edge = start_edge;

		// draw cell point
		auto idx = cell.source_index();
		Vec2f p = points2d[ idx ];
		gl::color( Color::white() );
		gl::drawStrokedCircle( p, .5f, 10 );

		float minD = 9999999.f; // TODO: use limits<float>::max
		bool isRectFit = false;
		do
		{
			if ( edge->is_primary() && edge->is_linear() && edge->is_finite() )
			{
				auto *v0 = edge->vertex0();
				auto *v1 = edge->vertex1();

				Vec2f p0, p1;
				p0 = Vec2f( v0->x(), v0->y() );
				p1 = Vec2f( v1->x(), v1->y() );

				// draw edge
				gl::color( ColorA::gray( .3 ) );
				gl::drawLine( p0, p1 );

				for ( int i = 0; i < 4; i++ )
				{
					Vec2f res;
					if ( intersectLines( p, p + cornerVectors[ i ], p0, p1, &res ) )
					{
						float d = p.distanceSquared( res );
						if ( d < minD )
						{
							minD = d;
							isRectFit = true;
						}
					}
				}
			}
			edge = edge->next();
		} while ( edge != start_edge );

		// fit rect in cell
		if ( isRectFit )
		{
			minD = math< float >::sqrt( minD );
			Rectf cellRect( p + minD * cornerVectors[ 0 ], p + minD * cornerVectors[ 2 ] );

			unsigned long seed = math< int >::abs( int( p.x ) + int( p.y ) );
			Rand::randSeed( seed );
			cellRect.canonicalize();
			gl::color( ColorA( CM_HSV, Rand::randFloat(), .8, .8, .7 ) );
			//gl::drawStrokedRect( cellRect );
			gl::drawSolidRect( cellRect );
		}
	}
	gl::disableAlphaBlending();

	if ( mIsDragging )
	{
		gl::color( Color( 1, 0, 0 ) );
		Rectf rect;
		if ( calcScreenRectBounds( mMousePos0, mMousePos1, &rect ) )
		{
			gl::drawStrokedRect( rect );
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
	if ( event.isLeft() )
	{
		mMousePos0 = mMousePos1 = event.getPos();
		mIsDragging = true;
	}
	else
	{
		mPanZoomCam.mouseDown( event.getPos() );
	}
}

void VoronoiGridApp::mouseDrag( MouseEvent event )
{
	if ( mIsDragging )
	{
		mMousePos1 = event.getPos();
	}
	else
	{
		mPanZoomCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
	}
}

void VoronoiGridApp::mouseUp( MouseEvent event )
{
	if ( event.isLeft() && mIsDragging )
	{
		mMousePos1 = event.getPos();
		Rectf rect;
		if ( calcScreenRectBounds( mMousePos0, mMousePos1, &rect ) )
		{
			mSlideRects.push_back( rect );
		}
		mIsDragging = false;
	}
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

		case KeyEvent::KEY_SPACE:
			mSlideRects.clear();
			break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

CINDER_APP_BASIC( VoronoiGridApp, RendererGl )

