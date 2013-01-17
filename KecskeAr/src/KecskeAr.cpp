/*
 Copyright (C) 2013 Gabor Papp

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

#include "cinder/app/AppBasic.h"

#include "cinder/gl/gl.h"
#include "cinder/gl/DisplayList.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Light.h"

#include "cinder/Camera.h"
#include "cinder/Cinder.h"
#include "cinder/MayaCamUI.h"

#include "AssimpLoader.h"
#include "PParams.h"
#include "TrackerManager.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class KecskeAr : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void resize();
		void keyDown( KeyEvent event );
		void mouseDown( MouseEvent event );
		void mouseDrag( MouseEvent event );

		void update();
		void draw();

		void shutdown();

	private:
		// params
		params::PInterfaceGl mParams;

		// tracker
		TrackerManager mTrackerManager;

		float mFps;

		void loadModel();
		void drawModel();
		mndl::assimp::AssimpLoader mAssimpLoader;
		gl::DisplayList mDisplayList;

		// cameras
		typedef enum
		{
			TYPE_INDOOR = 0,
			TYPE_OUTDOOR = 1
		} CameraType;

		struct CameraExt
		{
			string mName;
			CameraType mType;
			CameraPersp mCam;
			bool mLightingEnabled;
		};

		int mCameraIndex;
		vector< CameraExt > mCameras;
		CameraExt mCurrentCamera;
		MayaCamUI mMayaCam;
		float mFov;

		enum
		{
			MODE_MOUSE = 0,
			MODE_AR = 1
		};
		int mInteractionMode;

		// light/shadow mapping - based on the work by Paul Houx
		// https://forum.libcinder.org/topic/triangle-normals-not-working-for-complete-mesh#23286000001332003
		void setupShadowMap();
		void renderShadowMap();

		void enableLights();
		void disableLights();

		float mModelRadius;
		Vec3f mLightDirection;
		Color mLightAmbient;
		Color mLightDiffuse;
		Color mLightSpecular;
		float mShadowStrength;
		float mGamma;

		Matrix44f mShadowMatrix;
		CameraPersp mShadowCamera;

		int mDepthSize;
		gl::Fbo mDepthFbo;
		gl::GlslProg mRenderShader;
		gl::GlslProg mDepthShader;

		bool mDrawShadowMap;
		bool mEnableLighting;
		bool mShadowMapUpdateNeeded = true;

		// postprocess
		static const int FBO_WIDTH = 1280;
		static const int FBO_HEIGHT = 800;
		gl::Fbo mFbo;
		gl::GlslProg mPostProcessShader;
		float mHue;
		float mSaturation;
		float mLightness;

		gl::Texture mLogo;
};

void KecskeAr::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1024, 768 );
}

void KecskeAr::setup()
{
	gl::disableVerticalSync();

	getWindow()->setTitle( "KecskeAr" );

	// params
	params::PInterfaceGl::load( "params.xml" );

	mParams = params::PInterfaceGl( "Parameters", Vec2i( 200, 300 ) );
	mParams.addPersistentSizeAndPosition();

	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addSeparator();

	vector< string > interactionNames = { "mouse", "ar" };
	mParams.addPersistentParam( "Interaction mode", interactionNames, &mInteractionMode, 0 );
	mFov = 60.f;
	mParams.addParam( "Fov", &mFov, "min=10 max=80" );
	mParams.addPersistentParam( "Light direction", &mLightDirection, Vec3f( 1.45696115f, -1.05111349, 0.812103748 ) );
	mParams.addPersistentParam( "Light ambient", &mLightAmbient, Color::black() );
	mParams.addPersistentParam( "Light diffuse", &mLightDiffuse, Color::white() );
	mParams.addPersistentParam( "Light specular", &mLightDiffuse, Color::white() );
	mParams.addPersistentParam( "Shadow strength", &mShadowStrength, .55f, "min=0 max=1 step=.05" );
	mParams.addPersistentParam( "Gamma", &mGamma, 1.0f, "min=1 max=5 step=.1" );

	mParams.addPersistentParam( "Enable lighting", &mEnableLighting, true );
	mDrawShadowMap = false;
	mParams.addParam( "Draw shadowmap", &mDrawShadowMap );
	mParams.addPersistentParam( "Shadowmap size", &mDepthSize, 2048, "min=256 max=4096 step=16" );
	mParams.addSeparator();
	mParams.addText( "Postprocessing" );
	mParams.addPersistentParam( "Hue", &mHue, 0.f, "min=0 max=1 step=.01" );
	mParams.addPersistentParam( "Saturation", &mSaturation, 1.f, "min=0.1 max=10 step=.05" );
	mParams.addPersistentParam( "Lightness", &mLightness, 1.f, "min=0.1 max=10 step=.05" );
	mParams.addSeparator();

	loadModel();
	setupShadowMap();

	try
	{
		mRenderShader = gl::GlslProg( loadAsset( "shaders/RenderModel.vert" ),
									  loadAsset( "shaders/RenderModel.frag" ) );
	}
	catch( const std::exception &e )
	{
		console() << "Could not compile shader:" << e.what() << std::endl;
	}

	try
	{
		mDepthShader = gl::GlslProg( loadAsset( "shaders/PassThrough.vert" ),
									 loadAsset( "shaders/ShowDepth.frag" ) );
	}
	catch( const std::exception &e )
	{
		console() << "Could not compile shader:" << e.what() << std::endl;
	}

	mTrackerManager.setup();

	mFbo = gl::Fbo( FBO_WIDTH, FBO_HEIGHT );
	try
	{
		mPostProcessShader = gl::GlslProg( loadAsset( "shaders/PassThrough.vert" ),
										   loadAsset( "shaders/PostProcess.frag" ) );
	}
	catch( const std::exception &e )
	{
		console() << "Could not compile shader:" << e.what() << std::endl;
	}

	for ( CameraExt &ce : mCameras )
		ce.mCam.setAspectRatio( mFbo.getAspectRatio() );

	mLogo = loadImage( loadAsset( "logo.png" ) );

	setFullScreen( true );
	params::PInterfaceGl::showAllParams( false );
}

void KecskeAr::loadModel()
{
	XmlTree doc( loadFile( getAssetPath( "config.xml" ) ) );
	XmlTree xmlLicense = doc.getChild( "model" );
	string filename = xmlLicense.getAttributeValue< string >( "name", "" );

	try
	{
		mAssimpLoader = mndl::assimp::AssimpLoader( getAssetPath( filename ) );
	}
	catch ( mndl::assimp::AssimpLoaderExc &exc )
	{
		console() << exc.what() << endl;
		quit();
	};

	// set up cameras
	vector< string > cameraNames;
	mCameras.resize( mAssimpLoader.getNumCameras() );
	for ( size_t i = 0; i < mAssimpLoader.getNumCameras(); i++ )
	{
		string name = mAssimpLoader.getCameraName( i );
		cameraNames.push_back( name );
		mCameras[ i ].mName = name;
		mCameras[ i ].mType = TYPE_INDOOR;
		mCameras[ i ].mCam = mAssimpLoader.getCamera( i );

		// load camera parameter from config file
		for ( XmlTree::Iter cIt = doc.begin( "camera" ); cIt != doc.end(); ++cIt )
		{
			string cName = cIt->getAttributeValue< string >( "name", "" );
			if ( cName != name )
				continue;
			string cTypeStr = cIt->getAttributeValue< string >( "type", "indoor" );
			if ( cTypeStr == "indoor" )
				mCameras[ i ].mType = TYPE_INDOOR;
			else
			if ( cTypeStr == "outdoor" )
				mCameras[ i ].mType = TYPE_OUTDOOR;

			mCameras[ i ].mLightingEnabled = cIt->getAttributeValue< bool >( "lighting", false );
		}
	}
	mCameraIndex = 0;
	mParams.addParam( "Cameras", cameraNames, &mCameraIndex );

	AxisAlignedBox3f bbox = mAssimpLoader.getBoundingBox();
	mModelRadius = ( bbox.getSize() * .5f ).length();

	mDisplayList = gl::DisplayList( GL_COMPILE );
	mDisplayList.newList();
	mAssimpLoader.draw();
	mDisplayList.endList();
}

void KecskeAr::setupShadowMap()
{
	// create a frame buffer object (FBO) containing only a depth buffer
	// NOTE: without a color buffer it hangs on OSX 10.8 with some Geforce cards
	// https://forum.libcinder.org/topic/anyone-else-experiencing-shadow-mapping-problems-with-the-new-xcode
	mDepthFbo = gl::Fbo( mDepthSize, mDepthSize, false, true, true );

	// set it up for shadow mapping
	mDepthFbo.bindDepthTexture();
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	mDepthFbo.unbindTexture();

	mShadowMapUpdateNeeded = true;
}

void KecskeAr::renderShadowMap()
{
	if ( !mShadowMapUpdateNeeded )
		return;

	gl::SaveFramebufferBinding binderSaver;

	// store the current OpenGL state,
	// so we can restore it when done
	glPushAttrib( GL_VIEWPORT_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT );

	// bind the shadow map FBO
	mDepthFbo.bindFramebuffer();

	// set the viewport to the correct dimensions and clear the FBO
	glViewport( 0, 0, mDepthFbo.getWidth(), mDepthFbo.getHeight() );
	glClear( GL_DEPTH_BUFFER_BIT );

	// to reduce artefacts, offset the polygons a bit
	glPolygonOffset( 2.0f, 5.0f );
	glEnable( GL_POLYGON_OFFSET_FILL );

	// render the mesh
	gl::enableDepthWrite();

	gl::pushMatrices();
	gl::setMatrices( mShadowCamera );
	drawModel();
	gl::popMatrices();

	// unbind the FBO and restore the OpenGL state
	//mDepthFbo.unbindFramebuffer();

	glPopAttrib();
	mShadowMapUpdateNeeded = false;
}

void KecskeAr::update()
{
	static int lastCameraIndex = -1;
	static int lastInteractionMode = -1;
	if ( ( lastCameraIndex != mCameraIndex ) ||
		 ( lastInteractionMode != mInteractionMode ) )
	{
		mCurrentCamera = mCameras[ mCameraIndex ];
		lastCameraIndex = mCameraIndex;
		lastInteractionMode = mInteractionMode;
		mShadowMapUpdateNeeded = true;
	}

	mFps = getAverageFps();

	mTrackerManager.update();
}

void KecskeAr::enableLights()
{
	static Vec3f lastLightDirection;

	if ( lastLightDirection != mLightDirection )
	{
		lastLightDirection = mLightDirection;
		mShadowMapUpdateNeeded = true;
	}

	// setup light 0
	gl::Light light( gl::Light::POINT, 0 );

	Vec3f lightPosition = -mLightDirection * mModelRadius;
	light.lookAt( lightPosition, Vec3f( 0.0f, 0.0f, 0.0f ) );
	light.setAmbient( mLightAmbient );
	light.setDiffuse( mLightDiffuse );
	light.setSpecular( mLightSpecular );
	light.setShadowParams( 60.0f, 5.0f, 5000.0f );
	light.enable();

	// enable lighting
	gl::enable( GL_LIGHTING );

	mShadowCamera = light.getShadowCamera();
	if ( mCurrentCamera.mType == TYPE_INDOOR )
	{
		mShadowMatrix = light.getShadowTransformationMatrix( mCurrentCamera.mCam );
	}
	else // outdoor camera
	{
		// set the originial camera without rotation and rotate the object instead
		CameraPersp cam = mCameras[ mCameraIndex ].mCam;
		cam.setFov( mFov );
		mShadowMatrix = light.getShadowTransformationMatrix( cam );

		/*
		Quatf q = mCurrentCamera.mCam.getOrientation();
		q.v = -q.v;
		Matrix44f rotinv( q );
		rotinv.invert();

		Matrix44f shadowTransMatrix = mShadowCamera.getProjectionMatrix();
		//shadowTransMatrix *= rotinv;
		shadowTransMatrix *= mShadowCamera.getModelViewMatrix();
		shadowTransMatrix *= cam.getInverseModelViewMatrix();
		shadowTransMatrix *= rotinv;
		mShadowMatrix = shadowTransMatrix;
		*/

		mShadowMapUpdateNeeded = true;
	}
}

