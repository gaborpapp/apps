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

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/Filesystem.h"
#include "cinder/Font.h"
#include "cinder/ip/Resize.h"
#include "cinder/Rect.h"

#include "ciMsaFluidSolver.h"
#include "ciMsaFluidDrawerGl.h"

#include "CinderOpenCV.h"

#include "PParams.h"
#include "NI.h"
#include "Leaves.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Acacia : public ci::app::AppBasic
{
	public:
		void prepareSettings(Settings *settings);
		void setup();
		void shutdown();

		void resize(ResizeEvent event);
		void keyDown(KeyEvent event);

		void mouseDown(MouseEvent event);
		void mouseDrag(MouseEvent event);

		void update();
		void draw();

	private:
		ci::params::PInterfaceGl mParams;
		float mGravity;
		float mVelThres;
		float mVelDiv;
		bool mAddLeaves;
		bool mDrawAtmosphere;
		bool mDrawCamera;
		bool mDrawFeatures;

		void clearLeaves();
		vector< gl::Texture > loadTextures( const fs::path &relativeDir );

		vector< gl::Texture > mBWTextures;

		ciMsaFluidSolver mFluidSolver;
		ciMsaFluidDrawerGl mFluidDrawer;
		static const int sFluidSizeX;

		void addToFluid( Vec2f pos, Vec2f vel, bool addColor, bool addForce );

		LeafManager mLeaves;

		Vec2i mPrevMouse;

		OpenNI mNI;
		gl::Texture mOptFlowTexture;

		void chooseFeatures( cv::Mat currentFrame );
		void trackFeatures( cv::Mat currentFrame );

		cv::Mat mPrevFrame;
		vector<cv::Point2f> mPrevFeatures, mFeatures;
		vector<uint8_t> mFeatureStatuses;

		static const int MAX_FEATURES = 128;
		#define CAMERA_WIDTH 160
		#define CAMERA_HEIGHT 120
		static const Vec2f CAMERA_SIZE; //( CAMERA_WIDTH, CAMERA_HEIGHT );

		ci::Font mFont;
};

const int Acacia::sFluidSizeX = 128;
const Vec2f Acacia::CAMERA_SIZE( CAMERA_WIDTH, CAMERA_HEIGHT );

void Acacia::prepareSettings(Settings *settings)
{
	settings->setWindowSize(640, 480);
}

void Acacia::setup()
{
	// params
	string paramsXml = getResourcePath().string() + "/params.xml";
	params::PInterfaceGl::load( paramsXml );
	mParams = params::PInterfaceGl("Japan akac", Vec2i(200, 300));
	mParams.addPersistentSizeAndPosition();

	mParams.addPersistentParam( "Gravity", &mGravity, 0.8, " min=0, max=10, step=.25 " );
	mParams.addPersistentParam( "Add leaves", &mAddLeaves, true );
	mParams.addPersistentParam( "Velocity threshold", &mVelThres, 5., " min=0, max=50, step=.1 " );
	mParams.addPersistentParam( "Velocity divisor", &mVelDiv, 5, " min=1, max=50 " );
	mParams.addButton( "Clear leaves", std::bind(&Acacia::clearLeaves, this), " key=SPACE ");


	mParams.addSeparator();
	mParams.addPersistentParam( "Atmosphere", &mDrawAtmosphere, false );
	mParams.addPersistentParam( "Camera", &mDrawCamera, false );
	mParams.addPersistentParam( "Features", &mDrawFeatures, false );

	mBWTextures = loadTextures( "bw" );

	// fluid
	mFluidSolver.setup( sFluidSizeX, sFluidSizeX );
	mFluidSolver.enableRGB(false).setFadeSpeed(0.002).setDeltaT(.5).setVisc(0.00015).setColorDiffusion(0);
	mFluidSolver.setWrap( false, true );
	mFluidDrawer.setup( &mFluidSolver );

	mLeaves.setFluidSolver( &mFluidSolver );

	// OpenNI
	try
	{
		mNI = OpenNI( OpenNI::Device() );

		/*
		string path = getAppPath().string();
#ifdef CINDER_MAC
		path += "/../";
#endif
		path += "rec-12022610062000.oni";
		mNI = OpenNI( path );
		*/
	}
	catch (...)
	{
		console() << "Could not open Kinect" << endl;
		quit();
	}
	mNI.setMirrored( true );
	mNI.setDepthAligned();
	mNI.start();

	mFont = Font("Lucida Grande", 12.0f);

	gl::enableAlphaBlending();
	gl::disableDepthWrite();
	gl::disableDepthRead();

	gl::disableVerticalSync();
}

void Acacia::shutdown()
{
	params::PInterfaceGl::save();
}

void Acacia::clearLeaves()
{
	mLeaves.clear();
}

vector< gl::Texture > Acacia::loadTextures( const fs::path &relativeDir )
{
	vector< gl::Texture > textures;

	fs::path dataPath = getAssetPath( relativeDir );

	for (fs::directory_iterator it( dataPath ); it != fs::directory_iterator(); ++it)
	{
		if (fs::is_regular_file(*it) && (it->path().extension().string() == ".png"))
		{
			console() << relativeDir.string() + "/" + it->path().filename().string() << endl;
			gl::Texture t = loadImage( loadAsset( relativeDir / it->path().filename() ) );
			textures.push_back( t );
		}
	}

	return textures;
}

void Acacia::resize(ResizeEvent event)
{
	mFluidSolver.setSize( sFluidSizeX, sFluidSizeX / getWindowAspectRatio() );
	mFluidDrawer.setup( &mFluidSolver );
	mLeaves.setWindowSize( event.getSize() );
}

