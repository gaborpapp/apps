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

#include <list>
#include <map>

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
//#include "cinder/params/Params.h"
#include "cinder/Timeline.h"
#include "cinder/Rand.h"
#include "cinder/audio/Output.h"
#include "cinder/audio/Io.h"

#include "AntTweakBar.h"

#include "ciMsaFluidSolver.h"
#include "ciMsaFluidDrawerGl.h"

#include "Resources.h"

#include "NI.h"

#include "PParams.h"
#include "DynaStroke.h"
#include "Particles.h"
#include "Utils.h"

#include "TimerDisplay.h"
#include "HandCursor.h"
#include "Gallery.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DynaApp : public AppBasic, UserTracker::Listener
{
	public:
		DynaApp();

		void prepareSettings(Settings *settings);
		void setup();
		void shutdown();

		void resize(ResizeEvent event);

		void keyDown(KeyEvent event);

		void mouseDown(MouseEvent event);
		void mouseDrag(MouseEvent event);
		void mouseUp(MouseEvent event);

		void newUser(UserTracker::UserEvent event);
		void lostUser(UserTracker::UserEvent event);
		void calibrationEnd(UserTracker::UserEvent event);

		void update();
		void draw();

	private:
		params::PInterfaceGl mParams;

		void drawGallery();
		void drawGame();

		bool mLeftButton;
		Vec2i mMousePos;
		Vec2i mPrevMousePos;

		void clearStrokes();
		list< DynaStroke > mDynaStrokes;

		static vector< gl::Texture > sBrushes;
		float mBrushColor;

		float mK;
		float mDamping;
		float mStrokeMinWidth;
		float mStrokeMaxWidth;

		int mParticleMin;
		int mParticleMax;
		float mMaxVelocity;
		float mVelParticleMult;
		float mVelParticleMin;
		float mVelParticleMax;

		ciMsaFluidSolver mFluidSolver;
		ciMsaFluidDrawerGl mFluidDrawer;
		static const int sFluidSizeX = 128;

		static fs::path sScreenshotFolder;
		void saveScreenshot();

		void endGame();

		void addToFluid(Vec2f pos, Vec2f vel, bool addParticles, bool addForce);

		ParticleManager mParticles;

		ci::Vec2i mPrevMouse;

		gl::Fbo mFbo;
		gl::Fbo mBloomFbo;
		gl::Fbo mDofFbo;
		gl::Fbo mOutputFbo;
		gl::GlslProg mBloomShader;
		gl::GlslProg mMixerShader;
		gl::GlslProg mDofShader;

		int mBloomIterations;
		float mBloomStrength;

		bool mDof;
		float mDofAmount;
		float mDofAperture;
		float mDofFocus;

		OpenNI mNI;
		UserTracker mNIUserTracker;
		gl::Texture mColorTexture;
		gl::Texture mDepthTexture;
		float mZClip;
		float mVideoOpacity;
		float mVideoNoise;
		float mVideoNoiseFreq;

		bool mEnableVignetting;
		bool mEnableTvLines;

		ci::Anim< float > mFlash;
		float mFps;
		bool mShowHands;

		struct UserStrokes
		{
			UserStrokes()
				: mInitialized( false )
			{
				for (int i = 0; i < JOINTS; i++)
				{
					mPrevActive[i] = false;
					mStrokes[i] = NULL;
				}
			}

			static const int JOINTS = 2;

			DynaStroke *mStrokes[JOINTS];

			bool mActive[JOINTS];
			bool mPrevActive[JOINTS];
			Vec2f mHand[JOINTS];

			bool mInitialized; // start gesture detected
		};
		map< unsigned, UserStrokes > mUserStrokes;

		// start gesture is recognized, the user can draw
		// it the hands are active
		struct UserInit
		{
			UserInit()
			{
				reset();
				mRecognized = false;
				mBrush = sBrushes[ Rand::randInt( 0, sBrushes.size() ) ];
			}

			void reset()
			{
				for (int i = 0; i < JOINTS; i++)
				{
					mPoseTimeStart[i] = -1;
					mJointMovement[i].set( 0, 0, 0, 0 );
				}
			}

			bool mRecognized; // gesture recognized
			static const int JOINTS = 2;

			double mPoseTimeStart[JOINTS];
			Rectf mJointMovement[JOINTS];

			gl::Texture mBrush;
		};

		map< unsigned, UserInit > mUserInitialized;

		audio::SourceRef mAudioShutter;

		float mPoseDuration; // maximum duration to hold start pose
		float mPoseHoldDuration; // current pose hold duration
		float mPoseHoldAreaThr; // hand movement area threshold during pose
		float mSkeletonSmoothing;
		float mGameDuration;
		Anim< float > mGameTimer;

		TimerDisplay mPoseTimerDisplay;
		TimerDisplay mGameTimerDisplay;

		vector< HandCursor > mHandCursors;
		float mHandPosCoeff;
		float mHandTransparencyCoeff;

		enum
		{
			STATE_POSE = 0,
			STATE_GAME,
			STATE_IDLE
		} States;
		int mState;

		Gallery mGallery;
};