void KecskeAr::disableLights()
{
	gl::disable( GL_LIGHTING );
}

void KecskeAr::draw()
{
	int state = 1 + mCameraIndex;

	if ( mInteractionMode == MODE_AR )
	{
		state = mTrackerManager.getState();
		if ( state <  TrackerManager::STATE_IDLE )
		{
			mCameraIndex = state;
		}

		mFov = lmap( mTrackerManager.getZoom(), .0f, 1.f, 10.f, 80.f );

		// original orientation * ar orientation
		Quatf oriInit = mCameras[ mCameraIndex ].mCam.getOrientation();
		// mirror rotation
		Quatf q = mTrackerManager.getRotation();
		q.v = -q.v;
		mCurrentCamera.mCam.setOrientation( oriInit * q );
	}

	if ( state == TrackerManager::STATE_IDLE )
	{
		gl::clear( Color::black() );
		gl::disableDepthRead();
		gl::disableDepthWrite();
		gl::setViewport( getWindowBounds() );
		gl::setMatricesWindow( getWindowSize() );

		if ( mLogo )
			gl::draw( mLogo, getWindowBounds() );
	}
	else
	{
		glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT );

		mFbo.bindFramebuffer();

		gl::clear( Color::black() );

		//gl::setViewport( getWindowBounds() );
		gl::setViewport( mFbo.getBounds() );
		mCurrentCamera.mCam.setFov( mFov );

		if ( mCurrentCamera.mType == TYPE_INDOOR )
		{
			gl::setMatrices( mCurrentCamera.mCam );
		}
		else // outdoor camera
		{
			// set the originial camera without rotation and rotate the object instead
			CameraPersp cam = mCameras[ mCameraIndex ].mCam;
			cam.setFov( mFov );
			gl::setMatrices( cam );
		}

		gl::enableDepthRead();
		gl::enableDepthWrite();

		// draw light position
		if ( mDrawShadowMap )
		{
			gl::color( Color( 1.f, 1.f, 0.f ) );
			gl::drawFrustum( mShadowCamera );
		}

		if ( mEnableLighting && mCurrentCamera.mLightingEnabled )
		{
			enableLights();
			renderShadowMap();
		}

		// enable texturing and bind texture to texture unit 1
		gl::enable( GL_TEXTURE_2D );
		mDepthFbo.bindDepthTexture( 1 );

		// bind the shader and set the uniform variables
		if ( mRenderShader && mEnableLighting && mCurrentCamera.mLightingEnabled )
		{
			mRenderShader.bind();
			mRenderShader.uniform( "diffuseTexture", 0 );
			mRenderShader.uniform( "shadowTexture", 1 );
			mRenderShader.uniform( "shadowMatrix", mShadowMatrix );
			mRenderShader.uniform( "shadowStrength", mShadowStrength );
			mRenderShader.uniform( "gamma", mGamma );
		}

		drawModel();
		if ( mEnableLighting && mCurrentCamera.mLightingEnabled )
		{
			disableLights();
		}

		if ( mRenderShader && mEnableLighting && mCurrentCamera.mLightingEnabled )
		{
			mRenderShader.unbind();
		}
		mDepthFbo.unbindTexture();

		glPopAttrib();
		mFbo.unbindFramebuffer();

		gl::disableDepthRead();
		gl::disableDepthWrite();
		gl::setMatricesWindow( getWindowSize() );

		// draw output on screen
		Rectf outputRect( mFbo.getBounds() );
		Rectf screenRect( getWindowBounds() );
		outputRect = outputRect.getCenteredFit( screenRect, true );
		if ( screenRect.getAspectRatio() > outputRect.getAspectRatio() )
			outputRect.scaleCentered( screenRect.getWidth() / outputRect.getWidth() );
		else
			outputRect.scaleCentered( screenRect.getHeight() / outputRect.getHeight() );
		if ( mPostProcessShader )
		{
			mPostProcessShader.bind();
			mPostProcessShader.uniform( "tex", 0 );
			mPostProcessShader.uniform( "hue", mHue );
			mPostProcessShader.uniform( "saturation", mSaturation );
			mPostProcessShader.uniform( "lightness", mLightness );
		}

		mFbo.getTexture().setFlipped();
		gl::draw( mFbo.getTexture(), outputRect );

		if ( mPostProcessShader )
		{
			mPostProcessShader.unbind();
		}
	}

	if ( mDrawShadowMap )
	{
		gl::setMatricesWindow( getWindowSize() );

		if ( mDepthShader )
		{
			mDepthShader.bind();
			mDepthShader.uniform( "tDepth", 0 );
			mDepthShader.uniform( "zNear", mShadowCamera.getNearClip() );
			mDepthShader.uniform( "zFar", mShadowCamera.getFarClip() );
		}

		mDepthFbo.bindDepthTexture();
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE );
		gl::color( Color::white() );
		gl::drawSolidRect( Rectf( 0, 256, 256, 0 ) );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
		mDepthFbo.unbindTexture();

		if ( mDepthShader )
		{
			mDepthShader.unbind();
		}
	}

	mTrackerManager.draw();

	params::InterfaceGl::draw();
}

