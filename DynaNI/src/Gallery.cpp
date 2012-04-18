#include "cinder/app/App.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"
#include "cinder/Easing.h"
#include "cinder/CinderMath.h"

#include "Gallery.h"

#include "Resources.h"

using namespace ci;
using namespace std;

Gallery::Gallery( fs::path &folder, int rows /* = 3 */, int columns /* = 4 */ )
	: mGalleryFolder( folder ),
	  mLastRows( -1 ),
	  mLastColumns( -1 )
{
	refreshList();

	resize( rows, columns );

	mGalleryShader = gl::GlslProg( app::loadResource( RES_PASSTHROUGH_VERT ),
								   app::loadResource( RES_GALLERY_FRAG ) );

	mParams = params::PInterfaceGl("Gallery", Vec2i(350, 500));
	mParams.addPersistentSizeAndPosition();
	mParams.addPersistentParam("Rows", &mRows, mRows, "min=1 max=10");
	mParams.addPersistentParam("Columns", &mColumns, mColumns, "min=1 max=10");

	mParams.addPersistentParam("Horizontal margin", &mHorizontalMargin, .05, "min=.0 max=.5 step=.005");

	mParams.addPersistentParam("Vertical margin", &mVerticalMargin, .05, "min=.0 max=.5 step=.005");
	mParams.addPersistentParam("Horizontal spacing", &mCellHorizontalSpacing, .04, "min=.0 max=.5 step=.005");
	mParams.addPersistentParam("Vertical spacing", &mCellVerticalSpacing, .06, "min=.0 max=.5 step=.005");

	mParams.addPersistentParam("Flip duration", &Picture::sFlipDuration, 2.5, "min=.5 max=10. step=.5");
	mParams.addPersistentParam("Flip frequency", &mFlipFrequency, 3, "min=.5 max=20. step=.5");
}

void Gallery::resize( int rows, int columns )
{
	if ( ( rows != mLastRows ) || ( columns != mLastColumns ) )
	{
		mRows = rows;
		mColumns = columns;

		mTextures.clear();
		mMaxTextures = 3 * mRows * mColumns;
		int n = min( mMaxTextures, (int)mFiles.size() );
		int filesStart = mFiles.size() - n;
		for ( int i = filesStart; i < filesStart + n; i++ )
		{
			mTextures.push_back( gl::Texture( loadImage( mFiles[ i ] ) ) );
		}

		mPictures.clear();
		for (int i = 0; i < mRows * mColumns; i++)
			mPictures.push_back( Picture( this ) );

		mLastRows = mRows;
		mLastColumns = mColumns;
	}
}

void Gallery::addImage( fs::path imagePath )
{
	mTextures.push_back( gl::Texture( loadImage( imagePath ) ) );
	if ( mTextures.size() > mMaxTextures )
		mTextures.erase( mTextures.begin(), mTextures.begin() + mTextures.size() - mMaxTextures );
}

void Gallery::refreshList()
{
	mFiles.clear();

	for ( fs::directory_iterator it( mGalleryFolder );
		  it != fs::directory_iterator(); ++it )
	{
		if ( fs::is_regular_file( *it ) &&
			 ( it->path().extension().string() == ".png" ) )
		{
			//app::console() << "   " << it->path().filename() << endl;
			mFiles.push_back( mGalleryFolder / it->path().filename() );
		}
	}
}

void Gallery::update()
{
	static double lastFlip = app::getElapsedSeconds();
	double currentTime = app::getElapsedSeconds();

	if ( ( currentTime - lastFlip ) >= mFlipFrequency )
	{
		int r = Rand::randInt( mPictures.size() );
		mPictures[ r ].flip();
		lastFlip = currentTime;
	}
}

void Gallery::render( const Area &area )
{
	if ( ( mRows != mLastRows ) || ( mColumns != mLastColumns ) )
	{
		resize( mRows, mColumns );
	}

	mGalleryShader.bind();
	mGalleryShader.uniform( "time", static_cast< float >( app::getElapsedSeconds() ) );
	mGalleryShader.uniform( "enableTvLines", mEnableTvLines );
	mGalleryShader.uniform( "enableVignetting", mEnableVignetting );
	mGalleryShader.uniform( "randFreqMultiplier", mRandFreqMultiplier );
	gl::drawSolidRect( area );
	mGalleryShader.unbind();

	unsigned i = 0;

	float hmargin = area.getWidth() * mHorizontalMargin;
	float vmargin = area.getHeight() * mVerticalMargin;
	float chspacing = area.getWidth() * mCellHorizontalSpacing;
	float cvspacing = area.getHeight() * mCellVerticalSpacing;
	Vec2f picSize( ( area.getWidth() - hmargin * 2 - chspacing * ( mColumns - 1 ) ) / mColumns,
			( area.getHeight() - vmargin * 2 - cvspacing * ( mRows - 1 ) ) / mRows );

	float hStep = chspacing + picSize.x;
	float vStep = cvspacing + picSize.y;

	gl::enable( GL_TEXTURE_2D );
	for ( int y = 0; y < mRows; y++)
	{
		Vec2f p( hmargin, vmargin + y * vStep );
		for ( int x = 0; x < mColumns; x++)
		{
			mPictures[ i ].render( Rectf( p, p + picSize ) );
			p += Vec2f( hStep, 0 );
			i++;
		}
	}
	gl::disable( GL_TEXTURE_2D );
}

Gallery::Picture::Picture( Gallery *g )
	: mGallery( g ),
	  flipping( false )
{
	setRandomTexture();
}

void Gallery::Picture::setRandomTexture()
{
	if ( !mGallery->mTextures.empty() )
	{
		int r = Rand::randInt( mGallery->mTextures.size() );
		mTexture = mGallery->mTextures[ r ];
	}
}

float Gallery::Picture::sFlipDuration = 2.5;

void Gallery::Picture::flip()
{
	if ( flipping )
		return;

	flipStart = app::getElapsedSeconds();
	flipping = true;
	flipTextureChanged = false;
}

void Gallery::Picture::render( const Rectf &rect )
{
	Rectf txtRect;

	gl::pushModelView();

	Vec2f flipScale( 1, 1 );

	if ( flipping )
	{
		float flipU = easeInOutQuart( math<float>::clamp(
				( app::getElapsedSeconds() - flipStart ) / sFlipDuration ) );

		if ( ( flipU >= .5 ) && !flipTextureChanged )
		{
			setRandomTexture();
			flipTextureChanged = true;
		}

		flipScale = Vec2f( math<float>::abs( flipU * 2 - 1 ), 1 );
		if ( flipU >= 1 )
			flipping = false;
	}

	if ( mTexture )
	{
		txtRect = mTexture.getBounds();
		mTexture.bind();
		gl::color( Color::white() );
	}
	else
	{
		txtRect = Rectf( 0, 0, 1024, 768 );
		gl::color( Color::black() );
	}

	txtRect = txtRect.getCenteredFit( rect, false );

	gl::translate( txtRect.getCenter() );
	gl::scale( flipScale );
	gl::translate( -txtRect.getCenter() );
	gl::drawSolidRect( txtRect );

	if ( mTexture )
		mTexture.unbind();
	else
		gl::color( Color::white() );

	gl::popModelView();
}

