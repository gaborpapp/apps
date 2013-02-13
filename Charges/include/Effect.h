#ifndef __EFFECT_H__
#define __EFFECT_H__

#include "cinder/Area.h"

class Effect
{
	public:

		Effect() {};

		virtual ~Effect() {};

		virtual void setup() {}
		virtual void draw() {}
		virtual void instantiate() {}
		virtual void deinstantiate() {}

		virtual void addCursor( int id, float x, float y, float r ) {};
		virtual void updateCursor( int id, float x, float y, float r ) {};
		virtual void removeCursor( int id ) {};

		void setBounds( const ci::Area &area ) { mBounds = area; }

	protected:
		ci::Area mBounds;

};

#endif

