#include "cinder/CinderMath.h"

#include "HomogeneousTriangleGridTorsion.h"

const float HomogeneousTriangleGridTorsion::CELL_WIDTH = 10.f;
const float HomogeneousTriangleGridTorsion::CELL_HEIGHT = CELL_WIDTH * ci::math< float >::sqrt( 3. ) / 2.f;

void HomogeneousTriangleGridTorsion::calcCurrentGridPoints( const ci::Rectf &bounds )
{
	mPoints.clear();

	ci::Vec2i gridPos( ci::math< float >::ceil( bounds.x1 / CELL_WIDTH ),
					   ci::math< float >::ceil( bounds.y1 / CELL_HEIGHT ) );
	ci::Vec2f start = ci::Vec2f( CELL_WIDTH , CELL_HEIGHT ) * ci::Vec2f( gridPos );
	ci::Vec2i count( ci::math< float >::floor( bounds.getWidth() / CELL_WIDTH ),
					 ci::math< float >::floor( bounds.getHeight() / CELL_HEIGHT ) );

	for ( int y = 0; y < count.y; y++ )
	{
		ci::Vec2f p, pAdd;
		if ( ( y + gridPos.x + gridPos.y ) & 1 )
		{
			p = ci::Vec2f( start + ci::Vec2f( 0.f, y * CELL_HEIGHT - CELL_HEIGHT / 3.f ) );
			pAdd = ci::Vec2f( CELL_WIDTH, CELL_HEIGHT / 3.f );
		}
		else
		{
			p = ci::Vec2f( start + ci::Vec2f( 0.f, y * CELL_HEIGHT - 2.f * CELL_HEIGHT / 3.f ) );
			pAdd = ci::Vec2f( CELL_WIDTH, -CELL_HEIGHT / 3.f );
		}

		for ( int x = 0; x < count.x; x++, p += pAdd )
		{
			mPoints.push_back( p );
			pAdd.y = -pAdd.y;
		}
	}
}
