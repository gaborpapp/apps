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

		Matrix44f mShadowMatrix;
		CameraPersp mShadowCamera;

		gl::Fbo mDepthFbo;
		gl::GlslProg mRenderShader;

		bool mDrawShadowMap;
		bool mShadowMapUpdateNeeded = true;
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
	mParams.addParam( "Fov", &mFov, "min=10 max=90" );
	mParams.addPersistentParam( "Light direction", &mLightDirection, Vec3f( -1.f, -1.f, 1.f ) );
	mParams.addPersistentParam( "Light ambient", &mLightAmbient, Color::black() );
	mParams.addPersistentParam( "Light diffuse", &mLightDiffuse, Color::white() );
	mParams.addPersistentParam( "Light specular", &mLightDiffuse, Color::white() );
	mParams.addPersistentParam( "Shadow strength", &mShadowStrength, .9f, "min=0 max=1 step=.05" );

	mDrawShadowMap = false;
	mParams.addParam( "Draw shadowmap", &mDrawShadowMap );

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



	mTrackerManager.setup();
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
    static const int size = 2048;

    // create a frame buffer object (FBO) containing only a depth buffer
    // NOTE: without a color buffer it hangs on OSX 10.8 with some Geforce cards
    // https://forum.libcinder.org/topic/anyone-else-experiencing-shadow-mapping-problems-with-the-new-xcode
    mDepthFbo = gl::Fbo( size, size, false, true, true );

    // set it up for shadow mapping
    mDepthFbo.bindDepthTexture();
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    mDepthFbo.unbindTexture();
}

void KecskeAr::renderShadowMap()
{
	if ( !mShadowMapUpdateNeeded )
		return;

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
	mDepthFbo.unbindFramebuffer();

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
	light.setShadowParams( 60.0f, 5.0f, 3000.0f );
	light.enable();

	// enable lighting
	gl::enable( GL_LIGHTING );

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
	}
	mShadowCamera = light.getShadowCamera();
}

void KecskeAr::disableLights()
{
	gl::disable( GL_LIGHTING );
}

void KecskeAr::draw()
{
	glPushAttrib( GL_ENABLE_BIT | GL_CURRENT_BIT );

	gl::clear( Color::black() );

	if ( mInteractionMode == MODE_AR )
	{
		mFov = lmap( mTrackerManager.getZoom(), .0f, 1.f, 10.f, 90.f );

		// original orientation * ar orientation
		Quatf oriInit = mCameras[ mCameraIndex ].mCam.getOrientation();
		// mirror rotation
		Quatf q = mTrackerManager.getRotation();
		q.v = -q.v;
		mCurrentCamera.mCam.setOrientation( oriInit * q );
	}

	gl::setViewport( getWindowBounds() );
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

	enableLights();

	renderShadowMap();

	// enable texturing and bind texture to texture unit 1
	gl::enable( GL_TEXTURE_2D );
	mDepthFbo.bindDepthTexture( 1 );

	// bind the shader and set the uniform variables
	if ( mRenderShader )
	{
		mRenderShader.bind();
		mRenderShader.uniform( "diffuseTexture", 0 );
		mRenderShader.uniform( "shadowTexture", 1 );
		mRenderShader.uniform( "shadowMatrix", mShadowMatrix );
		mRenderShader.uniform( "shadowStrength", mShadowStrength );
	}

	drawModel();
	disableLights();

	if ( mRenderShader )
	{
		mRenderShader.unbind();
	}
	mDepthFbo.unbindTexture();


	mTrackerManager.draw();

	glPopAttrib();

	if ( mDrawShadowMap )
	{
		/*
		gl::setMatricesWindow( getWindowSize() );

		gl::disableDepthRead();
		gl::disableDepthWrite();

        mDepthFbo.getDepthTexture().enableAndBind();

        // we'd like to draw the depth map as a normal texture,
        // so let's temporarily disable the texture compare mode
        glPushAttrib( GL_TEXTURE_BIT );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE );
        glTexParameteri( GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE );

        // optional: invert colors when drawing, so black is far away and white is nearby
        //glEnable( GL_BLEND );
        //glBlendFuncSeparate( GL_ONE_MINUS_SRC_COLOR, GL_ZERO, GL_SRC_ALPHA, GL_DST_ALPHA );

        gl::color( Color::white() );
        gl::drawSolidRect( Rectf( 0, 256, 256, 0 ), false );

        //glDisable( GL_BLEND );

        glPopAttrib();

        mDepthFbo.unbindTexture();
		*/
	}

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
		gl::rotate( q );
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
	for ( CameraExt &ce : mCameras )
		ce.mCam.setAspectRatio( getWindowAspectRatio() );
	mCurrentCamera.mCam.setAspectRatio( getWindowAspectRatio() );
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

