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
#include "cinder/gl/Material.h"
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
		float mLightConstantAttenuation;
		float mLightLinearAttenuation;
		float mLightQuadraticAttenuation;

		void loadModel();
		TriMesh mTriMesh;

		enum InteractionState {
			INTERACTION_NONE = 0, // no interaction, happens when picking fails
			INTERACTION_CAMERA, // moving the camera
			INTERACTION_PICK, // picking chair
			INTERACTION_MOVE,
			INTERACTION_ROTATE
		};
		InteractionState mInteractionState = INTERACTION_CAMERA;
		Vec3f mPickedPoint; // point in the bounding box of the chair

		#define NUM_CHAIRS 3
		#define NUM_DANCERS 1
		struct Entity
		{
			public:
				void setPitch( float p ) { mPitch = p; mTransformCached = false; }
				void setPosition( const Vec3f &p ) { mPosition = p; mTransformCached = false; }
				float getPitch() const { return mPitch; }
				const Vec3f &getPosition() const { return mPosition; }
				float *getPitchRef() { return &mPitch; }
				Vec3f *getPositionRef() { return &mPosition; }

				void setColor( const Color &c ) { mColor = c; }
				const Color &getColor() { return mColor; }
				Color *getColorRef() { return &mColor; }

				const Matrix44f &getTransform()
				{
					if ( !mTransformCached )
						calcTransform();
					return mTransform;
				}

				void calcTransform()
				{
					mTransform.setToIdentity();
					mTransform = Quatf( Vec3f::yAxis(), mPitch );
					mTransform.setTranslate( mPosition );
				}

			protected:
				Matrix44f mTransform;
				Vec3f mPosition = Vec3f::zero();
				float mPitch = 0.f; // rotation around y
				bool mTransformCached = false;
				Color mColor = Color::gray( .5f );
		};

		int mPickedEntity = -1;
		Entity mEntities[ NUM_CHAIRS + NUM_DANCERS ];
		float mLastPitch; // last pitch of picked entity before picking
		Vec3f mLastPosition; // last position

		Vec2i mMousePos; // current mouse position
		Vec2i mMousePosPicked; // mouse position during picking

		bool performPicking();
		void performTransformation();

		MayaCamUI mMayaCam;
		CameraPersp mCamera;
		Vec3f mCameraPosition;
		Vec3f mCameraCenterOfInterestPoint;

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
	mParams.addText( "Light" );
	mParams.addPersistentParam( "Constant attenuation", &mLightConstantAttenuation, 1.f, "min=0 step=.01" );
	mParams.addPersistentParam( "Linear attenuation", &mLightLinearAttenuation, .0f, "min=0 step=.01" );
	mParams.addPersistentParam( "Quadratic attenuation", &mLightQuadraticAttenuation, .0f, "min=0 step=.01" );
	mParams.addSeparator();
	mParams.addText( "Chairs" );
	for ( int i = 0; i < NUM_CHAIRS; i++ )
	{
		mParams.addPersistentParam( "Position " + toString( i ),
				mEntities[ i ].getPositionRef(), Vec3f( ( i - (float)NUM_CHAIRS / 2.f ) * 2.f, 0.f, 0.f ),
				"group='Chair " + toString( i ) + "'" );
		mParams.addPersistentParam( "Pitch " + toString( i ), mEntities[ i ].getPitchRef(), 0.f,
				"group='Chair " + toString( i ) + "'" );
		mParams.setOptions( "Chair " + toString( i ), "opened=false" );
	}
	mParams.addSeparator();
	mParams.addText( "Dancers" );
	for ( int j = NUM_CHAIRS, i = 0; j < NUM_CHAIRS + NUM_DANCERS; j++, i++ )
	{
		mEntities[ j ].setColor( Color( 1.f, .9f, .1f ) );
		mParams.addPersistentParam( "Position " + toString( j ),
				mEntities[ j ].getPositionRef(), Vec3f( ( i - (float)NUM_DANCERS / 2.f ) * 2.f, 1.f, 0.f ),
				"group='Dancer " + toString( i ) + "'" );
		mParams.setOptions( "Dancer " + toString( i ), "opened=false" );
	}
	mParams.addText( "Camera" );
	mParams.addPersistentParam( "Eye", &mCameraPosition, Vec3f( 0, 1, 10 ), "", true );
	mParams.addPersistentParam( "Center of Interest", &mCameraCenterOfInterestPoint,
			Vec3f::zero(), "", true );
	mCamera.setPerspective( 60, getWindowAspectRatio(), 1, 500 );
	mCamera.setEyePoint( mCameraPosition );
	mCamera.setCenterOfInterestPoint( mCameraCenterOfInterestPoint );
	mMayaCam.setCurrentCam( mCamera );

	loadModel();
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

	// interaction state - pick, move, rotate
	switch ( mInteractionState )
	{
		case INTERACTION_PICK:
			if ( performPicking() )
			{
				mLastPosition = mEntities[ mPickedEntity ].getPosition();
				mLastPitch = mEntities[ mPickedEntity ].getPitch();
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

	// update camera params
	if ( mCameraPosition != mCamera.getEyePoint() )
	{
		mCameraPosition = mCamera.getEyePoint();
	}

	if ( mCameraCenterOfInterestPoint != mCamera.getCenterOfInterestPoint() )
	{
		mCameraCenterOfInterestPoint = mCamera.getCenterOfInterestPoint();
	}
}

void Room::draw()
{
	gl::clear( Color::gray( .1f ) );
	gl::setViewport( getWindowBounds() );
	gl::setMatrices( mCamera );

	gl::enableDepthRead();
	gl::enableDepthWrite();

	gl::enable( GL_LIGHTING );

	gl::Light light( gl::Light::POINT, 0 );
	light.setPosition( mEntities[ NUM_CHAIRS ].getPosition() );
	light.setAmbient( ColorAf::gray( 0.5 ) );
	light.setDiffuse( ColorAf( 1.0f, 1.0f, 1.0f, 1.0f ) );
	light.setConstantAttenuation( mLightConstantAttenuation );
	light.setLinearAttenuation( mLightLinearAttenuation );
	light.setQuadraticAttenuation( mLightQuadraticAttenuation );
	light.update( mCamera );
	light.enable();

	// default opengl material
	gl::Material material( Color::gray( .2f ), Color::gray( .8f ) );
	for ( int i = 0; i < NUM_CHAIRS + NUM_DANCERS; i++ )
	{
		material.setAmbient( mEntities[ i ].getColor() );
		material.apply();

		gl::pushModelView();
		gl::multModelView( mEntities[ i ].getTransform() );
		if ( i < NUM_CHAIRS )
			gl::draw( mTriMesh );
		else
			gl::drawSphere( Vec3f::zero(), .1f );
		gl::popModelView();
	}

	light.disable();
	gl::disable( GL_LIGHTING );

	kit::params::PInterfaceGl::draw();
}

bool Room::performPicking()
{
	// save mouse position for rotation
	mMousePosPicked = mMousePos;

	// generate a ray from the camera into our world
	float u = mMousePos.x / (float)getWindowWidth();
	float v = mMousePos.y / (float)getWindowHeight();
	// because OpenGL and Cinder use a coordinate system
	// where (0, 0) is in the LOWERleft corner, we have to flip the v-coordinate
	Ray ray = mCamera.generateRay( u, 1.0f - v, mCamera.getAspectRatio() );

	AxisAlignedBox3f mObjectBounds = mTriMesh.calcBoundingBox();

	float closest = numeric_limits< float >::max();
	mPickedEntity = -1;
	for ( int i = 0; i < NUM_CHAIRS + NUM_DANCERS; i++ )
	{
		const Matrix44f &transform = mEntities[ i ].getTransform();
		AxisAlignedBox3f worldBounds = mObjectBounds.transformed( transform );
		float intersections[ 2 ];
		int numIntersections = worldBounds.intersect( ray, intersections );
		if ( numIntersections > 0 )
		{
			if ( intersections[ 0 ] < closest )
			{
				closest = intersections[ 0 ];
				mPickedEntity = i;
			}
		}
	}

	if ( mPickedEntity != -1 )
	{
		mPickedPoint = ray.calcPosition( closest );
		return true;
	}
	else
	{
		return false;
	}
}

void Room::performTransformation()
{
	if ( mPickedEntity == -1)
		return;

	if ( mInteractionState == INTERACTION_MOVE )
	{
		// generate a ray from the camera into our world
		float u = mMousePos.x / (float)getWindowWidth();
		float v = mMousePos.y / (float)getWindowHeight();
		// because OpenGL and Cinder use a coordinate system
		// where (0, 0) is in the LOWERleft corner, we have to flip the v-coordinate
		Ray ray = mCamera.generateRay( u, 1.0f - v, mCamera.getAspectRatio() );

		float result;
		if ( ray.calcPlaneIntersection( Vec3f( 0.f, mPickedPoint.y, 0.f ), Vec3f::yAxis(), &result ) )
		{
			Vec3f intersection = ray.calcPosition( result );
			mEntities[ mPickedEntity ].setPosition( mLastPosition +
												 Vec3f( intersection.x - mPickedPoint.x, 0.f,
														intersection.z - mPickedPoint.z ) );
		}
	}
	else
	if ( mInteractionState == INTERACTION_ROTATE )
	{
		if ( mPickedEntity >= NUM_CHAIRS ) // move along y for dancer entities
		{
			// generate a ray from the camera into our world
			float u = mMousePos.x / (float)getWindowWidth();
			float v = mMousePos.y / (float)getWindowHeight();
			Ray ray = mCamera.generateRay( u, 1.0f - v, mCamera.getAspectRatio() );

			Vec3f right, up;
			mCamera.getBillboardVectors( &right, &up );
			float result;
			if ( ray.calcPlaneIntersection( Vec3f( mPickedPoint.x, 0.f, mPickedPoint.z ),
						right.cross( Vec3f( 0.f, 1.f, 0.f ) ), &result ) )
			{
				Vec3f intersection = ray.calcPosition( result );
				mEntities[ mPickedEntity ].setPosition( mLastPosition +
						Vec3f( 0.f, intersection.y - mPickedPoint.y, 0.f ) );
			}
		}
		else // rotation for chairs
		{
			float v = ( mMousePos.x - mMousePosPicked.x ) / (float)getWindowWidth();
			mEntities[ mPickedEntity ].setPitch( mLastPitch + v * 2 * M_PI );
		}
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
		mMayaCam.setCurrentCam( mCamera );
		mMayaCam.mouseDown( event.getPos() );
		mCamera = mMayaCam.getCamera();
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
		mMayaCam.setCurrentCam( mCamera );
		mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
		mCamera = mMayaCam.getCamera();
	}
}

void Room::mouseUp( MouseEvent event )
{
	mInteractionState = INTERACTION_CAMERA;
}

void Room::resize()
{
	mCamera.setAspectRatio( getWindowAspectRatio() );
}

CINDER_APP_BASIC( Room, RendererGl )

