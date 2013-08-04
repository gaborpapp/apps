#pragma once

#include <vector>

#include "cinder/Cinder.h"
#include "cinder/Rect.h"
#include "cinder/Vector.h"

#include "GridTorsion.h"

typedef std::shared_ptr< class GridManager > GridManagerRef;

class GridManager
{
	public:
		static GridManagerRef create( GridTorsionRef gridTorsionRef ) { return GridManagerRef( new GridManager( gridTorsionRef ) ); }

		void calcCurrentGridPoints( const ci::Rectf &bounds ) { mGridTorsionRef->calcCurrentGridPoints( bounds ); }

		const std::vector< ci::Vec2f > & getPoints() const { return mGridTorsionRef->getPoints(); }

	protected:
		GridManager( GridTorsionRef gridTorsionRef ) : mGridTorsionRef( gridTorsionRef ) {}

		GridTorsionRef mGridTorsionRef;
};
