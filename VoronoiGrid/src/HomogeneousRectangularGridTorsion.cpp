#include "cinder/app/App.h"
#include "cinder/CinderMath.h"

#include "HomogeneousRectangularGridTorsion.h"

const float HomogeneousRectangularGridTorsion::CELL_WIDTH = 1.4f;
const float HomogeneousRectangularGridTorsion::CELL_HEIGHT = 1.f;
const ci::Vec2f HomogeneousRectangularGridTorsion::CELL_SIZE( CELL_WIDTH, CELL_HEIGHT );

void HomogeneousRectangularGridTorsion::calcCurrentGridPoints( const ci::Rectf &bounds )
{
	mPoints.clear();

	ci::Vec2f start( CELL_WIDTH * ci::math< float >::ceil( bounds.x1 / CELL_WIDTH ),
					 CELL_HEIGHT * ci::math< float >::ceil( bounds.y1 / CELL_HEIGHT ) );
	ci::Vec2i count( ci::math< float >::floor( bounds.getWidth() / CELL_WIDTH ),
					 ci::math< float >::floor( bounds.getHeight() / CELL_HEIGHT ) );

	for ( int y = 0; y < count.y; y++, start += ci::Vec2f( 0.f, CELL_HEIGHT ) )
	{
		ci::Vec2f p( start );
		for ( int x = 0; x < count.x; x++, p += ci::Vec2f( CELL_WIDTH, 0.f ) )
		{
			mPoints.push_back( p );
		}
	}
}
