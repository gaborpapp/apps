#include "cinder/CinderMath.h"

#include "Voronoi.h"
#include "voronoi_visual_utils.hpp"

using namespace ci;

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

	template <>
	struct geometry_concept< Segment2d > { typedef segment_concept type; };

	template <>
	struct segment_traits< Segment2d >
	{
		typedef double coordinate_type;
		typedef ci::Vec2d point_type;

		static inline point_type get( const Segment2d &segment, direction_1d dir )
		{
			return dir.to_int() ? segment.p1 : segment.p0;
		}
	};
} } // namespace boost::polygon

void Voronoi::setup()
{
	mParams = mndl::kit::params::PInterfaceGl( "Voronoi", Vec2i( 200, 300 ), Vec2i( 220, 16 ) );
	mParams.addPersistentSizeAndPosition();

	mParams.addPersistentParam( "Point size", &mPointSize, 5.f, "min=.0 max=50 step=.5" );
	mParams.addPersistentParam( "Line width", &mLineWidth, 10.f, "min=.5 max=10 step=.5" );

	mFbo = gl::Fbo( FBO_RESOLUTION, FBO_RESOLUTION, false, true, false );
}

void Voronoi::addSegment( const Segment2d &s )
{
	const ci::Vec2d size( mFbo.getSize() );
	Segment2d newSegment( s.p0 * size, s.p1 * size );
	bool foundIntersecting = false;
	for ( auto &segment : mSegments )
	{
		if ( newSegment.intersects( segment ) )
		{
			foundIntersecting = true;
			break;
		}
	}
	if ( !foundIntersecting )
		mSegments.push_back( newSegment );
}

Vec2d Voronoi::retrievePoint( const CellType &cell )
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

Segment2d Voronoi::retrieveSegment( const CellType &cell )
{
	SourceIndexType index = cell.source_index() - mPoints.size();
	return mSegments[ index ];
}

void Voronoi::clipInfiniteEdge( const EdgeType &edge, Vec2f *e0, Vec2f *e1 )
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

	double side = mFbo.getWidth();
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

void Voronoi::sampleCurvedEdge( const EdgeType &edge, std::vector< Vec2d > *sampledEdge )
{
	Vec2d point = edge.cell()->contains_point() ?
		retrievePoint( *edge.cell() ) :
		retrievePoint( *edge.twin()->cell() );
	Segment2d segment = edge.cell()->contains_point() ?
		retrieveSegment( *edge.twin()->cell() ) :
		retrieveSegment( *edge.cell() );

	// FIXME: solve this without the conversion step
	PointType p( point.x, point.y );
	SegmentType s( PointType( segment.p0.x, segment.p0.y ), PointType( segment.p1.x, segment.p1.y ) );

	std::vector< PointType > se;
	se.push_back( PointType( edge.vertex0()->x(), edge.vertex0()->y() ) );
	se.push_back( PointType( edge.vertex1()->x(), edge.vertex1()->y() ) );

	boost::polygon::voronoi_visual_utils< CoordinateType >::discretize( p, s, 1., &se );

	for ( auto &pt: se )
	{
		sampledEdge->push_back( Vec2d( pt.x(), pt.y() ) );
	}
}


void Voronoi::draw()
{
	glPushAttrib( GL_VIEWPORT_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT );
	gl::pushMatrices();

	mVd.clear();
	construct_voronoi( mPoints.begin(), mPoints.end(),
					   mSegments.begin(), mSegments.end(),
					   &mVd );

	gl::SaveFramebufferBinding bindingSaver;

	mFbo.bindFramebuffer();

	gl::setViewport( mFbo.getBounds() );
	gl::setMatricesWindow( mFbo.getSize(), false );

	gl::clear( Color::white() );
	gl::color( Color::black() );

	gl::enable( GL_POINT_SMOOTH );
	glPointSize( mPointSize );
	glLineWidth( mLineWidth );

	gl::begin( GL_POINTS );
	for ( auto &point : mPoints )
	{
		gl::vertex( point );
	}
	gl::end();

	gl::begin( GL_LINES );
	for ( auto &segment : mSegments )
	{
		gl::vertex( segment.p0 );
		gl::vertex( segment.p1 );
	}
	gl::end();

	Vec2d size( mFbo.getSize() );
	// voronoi_diagram< double >::const_vertex_iterator it = mVd.begin()
	for ( auto &edge : mVd.edges() )
	{
		if ( edge.is_secondary() )
			continue;

		if ( edge.is_linear() )
		{
			const VertexType *v0 = edge.vertex0();
			const VertexType *v1 = edge.vertex1();

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
		if ( edge.is_finite() )
		{
			std::vector< Vec2d > se;
			sampleCurvedEdge( edge, &se );

			gl::begin( GL_LINE_STRIP );
			for ( auto &pt : se )
			{
				gl::vertex( pt );
			}
			gl::end();
		}
	}

	glPointSize( 1.f );
	glLineWidth( 1.f );

	gl::popMatrices();
	glPopAttrib();
}


