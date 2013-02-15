/*
 Copyright (C) 2013 Gabor Papp

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
#include "cinder/CinderMath.h"
#include "cinder/MayaCamUI.h"
#include "cinder/ObjLoader.h"
#include "cinder/Quaternion.h"
#include "cinder/TriMesh.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/gl.h"

#include "mndlkit/params/PParams.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace mndl;

class Room : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void keyDown( KeyEvent event );
		void mouseDown( MouseEvent event );
		void mouseDrag( MouseEvent event );
		void mouseUp( MouseEvent event );
		void resize();

		void update();
		void draw();

		void shutdown();

	private:
		kit::params::PInterfaceGl mParams;
		Vec3f mLightDirection;

		void loadModel();
		TriMesh mTriMesh;
		MayaCamUI mMayaCam;

		enum InteractionState {
			INTERACTION_NONE = 0, // no interaction, happens when picking fails
			INTERACTION_CAMERA, // moving the camera
			INTERACTION_PICK, // picking chair
			INTERACTION_MOVE,
			INTERACTION_ROTATE
		};
		InteractionState mInteractionState = INTERACTION_CAMERA;
		Vec3f mPickedPoint; // point in the bounding box of the chair
		Matrix44f mTransform = Matrix44f::identity(); // chair transformation
		Matrix44f mLastTransform; // chair transformation before picking

		Vec2i mMousePos; // current mouse position
		Vec2i mMousePosPicked; // mouse position during picking

		bool performPicking( Vec3f *pickedPoint );
		void performTransformation();

		float mFps;
};

void Room::prepareSettings( Settings *settings )
{
	settings->setWindowSize( 1024, 640 );
}

void Room::setup()
{
	gl::disableVerticalSync();

	kit::params::PInterfaceGl::load( "params.xml" );
	mParams = kit::params::PInterfaceGl( "Parameters", Vec2i( 200, 300 ) );
	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addSeparator();
	mParams.addPersistentParam( "Light direction", &mLightDirection, Vec3f( -.2, -.1, 1 ).normalized() );

	loadModel();

	CameraPersp cam;
	cam.setPerspective( 60, getWindowAspectRatio(), 1, 500 );
	cam.setEyePoint( Vec3f( 0, 1, 10 ) );
	cam.setCenterOfInterestPoint( Vec3f::zero() );

	mMayaCam.setCurrentCam( cam );
}

void Room::loadModel()
{
	const fs::path objName = "chair.obj";
	fs::path meshName( objName );
	meshName.replace_extension( "msh" );

	try
	{
		// try to load the mesh from its binary file
		fs::path objFile = getAssetPath( fs::path( "model" ) / meshName );
		if ( !objFile.empty() )
			mTriMesh.read( loadFile( objFile ) );
		else
			throw ci::Exception();
	}
	catch ( ... )
	{
		// load model into mesh
		ObjLoader loader( loadAsset( fs::path( "model" ) / objName ) );
		loader.load( &mTriMesh );

		// one-time only: write mesh to binary file
		mTriMesh.write( writeFile( getAssetPath( "model" ) / meshName ) );
	}
}

void Room::update()
{
	mFps = getAverageFps();
}

void Room::draw()
{
	gl::clear( Color::white() );
	gl::setViewport( getWindowBounds() );
	gl::setMatrices( mMayaCam.getCamera() );

	gl::color( Color::gray( .5f ) );

	gl::enableDepthRead();
	gl::enableDepthWrite();

	switch ( mInteractionState )
	{
		case INTERACTION_PICK:
			if ( performPicking( &mPickedPoint ) )
			{
				mLastTransform = mTransform;
			}
			else
			{
				mInteractionState = INTERACTION_NONE;
			}
			break;

		case INTERACTION_MOVE:
		case INTERACTION_ROTATE:
			performTransformation();
			break;

		default:
			break;
	}

	gl::enable( GL_LIGHTING );

	gl::Light light( gl::Light::DIRECTIONAL, 0 );
	light.setDirection( -mLightDirection.normalized() );
	light.setAmbient( ColorAf::gray( 0.5 ) );
	light.setDiffuse( ColorAf( 1.0f, 1.0f, 1.0f, 1.0f ) );
	light.update( mMayaCam.getCamera() );
	light.enable();

	gl::pushModelView();
	gl::multModelView( mTransform );
	gl::draw( mTriMesh );
	gl::popModelView();

	light.disable();
	gl::disable( GL_LIGHTING );

	kit::params::PInterfaceGl::draw();
}

bool Room::performPicking( Vec3f *pickedPoint )
{
	// get our camera
	CameraPersp cam = mMayaCam.getCamera();

	// save mouse position for rotation
	mMousePosPicked = mMousePos;

	// generate a ray from the camera into our world
	float u = mMousePos.x / (float)getWindowWidth();
	float v = mMousePos.y / (float)getWindowHeight();
	// because OpenGL and Cinder use a coordinate system
	// where (0, 0) is in the LOWERleft corner, we have to flip the v-coordinate
	Ray ray = cam.generateRay( u,  1.0f - v, cam.getAspectRatio() );

	AxisAlignedBox3f mObjectBounds = mTriMesh.calcBoundingBox();
	AxisAlignedBox3f worldBoundsExact = mObjectBounds.transformed( mTransform );

	float intersections[ 2 ];
	int numIntersections = worldBoundsExact.intersect( ray, intersections );
	console() << numIntersections << " " << intersections[ 0 ] << endl;
	if ( numIntersections > 0 )
	{
		*pickedPoint = ray.calcPosition( intersections[ 0 ] );
		return true;
	}
	else
		return false;
}

void Room::performTransformation()
{
	if ( mInteractionState == INTERACTION_MOVE )
	{
		// get our camera
		CameraPersp cam = mMayaCam.getCamera();

		// generate a ray from the camera into our world
		float u = mMousePos.x / (float)getWindowWidth();
		float v = mMousePos.y / (float)getWindowHeight();
		// because OpenGL and Cinder use a coordinate system
		// where (0, 0) is in the LOWERleft corner, we have to flip the v-coordinate
		Ray ray = cam.generateRay( u,  1.0f - v, cam.getAspectRatio() );

		float result;
		if ( ray.calcPlaneIntersection( Vec3f( 0.f, mPickedPoint.y, 0.f ), Vec3f::yAxis(), &result ) )
		{
			Vec3f intersection = ray.calcPosition( result );
			Vec4f lastPosition = mLastTransform.getTranslate();
			mTransform.setTranslate( lastPosition.xyz() +
					Vec3f( intersection.x - mPickedPoint.x, 0.f,
						intersection.z - mPickedPoint.z ) );
		}
	}
	else
	if ( mInteractionState == INTERACTION_ROTATE )
	{
		float v = ( mMousePos.x - mMousePosPicked.x ) / (float)getWindowWidth();
		//Quatf lastQuat = Quatf( mLastTransform );
		mTransform = mLastTransform * Quatf( Vec3f::yAxis(), v * 2 * M_PI ).normalized();
	}
}

void Room::keyDown( KeyEvent event )
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
			kit::params::PInterfaceGl::showAllParams( !mParams.isVisible() );
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

void Room::shutdown()
{
	kit::params::PInterfaceGl::save();
}

void Room::mouseDown( MouseEvent event )
{
	mMousePos = event.getPos();

	if ( event.isShiftDown() || event.isControlDown() )
	{
		mInteractionState = INTERACTION_PICK;
	}
	else
	{
		mInteractionState = INTERACTION_CAMERA;
		mMayaCam.mouseDown( event.getPos() );
	}
}

void Room::mouseDrag( MouseEvent event )
{
	mMousePos = event.getPos();

	if ( mInteractionState == INTERACTION_PICK )
	{
		if ( event.isShiftDown() )
		{
			mInteractionState = INTERACTION_MOVE;
		}
		else
		if ( event.isControlDown() )
		{
			mInteractionState = INTERACTION_ROTATE;
		}
	}
	else
	if ( mInteractionState == INTERACTION_CAMERA )
	{
		mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
	}
}

void Room::mouseUp( MouseEvent event )
{
	mInteractionState = INTERACTION_CAMERA;
}

void Room::resize()
{
	CameraPersp cam = mMayaCam.getCamera();
	cam.setAspectRatio( getWindowAspectRatio() );
	mMayaCam.setCurrentCam( cam );
}

CINDER_APP_BASIC( Room, RendererGl )

