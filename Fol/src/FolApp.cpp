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
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/ImageIo.h"
#include "cinder/params/Params.h"

#include "Utils.h"
#include "NI.h"

#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class FolApp : public AppBasic
{
	public:
		FolApp();

		void prepareSettings(Settings *settings);
		void setup();

		void keyDown(KeyEvent event);

		void update();
		void draw();

	private:
		void toggleRecording();
		void createVbo();

		params::InterfaceGl mParams;

		OpenNI mNI;
		gl::Texture mDepthTexture;
		gl::Texture mBlurKernelTexture;
		gl::Fbo mDepthFbo;
		gl::GlslProg mBlurShader;

		static const int VBO_X_SIZE = 640 * 2.;
		static const int VBO_Y_SIZE = 480 * 2.;
		gl::VboMesh mVboMesh;
		gl::GlslProg mWaveShader;

		float mBlurAmount;
		float mStep;
		float mClip;
		float mFps;
};

FolApp::FolApp()
	: mBlurAmount( 20. ),
	  mStep( 2. ),
	  mClip( .2 )
{
}

void FolApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize(640, 480);
}

void FolApp::setup()
{
	gl::disableVerticalSync();

	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 300 ) );
	mParams.addParam( "Blur amount", &mBlurAmount, "min=0 max=128 step=1" );
	mParams.addParam( "Clip", &mClip, "min=0 max=1 step=.005" );
	mParams.addParam( "Step", &mStep, "min=1 max=128 step=.5" );
	mParams.addSeparator();
	mParams.addParam( "Fps", &mFps, "", true );

	mParams.addSeparator();
	mParams.addButton( "Start recording", std::bind(&FolApp::toggleRecording, this ));

	mBlurKernelTexture = loadImage( loadResource( RES_BLUR_KERNEL ) );
	mBlurShader = gl::GlslProg( loadResource( RES_BLUR_VERT ),
								loadResource( RES_BLUR_FRAG ) );
	mWaveShader = gl::GlslProg( loadResource( RES_WAVE_VERT ),
								loadResource( RES_WAVE_FRAG ) );

	gl::Fbo::Format format;
	format.enableColorBuffer( true, 2 );
	format.enableDepthBuffer( false );
	mDepthFbo = gl::Fbo( 640, 480, format );
	/*
		 0 0, 0, 0
		 1 0.00392157, 0, 0
		 2 0.0352941, 0.0235294, 0
		 3 0.0745098, 0.0627451, 0
		 4 0.117647, 0.12549, 0
		 5 0.172549, 0.239216, 0
		 6 0.227451, 0.388235, 0
		 7 0.278431, 0.533333, 0
		 8 0.345098, 0.717647, 0
		 9 0.407843, 0.898039, 0
		10 0.466667, 0.980392, 0
		11 0.529412, 0.898039, 0
		12 0.6, 0.717647, 0
		13 0.662745, 0.533333, 0
		14 0.721569, 0.388235, 0
		15 0.780392, 0.243137, 0
		16 0.835294, 0.129412, 0
		17 0.882353, 0.0627451, 0
		18 0.933333, 0.0235294, 0
		19 0.980392, 0, 0
		20 1, 0, 0
	 */

	createVbo();

	// start OpenNI
	try
	{
		//mNI = OpenNI( OpenNI::Device() );

		//*
		string path = getAppPath().string();
#ifdef CINDER_MAC
		path += "/../";
#endif
		path += "rec-12033015082700.oni";
		mNI = OpenNI( path );
		//*/
	}
	catch (...)
	{
		console() << "Could not open Kinect" << endl;
		quit();
	}

	mNI.setMirrored( true );
	mNI.start();
}

