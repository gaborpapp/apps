#pragma once

#include <vector>

#include "boost/polygon/segment_data.hpp"
#include "boost/polygon/voronoi.hpp"

#include "cinder/Vector.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"

#include "mndlkit/params/PParams.h"

class Segment2i
{
	public:
		Segment2i( const ci::Vec2i &a, const ci::Vec2i &b ) : p0( a ), p1( b ) {}

		bool intersects( const Segment2i &other ) const
		{
			return ( ccw( p0, other.p0, other.p1 ) != ccw( p1, other.p0, other.p1 ) ) &&
				   ( ccw( p0, p1, other.p0 ) != ccw( p0, p1, other.p1 ) );
		}

		ci::Vec2i p0, p1;

	protected:
		bool ccw( const ci::Vec2i &p0, const ci::Vec2i &p1, const ci::Vec2i &p2 ) const
		{
			return ( p2.y - p0.y ) * ( p1.x - p0.x ) > ( p1.y - p0.y ) * ( p2.x - p0.x );
		}
};

class Voronoi
{
	public:
		void setup();

		void clear() { mPoints.clear(); mSegments.clear(); }

		//! Adds point with normalized coordinates to the diagram.
		template< typename T >
		void addPoint( const ci::Vec2< T > &p )
		{
			mPoints.push_back( p * ci::Vec2< T >( mFbo.getSize() ) );
		}

		/** Adds segment with integer coordinates to the diagram. The segment is rejected
		 *  if it intersects previous segments. **/
		void addSegment( const Segment2i &s );

		/** Adds two points as a segment with normalized coordinates to the diagram. The
		 *  segment is rejected if it intersects previous segments. **/
		template< typename T >
		void addSegment( const ci::Vec2< T > &p0, const ci::Vec2< T > &p1 )
		{
			const ci::Vec2d size( mFbo.getSize() );
			Segment2i newSegment( p0 * size, p1 * size );
			addSegment( newSegment );
		}

		void draw();

		ci::gl::Texture getTexture() { return mFbo.getTexture(); }
		void bindTexture() { mFbo.bindTexture(); }
		void unbindTexture() { mFbo.unbindTexture(); }

	private:
		typedef double CoordinateType;
		typedef boost::polygon::point_data< CoordinateType > PointType;
		typedef boost::polygon::segment_data< CoordinateType > SegmentType;
		typedef boost::polygon::voronoi_diagram< CoordinateType > VoronoiDiagram;
		typedef VoronoiDiagram::cell_type CellType;
		typedef VoronoiDiagram::cell_type::source_index_type SourceIndexType;
		typedef VoronoiDiagram::cell_type::source_category_type SourceCategoryType;
		typedef VoronoiDiagram::edge_type EdgeType;
		typedef VoronoiDiagram::vertex_type VertexType;

		VoronoiDiagram mVd;
		std::vector< ci::Vec2i > mPoints;
		std::vector< Segment2i > mSegments;

		ci::Vec2i retrievePoint( const CellType &cell );
		Segment2i retrieveSegment( const CellType &cell );
		void clipInfiniteEdge( const EdgeType &edge, ci::Vec2f *e0, ci::Vec2f *e1 );
		void sampleCurvedEdge( const EdgeType &edge, std::vector< ci::Vec2d > *sampledEdge );

		static const int FBO_RESOLUTION = 2048;
		ci::gl::Fbo mFbo;

		float mPointSize;
		float mLineWidth;
		mndl::kit::params::PInterfaceGl mParams;
};

