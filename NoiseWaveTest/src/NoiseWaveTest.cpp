/*
 Copyright (C) 2014 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "cinder/Cinder.h"
#include "cinder/Perlin.h"
#include "cinder/PolyLine.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class NoiseWaveTest : public AppBasic
{
 public:
	void prepareSettings( Settings *settings );
	void setup();

	void keyDown( KeyEvent event );

	void update();
	void draw();

 private:
	params::InterfaceGlRef mParams;

	Perlin mPerlin;

	float mNoiseAmplitude;
	float mNoiseAmplitude1;
	float mNoiseAmplitude2;
	float mNoiseAmplitude3;
	float mMaxWaveElevationDistance;

	float mNoiseSpeed;
	float mNoiseSpeed1;
	float mNoiseSpeed2;
	float mNoiseSpeed3;

	float mNoiseFrequency;
	float mNoiseFrequency1;
	float mNoiseFrequency2;
	float mNoiseFrequency3;
};

void NoiseWaveTest::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 800, 600 );
}

void NoiseWaveTest::setup()
{
	mParams = params::InterfaceGl::create( "Parameters", Vec2i( 400, 300 ) );

	mNoiseAmplitude = -2.0f;
	mParams->addParam( "Noise amplitude", &mNoiseAmplitude ).step( 0.05f );
	mNoiseAmplitude1 = 25.0f;
	mParams->addParam( "Noise amplitude 1", &mNoiseAmplitude1 ).step( 0.1f );
	mNoiseAmplitude2 = 20.0f;
	mParams->addParam( "Noise amplitude 2", &mNoiseAmplitude2 ).step( 0.1f );
	mNoiseAmplitude3 = 4.f;
	mParams->addParam( "Noise amplitude 3", &mNoiseAmplitude3 ).step( 0.1f );
	mMaxWaveElevationDistance = 1.0f;
	mParams->addParam( "Max wave elevation distance", &mMaxWaveElevationDistance ).min( 0.0f ).max( 1.0f ).step( 0.005f );
	mParams->addSeparator();

	mNoiseSpeed = 1.09f;
	mParams->addParam( "Noise speed", &mNoiseSpeed ).step( 0.01f );
	mNoiseSpeed1 = -1.4f;
	mParams->addParam( "Noise speed 1", &mNoiseSpeed1 ).step( 0.01f );
	mNoiseSpeed2 = -1.4f;
	mParams->addParam( "Noise speed 2", &mNoiseSpeed2 ).step( 0.01f );
	mNoiseSpeed3 = -4.0f;
	mParams->addParam( "Noise speed 3", &mNoiseSpeed3 ).step( 0.01f );
	mParams->addSeparator();

	mNoiseFrequency = 1.0f;
	mParams->addParam( "Noise frequency", &mNoiseFrequency ).step( 0.1f );
	mNoiseFrequency1 = 7.0f;
	mParams->addParam( "Noise frequency 1", &mNoiseFrequency1 ).step( 0.1f );
	mNoiseFrequency2 = 10.1f;
	mParams->addParam( "Noise frequency 2", &mNoiseFrequency2 ).step( 0.1f );
	mNoiseFrequency3 = 45.0f;
	mParams->addParam( "Noise frequency 3", &mNoiseFrequency3 ).step( 0.1f );
}

void NoiseWaveTest::update()
{
}

void NoiseWaveTest::draw()
{
	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowSize() );
	gl::clear();

	PolyLine2f pl;
	int w = getWindowWidth();
	float y = getWindowHeight() * 0.5f;
	float t = static_cast< float >( getElapsedSeconds() );
	float tNoise1 = mNoiseSpeed * mNoiseSpeed1 * t;
	float tNoise2 = mNoiseSpeed * mNoiseSpeed2 * t;
	float tNoise3 = mNoiseSpeed * mNoiseSpeed3 * t;
	float nT = 0.0f;
	float nTStep = 1.0f / (float)w;
	float noiseFreq1 = mNoiseFrequency1 * mNoiseFrequency;
	float noiseFreq2 = mNoiseFrequency2 * mNoiseFrequency;
	float noiseFreq3 = mNoiseFrequency3 * mNoiseFrequency;

	auto smoothstep = []( float edge0, float edge1, float x ) -> float
	{
		float t = math< float >::clamp( ( x - edge0 ) / ( edge1 - edge0 ) );
		return t * t * ( 3.f - 2.f * t );
	};
	for ( int i = 0; i < w; i++, nT += nTStep )
	{
		float waveShape = mNoiseAmplitude * smoothstep( 0.0f, mMaxWaveElevationDistance, nT );
		float noiseShape = ( mNoiseAmplitude1 + mNoiseAmplitude2 + mNoiseAmplitude3 ) +
			mPerlin.noise( nT * noiseFreq1 + tNoise1 ) * mNoiseAmplitude1 +
			mPerlin.noise( nT * noiseFreq2 + tNoise2 ) * mNoiseAmplitude2 +
			mPerlin.noise( nT * noiseFreq3 + tNoise3 ) * mNoiseAmplitude3;

		pl.push_back( Vec2f( i, y + waveShape * noiseShape ) );
	}

	gl::draw( pl );

	mParams->draw();
}

void NoiseWaveTest::keyDown( KeyEvent event )
{
	switch ( event.getCode() )
	{
		case KeyEvent::KEY_f:
			if ( ! isFullScreen() )
			{
				setFullScreen( true );
				if ( mParams->isVisible() )
				{
					showCursor();
				}
				else
				{
					hideCursor();
				}
			}
			else
			{
				setFullScreen( false );
				showCursor();
			}
			break;

		case KeyEvent::KEY_s:
			mParams->show( ! mParams->isVisible() );
			if ( isFullScreen() )
			{
				if ( mParams->isVisible() )
				{
					showCursor();
				}
				else
				{
					hideCursor();
				}
			}
			break;

		case KeyEvent::KEY_ESCAPE:
			quit();
			break;

		default:
			break;
	}
}

CINDER_APP_BASIC( NoiseWaveTest, RendererGl )

