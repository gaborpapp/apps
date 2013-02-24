#include "cinder/CinderMath.h"

#include "Voronoi.h"

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
} } // namespace boost::polygon

void Voronoi::setup()
{
	mParams = mndl::kit::params::PInterfaceGl( "Voronoi", Vec2i( 200, 300 ), Vec2i( 220, 16 ) );
	mParams.addPersistentSizeAndPosition();

	mParams.addPersistentParam( "Point size", &mPointSize, 5.f, "min=.0 max=50 step=.5" );
	mParams.addPersistentParam( "Line width", &mLineWidth, 10.f, "min=.5 max=10 step=.5" );

	mFbo = gl::Fbo( FBO_RESOLUTION, FBO_RESOLUTION, false, true, false );
}

void Voronoi::draw()
{
	glPushAttrib( GL_VIEWPORT_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT );
	gl::pushMatrices();

	mVd.clear();
	construct_voronoi( mPoints.begin(), mPoints.end(), &mVd );

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
	for ( auto point : mPoints )
	{
		gl::vertex( point );
	}
	gl::end();

	Vec2d size( mFbo.getSize() );
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

	glPointSize( 1.f );
	glLineWidth( 1.f );

	gl::popMatrices();
	glPopAttrib();
}


