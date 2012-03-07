#include "Moon.h"

using namespace ci;
using namespace std;

Moon::Moon( ci::app::App *app )
	: Effect( app )
{
}

void Moon::setup()
{
	mParams = params::PInterfaceGl("Hold", Vec2i(200, 300));
	mParams.addPersistentSizeAndPosition();

	mParams.addButton("Moon start", std::bind(&Moon::startMovie, this));
	mParams.addPersistentParam("Moon speed", &mRate, 1.0f, "min=-5 max=5 step=.1");

	fs::path moviePath = mApp->getResourcePath() / "assets/Hold/moon.mov";
	mMovie = qtime::MovieGl( moviePath );
}

void Moon::instantiate()
{
	mMovie.stop();
}

void Moon::deinstantiate()
{
	mMovie.stop();
}

void Moon::startMovie()
{
	if ( mMovie.isPlaying() )
		mMovie.stop();

	mMovie.seekToStart();
	mMovie.play();

	mFrameTexture.reset();
	mStartFrame = mApp->getElapsedFrames();
}

void Moon::update()
{
	if ( mMovie && mMovie.isPlaying() )
	{
		mMovie.setRate( mRate );
		mFrameTexture = mMovie.getTexture();
	}
	else
	{
		mFrameTexture.reset();
	}
}

void Moon::draw()
{
	gl::clear( Color( 0, 0, 0 ) );

	if ( mMovie.isPlaying() && mFrameTexture &&
		 ( mApp->getElapsedFrames() > ( mStartFrame + 1 ) ) // fixes remaining last frame from old play
		 )
	{
		Rectf centeredRect = Rectf( mFrameTexture.getBounds() ).getCenteredFit( mApp->getWindowBounds(), true );
		gl::draw( mFrameTexture, centeredRect );
	}
}

