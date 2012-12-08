/*
 Copyright (C) 2012 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <sstream>
#include <iomanip>

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"
#include "cinder/Surface.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"


using namespace ci;
using namespace ci::app;
using namespace std;

class PicPair : public AppBasic
{
	public:
		void prepareSettings(Settings *settings);
		void setup();

		void keyDown(KeyEvent event);

		void update();
		void draw();

	private:
		params::InterfaceGl mParams;

		fs::path mPath0, mPath1;
		vector< Surface > mImages0, mImages1;

		int mRows, mColumns;
		int mMarginH, mMarginV;

		void openLeftFolder();
		void openRightFolder();

		void makePairs();
		Surface makeRandomPair();
		vector< Surface > loadSurfaces( const fs::path &dir );
};

void PicPair::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1000, 600 );
}

void PicPair::setup()
{
	gl::disableVerticalSync();

	mParams = params::InterfaceGl( "Parameters", Vec2i( 400, 300 ) );
	mParams.addButton( "Add left image folder",
			std::bind( &PicPair::openLeftFolder, this ) );
	mParams.addButton( "Add right image folder",
			std::bind( &PicPair::openRightFolder, this ) );
	mParams.addSeparator();
	mRows = 5;
	mParams.addParam( "Rows", &mRows, "min=1 max=15" );
	mColumns = 10;
	mParams.addParam( "Columns", &mColumns, "min=1 max=15" );
	mMarginH = 225;
	mParams.addParam( "Horizontal margin", &mMarginH, "min=0 max=500" );
	mMarginV = 225;
	mParams.addParam( "Vertical margin", &mMarginV, "min=0 max=500" );
	mParams.addSeparator();
	mParams.addButton( "Make pairs",
			std::bind( &PicPair::makePairs, this ) );

}

void PicPair::openLeftFolder()
{
	mPath0 = getFolderPath();
}

void PicPair::openRightFolder()
{
	mPath1 = getFolderPath();
}

void PicPair::makePairs()
{
	console() << mPath0 << " - " << mPath1 << endl;
	if ( mPath0.empty() || mPath1.empty() )
		return;
	mImages0 = loadSurfaces( mPath0 );
	mImages1 = loadSurfaces( mPath1 );

	if ( mImages0.empty() || mImages1.empty() )
		return;

	fs::path outDir = getAppPath();
#ifdef CINDER_MAC
    outDir /= "..";
#endif
	outDir /= mPath0.filename().string() + "-" + mPath1.filename().string();
    fs::create_directory( outDir );

	fs::path pairDir = outDir / "pairs";
    fs::create_directory( pairDir );

	// montage size
	const int iw = 500 * 2;
	const int ih = 600;
	int mw = mMarginH * mColumns * 2 + mColumns * iw;
	int mh = mMarginV * mRows * 2 + mRows * ih;

	// make montage surface
	Surface montage( mw, mh, false );
	Surface::Iter iter = montage.getIter( montage.getBounds() );
	while ( iter.line() )
	{
		while ( iter.pixel() )
		{
			iter.r() = 255;
			iter.g() = 255;
			iter.b() = 255;
		}
	}

	// make pairs
	for ( int y = 0; y < mRows; y++ )
	{
		for ( int x = 0; x < mColumns; x++ )
		{
			int i = y * mColumns + mRows;

			stringstream ss;
			ss << setfill('0') << setw(2) << i;

			Surface pair = makeRandomPair();
			fs::path outName = pairDir / fs::path( ss.str() + ".jpg" );
			writeImage( outName, pair );

			montage.copyFrom( pair, pair.getBounds(),
					Vec2i( x * ( mMarginH * 2 + iw ) + mMarginH,
						   y * ( mMarginV * 2 + ih ) + mMarginV ) );
		}
	}
	writeImage( outDir / "pairs.jpg", montage );
}

Surface PicPair::makeRandomPair()
{
	int i0 = Rand::randInt( mImages0.size() );
	int i1 = Rand::randInt( mImages1.size() );

	Surface s0 = mImages0[ i0 ];
	Surface s1 = mImages1[ i1 ];

	Surface out( s0.getWidth() + s1.getWidth(), max( s0.getHeight(), s1.getHeight() ), false );

	out.copyFrom( s0, s0.getBounds() );
	out.copyFrom( s1, s1.getBounds(), Vec2i( s0.getBounds().getX2(), 0 ) );

	return out;
}

vector< Surface > PicPair::loadSurfaces( const fs::path &dir )
{
	vector< Surface > surfaces;

	console() << dir.string() << ":\n------\n" << endl;
	for ( fs::directory_iterator it( dir ); it != fs::directory_iterator(); ++it )
	{
		if ( fs::is_regular_file(*it) )
		{
			console() << it->path().filename().string() << endl;

			try
			{
				Surface s = loadImage( dir / it->path().filename() );
				surfaces.push_back( s );
			}
			catch ( const ImageIoException &exc )
			{
				console() << exc.what() << endl;
			}
		}
	}

	return surfaces;
}

void PicPair::keyDown(KeyEvent event)
{
	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void PicPair::update()
{
}

void PicPair::draw()
{
	gl::clear( Color::black() );

	params::InterfaceGl::draw();
}

CINDER_APP_BASIC(PicPair, RendererGl( RendererGl::AA_NONE ))

