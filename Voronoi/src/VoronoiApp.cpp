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
#include "voronoi_visual_utils.hpp"

using namespace ci;
using namespace ci::app;
using namespace std;

class VoronoiApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void mouseDown( MouseEvent event );
		void mouseDrag( MouseEvent event );
		void mouseUp( MouseEvent event );
		void keyDown( KeyEvent event );

		void update();
		void draw();

		struct Segment2d
		{
			Segment2d( const Vec2d &a, const Vec2d &b ) : p0( a ), p1( b ) {}
			Vec2d p0, p1;
		};
	private:
		params::InterfaceGl mParams;

		typedef double CoordinateType;
		typedef boost::polygon::point_data< CoordinateType > PointType;
		typedef boost::polygon::segment_data< CoordinateType > SegmentType;
		typedef boost::polygon::voronoi_diagram< CoordinateType > VoronoiDiagram;
		typedef VoronoiDiagram::cell_type CellType;
		typedef VoronoiDiagram::cell_type::source_index_type SourceIndexType;
		typedef VoronoiDiagram::cell_type::source_category_type SourceCategoryType;
		typedef VoronoiDiagram::edge_type EdgeType;

		vector< Vec2d > mPoints;
		vector< Segment2d > mSegments;
		VoronoiDiagram mVd;

		void sampleCurvedEdge( const EdgeType &edge, vector< Vec2d > *sampledEdge );
		Vec2d retrievePoint( const CellType &cell );
		Segment2d retrieveSegment( const CellType &cell );
		void clipInfiniteEdge( const EdgeType &edge, Vec2f *e0, Vec2f *e1 );

		bool ccw( const Vec2d &p0, const Vec2d &p1, const Vec2d &p2 );
		bool intersects( const Segment2d &s0, const Segment2d &s1 );

		Vec2i mMousePos;
		Vec2i mMouseDragPos;
		bool mDragging = false;

		float mFps;
};

// register Vec2d and Segment2d with boost polygon
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

	template <>
	struct geometry_concept< VoronoiApp::Segment2d > { typedef segment_concept type; };

	template <>
	struct segment_traits< VoronoiApp::Segment2d >
	{
		typedef double coordinate_type;
		typedef ci::Vec2d point_type;

		static inline point_type get( const VoronoiApp::Segment2d &segment, direction_1d dir )
		{
			return dir.to_int() ? segment.p1 : segment.p0;
		}
	};
} } // namespace boost::polygon

void VoronoiApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1280, 800 );
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
	construct_voronoi( mPoints.begin(), mPoints.end(),
					   mSegments.begin(), mSegments.end(),
					   &mVd );
}

Vec2d VoronoiApp::retrievePoint( const CellType &cell )
{
	SourceIndexType index = cell.source_index();
	SourceCategoryType category = cell.source_category();
	if ( category == boost::polygon::SOURCE_CATEGORY_SINGLE_POINT )
	{
		return mPoints[ index ];
	}

	index -= mPoints.size();
	if ( category == boost::polygon::SOURCE_CATEGORY_SEGMENT_START_POINT )
	{
		return mSegments[ index ].p0;
	}
	else
	{
		return mSegments[ index ].p1;
	}
}

VoronoiApp::Segment2d VoronoiApp::retrieveSegment( const CellType &cell )
{
	SourceIndexType index = cell.source_index() - mPoints.size();
	return mSegments[ index ];
}

void VoronoiApp::sampleCurvedEdge( const EdgeType &edge, vector< Vec2d > *sampledEdge )
{
	Vec2d point = edge.cell()->contains_point() ?
		retrievePoint( *edge.cell() ) :
		retrievePoint( *edge.twin()->cell() );
	Segment2d segment = edge.cell()->contains_point() ?
		retrieveSegment( *edge.twin()->cell() ) :
		retrieveSegment( *edge.cell() );

	PointType p( point.x, point.y );
	SegmentType s( PointType( segment.p0.x, segment.p0.y ), PointType( segment.p1.x, segment.p1.y ) );

	vector< PointType > se;
	se.push_back( PointType( edge.vertex0()->x(), edge.vertex0()->y() ) );
	se.push_back( PointType( edge.vertex1()->x(), edge.vertex1()->y() ) );

	boost::polygon::voronoi_visual_utils< CoordinateType >::discretize( p, s, 1., &se );

	for ( auto &pt: se )
	{
		sampledEdge->push_back( Vec2d( pt.x(), pt.y() ) );
	}
}