vector< gl::Texture > DynaApp::sBrushes;
#define SCREENSHOT_FOLDER "screenshots/"
fs::path DynaApp::sScreenshotFolder( "" );


void DynaApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize(640, 480);
}

DynaApp::DynaApp() :
	mBrushColor( .5 ),
	mK( .06 ),
	mDamping( .7 ),
	mStrokeMinWidth( 10 ),
	mStrokeMaxWidth( 16 ),
	mMaxVelocity( 40 ),
	mParticleMin( 0 ),
	mParticleMax( 40 ),
	mVelParticleMult( .26 ),
	mVelParticleMin( 1 ),
	mVelParticleMax( 60 ),
	mBloomIterations( 8 ),
	mBloomStrength( .8 ),
	mZClip( 1085 ),
	mPoseHoldAreaThr( 300 ),
	mVideoOpacity( .55 ),
	mVideoNoise( .15 ),
	mVideoNoiseFreq( 11. ),
	mEnableVignetting( true ),
	mEnableTvLines( true ),
	mFlash( .0 ),
	mDof( false ),
	mDofAmount( 190. ),
	mDofAperture( .99 ),
	mDofFocus( .2 ),
	mPoseDuration( 2. ),
	mGameDuration( 20. ),
	mHandPosCoeff( 500. ),
	mHandTransparencyCoeff( 465. ),
	mState( STATE_IDLE ),
	mShowHands( true )
{
}

