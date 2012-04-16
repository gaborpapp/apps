#include "cinder/app/App.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"

#include "Gallery.h"

#include "Resources.h"

using namespace ci;
using namespace std;

Gallery::Gallery( fs::path &folder, int rows /* = 3 */, int columns /* = 4 */ )
	: mGalleryFolder( folder ),
	  mHorizontalMargin( .05 ),
	  mVerticalMargin( 0.05 ),
	  mCellHorizontalSpacing( 0.04 ),
	  mCellVerticalSpacing( 0.06 )
{
	refreshList();

	resize( rows, columns );

	mGalleryShader = gl::GlslProg( app::loadResource( RES_PASSTHROUGH_VERT ),
								   app::loadResource( RES_GALLERY_FRAG ) );
}

void Gallery::resize( int rows, int columns)
{
	if ( ( rows != mRows ) || ( columns != mColumns ) )
	{
		mRows = rows;
		mColumns = columns;

		mPictures.clear();
		for (int i = 0; i < mRows * mColumns; i++)
			mPictures.push_back( Picture( this ) );
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

void Gallery::update()
{
}

void Gallery::render( const Area &area )
{
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
	: mGallery( g )
{
	if ( !mGallery->mFiles.empty() )
	{
		int r = Rand::randInt( mGallery->mFiles.size() );
		fs::path path = mGallery->mFiles[ r ];
		mTexture = loadImage( path );
	}
}

void Gallery::Picture::render( const Rectf &rect )
{
	Rectf txtRect;

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
	gl::drawSolidRect( txtRect );

	if ( mTexture )
		mTexture.unbind();
	else
		gl::color( Color::white() );
}

