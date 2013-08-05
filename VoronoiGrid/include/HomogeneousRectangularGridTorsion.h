#pragma once

#include "GridTorsion.h"

class HomogeneousRectangularGridTorsion : public GridTorsion
{
	public:
		void calcCurrentGridPoints( const ci::Rectf &bounds );

	protected:
		static const float CELL_WIDTH;
		static const float CELL_HEIGHT;
};