void DynaApp::setup()
{
	GLint n;
	glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS_EXT, &n );
	console() << "max fbo color attachments " << n << endl;

	gl::disableVerticalSync();

	// params
	string paramsXml = getResourcePath().string() + "/params.xml";
	params::PInterfaceGl::load( paramsXml );

	mParams = params::PInterfaceGl("Parameters", Vec2i(350, 700));
	mParams.addPersistentSizeAndPosition();

	// mParams.setOptions(" TW_HELP ", " visible=false "); // FIXME: not working
	TwDefine(" TW_HELP visible=false ");

	mParams.addText("Brush simulation");
	mParams.addPersistentParam("Brush color", &mBrushColor, mBrushColor, "min=.0 max=1 step=.02");
	mParams.addPersistentParam("Stiffness", &mK, mK, "min=.01 max=.2 step=.01");
	mParams.addPersistentParam("Damping", &mDamping, mDamping, "min=.25 max=.999 step=.02");
	mParams.addPersistentParam("Stroke min", &mStrokeMinWidth, mStrokeMinWidth, "min=0 max=50 step=.5");
	mParams.addPersistentParam("Stroke width", &mStrokeMaxWidth, mStrokeMaxWidth, "min=-50 max=50 step=.5");

	mParams.addSeparator();
	mParams.addText("Particles");
	mParams.addPersistentParam("Particle min", &mParticleMin, mParticleMin, "min=0 max=50");
	mParams.addPersistentParam("Particle max", &mParticleMax, mParticleMax, "min=0 max=50");
	mParams.addPersistentParam("Velocity max", &mMaxVelocity, mMaxVelocity, "min=1 max=100");
	mParams.addPersistentParam("Velocity particle multiplier", &mVelParticleMult, mVelParticleMult, "min=0 max=2 step=.01");
	mParams.addPersistentParam("Velocity particle min", &mVelParticleMin, mVelParticleMin, "min=1 max=100 step=.5");
	mParams.addPersistentParam("Velocity particle max", &mVelParticleMax, mVelParticleMax, "min=1 max=100 step=.5");

	mParams.addSeparator();
	mParams.addText("Visuals");
	mParams.addPersistentParam("Video opacity", &mVideoOpacity, mVideoOpacity, "min=0 max=1. step=.05");
	mParams.addPersistentParam("Video noise", &mVideoNoise, mVideoNoise, "min=0 max=1. step=.05");
	mParams.addPersistentParam("Video noise freq", &mVideoNoiseFreq, mVideoNoiseFreq, "min=0 max=11. step=.01");
	mParams.addPersistentParam("Vignetting", &mEnableVignetting, mEnableVignetting);
	mParams.addPersistentParam("TV lines", &mEnableTvLines, mEnableTvLines);
	mParams.addSeparator();

	mParams.addPersistentParam("Bloom iterations", &mBloomIterations, mBloomIterations, "min=0 max=8");
	mParams.addPersistentParam("Bloom strength", &mBloomStrength, mBloomStrength, "min=0 max=1. step=.05");
	mParams.addSeparator();
	mParams.addPersistentParam("Dof", &mDof, mDof);
	mParams.addParam("Dof amount", &mDofAmount, "min=1 max=250. step=.5");
	mParams.addParam("Dof aperture", &mDofAperture, "min=0 max=1. step=.01");
	mParams.addParam("Dof focus", &mDofFocus, "min=0 max=1. step=.01");

	mParams.addSeparator();
	mParams.addPersistentParam("Enable cursors", &mShowHands, mShowHands);
	mParams.addPersistentParam("Cursor persp", &mHandPosCoeff, mHandPosCoeff, "min=100. max=20000. step=1");
	mParams.addPersistentParam("Cursor transparency", &mHandTransparencyCoeff, mHandTransparencyCoeff, "min=100. max=20000. step=1");

	mParams.addSeparator();
	mParams.addText("Tracking");
	mParams.addPersistentParam("Z clip", &mZClip, mZClip, "min=1 max=10000");
	mParams.addPersistentParam("Skeleton smoothing", &mSkeletonSmoothing, 0.7,
			"min=0 max=1 step=.05");
	mParams.addPersistentParam("Start pose movement", &mPoseHoldAreaThr, mPoseHoldAreaThr,
			"min=10 max=10000 "
			"help='allowed area of hand movement during start pose without losing the pose'");

	mParams.addSeparator();
	mParams.addText("Game logic");
	mParams.addPersistentParam("Pose duration", &mPoseDuration, mPoseDuration, "min=1. max=10 step=.5");
	mParams.addPersistentParam("Game duration", &mGameDuration, mGameDuration, "min=10 max=200");

	mParams.addSeparator();
	mParams.addText("Debug");
	mParams.addParam("Fps", &mFps, "", true);

	// fluid
	mFluidSolver.setup( sFluidSizeX, sFluidSizeX );
	mFluidSolver.enableRGB(false).setFadeSpeed(0.002).setDeltaT(.5).setVisc(0.00015).setColorDiffusion(0);
	mFluidSolver.setWrap( false, true );
	mFluidDrawer.setup( &mFluidSolver );

	mParticles.setFluidSolver( &mFluidSolver );

	gl::Fbo::Format format;
	format.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );

	mFbo = gl::Fbo( 1024, 768, format );
	mFluidSolver.setSize( sFluidSizeX, sFluidSizeX / mFbo.getAspectRatio() );
	mFluidDrawer.setup( &mFluidSolver );
	mParticles.setWindowSize( mFbo.getSize() );

	format.enableColorBuffer( true, 8 );
	format.enableDepthBuffer( false );
	format.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );

	mBloomFbo = gl::Fbo( mFbo.getWidth() / 4, mFbo.getHeight() / 4, format );
	mBloomShader = gl::GlslProg( loadResource( RES_KAWASE_BLOOM_VERT ),
								 loadResource( RES_KAWASE_BLOOM_FRAG ) );
	mBloomShader.bind();
	mBloomShader.uniform( "tex", 0 );
	mBloomShader.uniform( "pixelSize", Vec2f( 1. / mBloomFbo.getWidth(), 1. / mBloomFbo.getHeight() ) );
	mBloomShader.unbind();

	format = gl::Fbo::Format();
	format.enableDepthBuffer( false );
	mOutputFbo = gl::Fbo( 1024, 768, format );

	mMixerShader = gl::GlslProg( loadResource( RES_PASSTHROUGH_VERT ),
								 loadResource( RES_MIXER_FRAG ) );
	mMixerShader.bind();
	int texUnits[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
	mMixerShader.uniform("tex", texUnits, 9);
	mMixerShader.unbind();

	mDofFbo = gl::Fbo( mFbo.getWidth(), mFbo.getHeight() );
	mDofShader = gl::GlslProg( loadResource( RES_PASSTHROUGH_VERT ), loadResource( RES_DOF_FRAG ) );
	mDofShader.bind();
	mDofShader.uniform( "rgb", 0 );
	mDofShader.uniform( "depth", 1 );
	mDofShader.uniform( "isize", Vec2f( 1.0 / mDofFbo.getWidth(), 1.0 / mDofFbo.getHeight() ) );
	mDofShader.unbind();

	sBrushes = loadTextures("brushes");
	//mBrush = sBrushes[0]; // TEST
	//loadImage( loadResource( RES_BRUSH ) );

	// audio
	mAudioShutter = audio::load( loadResource( RES_SHUTTER ) );

	// OpenNI
	try
	{
		//mNI = OpenNI( OpenNI::Device() );

		//*
		string path = getAppPath().string();
	#ifdef CINDER_MAC
		path += "/../";
	#endif
		//path += "rec-12032014223600.oni";
		path += "captured.oni";
		mNI = OpenNI( path );
		//*/
	}
	catch (...)
	{
		console() << "Could not open Kinect" << endl;
		quit();
	}
	mNI.setMirrored( true );
	mNI.setDepthAligned();
	mNI.start();
	mNIUserTracker = mNI.getUserTracker();
	mNIUserTracker.addListener( this );

#ifdef CINDER_MAC
	fs::create_directory( "/../" SCREENSHOT_FOLDER );
#else
	fs::create_directory( SCREENSHOT_FOLDER );
#endif

	mPoseTimerDisplay = TimerDisplay( RES_TIMER_POSE_BOTTOM_LEFT,
			RES_TIMER_POSE_BOTTOM_MIDDLE,
			RES_TIMER_POSE_BOTTOM_RIGHT,
			RES_TIMER_POSE_DOT_0,
			RES_TIMER_POSE_DOT_1 );
	mGameTimerDisplay = TimerDisplay( RES_TIMER_GAME_BOTTOM_LEFT,
			RES_TIMER_GAME_BOTTOM_MIDDLE,
			RES_TIMER_GAME_BOTTOM_RIGHT,
			RES_TIMER_GAME_DOT_0,
			RES_TIMER_GAME_DOT_1 );

	Rand::randomize();

	// gallery
	fs::path sScreenshotFolder = getAppPath();
#ifdef CINDER_MAC
	sScreenshotFolder /= "..";
#endif
	sScreenshotFolder /= SCREENSHOT_FOLDER;

	mGallery = Gallery( sScreenshotFolder );

	//
	//setFullScreen( true );
	hideCursor();

	mParams.hide();
}

