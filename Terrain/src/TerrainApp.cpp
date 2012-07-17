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

#include <vector>

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/Material.h"
#include "cinder/MayaCamUI.h"
#include "cinder/params/Params.h"
#include "cinder/TriMesh.h"
#include "cinder/Perlin.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class TerrainApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void keyDown( KeyEvent event );
		void mouseDown( MouseEvent event );
		void mouseDrag( MouseEvent event );
		void resize( ResizeEvent event );

		void update();
		void draw();

	private:
		params::InterfaceGl mParams;
		void generateTerrain();

		void addQuad( int x, int y );

		TriMesh mTriMesh;
		static const int xSize = 64;
		static const int ySize = 64;
		float mHeight;
		float mNoiseScale;

		Color mAmbient;
		Color mDiffuse;
		Color mSpecular;
		float mShininess;
		Color mEmission;

		gl::Material mMaterial;

		Perlin mPerlin;

		shared_ptr< gl::Light > mLight;
		Vec3f mLightDirection;

		MayaCamUI mMayaCam;

		float mFps;
};

void TerrainApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 640, 480 );
}

void TerrainApp::addQuad( int x, int y )
{
	Vec2f p00( x, y );
	Vec2f p10( x + 1, y );
	Vec2f p11( x + 1, y + 1 );
	Vec2f p01( x, y + 1 );

	float zScale = mHeight;
	float noiseScale = mNoiseScale;
	float z00 = zScale * mPerlin.fBm( p00 * noiseScale );
	float z10 = zScale * mPerlin.fBm( p10 * noiseScale );
	float z11 = zScale * mPerlin.fBm( p11 * noiseScale );
	float z01 = zScale * mPerlin.fBm( p01 * noiseScale );

	size_t idx = mTriMesh.getNumVertices();

	// positions
	Vec3f t00( p00.x - xSize / 2., z00, p00.y - ySize / 2. );
	Vec3f t10( p10.x - xSize / 2., z10, p10.y - ySize / 2. );
	Vec3f t11( p11.x - xSize / 2., z11, p11.y - ySize / 2. );
	Vec3f t01( p01.x - xSize / 2., z01, p01.y - ySize / 2. );

	mTriMesh.appendVertex( t00 );
	mTriMesh.appendVertex( t10 );
	mTriMesh.appendVertex( t01 );

	mTriMesh.appendVertex( t10 );
	mTriMesh.appendVertex( t11 );
	mTriMesh.appendVertex( t01 );

	// normals
	Vec3f n0 = ( t10 - t00 ).cross( t10 - t01 ).normalized();
	Vec3f n1 = ( t11 - t10 ).cross( t11 - t01 ).normalized();
	mTriMesh.appendNormal( n0 );
	mTriMesh.appendNormal( n0 );
	mTriMesh.appendNormal( n0 );
	mTriMesh.appendNormal( n1 );
	mTriMesh.appendNormal( n1 );
	mTriMesh.appendNormal( n1 );

	mTriMesh.appendTriangle( idx, idx + 1, idx + 2 );
	mTriMesh.appendTriangle( idx + 3, idx + 4, idx + 5 );
}

void TerrainApp::generateTerrain()
{
	mTriMesh.clear();

	for ( int y = 0; y < ySize; ++y )
	{
		for ( int x = 0; x < xSize; ++x )
		{
			addQuad( x, y );
		}
	}
}

void TerrainApp::setup()
{
	gl::disableVerticalSync();

	CameraPersp cam;
	cam.lookAt( Vec3f( 40, 50, -10 ), Vec3f( 0, 0, 0 ) );
	cam.setPerspective( 60, getWindowAspectRatio(), 0.1f, 1000.0f );
	mMayaCam.setCurrentCam( cam );

	mLight = shared_ptr< gl::Light >( new gl::Light( gl::Light::DIRECTIONAL, 0 ) );
	mLight->setAmbient( Color( .2f, .2f, .2f ) );
	mLight->setDiffuse( Color( .5f, .5f, .5f ) );
	mLight->setSpecular( Color( 1.0f, 1.0f, 1.0f ) );
	mLight->enable();

	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );

	mLightDirection = Vec3f( -1, 0, .5 );
	mParams.addParam( "Light direction", &mLightDirection );
	mParams.addSeparator();

	mAmbient = Color::gray( .25 );
	mParams.addParam( "Ambient", &mAmbient );
	mDiffuse = Color::gray( .5 );
	mParams.addParam( "Diffuse", &mDiffuse );
	mSpecular = Color::white();
	mParams.addParam( "Specular", &mSpecular );
	mShininess = 30;
	mParams.addParam( "Shininess", &mShininess, "min=0 max=150 step=.25" );
	mEmission = Color::black();
	mParams.addParam( "Emission", &mEmission );
	mParams.addSeparator();

	mHeight = 50;
	mParams.addParam( "Heigth", &mHeight, "min=0 max=100 step=.4" );
	mNoiseScale = .034;
	mParams.addParam( "Noise scale", &mNoiseScale, "min=0 max=1 step=.002" );
	mParams.addSeparator();

	mParams.addParam( "Fps", &mFps, "", true );
}

void TerrainApp::update()
{
	static float height = 0;
	static float noiseScale = 0;

	if ( ( height != mHeight ) || ( noiseScale != mNoiseScale ) )
	{
		generateTerrain();
		height = mHeight;
		noiseScale = mNoiseScale;
	}

	mMaterial.setAmbient( mAmbient );
	mMaterial.setDiffuse( mDiffuse );
	mMaterial.setSpecular( mSpecular );
	mMaterial.setShininess( mShininess );
	mMaterial.setEmission( mEmission );

	mLight->setDirection( mLightDirection );
	mLight->update( mMayaCam.getCamera() );

	mFps = getAverageFps();
}

void TerrainApp::draw()
{
	gl::clear( Color::black() );

	gl::setMatrices( mMayaCam.getCamera() );
	gl::enable( GL_LIGHTING );

	gl::enableDepthRead();
	gl::enableDepthWrite();

	mMaterial.apply();
	gl::draw( mTriMesh );

	gl::disable( GL_LIGHTING );

	params::InterfaceGl::draw();
}

void TerrainApp::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_f:
			if ( !isFullScreen() )
			{
				setFullScreen( true );
				if ( mParams.isVisible() )
					showCursor();
				else
					hideCursor();
			}
			else
			{
				setFullScreen( false );
				showCursor();
			}
			break;

		case KeyEvent::KEY_s:
			mParams.show( !mParams.isVisible() );
			if ( isFullScreen() )
			{
				if ( mParams.isVisible() )
					showCursor();
				else
					hideCursor();
			}
			break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

void TerrainApp::mouseDown( MouseEvent event )
{
	mMayaCam.mouseDown( event.getPos() );
}

void TerrainApp::mouseDrag( MouseEvent event )
{
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void TerrainApp::resize( ResizeEvent event )
{
	CameraPersp cam = mMayaCam.getCamera();
	cam.setAspectRatio( getWindowAspectRatio() );
	mMayaCam.setCurrentCam( cam );
}

CINDER_APP_BASIC( TerrainApp, RendererGl( RendererGl::AA_NONE ) )

