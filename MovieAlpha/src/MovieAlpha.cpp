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
#include "cinder/Thread.h"
#include "cinder/qtime/MovieWriter.h"


using namespace ci;
using namespace ci::app;
using namespace std;

class MovieAlpha : public AppBasic
{
	public:
		void prepareSettings(Settings *settings);
		void setup();

		void keyDown(KeyEvent event);

		void update();
		void draw();

	private:
		params::InterfaceGl mParams;

		fs::path mImagePath;

		void openFolder();

		void startMovieThread();
		shared_ptr< thread > mThread;

		static void makeMovie( MovieAlpha *ptr, const fs::path &imagePath );

		static Surface makeAlphaImage( const Surface &image );

		static float sProgress;

		static fs::path sMoviePath;
		static qtime::MovieWriter sMovieWriter;
		static qtime::MovieWriter::Format sMovieFormat;
};

void MovieAlpha::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1000, 600 );
}

void MovieAlpha::setup()
{
	gl::disableVerticalSync();

	mParams = params::InterfaceGl( "Parameters", Vec2i( 400, 300 ) );
	mParams.addButton( "Add image folder",
			std::bind( &MovieAlpha::openFolder, this ) );
	mParams.addSeparator();
	mParams.addButton( "Make movie",
			std::bind( &MovieAlpha::startMovieThread, this ) );

	sMoviePath = getSaveFilePath();
    if ( !qtime::MovieWriter::getUserCompressionSettings( &sMovieFormat ) )
		quit();

}

void MovieAlpha::openFolder()
{
	mImagePath = getFolderPath();
}

void MovieAlpha::startMovieThread()
{
	mThread = shared_ptr< thread >( new thread( makeMovie, this, mImagePath ) );
}

float MovieAlpha::sProgress = 0;
qtime::MovieWriter MovieAlpha::sMovieWriter;
qtime::MovieWriter::Format MovieAlpha::sMovieFormat;
fs::path MovieAlpha::sMoviePath;

void MovieAlpha::makeMovie( MovieAlpha *ptr, const fs::path &imagePath )
{
	if ( imagePath.empty() )
		return;

	if ( sMoviePath.empty() )
		return; // user cancelled save

	fs::path outDir = app::getAppPath();
#ifdef CINDER_MAC
    outDir /= "..";
#endif
	outDir /= imagePath.filename().string() + "-alpha";
    //fs::create_directory( outDir );

	app::console() << imagePath.string() << ":\n------\n" << endl;
	int imageCount = 0;
	for ( fs::directory_iterator it( imagePath ); it != fs::directory_iterator(); ++it )
	{
		if ( fs::is_regular_file(*it) &&
				(it->path().extension().string() == ".png") )
		{
			imageCount++;
		}
	}

	float step = 1.f / imageCount;

	sProgress = 0;
	bool initMovie = false;
	for ( fs::directory_iterator it( imagePath ); it != fs::directory_iterator(); ++it )
	{
		if ( fs::is_regular_file(*it) &&
				(it->path().extension().string() == ".png") )
		{
			app::console() << it->path().filename().string() << endl;

			Surface s = loadImage( imagePath / it->path().filename() );

			Surface alphaSurface = makeAlphaImage( s );

			if ( !initMovie )
			{
				sMovieWriter = qtime::MovieWriter( sMoviePath, alphaSurface.getWidth(),
						alphaSurface.getHeight(), sMovieFormat );
				initMovie = true;
			}

			fs::path outName = outDir / ( it->path().stem().string() + "-alpha.png" );
			//writeImage( outName, alphaSurface );

			sMovieWriter.addFrame( alphaSurface );

			sProgress += step;
		}
	}
	sMovieWriter.finish();
}

Surface MovieAlpha::makeAlphaImage( const Surface &image )
{
	Surface out( image.getWidth() * 2, image.getHeight(), true );

	out.copyFrom( image, image.getBounds() );

	const Channel alpha = image.getChannelAlpha();
	Surface alphaSurface( alpha );
	out.copyFrom( alphaSurface, image.getBounds(), Vec2i( image.getBounds().getX2(), 0 ) );

	return out;
}

void MovieAlpha::keyDown(KeyEvent event)
{
	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void MovieAlpha::update()
{
}

void MovieAlpha::draw()
{
	gl::clear( Color::black() );

	gl::setMatricesWindow( getWindowSize() );
	gl::color( Color::white() );

	Rectf progress( getWindowBounds() );
	progress.scaleCentered( Vec2f( sProgress, .1 ) );
	progress.offset( Vec2f( -progress.getX1(), 0 ) );

	gl::drawSolidRect( progress );

	params::InterfaceGl::draw();
}

CINDER_APP_BASIC(MovieAlpha, RendererGl( RendererGl::AA_NONE ))

