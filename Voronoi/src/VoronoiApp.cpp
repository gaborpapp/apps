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

#include <vector>

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"
#include "cinder/Rand.h"

#include "boost/polygon/voronoi.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;

using boost::polygon::voronoi_diagram;

class VoronoiApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void mouseDown( MouseEvent event );
		void keyDown( KeyEvent event );

		void update();
		void draw();

	private:
		params::InterfaceGl mParams;

		vector< Vec2d > mPoints;
		voronoi_diagram< double > mVd;

		float mFps;
};

// register ci::Vec2d as a point with boost polygon
namespace boost { namespace polygon {
	template <>
	struct geometry_concept< ci::Vec2d > { typedef point_concept type; };

	template <>
	struct point_traits< ci::Vec2d >
	{
		typedef int coordinate_type;

		static inline coordinate_type get( const ci::Vec2d &point, orientation_2d orient )
		{
			return ( orient == HORIZONTAL ) ? point.x : point.y;
		}
	};
} } // namespace boost::polygon

void VoronoiApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 640, 480 );
}

void VoronoiApp::setup()
{
	gl::disableVerticalSync();

	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );

	mFps = 0.f;
	mParams.addParam( "Fps", &mFps, "", false );

	mPoints.clear();
}

void VoronoiApp::update()
{
	mFps = getAverageFps();

	mVd.clear();
	construct_voronoi( mPoints.begin(), mPoints.end(), &mVd );
}

void VoronoiApp::draw()
{
	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );

	gl::clear( Color::black() );

	gl::color( Color::white() );

	gl::begin( GL_POINTS );
	for ( auto point : mPoints )
	{
		gl::vertex( point );
	}
	gl::end();

	Vec2d size( getWindowSize() );
	// voronoi_diagram< double >::const_vertex_iterator it = mVd.begin()
	for ( auto edge : mVd.edges() )
	{
		if ( edge.is_secondary() )
			continue;

		//voronoi_diagram< double >::vertex_type *v0 = edge.vertex0();
		//voronoi_diagram< double >::vertex_type *v1 = edge.vertex1();
		auto *v0 = edge.vertex0();
		auto *v1 = edge.vertex1();

		Vec2f p0, p1;
		if ( edge.is_infinite() )
		{
			//voronoi_diagram< double >::cell_type *cell0 = edge.cell();
			//voronoi_diagram< double >::cell_type *cell1 = edge.twin()->cell();
			auto *cell0 = edge.cell();
			auto *cell1 = edge.twin()->cell();
			Vec2d c0 = mPoints[ cell0->source_index() ];
			Vec2d c1 = mPoints[ cell1->source_index() ];
			Vec2d origin = ( c0 + c1 ) * 0.5;
			Vec2d direction( c0.y - c1.y, c1.x - c0.x );

			Vec2d coef = size / math< double >::max(
					math< double >::abs( direction.x ),
					math< double >::abs( direction.y ) );
			if ( v0 == NULL )
				p0 = origin - direction * coef;
			else
				p0 = Vec2f( v0->x(), v0->y() );

			if ( v1 == NULL )
				p1 = origin + direction * coef;
			else
				p1 = Vec2f( v1->x(), v1->y() );
		}
		else
		{
			p0 = Vec2f( v0->x(), v0->y() );
			p1 = Vec2f( v1->x(), v1->y() );
		}

		gl::drawLine( p0, p1 );
	}

	params::InterfaceGl::draw();
}

void VoronoiApp::mouseDown( MouseEvent event )
{
	mPoints.push_back( event.getPos() );
}

void VoronoiApp::keyDown( KeyEvent event )
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

CINDER_APP_BASIC( VoronoiApp, RendererGl )

