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

	reset();
	mTimeline = Timeline::create();
	app::timeline().add( mTimeline );
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

void Gallery::addImage( fs::path imagePath, int pictureIndex /* = -1 */ )
{
	mTextures.push_back( gl::Texture( loadImage( imagePath ) ) );
	if ( mTextures.size() > mMaxTextures )
		mTextures.erase( mTextures.begin(), mTextures.begin() + mTextures.size() - mMaxTextures );

	if ( ( pictureIndex >= 0 ) && ( pictureIndex < mPictures.size() ) )
	{
		mPictures[ pictureIndex ].setTexture( mTextures.back() );
	}
}

void Gallery::zoomImage( int pictureIndex )
{
	if ( ( pictureIndex >= 0 ) && ( pictureIndex < mPictures.size() ) )
	{
		mPictures[ pictureIndex ].startZoom();
	}
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

void Gallery::reset()
{
	for ( vector< Gallery::Picture >::iterator it = mPictures.begin();
			it != mPictures.end(); ++it )
	{
		it->reset();
	}
	mLastFlip = app::getElapsedSeconds();
}

void Gallery::update()
{
	double currentTime = app::getElapsedSeconds();

	if ( ( currentTime - mLastFlip ) >= mFlipFrequency )
	{
		int r = Rand::randInt( mPictures.size() );
		mPictures[ r ].flip();
		mLastFlip = currentTime;
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
	int zoomedOne = -1;
	Rectf zoomedRect;
	for ( int y = 0; y < mRows; y++)
	{
		Vec2f p( hmargin, vmargin + y * vStep );
		for ( int x = 0; x < mColumns; x++)
		{
			if ( mPictures[i].isZooming() )
			{
				zoomedOne = i;
				zoomedRect = Rectf( p, p + picSize );
			}
			else
			{
				mPictures[ i ].render( Rectf( p, p + picSize ) );
			}
			p += Vec2f( hStep, 0 );
			i++;
		}
	}

	// awful hack to draw the zooming picture last
	if ( zoomedOne >= 0 )
	{
		mPictures[ zoomedOne ].render( zoomedRect );
	}
	gl::disable( GL_TEXTURE_2D );
}

Gallery::Picture::Picture( Gallery *g )
	: mGallery( g ),
	zooming( false )
{
	setRandomTexture();
	reset();
}

void Gallery::Picture::reset()
{
	// FIXME: zoomImage is called before reset
	if ( zooming )
		return;

	flipping = false;
	zooming = false;
	appearanceTime = app::getElapsedSeconds() + 1. + Rand::randFloat( 2. );
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
	if ( flipping || zooming )
		return;

	flipStart = app::getElapsedSeconds();
	flipping = true;
	flipTextureChanged = false;
}

void Gallery::Picture::startZoom()
{
	zooming = true;
	flipping = false;
	mZoom = 0.;
	mGallery->mTimeline->apply( &mZoom, 0.f, 0.f, 1.f );
	mGallery->mTimeline->appendTo( &mZoom, 0.f, 1.f, 1.5f, EaseOutCirc() );
	appearanceTime = -1;
}

void Gallery::Picture::render( const Rectf &rect )
{
	Rectf txtRect;
	Rectf outRect = rect;

	gl::pushModelView();

	Vec2f flipScale( 1, 1 );

	double currentTime = app::getElapsedSeconds();

	if ( zooming )
	{
		Rectf zoomRect;
		if ( mTexture )
			zoomRect = mTexture.getBounds();
		else
			zoomRect = Rectf( 0, 0, 1024, 768 );

		Rectf screenRect( app::getWindowBounds() );
		zoomRect = zoomRect.getCenteredFit( screenRect, true );
		if ( screenRect.getAspectRatio() > zoomRect.getAspectRatio() )
			zoomRect.scaleCentered( screenRect.getWidth() / zoomRect.getWidth() );
		else
			zoomRect.scaleCentered( screenRect.getHeight() / zoomRect.getHeight() );

		Vec2f center = lerp< Vec2f >( zoomRect.getCenter(),
				outRect.getCenter(), mZoom );
		float scale = lerp< float >( zoomRect.getWidth(),
				outRect.getWidth(), mZoom ) /
			outRect.getWidth();

		outRect.offsetCenterTo( center );
		outRect.scaleCentered( scale );

		if ( mZoom == 1. )
			zooming = false;
	}
	else
	if ( currentTime < appearanceTime )
	{
		flipScale = Vec2f( 0, 1 );
	}
	else
	if ( currentTime < appearanceTime + sFlipDuration * .5 )
	{
		float flipU = easeOutQuart(
				( currentTime - appearanceTime ) / ( sFlipDuration * .5 ) );
		flipScale = Vec2f( flipU, 1 );
	}
	else
	if ( flipping )
	{
		float flipU = easeInOutQuart( math<float>::clamp(
				( currentTime - flipStart ) / sFlipDuration ) );

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

	txtRect = txtRect.getCenteredFit( outRect, true );

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