void DynaApp::shutdown()
{
	params::PInterfaceGl::save();
}

void DynaApp::saveScreenshot()
{
	fs::path pngPath( sScreenshotFolder / fs::path( timeStamp() ) / ".png" );

	// flash
	mFlash = 0;
	timeline().apply( &mFlash, .9f, .2f, EaseOutQuad() );
	timeline().appendTo( &mFlash, .0f, .4f, EaseInQuad() );

	audio::Output::play( mAudioShutter );

	try
	{
		if (!pngPath.empty())
		{
			writeImage( pngPath, mOutputFbo.getTexture() );
		}
	}
	catch ( ... )
	{
		console() << "unable to save image file " << pngPath << endl;
	}
}

void DynaApp::newUser(UserTracker::UserEvent event)
{
	console() << "app new " << event.id << endl;
}

void DynaApp::calibrationEnd(UserTracker::UserEvent event)
{
	console() << "app calib end " << event.id << endl;
	/*
	map< unsigned, UserStrokes >::iterator it;
	it = mUserStrokes.find( event.id );
	if ( it == mUserStrokes.end() )
		mUserStrokes[ event.id ] = UserStrokes();
	*/

	map< unsigned, UserInit >::iterator it;
	it = mUserInitialized.find( event.id );
	if ( it == mUserInitialized.end() )
		mUserInitialized[ event.id ] = UserInit();
}

