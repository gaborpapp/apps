#include "cinder/app/App.h"

#include "cinder/gl/gl.h"

#include "cinder/Area.h"
#include "cinder/Cinder.h"
#include "cinder/CinderMath.h"
#include "cinder/Matrix.h"
#include "cinder/Timeline.h"

#include "TrackerManager.h"

using namespace ci;
using namespace std;

TrackerManager::~TrackerManager()
{
	if ( mCapture )
	{
		mCapture.stop();
	}
}

void TrackerManager::setup()
{
	mParams = params::PInterfaceGl( "AR Tracker", Vec2i( 200, 300 ), Vec2i( 232, 16 ) );
	mParams.addPersistentSizeAndPosition();

	setupCapture();
	mParams.addPersistentParam( "Rotation smoothness", &mRotationSmoothness, 0.1f, "min=0.01 max=1.00 step=.01" );
	mParams.addPersistentParam( "Tolerance", &mTolerance, 1.f, "min=0 max=5 step=0.1 "
			"help='duration in seconds for which the missing marker cube is tolerated' " );
	mParams.addPersistentParam( "Minimum distance", &mMinZ, 300.f, "min=10 max=2000 step=10" );
	mParams.addPersistentParam( "Maximum distance", &mMaxZ, 1000.f, "min=10 max=3000 step=10" );
	mParams.addPersistentParam( "Minimum scale", &mMinScale, .1f, "min=.05 max=1 step=.05" );
	mParams.addPersistentParam( "Maximum scale", &mMaxScale, 3.f, "min=1 max=20 step=.05" );

	mParams.addSeparator();
	mParams.addParam( "Rotation", &mRotation, "", false );
	mParams.addParam( "X", &mPosition.x, "group=ArPosition", false );
	mParams.addParam( "Y", &mPosition.y, "group=ArPosition", false );
	mParams.addParam( "Z", &mPosition.z, "group=ArPosition", false );

	mParams.addSeparator();
	mParams.addPersistentParam( "Debug tracking", &mDebugTracking, false );
	mParams.addPersistentParam( "Debug size", &mDebugSize, .3f, "min=.1f max=.8f step=.05f" );

	// ar tracker
	mndl::artkp::ArTracker::Options options;
	options.setCameraFile( app::getAssetPath( "camera_para.dat" ) );
	options.setMultiMarker( app::getAssetPath( "marker_cube.cfg" ) );
	mArTracker = mndl::artkp::ArTracker( CAPTURE_WIDTH, CAPTURE_HEIGHT, options );

	// rotation
	mBackToInit = true;
}

void TrackerManager::setupCapture()
{
	// list out the capture devices
	vector< Capture::DeviceRef > devices( Capture::getDevices() );
	vector< string > deviceNames;

	for ( vector< Capture::DeviceRef >::const_iterator deviceIt = devices.begin();
			deviceIt != devices.end(); ++deviceIt )
	{
		Capture::DeviceRef device = *deviceIt;
		string deviceName = device->getName();

		try
		{
			if ( device->checkAvailable() )
			{
				mCaptures.push_back( Capture( CAPTURE_WIDTH, CAPTURE_HEIGHT,
							device ) );
				deviceNames.push_back( deviceName );
			}
			else
			{
				mCaptures.push_back( Capture() );
				deviceNames.push_back( deviceName + " not available" );
			}
		}
		catch ( CaptureExc & )
		{
			app::console() << "Unable to initialize device: " << device->getName() <<
				endl;
		}
	}

	if ( deviceNames.empty() )
	{
		deviceNames.push_back( "Camera not available" );
		mCaptures.push_back( Capture() );
	}

	mParams.addPersistentParam( "Capture", deviceNames, &mCurrentCapture, 0 );
	if ( mCurrentCapture >= deviceNames.size() )
		mCurrentCapture = 0;
}

