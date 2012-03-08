#include "cinder/Easing.h"
#include "cinder/Area.h"

#include "VTT.h"

using namespace ci;
using namespace std;

VTT::VTT( ci::app::App *app )
	: Effect( app ),
	mFade( 1. )

{
}

void VTT::setup()
{
	mParams = params::PInterfaceGl("Varos es termeszet", Vec2i(200, 300));
	mParams.addPersistentSizeAndPosition();

	mParams.addButton("VTT start", std::bind(&VTT::startMovie, this));
	mParams.addButton("VTT fade", std::bind(&VTT::fadeOut, this));
	mParams.addPersistentParam("VTT speed", &mRate, 1.0f, "min=-5 max=5 step=.1");
	mParams.addPersistentParam("VTT fade duration", &mFadeDuration, 8.0f, "min=.5 max=20 step=.5");

	fs::path moviePath = mApp->getResourcePath() / "assets/VarosEsTermeszet/vtt.mov";
	mMovie = qtime::MovieGl( moviePath );
}

void VTT::instantiate()
{
	mMovie.stop();
}

void VTT::deinstantiate()
{
	mMovie.stop();
}

void VTT::startMovie()
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

void VTT::fadeOut()
{
	mFade = 1.0f;
	app::timeline().apply( &mFade, 0.0f, mFadeDuration, EaseOutQuint() );
}

void VTT::update()
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

void VTT::draw()
{
	gl::clear( Color( 0, 0, 0 ) );

	if ( mMovie.isPlaying() && mFrameTexture &&
		 ( mApp->getElapsedFrames() > ( mStartFrame + 1 ) ) // fixes remaining last frame from old play
		 )
	{
		Rectf centeredRect = Rectf( mFrameTexture.getBounds() ).getCenteredFit(
				Area( 0, 0, 1024, 768 ), true ); // FIXME: fbo size
				//mApp->getWindowBounds(), true );

		// FIXME: fbo flip
		gl::pushMatrices();
		gl::scale( Vec3f( 1, -1, 1 ) );
		gl::translate( Vec2f( 0, -768 ) );

		gl::color( Color( mFade, mFade, mFade ) );
		gl::draw( mFrameTexture, centeredRect );

		gl::popMatrices();
	}
}

