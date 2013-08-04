#pragma once

#include "cinder/Cinder.h"
#include "cinder/Rect.h"
#include "cinder/Vector.h"

typedef std::shared_ptr< class GridTorsion > GridTorsionRef;

class GridTorsion
{
	public:
		virtual void calcCurrentGridPoints( const ci::Rectf &bounds ) = 0;

		const std::vector< ci::Vec2f > & getPoints() const { return mPoints; }

	protected:
		std::vector< ci::Vec2f > mPoints;
};
