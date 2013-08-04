#pragma once

#include "GridTorsion.h"

class HomogeneousTriangleGridTorsion : public GridTorsion
{
	public:
		void calcCurrentGridPoints( const ci::Rectf &bounds );

	protected:
		static const float CELL_WIDTH;
		static const float CELL_HEIGHT;
		static const ci::Vec2f CELL_SIZE;
};