void FolApp::createVbo()
{
	gl::VboMesh::Layout layout;

	layout.setStaticPositions();
	layout.setStaticTexCoords2d();
	layout.setStaticIndices();

	std::vector<Vec3f> positions;
	std::vector<Vec2f> texCoords;
	std::vector<uint32_t> indices;

	int numVertices = VBO_X_SIZE * VBO_Y_SIZE;
	int numShapes = ( VBO_X_SIZE - 1 ) * ( VBO_Y_SIZE - 1 );

	mVboMesh = gl::VboMesh( numVertices, numShapes, layout, GL_POINTS );

	for ( int x = 0; x < VBO_X_SIZE; x++ )
	{
		for ( int y = 0; y < VBO_Y_SIZE; y++ )
		{
			indices.push_back( x * VBO_Y_SIZE + y );
			float xPer = x / (float)( VBO_X_SIZE - 1 );
			float yPer = y / (float)( VBO_Y_SIZE - 1 );

			/*
			positions.push_back( Vec3f( ( xPer * 2.0f - 1.0f ) * VBO_X_SIZE,
										( yPer * 2.0f - 1.0f ) * VBO_Y_SIZE,
										0.0f ) );
			*/
			positions.push_back( Vec3f( x, y, 0 ) );
			texCoords.push_back( Vec2f( xPer, yPer ) );
		}
	}

	mVboMesh.bufferPositions( positions );
	mVboMesh.bufferIndices( indices );
	mVboMesh.bufferTexCoords2d( 0, texCoords );
	//mVboMesh.unbindBuffers();
}

void FolApp::toggleRecording()
{
	if ( mNI.isRecording() )
	{
		mNI.stopRecording();
		mParams.setOptions( "Stop recording", " label='Start recording' " );
	}
	else
	{
		string path = getAppPath().string();
#ifdef CINDER_MAC
		path += "/../";
#endif
		path += "rec-" + timeStamp() + ".oni";
		fs::path oniPath(path);

		mNI.startRecording( oniPath );
		mParams.setOptions( "Start recording", " label='Stop recording' " );
	}
}

void FolApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());
	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void FolApp::update()
{
	mFps = getAverageFps();

	if (mNI.checkNewDepthFrame())
		mDepthTexture = mNI.getDepthImage();
}


void FolApp::draw()
{
	gl::clear( Color::black() );

	if ( !mDepthTexture )
		return;

	// blur depth
	mDepthFbo.bindFramebuffer();
	gl::setMatricesWindow( mDepthFbo.getSize(), false );
	gl::setViewport( mDepthFbo.getBounds() );

	mBlurShader.bind();

	mBlurShader.uniform( "kernelSize", (float)mBlurKernelTexture.getWidth() );
	mBlurShader.uniform( "invKernelSize", 1.f / mBlurKernelTexture.getWidth() );
	mBlurShader.uniform( "imageTex", 0 );
	mBlurShader.uniform( "kernelTex", 1 );
	mBlurShader.uniform( "blurAmount", mBlurAmount );

	// pass 1
	glDrawBuffer( GL_COLOR_ATTACHMENT0_EXT );

	//gl::enable( GL_TEXTURE_2D );
	mDepthTexture.bind( 0 );
	mBlurKernelTexture.bind( 1 );
	mBlurShader.uniform( "stepVector", Vec2f( 1. / mDepthTexture.getWidth(), 0. ) );

	gl::drawSolidRect( mDepthFbo.getBounds() );

	mDepthTexture.unbind();

	// pass 2
	glDrawBuffer( GL_COLOR_ATTACHMENT1_EXT );

	mDepthFbo.bindTexture( 0 );
	mBlurShader.uniform( "stepVector", Vec2f( 0., 1. / mDepthFbo.getHeight() ) );
	gl::drawSolidRect( mDepthFbo.getBounds() );

	mDepthFbo.unbindTexture();
	mBlurKernelTexture.unbind();
	mBlurShader.unbind();

	mDepthFbo.unbindFramebuffer();

	// final
	gl::setMatricesWindow( getWindowSize() );
	gl::setViewport( getWindowBounds() );

	gl::disableDepthRead();
	gl::disableDepthWrite();

	gl::enableAdditiveBlending();
	gl::color( ColorA( 1, 1, 1, .03 ) );

	gl::pushMatrices();
	gl::scale( Vec2f( getWindowWidth() / (float)VBO_X_SIZE,
					  getWindowHeight() / (float)VBO_Y_SIZE) );

	mDepthFbo.getTexture( 1 ).bind();
	mWaveShader.bind();
	mWaveShader.uniform( "tex", 0 );
	mWaveShader.uniform( "invSize", Vec2f( mStep / mDepthFbo.getWidth(),
										   mStep / mDepthFbo.getHeight() ) );
	mWaveShader.uniform( "clip", mClip );
	gl::draw( mVboMesh );
	mWaveShader.unbind();

	gl::popMatrices();

	gl::disableAlphaBlending();

	//gl::draw( mDepthFbo.getTexture( 1 ), getWindowBounds() );

	params::InterfaceGl::draw();
}

CINDER_APP_BASIC(FolApp, RendererGl( RendererGl::AA_NONE ))