void VoronoiApp::clipInfiniteEdge( const EdgeType &edge, Vec2f *e0, Vec2f *e1 )
{
	const CellType &cell1 = *edge.cell();
	const CellType &cell2 = *edge.twin()->cell();

	Vec2d origin, direction;

	// Infinite edges could not be created by two segment sites.
	if ( cell1.contains_point() && cell2.contains_point() )
	{
		Vec2d p1 = retrievePoint( cell1 );
		Vec2d p2 = retrievePoint( cell2 );
		origin = ( p1 + p2 ) * 0.5;
		direction= Vec2d( p1.y - p2.y, p2.x - p1.x );
	}
	else
	{
		origin = cell1.contains_segment() ?
						retrievePoint( cell2 ) : retrievePoint( cell1 );
		Segment2d segment = cell1.contains_segment() ?
						retrieveSegment( cell1 ) : retrieveSegment( cell2 );
		Vec2d dseg = segment.p1 - segment.p0;
		if ( ( segment.p0 == origin ) ^ cell1.contains_point() )
		{
			direction.x = dseg.y;
			direction.y = -dseg.x;
		}
		else
		{
			direction.x = -dseg.y;
			direction.y = dseg.x;
		}
	}

	double side = getWindowWidth();
	double coef = side / math< double >::max(
			math< double >::abs( direction.x ),
			math< double >::abs( direction.y ) );
	if ( edge.vertex0() == NULL )
		*e0 = origin - direction * coef;
	else
		*e0 = Vec2f( edge.vertex0()->x(), edge.vertex0()->y() );

	if ( edge.vertex1() == NULL )
		*e1 = origin + direction * coef;
	else
		*e1 = Vec2f( edge.vertex1()->x(), edge.vertex1()->y() );
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

	for ( auto &segment : mSegments )
	{
		gl::drawLine( segment.p0, segment.p1 );
	}

	if ( mDragging )
	{
		gl::drawLine( mMousePos, mMouseDragPos );
	}

	Vec2d size( getWindowSize() );
	// voronoi_diagram< double >::const_vertex_iterator it = mVd.begin()
	for ( auto edge : mVd.edges() )
	{
		if ( edge.is_secondary() )
			continue;

		if ( edge.is_linear() )
		{
			//voronoi_diagram< double >::vertex_type *v0 = edge.vertex0();
			//voronoi_diagram< double >::vertex_type *v1 = edge.vertex1();
			auto *v0 = edge.vertex0();
			auto *v1 = edge.vertex1();

			Vec2f p0, p1;
			if ( edge.is_infinite() )
			{
				clipInfiniteEdge( edge, &p0, &p1 );
			}
			else
			{
				p0 = Vec2f( v0->x(), v0->y() );
				p1 = Vec2f( v1->x(), v1->y() );
			}

			gl::drawLine( p0, p1 );
		}
		else
		{
			vector< Vec2d > se;
			sampleCurvedEdge( edge, &se );

			gl::begin( GL_LINE_STRIP );
			for ( auto &pt : se )
			{
				gl::vertex( pt );
			}
			gl::end();
		}
	}

	params::InterfaceGl::draw();
}

void VoronoiApp::mouseDown( MouseEvent event )
{
	mMousePos = mMouseDragPos = event.getPos();
	mDragging = true;
}

void VoronoiApp::mouseDrag( MouseEvent event )
{
	mMouseDragPos = event.getPos();
}

bool VoronoiApp::ccw( const Vec2d &p0, const Vec2d &p1, const Vec2d &p2 )
{
	return ( p2.y - p0.y ) * ( p1.x - p0.x ) > ( p1.y - p0.y ) * ( p2.x - p0.x );
}

bool VoronoiApp::intersects( const Segment2d &s0, const Segment2d &s1 )
{
	return ( ccw( s0.p0, s1.p0, s1.p1 ) != ccw( s0.p1, s1.p0, s1.p1 ) ) &&
		   ( ccw( s0.p0, s0.p1, s1.p0 ) != ccw( s0.p0, s0.p1, s1.p1 ) );
}

void VoronoiApp::mouseUp( MouseEvent event )
{
	mDragging = false;
	Vec2i pos = mMouseDragPos = event.getPos();
	if ( mMousePos.distanceSquared( pos ) > 10 )
	{
		Segment2d newSeg( mMousePos, pos );
		bool is = false;
		for ( auto &s : mSegments )
		{
			if ( ( is = intersects( s, newSeg ) ) )
				break;
		}
		if ( !is )
			mSegments.push_back( newSeg );
	}
	else
	{
		mPoints.push_back( pos );
	}
}

void VoronoiApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_SPACE:
			mPoints.clear();
			mSegments.clear();
			break;

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

