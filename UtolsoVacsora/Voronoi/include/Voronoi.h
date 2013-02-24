#pragma once

#include <vector>

#include "boost/polygon/voronoi.hpp"

#include "cinder/Vector.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"

#include "mndlkit/params/PParams.h"

class Voronoi
{
	public:
		void setup();

		void clear() { mPoints.clear(); }

		//! Adds point with normalized coordinates to the diagram.
		template< typename T >
		void addPoint( const ci::Vec2< T >& p )
		{
			ci::Vec2< T > size( FBO_RESOLUTION, FBO_RESOLUTION );
			mPoints.push_back( p * size );
		}

		void draw();

		ci::gl::Texture getTexture() { return mFbo.getTexture(); }
		void bindTexture() { mFbo.bindTexture(); }
		void unbindTexture() { mFbo.unbindTexture(); }

	private:
		boost::polygon::voronoi_diagram< double > mVd;
		std::vector< ci::Vec2d > mPoints;

		static const int FBO_RESOLUTION = 2048;
		ci::gl::Fbo mFbo;

		float mPointSize;
		float mLineWidth;
		mndl::kit::params::PInterfaceGl mParams;
};

