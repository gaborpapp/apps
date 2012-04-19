#include "cinder/gl/gl.h"
#include "cinder/ImageIo.h"
#include "cinder/app/App.h"
#include "cinder/CinderMath.h"

#include "Resources.h"

#include "HandCursor.h"

using namespace ci;

gl::Texture HandCursor::mTextures[2];
float HandCursor::mZClip = 0;
float HandCursor::mPosCoeff = 1.0;
float HandCursor::mTranspCoeff = 1.0;
Rectf HandCursor::mBounds;

HandCursor::HandCursor( int type, Vec2f pos, float z)
	: mType( type),
	  mPos( pos ),
	  mZ( z )
{
	if ( !mTextures[0] )
	{
		mTextures[0] = loadImage( app::loadResource( RES_CURSOR_LEFT ) );
		mTextures[1] = loadImage( app::loadResource( RES_CURSOR_RIGHT ) );
	}
}

void HandCursor::draw()
{
	float zDiff = math< float >::max( 0, mZ - mZClip );
	float scaleVal = 1. + zDiff / mPosCoeff;
	float transpVal = math< float >::clamp( 1. - zDiff / mTranspCoeff );

	Vec2f center = mBounds.getSize() / 2;
	Vec2f pos = mPos * mBounds.getSize();
	Vec2f handCenter = mTextures[ mType ].getSize() / 2.;

	gl::pushModelView();
	gl::translate( mBounds.getUpperLeft() );

	for ( int i = 0; i < 3; i++ )
	{
		gl::pushModelView();
		gl::translate( center );
		gl::scale( Vec2f( scaleVal, scaleVal ) );
		gl::translate( - center );
		gl::color( ColorA( 1, 1, 1, transpVal ) );
		gl::translate( - handCenter );
		gl::draw( mTextures[ mType ], pos );
		gl::popModelView();

		scaleVal *= scaleVal;
		transpVal *= transpVal;
	}
	gl::popModelView();

	gl::color( Color::white() );

}