void KecskeAr::drawModel()
{
	gl::pushModelView();

	// rotate the object if outdoor camera
	if ( mCurrentCamera.mType == TYPE_OUTDOOR )
	{
		Quatf q = mCurrentCamera.mCam.getOrientation();
		q.v = -q.v;
		//gl::rotate( q );
		gl::multModelView( q.toMatrix44() );
	}

	gl::color( Color::white() );
	//mAssimpLoader.draw();
	mDisplayList.draw();

	gl::popModelView();
}

void KecskeAr::shutdown()
{
	params::PInterfaceGl::save();
}

void KecskeAr::mouseDown( MouseEvent event )
{
	if ( mInteractionMode != MODE_MOUSE )
		return;

	mMayaCam.setCurrentCam( mCurrentCamera.mCam );
	mMayaCam.mouseDown( event.getPos() );
	mCurrentCamera.mCam = mMayaCam.getCamera();
}

void KecskeAr::mouseDrag( MouseEvent event )
{
	if ( mInteractionMode != MODE_MOUSE )
		return;

	mMayaCam.setCurrentCam( mCurrentCamera.mCam );
	mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
	mCurrentCamera.mCam = mMayaCam.getCamera();
}

void KecskeAr::resize()
{
	/*
	for ( CameraExt &ce : mCameras )
		ce.mCam.setAspectRatio( getWindowAspectRatio() );
	mCurrentCamera.mCam.setAspectRatio( getWindowAspectRatio() );
	*/
}

void KecskeAr::keyDown( KeyEvent event )
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
			params::PInterfaceGl::showAllParams( !mParams.isVisible(), false );
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

CINDER_APP_BASIC( KecskeAr, RendererGl )

