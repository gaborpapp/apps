#include "cinder/Easing.h"
#include "cinder/Area.h"

#include "Moon.h"

using namespace ci;
using namespace std;

Moon::Moon( ci::app::App *app )
	: Effect( app ),
	mFade( 1. )
{
}

void Moon::setup()
{
	mParams = params::PInterfaceGl("Hold", Vec2i(200, 300));
	mParams.addPersistentSizeAndPosition();

	mParams.addButton("Moon start", std::bind(&Moon::startMovie, this));
	mParams.addButton("Moon fade", std::bind(&Moon::fadeOut, this));
	mParams.addPersistentParam("Moon speed", &mRate, 1.0f, "min=-5 max=5 step=.1");
	mParams.addPersistentParam("Moon fade duration", &mFadeDuration, 8.0f, "min=.5 max=20 step=.5");

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
	app::timeline().clear();

	if ( mMovie.isPlaying() )
		mMovie.stop();

	mMovie.seekToStart();
	mMovie.play();
	mFade = 1.0;

	mFrameTexture.reset();
	mStartFrame = mApp->getElapsedFrames();
}

void Moon::fadeOut()
{
	mFade = 1.0f;
	app::timeline().apply( &mFade, 0.0f, mFadeDuration, EaseOutQuint() );
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
		Rectf centeredRect = Rectf( mFrameTexture.getBounds() ).getCenteredFit(
				//mApp->getWindowBounds(), true );
				Area( 0, 0, 1024, 768 ), true ); // FIXME: fbo size

		// FIXME: fbo flip
		gl::pushMatrices();
		gl::scale( Vec3f( 1, -1, 1 ) );
		gl::translate( Vec2f( 0, -768 ) );

		gl::color( Color( mFade, mFade, mFade ) );
		gl::draw( mFrameTexture, centeredRect );

		gl::popMatrices();
	}
}