void DynaApp::lostUser(UserTracker::UserEvent event)
{
	console() << "app lost " << event.id << endl;
	mUserStrokes.erase( event.id );
	mUserInitialized.erase( event.id );
}

void DynaApp::resize(ResizeEvent event)
{
	/*
	mFluidSolver.setSize( sFluidSizeX, sFluidSizeX / event.getAspectRatio() );
	mFluidDrawer.setup( &mFluidSolver );
	mParticles.setWindowSize( event.getSize() );
	*/
}

void DynaApp::clearStrokes()
{
	mUserStrokes.clear();
	mDynaStrokes.clear();
}

void DynaApp::endGame()
{
	saveScreenshot();
	clearStrokes();

	mState = STATE_IDLE;
}

void DynaApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
	{
		setFullScreen(!isFullScreen());
		if (isFullScreen())
			hideCursor();
		else
			showCursor();
	}
	else
	if (event.getChar() == 's')
	{
		mParams.show( !mParams.isVisible() );
		if (isFullScreen())
		{
			if ( !mParams.isVisible() )
				hideCursor();
			else
				showCursor();
		}
	}
	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
	else
	if (event.getCode() == KeyEvent::KEY_SPACE)
		clearStrokes();
	else
	if (event.getCode() == KeyEvent::KEY_RETURN)
		saveScreenshot();
}

void DynaApp::addToFluid( Vec2f pos, Vec2f vel, bool addParticles, bool addForce )
{
	// balance the x and y components of speed with the screen aspect ratio
	float speed = vel.x * vel.x +
		vel.y * vel.y * getWindowAspectRatio() * getWindowAspectRatio();

	if ( speed > 0 )
	{
		pos.x = constrain( pos.x, 0.0f, 1.0f );
		pos.y = constrain( pos.y, 0.0f, 1.0f );

		const float velocityMult = 30;

		if ( addParticles )
		{
			int count = static_cast<int>(
						lmap<float>( vel.length() * mVelParticleMult * getWindowWidth(),
									 mVelParticleMin, mVelParticleMax,
									 mParticleMin, mParticleMax ) );
			if (count > 0)
			{
				mParticles.addParticle( pos * Vec2f( mFbo.getSize() ), count);
			}
		}
		if ( addForce )
			mFluidSolver.addForceAtPos( pos, vel * velocityMult );
	}
}

void DynaApp::mouseDown(MouseEvent event)
{
	if (event.isLeft())
	{
		mDynaStrokes.push_back( DynaStroke( sBrushes[ Rand::randInt( 0, sBrushes.size() ) ] ) );
		DynaStroke *d = &mDynaStrokes.back();
		d->resize( ResizeEvent( mFbo.getSize() ) );
		d->setStiffness( mK );
		d->setDamping( mDamping );
		d->setStrokeMinWidth( mStrokeMinWidth );
		d->setStrokeMaxWidth( mStrokeMaxWidth );
		d->setMaxVelocity( mMaxVelocity );

		mLeftButton = true;
		mMousePos = event.getPos();
	}
}

void DynaApp::mouseDrag(MouseEvent event)
{
	if (event.isLeftDown())
	{
		mMousePos = event.getPos();

		Vec2f mouseNorm = Vec2f( mMousePos ) / getWindowSize();
		Vec2f mouseVel = Vec2f( mMousePos - mPrevMousePos ) / getWindowSize();
		addToFluid( mouseNorm, mouseVel, true, true );

		mPrevMousePos = mMousePos;
	}
}

void DynaApp::mouseUp(MouseEvent event)
{
	if (event.isLeft())
	{
		mLeftButton = false;
		mMousePos = mPrevMousePos = event.getPos();
	}
}