void Acacia::addToFluid( Vec2f pos, Vec2f vel, bool addColor, bool addForce )
{
	// balance the x and y components of speed with the screen aspect ratio
	float speed = vel.x * vel.x +
				  vel.y * vel.y * getWindowAspectRatio() * getWindowAspectRatio();

	if ( speed > 0 )
	{
		pos.x = constrain( pos.x, 0.0f, 1.0f );
		pos.y = constrain( pos.y, 0.0f, 1.0f );

		const float colorMult = 100;
		const float velocityMult = 30;

		if ( addColor )
		{
			//Color drawColor( CM_HSV, ( getElapsedFrames() % 360 ) / 360.0f, 1, 1 );
			Color drawColor( Color::white() );

			mFluidSolver.addColorAtPos( pos, drawColor * colorMult );

			mLeaves.addLeaf( pos * Vec2f( getWindowSize() ),
					mBWTextures[ Rand::randInt( mBWTextures.size() ) ] );
		}

		if ( addForce )
			mFluidSolver.addForceAtPos( pos, vel * velocityMult );
	}
}

void Acacia::chooseFeatures( cv::Mat currentFrame )
{
	cv::goodFeaturesToTrack( currentFrame, mFeatures, MAX_FEATURES, 0.005, 3.0 );
}

void Acacia::trackFeatures( cv::Mat currentFrame )
{
	vector<float> errors;
	mPrevFeatures = mFeatures;
	cv::calcOpticalFlowPyrLK( mPrevFrame, currentFrame, mPrevFeatures, mFeatures, mFeatureStatuses, errors );
}

void Acacia::update()
{
	mFluidSolver.update();

	mLeaves.setGravity( mGravity );
	mLeaves.update( getElapsedSeconds() );

	if (mNI.checkNewDepthFrame())
	{
		Surface surface( Channel8u( mNI.getVideoImage() ) );
		Surface smallSurface( CAMERA_WIDTH, CAMERA_HEIGHT, false );

		ip::resize( surface, &smallSurface );

		mOptFlowTexture = gl::Texture( smallSurface );

		cv::Mat currentFrame( toOcv( Channel( smallSurface ) ) );
		if ( mPrevFrame.data )
		{
			// pick new features once every 30 frames, or the first frame
			if ( mFeatures.empty() || getElapsedFrames() % 30 == 0 )
				chooseFeatures( mPrevFrame );
			trackFeatures( currentFrame );
		}
		mPrevFrame = currentFrame;
	}
}

void Acacia::draw()
{
	gl::clear(Color(0, 0, 0));
	gl::setMatricesWindow( getWindowSize() );

	if ( mDrawCamera && mOptFlowTexture )
	{
		gl::color( ColorA( 1, 1, 1, .3 ) );
		gl::draw( mOptFlowTexture, getWindowBounds() );
	}

	if (mDrawAtmosphere)
	{
		gl::color( Color::white() );
		mFluidDrawer.draw( 0, 0, getWindowWidth(), getWindowHeight() );
	}

	mLeaves.draw();

	if ( !mPrevFeatures.empty() )
	{
		RectMapping camera2Screen( Rectf(0, 0, CAMERA_WIDTH, CAMERA_HEIGHT),
				getWindowBounds() );

		if ( mDrawFeatures )
		{
			gl::color( Color::white() );
			for ( vector<cv::Point2f>::const_iterator featureIt = mFeatures.begin
					(); featureIt != mFeatures.end(); ++featureIt )
				gl::drawStrokedCircle( camera2Screen.map( fromOcv( *featureIt ) ), 2 );
		}

		for ( size_t idx = 0; idx < mFeatures.size(); ++idx )
		{
			if( mFeatureStatuses[idx] )
			{
				Vec2f p0 = fromOcv( mFeatures[idx] );
				Vec2f p1 = fromOcv( mPrevFeatures[idx] );
				if ( p0.distance( p1 ) > mVelThres )
				{
					addToFluid( p0 / CAMERA_SIZE,
								(p0 - p1) / mVelDiv / CAMERA_SIZE,
								mAddLeaves, true );

					gl::color( Color( 1, 0, 0 ) );
				}
				else
					gl::color( Color::white() );

				if ( mDrawFeatures )
				{
					gl::drawLine( camera2Screen.map( p0 ), camera2Screen.map( p1 ) );
				}
			}
		}
	}

	gl::drawString("FPS: " + toString(getAverageFps()), Vec2f(10.0f, 10.0f),
			Color::white(), mFont);

	params::PInterfaceGl::draw();
}

void Acacia::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());

	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void Acacia::mouseDrag(MouseEvent event)
{
	Vec2f mouseNorm = Vec2f( event.getPos() ) / getWindowSize();
	Vec2f mouseVel = Vec2f( event.getPos() - mPrevMouse ) / getWindowSize();
	addToFluid( mouseNorm, mouseVel, event.isLeftDown(), true );
	mPrevMouse = event.getPos();
}

void Acacia::mouseDown(MouseEvent event)
{
	Vec2f mouseNorm = Vec2f( event.getPos() ) / getWindowSize();
	Vec2f mouseVel = Vec2f( event.getPos() - mPrevMouse ) / getWindowSize();
	addToFluid( mouseNorm, mouseVel, false, true );
	mPrevMouse = event.getPos();
}

CINDER_APP_BASIC(Acacia, RendererGl(0) )

