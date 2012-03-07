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

#include "Acacia.h"

using namespace ci;
using namespace ci::app;
using namespace std;

const int Acacia::sFluidSizeX = 128;
const Vec2f Acacia::CAMERA_SIZE( CAMERA_WIDTH, CAMERA_HEIGHT );

Acacia::Acacia( App *app )
	: Effect( app )
{
}

void Acacia::setup()
{
	// params
	mParams = params::PInterfaceGl("Japan akac", Vec2i(200, 300));
	mParams.addPersistentSizeAndPosition();

	mParams.addPersistentParam( "Max leaves", &mMaxLeaves, 1000, " min=100, max=4000, step=100 " );
	mLeafCount = 0;
	mParams.addParam( "Leaf count", &mLeafCount, "", true );
	mParams.addPersistentParam( "Leaf Aging", &mAging, 0.975, " min=0, max=1, step=.005 " );
	mParams.addPersistentParam( "Gravity", &mGravity, 0.5, " min=0, max=10, step=.05 " );
	mAddLeaves = false;
	mParams.addParam( "Add leaves", &mAddLeaves );
	mAddParticles = true;
	mParams.addParam( "Add particles", &mAddParticles );
	mParams.addPersistentParam( "Velocity threshold", &mVelThres, 1.3, " min=0, max=50, step=.1 " );
	mParams.addPersistentParam( "Velocity divisor", &mVelDiv, 5, " min=1, max=50 " );
	mParams.addPersistentParam( "Particle Aging", &mParticleAging, 0.9, " min=0, max=1, step=.005 " );
	mParams.addPersistentParam( "Particle velocity threshold", &mParticleVelThres, 5., " min=0, max=50, step=.1 " );
	mParams.addPersistentParam( "Particle velocity divisor", &mParticleVelDiv, 5, " min=1, max=50 " );
	mParams.addButton( "Clear leaves", std::bind(&Acacia::clearLeaves, this) );

	mParams.addSeparator();
	mParams.addPersistentParam( "Atmosphere", &mDrawAtmosphere, false );
	mParams.addPersistentParam( "Camera", &mDrawCamera, false );
	mParams.addPersistentParam( "Features", &mDrawFeatures, false );

	mBWTextures = loadTextures( "Akac/bw" );

	// fluid
	mFluidSolver.setup( sFluidSizeX, sFluidSizeX );
	mFluidSolver.enableRGB(false).setFadeSpeed(0.002).setDeltaT(.5).setVisc(0.00015).setColorDiffusion(0);
	mFluidSolver.setWrap( false, true );
	mFluidDrawer.setup( &mFluidSolver );

	mLeaves.setFluidSolver( &mFluidSolver );
	mParticles.setFluidSolver( &mFluidSolver );
}

void Acacia::instantiate()
{
	gl::enableAdditiveBlending();
	gl::disableDepthWrite();
	gl::disableDepthRead();
}

void Acacia::deinstantiate()
{
	gl::disableAlphaBlending();
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
	mParticles.setWindowSize( event.getSize() );
}

void Acacia::addToFluid( Vec2f pos, Vec2f vel, bool addLeaves, bool addParticles, bool addForce )
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

		if ( addLeaves )
		{
			Color drawColor( Color::white() );

			mFluidSolver.addColorAtPos( pos, drawColor * colorMult );

			mLeaves.addLeaf( pos * Vec2f( getWindowSize() ),
					mBWTextures[ Rand::randInt( mBWTextures.size() ) ] );
		}

		if ( addParticles )
		{
			mParticles.addParticle( pos * Vec2f( getWindowSize() ), 15 );
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
	mLeaves.setMaximum( mMaxLeaves );
	mLeaves.setAging( mAging );
	mLeaves.update( getElapsedSeconds() );
	mLeafCount = mLeaves.getCount();

	mParticles.setAging( mParticleAging );
	mParticles.update( getElapsedSeconds() );

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
	mParticles.draw();

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
				bool addLeaves = mAddLeaves && ( p0.distance( p1 ) > mVelThres );
				bool addParticles = mAddParticles && ( p0.distance( p1 ) > mParticleVelThres );
				if (addLeaves || addParticles)
				{
					addToFluid( p0 / CAMERA_SIZE,
								(p0 - p1) / mVelDiv / CAMERA_SIZE,
								addLeaves, addParticles, true );

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

	/*
	gl::drawString("FPS: " + toString(getAverageFps()), Vec2f(10.0f, 10.0f),
			Color::white(), mFont);
	*/

}

void Acacia::mouseDrag(MouseEvent event)
{
	Vec2f mouseNorm = Vec2f( event.getPos() ) / getWindowSize();
	Vec2f mouseVel = Vec2f( event.getPos() - mPrevMouse ) / getWindowSize();
	addToFluid( mouseNorm, mouseVel, mAddLeaves && event.isLeftDown(), mAddParticles && event.isLeftDown(), true );
	mPrevMouse = event.getPos();
}

void Acacia::mouseDown(MouseEvent event)
{
	Vec2f mouseNorm = Vec2f( event.getPos() ) / getWindowSize();
	Vec2f mouseVel = Vec2f( event.getPos() - mPrevMouse ) / getWindowSize();
	addToFluid( mouseNorm, mouseVel, false, false, true );
	mPrevMouse = event.getPos();
}