void DynaApp::update()
{
	mFps = getAverageFps();

	if ( mLeftButton && !mDynaStrokes.empty() )
		mDynaStrokes.back().update( Vec2f( mMousePos ) / getWindowSize() );

	mNIUserTracker.setSmoothing( mSkeletonSmoothing );

	// detect start gesture
	vector< unsigned > users = mNIUserTracker.getUsers();
	mPoseHoldDuration = 0;
	for ( vector< unsigned >::const_iterator it = users.begin();
			it < users.end(); ++it )
	{
		unsigned id = *it;

		map< unsigned, UserInit >::iterator initIt = mUserInitialized.find( id );
		if ( initIt != mUserInitialized.end() )
		{
			UserInit *ui = &(initIt->second);

			if (!ui->mRecognized)
			{
				XnSkeletonJoint jointIds[] = { XN_SKEL_LEFT_HAND,
					XN_SKEL_RIGHT_HAND };

				XnSkeletonJoint limitIds[] = { XN_SKEL_LEFT_SHOULDER,
					XN_SKEL_RIGHT_SHOULDER };

				double currentTime = getElapsedSeconds();
				for ( int i = 0; i < UserInit::JOINTS; i++ )
				{
					Vec2f hand = mNIUserTracker.getJoint2d( id, jointIds[i] );
					float handConf = mNIUserTracker.getJointConfidance( id, jointIds[i] );
					Vec2f limit = mNIUserTracker.getJoint2d( id, limitIds[i] );
					float limitConf = mNIUserTracker.getJointConfidance( id, limitIds[i] );

					//console() << i << " " << hand << " [" << handConf << "] " << limit << " [" << limitConf << "]" << endl;
					bool initPose = (handConf > .5) && (limitConf > .5) &&
						(hand.y < limit.y) && (ui->mJointMovement[i].calcArea() <= mPoseHoldAreaThr );
					if (initPose)
					{
						if (ui->mPoseTimeStart[i] < 0)
							ui->mPoseTimeStart[i] = currentTime;

						if (ui->mJointMovement[i].calcArea() == 0)
						{
							ui->mJointMovement[i] = Rectf( hand, hand + Vec2f( 1, 1 ) );
						}
						else
						{
							ui->mJointMovement[i].include( hand );
						}


					}
					else
					{
						// reset if one hand is lost
						ui->reset();
					}
					//console() << i << " " << initPose << " " << ui->mPoseTimeStart[i] << endl;
				}

				bool init = true;
				float userPoseHoldDuration = 0;
				for ( int i = 0; i < UserInit::JOINTS; i++ )
				{
					init = init && ( ui->mPoseTimeStart[i] > 0 ) &&
						( ( currentTime - ui->mPoseTimeStart[i] ) >= mPoseDuration );

					if (ui->mPoseTimeStart[ i ] > 0)
						userPoseHoldDuration += currentTime - ui->mPoseTimeStart[ i ];
				}

				userPoseHoldDuration /= UserInit::JOINTS;
				if ( mPoseHoldDuration < userPoseHoldDuration)
					mPoseHoldDuration = userPoseHoldDuration;

				ui->mRecognized = init;

				if ( ui->mRecognized )
				{
					// init gesture found clear screen and strokes
					clearStrokes();

					mState = STATE_GAME;
					// clear pose start
					ui->reset();
					mPoseHoldDuration = 0;

					// add callback when game time ends
					mGameTimer = mGameDuration;
					mFlash = 0;
					timeline().clear(); // clear old callbacks
					timeline().apply( &mGameTimer, .0f, mGameDuration ).finishFn( std::bind( &DynaApp::endGame, this ) );
				}
			}
		}
		else /* users were cleared */
		{
		}
		//console() << "id: " << id << " " << mUserInitialized[id].mRecognized << endl;
	}

	// state change
	if ( ( mState == STATE_IDLE ) && ( mPoseHoldDuration > .1 ) )
	{
		mState = STATE_POSE;
	}
	else
	if ( ( mState == STATE_GAME ) && ( mPoseHoldDuration > 1. ) )
	{
		mState = STATE_POSE;
		// clear all user pose start times
		double currentTime = getElapsedSeconds();
		map< unsigned, UserInit >::iterator initIt;
		for ( initIt = mUserInitialized.begin(); initIt != mUserInitialized.end(); ++initIt )
		{
			UserInit *ui = &(initIt->second);
			for ( int i = 0; i < UserInit::JOINTS; i++ )
			{
				ui->mPoseTimeStart[ i ] = currentTime;
			}
		}
	}

	// change state when pose is cancelled
	if (mPoseHoldDuration <= 0)
	{
		if ( mGameTimer > .0 )
			mState = STATE_GAME;
		else
			mState = STATE_IDLE;
	}

	// NI user hands
	mHandCursors.clear();
	for ( vector< unsigned >::const_iterator it = users.begin();
			it < users.end(); ++it )
	{
		unsigned id = *it;

		//console() << "user hands " << id << " " << mUserInitialized[ id ].mInitialized << endl;
		map< unsigned, UserStrokes >::iterator strokeIt = mUserStrokes.find( id );
		map< unsigned, UserInit >::iterator initIt = mUserInitialized.find( id );
		UserInit *ui = &(initIt->second);

		// check if the user has strokes already
		if ( strokeIt != mUserStrokes.end() )
		{
			UserStrokes *us = &(strokeIt->second);

			XnSkeletonJoint jointIds[] = { XN_SKEL_LEFT_HAND,
				XN_SKEL_RIGHT_HAND };
			for ( int i = 0; i < UserStrokes::JOINTS; i++ )
			{
				Vec2f hand = mNIUserTracker.getJoint2d( id, jointIds[i] );
				Vec3f hand3d = mNIUserTracker.getJoint3d( id, jointIds[i] );
				us->mActive[i] = (hand3d.z < mZClip) && (hand3d.z > 0);

				mHandCursors.push_back( HandCursor( i, hand / Vec2f( 640, 480 ), hand3d.z ) );

				if (us->mActive[i] && !us->mPrevActive[i])
				{
					mDynaStrokes.push_back( DynaStroke( ui->mBrush ) );
					DynaStroke *d = &mDynaStrokes.back();
					d->resize( ResizeEvent( mFbo.getSize() ) );
					d->setStiffness( mK );
					d->setDamping( mDamping );
					d->setStrokeMinWidth( mStrokeMinWidth );
					d->setStrokeMaxWidth( mStrokeMaxWidth );
					d->setMaxVelocity( mMaxVelocity );
					us->mStrokes[i] = d;
				}

				if (us->mActive[i])
				{
					hand /= Vec2f( 640, 480 );
					us->mStrokes[i]->update( hand );
					if (us->mPrevActive[i])
					{
						Vec2f vel = Vec2f( hand - us->mHand[i] );
						addToFluid( hand, vel, true, true );
					}
					us->mHand[i] = hand;
				}
				us->mPrevActive[i] = us->mActive[i];
			}
		}
		else
		{
			// if the user has been initialized with start gesture
			if ( mUserInitialized[ id ].mRecognized )
			{
				mUserStrokes[ id ] = UserStrokes();
				mUserInitialized[ id ].mRecognized = false;
			}
		}
	}

	// kinect textures
	if ( mNI.checkNewVideoFrame() )
		mColorTexture = mNI.getVideoImage();
	if ( mNI.checkNewDepthFrame() )
		mDepthTexture = mNI.getDepthImage();

	// fluid & particles
	mFluidSolver.update();

	mParticles.setAging( 0.9 );
	mParticles.update( getElapsedSeconds() );
}

