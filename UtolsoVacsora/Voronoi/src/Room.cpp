/*

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
#include "cinder/Sphere.h"
#include "cinder/TriMesh.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/Material.h"
#include "cinder/gl/gl.h"

#include "mndlkit/params/PParams.h"

#include "NIBlobTracker.h"
#include "Voronoi.h"

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

		static const int PLANE_SIZE = 32;
		TriMesh createSquare( const Vec2i &resolution );
		TriMesh mTrimeshPlane;

		enum InteractionState {
			INTERACTION_NONE = 0, // no interaction, happens when picking fails
			INTERACTION_CAMERA, // moving the camera
			INTERACTION_PICK, // picking chair
			INTERACTION_MOVE,
			INTERACTION_ROTATE,
			INTERACTION_SCALE
		};
		InteractionState mInteractionState = INTERACTION_CAMERA;
		Vec3f mPickedPoint; // point in the bounding box of the chair

		#define NUM_CHAIRS 0
		#define NUM_DANCERS 4
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

				void setScale( float s ) { mScale = s; }
				float getScale() { return mScale; }
				float *getScaleRef() { return &mScale; }

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
				float mScale = 1.f;
		};

		int mPickedEntity = -1;
		Entity mEntities[ NUM_CHAIRS + NUM_DANCERS ];
		float mLastPitch; // last pitch of picked entity before picking
		Vec3f mLastPosition; // last position
		float mLastScale; // last scale

		Vec2i mMousePos; // current mouse position
		Vec2i mMousePosPicked; // mouse position during picking

		bool performPicking();
		void performTransformation();

		MayaCamUI mMayaCam;
		CameraPersp mCamera;
		Vec3f mCameraPosition;
		Vec3f mCameraCenterOfInterestPoint;

		Voronoi mVoronoi;

		float mFps;
		bool mVerticalSyncEnabled;

		mndl::NIBlobTracker mTracker;
		bool mTrackerEnabled;
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
	mParams.addPersistentSizeAndPosition();

	mParams.addParam( "Fps", &mFps, "", true );
	mParams.addPersistentParam( "Vertical sync", &mVerticalSyncEnabled, true );
	mParams.addPersistentParam( "Tracker", &mTrackerEnabled, false );
	mParams.addSeparator();
	mParams.addText( "Light" );
	mParams.addPersistentParam( "Constant attenuation", &mLightConstantAttenuation, .5f, "min=0 step=.01" );
	mParams.addPersistentParam( "Linear attenuation", &mLightLinearAttenuation, .25f, "min=0 step=.01" );
	mParams.addPersistentParam( "Quadratic attenuation", &mLightQuadraticAttenuation, .05f, "min=0 step=.01" );
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
		mEntities[ j ].setScale( .1f );
		mParams.addPersistentParam( "Position " + toString( j ),
				mEntities[ j ].getPositionRef(), Vec3f( ( i - (float)NUM_DANCERS / 2.f ) * 2.f, 2.5f, 0.f ),
				"group='Dancer " + toString( i ) + "'" );
		mParams.addPersistentParam( "Rotation " + toString( j ),
				mEntities[ j ].getPitchRef(), 0.f,
				"group='Dancer " + toString( i ) + "'" );
		mParams.addPersistentParam( "Scale " + toString( j ),
				mEntities[ j ].getScaleRef(), .1f,
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
	mTrimeshPlane = createSquare( Vec2i( 64, 64 ) );

	mVoronoi.setup();
	mTracker.setup();
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

// based on Cinder-MeshHelper by Ban the Rewind
// https://github.com/BanTheRewind/Cinder-MeshHelper/
TriMesh Room::createSquare( const Vec2i &resolution )
{
	vector< uint32_t > indices;
	vector< Vec3f > normals;
	vector< Vec3f > positions;
	vector< Vec2f > texCoords;

	Vec3f norm0( 0.0f, 1.0f, 0.0f );

	Vec2f scale( 1.0f / math< float >::max( (float)resolution.x, 1.0f ),
				 1.0f / math<float>::max( (float)resolution.y, 1.0f ) );
	uint32_t index = 0;
	for ( int32_t y = 0; y < resolution.y; ++y )
	{
		for ( int32_t x = 0; x < resolution.x; ++x, ++index )
		{
			float x1 = (float)x * scale.x;
			float y1 = (float)y * scale.y;
			float x2 = (float)( x + 1 ) * scale.x;
			float y2 = (float)( y + 1 ) * scale.y;

			Vec3f pos0( x1 - 0.5f, 0.0f, y1 - 0.5f );
			Vec3f pos1( x2 - 0.5f, 0.0f, y1 - 0.5f );
			Vec3f pos2( x1 - 0.5f, 0.0f, y2 - 0.5f );
			Vec3f pos3( x2 - 0.5f, 0.0f, y2 - 0.5f );

			Vec2f texCoord0( x1, y1 );
			Vec2f texCoord1( x2, y1 );
			Vec2f texCoord2( x1, y2 );
			Vec2f texCoord3( x2, y2 );

			positions.push_back( pos2 );
			positions.push_back( pos1 );
			positions.push_back( pos0 );
			positions.push_back( pos1 );
			positions.push_back( pos2 );
			positions.push_back( pos3 );

			texCoords.push_back( texCoord2 );
			texCoords.push_back( texCoord1 );
			texCoords.push_back( texCoord0 );
			texCoords.push_back( texCoord1 );
			texCoords.push_back( texCoord2 );
			texCoords.push_back( texCoord3 );

			for ( uint32_t i = 0; i < 6; ++i )
			{
				indices.push_back( index * 6 + i );
				normals.push_back( norm0 );
			}
		}
	}

	TriMesh mesh;

	mesh.appendIndices( &indices[ 0 ], indices.size() );
	for ( auto normal: normals )
	{
		mesh.appendNormal( normal );
	}

	mesh.appendVertices( &positions[ 0 ], positions.size() );

	for ( auto texCoord: texCoords )
	{
		mesh.appendTexCoord( texCoord );
	}

	return mesh;
}

void Room::update()
{
	mFps = getAverageFps();
	if ( mVerticalSyncEnabled != gl::isVerticalSyncEnabled() )
		gl::enableVerticalSync( mVerticalSyncEnabled );

	// interaction state - pick, move, rotate
	switch ( mInteractionState )
	{
		case INTERACTION_PICK:
			if ( performPicking() )
			{
				mLastPosition = mEntities[ mPickedEntity ].getPosition();
				mLastPitch = mEntities[ mPickedEntity ].getPitch();
				mLastScale = mEntities[ mPickedEntity ].getScale();
			}
			else
			{
				mInteractionState = INTERACTION_NONE;
			}
			break;

		case INTERACTION_MOVE:
		case INTERACTION_ROTATE:
		case INTERACTION_SCALE:
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

	// update tracker
	mTracker.update();

	if ( mTrackerEnabled )
	{
		int n = math< int >::min( mTracker.getBlobNum(), NUM_DANCERS );
		const Vec2f normInv = Vec2f( PLANE_SIZE, PLANE_SIZE ) * .5f;
		for ( int i = 0; i < n; i++ )
		{
			float y = mEntities[ NUM_CHAIRS + i ].getPosition().y;
			Vec2f bp = ( mTracker.getBlobCentroid( i ) - Vec2f( .5f, .5f ) ) * normInv;
			mEntities[ NUM_CHAIRS + i ].setPosition( Vec3f( bp.x, y, bp.y ) );

			mEntities[ NUM_CHAIRS + i ].setPitch( M_PI / 2 - toRadians( mTracker.getBlobRotation( i ) ) );
			mEntities[ NUM_CHAIRS + i ].setScale( mTracker.getBlobRotatedRect( i ).getHeight() * normInv.x );
		}
	}

	// voronoi
	mVoronoi.clear();
	Vec2d norm = Vec2d( 2. / PLANE_SIZE, 2. / PLANE_SIZE );
	for ( int i = 0; i < NUM_DANCERS; i++ )
	{
		Vec2d p = mEntities[ NUM_CHAIRS + i ].getPosition().xz();
		float s = mEntities[ NUM_CHAIRS + i ].getScale();
		if ( s > .1f )
		{
			float r = mEntities[ NUM_CHAIRS + i ].getPitch();
			Vec2d d( s * .5, 0. );
			d.rotate( -r );
			Vec2d p0 = p + d;
			Vec2d p1 = p - d;
			mVoronoi.addSegment( p0 * norm + Vec2d( .5, .5 ),
								 p1 * norm + Vec2d( .5, .5 ) );
		}
		else
		{
			mVoronoi.addPoint( Vec2d( p ) * norm + Vec2d( .5, .5 ) );
		}
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

	std::unique_ptr< gl::Light > lights[ NUM_DANCERS ];
	for ( int i = 0; i < NUM_DANCERS; i++ )
	{
		lights[ i ] = std::unique_ptr< gl::Light >( new gl::Light( gl::Light::POINT, i ) );
		lights[ i ]->setPosition( mEntities[ NUM_CHAIRS + i ].getPosition() );
		lights[ i ]->setAmbient( ColorAf::gray( .3f ) );
		lights[ i ]->setDiffuse( ColorAf::gray( .5f ) );
		lights[ i ]->setConstantAttenuation( mLightConstantAttenuation );
		lights[ i ]->setLinearAttenuation( mLightLinearAttenuation );
		lights[ i ]->setQuadraticAttenuation( mLightQuadraticAttenuation );
		lights[ i ]->update( mCamera );
		lights[ i ]->enable();
	}

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
			gl::drawCube( Vec3f::zero(), Vec3f( mEntities[ i ].getScale(), .1f, .1f ) );
		gl::popModelView();
	}

	mVoronoi.draw();

	gl::enable( GL_TEXTURE_2D );
	mVoronoi.bindTexture();

	gl::pushModelView();
	material.setAmbient( Color::gray( .5f ) );
	material.apply();
	gl::scale( Vec3f( PLANE_SIZE * .5f, 1.f, PLANE_SIZE * .5f ) );
	gl::draw( mTrimeshPlane );
	gl::popModelView();

	mVoronoi.unbindTexture();
	gl::disable( GL_TEXTURE_2D );

	for ( int i = 0; i < NUM_DANCERS; i++ )
		lights[ i ]->disable();
	gl::disable( GL_LIGHTING );

	mTracker.draw();

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
		if ( i < NUM_CHAIRS )
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
		else // dancer spheres
		{
			Sphere boundingSphere( mEntities[ i ].getPosition(), mEntities[ i ].getScale() );
			float intersection;
			if ( boundingSphere.intersect( ray, &intersection ) )
			{
				if ( intersection < closest )
				{
					closest = intersection;
					mPickedEntity = i;
				}
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
		float v = ( mMousePos.x - mMousePosPicked.x ) / (float)getWindowWidth();
		mEntities[ mPickedEntity ].setPitch( mLastPitch + v * 2 * M_PI );
	}
	else
	if ( mInteractionState == INTERACTION_SCALE )
	{
		if ( mPickedEntity >= NUM_CHAIRS ) // scale dancers only
		{
			float v = ( mMousePos.x - mMousePosPicked.x ) / (float)getWindowWidth();
			mEntities[ mPickedEntity ].setScale( mLastScale + v );
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
	mTracker.shutdown();
	kit::params::PInterfaceGl::save();
}

void Room::mouseDown( MouseEvent event )
{
	mMousePos = event.getPos();

	if ( event.isShiftDown() || event.isControlDown() || event.isAltDown() )
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
		else
		if ( event.isAltDown() )
		{
			mInteractionState = INTERACTION_SCALE;
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