void TrackerManager::update()
{
	static int lastCapture = -1;

	// switch between capture devices
	if ( lastCapture != mCurrentCapture )
	{
		if ( ( lastCapture >= 0 ) && ( mCaptures[ lastCapture ] ) )
			mCaptures[ lastCapture ].stop();

		if ( mCaptures[ mCurrentCapture ] )
			mCaptures[ mCurrentCapture ].start();

		mCapture = mCaptures[ mCurrentCapture ];
		lastCapture = mCurrentCapture;
	}

	// detect the markers
	if ( mCapture && mCapture.checkNewFrame() )
	{
		Surface8u captSurf( mCapture.getSurface() );
		mCaptTexture = gl::Texture( captSurf );
		mArTracker.update( captSurf );
	}

	// rotation quaternion calculation

	//static float scale;
	bool markerFound = false;
	for ( int i = 0; i < mArTracker.getNumMarkers(); i++ )
	{
		// id -1 means unknown marker, false positive
		if ( mArTracker.getMarkerId( i ) == -1 )
			continue;

		// gets the modelview matrix of the i'th marker
		Matrix44f m = mArTracker.getModelView( i );

		mPosition = m.getTranslate().xyz();

		float scale = lmap< float >( math< float >::clamp( mPosition.z, mMinZ, mMaxZ ),
									mMinZ, mMaxZ, mMaxScale, mMinScale );
		mScale = lerp( mScale, scale, mRotationSmoothness );

		// rotation
		Quatf q( m );
		q *= Quatf( Vec3f( 1, 0, 0 ), M_PI );

		mRotation = mRotation.slerp( mRotationSmoothness, q );

		markerFound = true;
		mBackToInit = boost::indeterminate;

		// multi marker, one id is enough
		break;
	}

	if ( !markerFound )
	{
		if ( boost::indeterminate( mBackToInit ) )
		{
			// allow loosing the cube for a small amount of time
			mBackToInit = false;
			app::timeline().add( [ & ] { mBackToInit = true; },
					app::timeline().getCurrentTime() + mTolerance );
		}
		else
		if ( mBackToInit == true )
		{
			// lost the cube for a longer period, start moving back to original position
			mRotation = mRotation.slerp( mRotationSmoothness, Quatf( Vec3f( 1, 0, 0 ), .0 ) );
			mScale = lerp( mScale, 1.f, mRotationSmoothness );
		}
	}

	mRotation.normalize();
}

void TrackerManager::draw()
{
	if ( !mDebugTracking )
		return;

	gl::pushMatrices();

	gl::disableDepthRead();
	gl::disableDepthWrite();

	// draws camera image
	Area mDebugArea = Area( Rectf( app::getWindowBounds() ) * mDebugSize +
			Vec2f( app::getWindowSize() ) * Vec2f( 1.f - mDebugSize, .0f ) );
	gl::color( Color::white() );
	Area outputArea = Area::proportionalFit( Area( 0, 0, CAPTURE_WIDTH, CAPTURE_HEIGHT ), mDebugArea, false );
	if ( outputArea.getX2() < app::getWindowWidth() )
	{
		outputArea.offset( Vec2i( app::getWindowWidth() - outputArea.getX2(), 0 ) );
	}

	gl::setViewport( app::getWindowBounds() );
	gl::setMatricesWindow( app::getWindowSize() );
	if ( mCaptTexture )
	{
		gl::draw( mCaptTexture, outputArea );
	}

	gl::enableDepthRead();
	gl::enableDepthWrite();

	gl::setViewport( outputArea + Vec2i( 0, app::getWindowHeight() - outputArea.getHeight() ) );
	// sets the projection matrix
	mArTracker.setProjection();

	// scales the pattern according the the pattern width
	Vec3f patternScale = Vec3f::one() * mArTracker.getOptions().getPatternWidth();
	for ( int i = 0; i < mArTracker.getNumMarkers(); i++ )
	{
		// id -1 means unknown marker, false positive
		if ( mArTracker.getMarkerId( i ) == -1 )
			continue;

		// sets the modelview matrix of the i'th marker
		mArTracker.setModelView( i );
		gl::scale( patternScale );
		gl::drawColorCube( Vec3f::zero(), Vec3f::one() );
		break;
	}

	gl::popMatrices();
}