void DynaApp::drawGallery()
{
	gl::clear( Color::black() );

	gl::setMatricesWindow( getWindowSize() );
	gl::setViewport( getWindowBounds() );

	mGallery.setNoiseFreq( exp( mVideoNoiseFreq ) );
	mGallery.enableVignetting( mEnableVignetting );
	mGallery.enableTvLines( mEnableTvLines );

	mGallery.render( getWindowBounds() );
}

void DynaApp::drawGame()
{
	mFbo.bindFramebuffer();
	gl::setMatricesWindow( mFbo.getSize(), false );
	gl::setViewport( mFbo.getBounds() );

	gl::clear( Color::black() );

	gl::color( Color::gray( mBrushColor ) );

	gl::enableAlphaBlending();
	gl::enable( GL_TEXTURE_2D );
	for (list< DynaStroke >::iterator i = mDynaStrokes.begin(); i != mDynaStrokes.end(); ++i)
	{
		i->draw();
	}
	gl::disableAlphaBlending();
	gl::disable( GL_TEXTURE_2D );

	mParticles.draw();

	mFbo.unbindFramebuffer();

	// bloom
	mBloomFbo.bindFramebuffer();

	gl::setMatricesWindow( mBloomFbo.getSize(), false );
	gl::setViewport( mBloomFbo.getBounds() );

	gl::color( Color::white() );
	mFbo.getTexture().bind();

	mBloomShader.bind();
	for (int i = 0; i < mBloomIterations; i++)
	{
		glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT + i );
		mBloomShader.uniform( "iteration", i );
		gl::drawSolidRect( mBloomFbo.getBounds() );
		mBloomFbo.bindTexture( 0, i );
	}
	mBloomShader.unbind();

	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	mBloomFbo.unbindFramebuffer();

	// dof
	if ( mDof )
	{
		mDofFbo.bindFramebuffer();
		gl::setMatricesWindow( mDofFbo.getSize(), false );
		gl::setViewport( mDofFbo.getBounds() );

		gl::clear( Color::black() );
		gl::color( Color::white() );
		if ( mDepthTexture && mColorTexture )
		{
			mColorTexture.bind( 0 );
			mDepthTexture.bind( 1 );

			mDofShader.bind();
			mDofShader.uniform( "amount", mDofAmount );
			mDofShader.uniform( "aperture", mDofAperture );
			mDofShader.uniform( "focus", mDofFocus );
			gl::drawSolidRect( mDofFbo.getBounds() );
			mDofShader.unbind();

			mColorTexture.unbind();
			mDepthTexture.unbind();
		}

		mDofFbo.unbindFramebuffer();
	}

	// final
	mOutputFbo.bindFramebuffer();
	gl::setMatricesWindow( mOutputFbo.getSize(), false );
	gl::setViewport( mOutputFbo.getBounds() );

	gl::enableAlphaBlending();

	mMixerShader.bind();
	mMixerShader.uniform( "mixOpacity", mVideoOpacity );
	mMixerShader.uniform( "flash", mFlash );
	mMixerShader.uniform( "randSeed", static_cast< float >( getElapsedSeconds() ) );
	mMixerShader.uniform( "randFreqMultiplier", exp( mVideoNoiseFreq ) );
	mMixerShader.uniform( "noiseStrength", mVideoNoise );
	mMixerShader.uniform( "enableVignetting", mEnableVignetting );
	mMixerShader.uniform( "enableTvLines", mEnableTvLines );

	gl::enable( GL_TEXTURE_2D );
	if ( mDof )
		mDofFbo.bindTexture( 0 );
	else
	if ( mColorTexture )
		mColorTexture.bind( 0 );

	mFbo.getTexture().bind( 1 );
	for (int i = 1; i < mBloomIterations; i++)
	{
		mBloomFbo.getTexture( i ).bind( i + 1 );
	}

	gl::drawSolidRect( mOutputFbo.getBounds() );
	gl::disable( GL_TEXTURE_2D );
	mMixerShader.unbind();

	// cursors
	if ( mShowHands )
	{
		gl::enable( GL_TEXTURE_2D );
		gl::enableAlphaBlending();

		HandCursor::setBounds( mOutputFbo.getBounds() );
		HandCursor::setZClip( mZClip );
		HandCursor::setPosCoeff( mHandPosCoeff );
		HandCursor::setTransparencyCoeff( mHandTransparencyCoeff );
		for ( vector< HandCursor >::iterator it = mHandCursors.begin();
				it != mHandCursors.end(); ++it )
		{
			it->draw();
		}

		gl::disableAlphaBlending();
		gl::disable( GL_TEXTURE_2D );
	}

	mOutputFbo.unbindFramebuffer();

	// draw output to window
	gl::setMatricesWindow( getWindowSize() );
	gl::setViewport( getWindowBounds() );

	gl::Texture outputTexture = mOutputFbo.getTexture();
	Rectf outputRect( mOutputFbo.getBounds() );
	Rectf screenRect( getWindowBounds() );
	outputRect = outputRect.getCenteredFit( screenRect, true );
	if ( screenRect.getAspectRatio() > outputRect.getAspectRatio() )
		outputRect.scaleCentered( screenRect.getWidth() / outputRect.getWidth() );
	else
		outputRect.scaleCentered( screenRect.getHeight() / outputRect.getHeight() );

	gl::draw( outputTexture, outputRect );

	gl::enableAlphaBlending();
	switch ( mState )
	{
		case STATE_POSE:
			mPoseTimerDisplay.draw( mPoseHoldDuration / mPoseDuration );
			break;

		case STATE_GAME:
			mGameTimerDisplay.draw( 1. - mGameTimer / mGameDuration );
			break;
	}
	gl::disableAlphaBlending();
}

void DynaApp::draw()
{
	gl::clear( Color::black() );

	drawGallery();

	params::InterfaceGl::draw();
}

CINDER_APP_BASIC(DynaApp, RendererGl( RendererGl::AA_NONE ))

