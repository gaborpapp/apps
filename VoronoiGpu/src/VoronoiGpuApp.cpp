#include <algorithm>

#include "cinder/app/AppBasic.h"
#include "cinder/ip/Hdr.h"
#include "cinder/gl/Texture.h"
#include "VoronoiGpu.h"

#include <vector>

using namespace ci;
using namespace ci::app;
using namespace std;

class VoronoiGpuApp : public AppBasic {
 public:
	void setup();
	void calculateVoronoiTexture();

	void mouseDown( MouseEvent event );
	void mouseDrag( MouseEvent event );
	void mouseUp( MouseEvent event );
	void keyDown( KeyEvent event );

	struct closePoint
	{
		closePoint( const Vec2i &pnt ) :
			compareTo( pnt) {}
		bool operator()( Vec2i pnt ) const
		{
			return ( compareTo.distanceSquared( pnt ) < 100 );
		}
		Vec2i compareTo;
	};

	void draw();

	vector<Vec2i>::iterator	mPointMovedIt;
	vector<Vec2i>	mPoints;
	gl::Texture		mTexture;
	bool			mShowDistance;
};

void VoronoiGpuApp::setup()
{
	mShowDistance = false;
}

void VoronoiGpuApp::calculateVoronoiTexture()
{
	if( mShowDistance ) {
		Channel32f rawDistanceMap = calcDistanceMapGpu( mPoints, getWindowWidth(), getWindowHeight() );
		// we need to convert the raw distances into a normalized range of 0-1 so we can show them sensibly
		ip::hdrNormalize( &rawDistanceMap );
		mTexture = gl::Texture( rawDistanceMap );
	}
	else {
		Surface32f rawDistanceMap = calcDiscreteVoronoiGpu( mPoints, getWindowWidth(), getWindowHeight() );
		// we need to convert the site locations into a normalized range of 0-1 so we can show them sensibly
		ip::hdrNormalize( &rawDistanceMap );
		mTexture = gl::Texture( rawDistanceMap );
	}
}

void VoronoiGpuApp::mouseDown( MouseEvent event )
{
	if ( event.isLeft() )
	{
		mPoints.push_back( event.getPos() );
	}
	else if ( event.isRight() )
	{
		vector< Vec2i >::iterator it;

		it = find_if( mPoints.begin(), mPoints.end(),
				closePoint( event.getPos() ) );
		if ( it != mPoints.end() )
		{
			mPoints.erase( it );
		}
	}
	calculateVoronoiTexture();
}

void VoronoiGpuApp::mouseDrag( MouseEvent event )
{
	if ( event.isMiddleDown() )
	{
		vector< Vec2i >::iterator it;

		it = find_if( mPoints.begin(), mPoints.end(),
				closePoint( event.getPos() ) );
		if ( it != mPoints.end() )
		{
			it->set( event.getPos() );
			mPointMovedIt = it;
		}
		else if ( mPointMovedIt != mPoints.end() )
		{
			mPointMovedIt->set( event.getPos() );
		}
	}

	calculateVoronoiTexture();
}

void VoronoiGpuApp::mouseUp( MouseEvent event )
{
	if ( event.isMiddle() )
	{
		mPointMovedIt = mPoints.end();
	}
}

void VoronoiGpuApp::keyDown( KeyEvent event )
{
	if( event.getChar() == 'd' ) {
		mShowDistance = ! mShowDistance;
		calculateVoronoiTexture();
	}
	else if( event.getChar() == 'x' ) {
		mPoints.clear();
		calculateVoronoiTexture();
	}
	else if( event.getChar() == 'f' )
	{
		setFullScreen( !isFullScreen() );
	}

	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void VoronoiGpuApp::draw()
{
	gl::clear();
	gl::setMatricesWindow( getWindowSize() );

	gl::color( Color( 1, 1, 1 ) );
	if( mTexture ) {
		gl::draw( mTexture );
		mTexture.disable();
	}

	// draw the voronoi sites in yellow
	gl::color( Color( 1.0f, 1.0f, 0.0f ) );	
	for( vector<Vec2i>::const_iterator ptIt = mPoints.begin(); ptIt != mPoints.end(); ++ptIt )
		gl::drawSolidCircle( Vec2f( *ptIt ), 2.0f );

	/*
	gl::enableAlphaBlending();
	gl::drawStringRight( "Click to add a point", Vec2f( getWindowWidth() - 10, getWindowHeight() - 20 ), Color( 1, 0.3, 0 ) );
	gl::disableAlphaBlending();
	*/
}

// This line tells Cinder to actually create the application
CINDER_APP_BASIC( VoronoiGpuApp, RendererGl )
