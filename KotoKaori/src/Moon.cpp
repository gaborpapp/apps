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
}

void Moon::update()
{
	if ( mMovie )
	{
		mMovie.setRate( mRate );
		mFrameTexture = mMovie.getTexture();
	}
}

void Moon::draw()
{
	gl::clear( Color( 0, 0, 0 ) );

	if ( mFrameTexture )
	{
		Rectf centeredRect = Rectf( mFrameTexture.getBounds() ).getCenteredFit( mApp->getWindowBounds(), true );
		gl::draw( mFrameTexture, centeredRect );
	}
}

