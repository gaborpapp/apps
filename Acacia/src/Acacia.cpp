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

#include "ciMsaFluidSolver.h"
#include "ciMsaFluidDrawerGl.h"

#include "PParams.h"
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
		bool mDrawAtmosphere;

		vector< gl::Texture > loadTextures( const fs::path &relativeDir );

		vector< gl::Texture > mBWTextures;

		ciMsaFluidSolver mFluidSolver;
		ciMsaFluidDrawerGl mFluidDrawer;
		static const int sFluidSizeX;

		void addToFluid( Vec2f pos, Vec2f vel, bool addColor, bool addForce );

		LeafManager mLeaves;

		Vec2i mPrevMouse;
};

const int Acacia::sFluidSizeX = 128;

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

	mParams.addPersistentParam( "Atmosphere", &mDrawAtmosphere, true );

	gl::disableVerticalSync();

	mBWTextures = loadTextures( "bw" );

	// fluid
	mFluidSolver.setup( sFluidSizeX, sFluidSizeX );
	mFluidSolver.enableRGB(false).setFadeSpeed(0.002).setDeltaT(.5).setVisc(0.00015).setColorDiffusion(0);
	mFluidSolver.setWrap( false, true );
	mFluidDrawer.setup( &mFluidSolver );

	mLeaves.setFluidSolver( &mFluidSolver );

	gl::enableAlphaBlending();
	gl::disableDepthWrite();
	gl::disableDepthRead();
}

void Acacia::shutdown()
{
	params::PInterfaceGl::save();
}

vector< gl::Texture > Acacia::loadTextures( const fs::path &relativeDir )
{
	vector< gl::Texture > textures;

	fs::path dataPath = getAssetPath( relativeDir );

	for (fs::directory_iterator it( dataPath ); it != fs::directory_iterator(); ++it)
	{
		if (fs::is_regular_file(*it) && (it->path().extension().string() == ".png"))
		{
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

			/*
			if( drawParticles )
				particleSystem.addParticles( pos * Vec2f( getWindowSize() ), 10 );
			*/
		}

		if ( addForce )
			mFluidSolver.addForceAtPos( pos, vel * velocityMult );
	}
}

void Acacia::update()
{
	mFluidSolver.update();
	mLeaves.update( getElapsedSeconds() );
}

void Acacia::draw()
{
	gl::clear(Color(0, 0, 0));
	gl::setMatricesWindow( getWindowSize() );

	if (mDrawAtmosphere)
	{
		gl::color( Color::white() );
		mFluidDrawer.draw( 0, 0, getWindowWidth(), getWindowHeight() );
	}

	mLeaves.draw();

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
	addToFluid( mouseNorm, mouseVel, true, true );
	mPrevMouse = event.getPos();
}

void Acacia::mouseDown(MouseEvent event)
{
	Vec2f mouseNorm = Vec2f( event.getPos() ) / getWindowSize();
	Vec2f mouseVel = Vec2f( event.getPos() - mPrevMouse ) / getWindowSize();
	addToFluid( mouseNorm, mouseVel, false, true );
	mPrevMouse = event.getPos();
}

CINDER_APP_BASIC(Acacia, RendererGl)

