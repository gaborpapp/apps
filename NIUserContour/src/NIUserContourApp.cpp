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
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Vbo.h"
#include "cinder/params/Params.h"

#include "CinderOpenCV.h"
#include "opencv2/imgproc/imgproc.hpp"

#include "Resources.h"

#include "CiNI.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace mndl;

class NIUserMaskApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void update();
		void draw();

	private:
		ni::OpenNI mNI;
		ni::UserTracker mNIUserTracker;

		float mBlurAmt;
		float mErodeAmt;
		float mDilateAmt;
		int mThres;
		float mOutlineWidth;

		Shape2d mShape;

		ci::gl::GlslProg mShader;
		std::vector< ci::gl::VboMesh > mVboMeshes;

		params::InterfaceGl mParams;
};

void NIUserMaskApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize( 640, 480 );
}

void NIUserMaskApp::setup()
{
	try
	{
		mNI = ni::OpenNI( ni::OpenNI::Device() );
		//mNI = ni::OpenNI( getAppPath() / "../recording.oni" );
	}
	catch ( ... )
	{
		console() << "Could not open Kinect" << endl;
		quit();
	}

	mNIUserTracker = mNI.getUserTracker();

	mNI.setMirrored();
	mNI.start();

	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 150 ) );
	mBlurAmt = 15.0;
	mParams.addParam( "blur", &mBlurAmt, "min=1 max=15 step=.5" );
	mErodeAmt = 13.0;
	mParams.addParam( "erode", &mErodeAmt, "min=1 max=15 step=.5" );
	mDilateAmt = 7.0;
	mParams.addParam( "dilate", &mDilateAmt, "min=1 max=15 step=.5" );
	mThres = 128;
	mParams.addParam( "threshold", &mThres, "min=1 max=254" );
	mOutlineWidth = 4.0;
	mParams.addParam( "width", &mOutlineWidth, "min=0.5 max=50 step=.05" );

	mShader = gl::GlslProg( app::loadResource( RES_LINE_VERT ),
							app::loadResource( RES_LINE_FRAG ),
							app::loadResource( RES_LINE_GEOM ),
							GL_LINES_ADJACENCY_EXT, GL_TRIANGLE_STRIP, 7 );
}

void NIUserMaskApp::update()
{
	if ( mNI.checkNewVideoFrame() )
	{
		Surface8u maskSurface = mNIUserTracker.getUserMask();

		cv::Mat cvMask, cvMaskFiltered;
		cvMask = toOcv( Channel8u( maskSurface ) );
		cv::blur( cvMask, cvMaskFiltered, cv::Size( mBlurAmt, mBlurAmt ) );

		cv::Mat dilateElm = cv::getStructuringElement( cv::MORPH_RECT,
				cv::Size( mDilateAmt, mDilateAmt ) );
		cv::Mat erodeElm = cv::getStructuringElement( cv::MORPH_RECT,
				cv::Size( mErodeAmt, mErodeAmt ) );
		cv::erode( cvMaskFiltered, cvMaskFiltered, erodeElm, cv::Point( -1, -1 ), 1 );
		cv::dilate( cvMaskFiltered, cvMaskFiltered, dilateElm, cv::Point( -1, -1 ), 3 );
		cv::erode( cvMaskFiltered, cvMaskFiltered, erodeElm, cv::Point( -1, -1 ), 1 );
		cv::blur( cvMaskFiltered, cvMaskFiltered, cv::Size( mBlurAmt, mBlurAmt ) );

		cv::threshold( cvMaskFiltered, cvMaskFiltered, mThres, 255, CV_THRESH_BINARY);

		vector< vector< cv::Point > > contours;
		cv::findContours( cvMaskFiltered, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE );

		mShape.clear();
		for ( vector< vector< cv::Point > >::const_iterator it = contours.begin();
				it != contours.end(); ++it )
		{
			vector< cv::Point >::const_iterator pit = it->begin();

			if ( it->empty() )
				continue;

			mShape.moveTo( fromOcv( *pit ) );

			++pit;
			for ( ; pit != it->end(); ++pit )
			{
				mShape.lineTo( fromOcv( *pit ) );
			}
			mShape.close();
		}
	}

	// update vbo from shape points
	// based on the work of Paul Houx
	// https://forum.libcinder.org/topic/smooth-thick-lines-using-geometry-shader#23286000001297067
	mVboMeshes.clear();

	for ( size_t i = 0; i < mShape.getNumContours(); ++i )
	{
		const Path2d &path = mShape.getContour( i );
		const vector< Vec2f > &points = path.getPoints();

			if ( points.size() > 1 )
			{
				// create a new vector that can contain 3D vertices
				vector< Vec3f > vertices;

				vertices.reserve( points.size() );

				// add all 2D points as 3D vertices
				vector< Vec2f >::const_iterator it;
				for ( it = points.begin() ; it != points.end(); ++it )
					vertices.push_back( Vec3f( *it ) );

				// now that we have a list of vertices, create the index buffer
				size_t n = vertices.size();

				vector< uint32_t > indices;
				indices.reserve( n * 4 );

				// line loop
				indices.push_back( n - 1 );
				indices.push_back( 0 );
				indices.push_back( 1 );
				indices.push_back( 2 );
				for ( size_t i = 1; i < vertices.size() - 2; ++i )
				{
					indices.push_back( i - 1 );
					indices.push_back( i );
					indices.push_back( i + 1 );
					indices.push_back( i + 2 );
				}
				indices.push_back( n - 3 );
				indices.push_back( n - 2 );
				indices.push_back( n - 1 );
				indices.push_back( 0 );

				indices.push_back( n - 2 );
				indices.push_back( n - 1 );
				indices.push_back( 0 );
				indices.push_back( 1 );


				// finally, create the mesh
				gl::VboMesh::Layout layout;
				layout.setStaticPositions();
				layout.setStaticIndices();
				gl::VboMesh vboMesh = gl::VboMesh( vertices.size(), indices.size(), layout, GL_LINES_ADJACENCY_EXT );
				vboMesh.bufferPositions( &(vertices.front()), vertices.size() );
				vboMesh.bufferIndices( indices );
				vboMesh.unbindBuffers();

				mVboMeshes.push_back( vboMesh );
			}
	}
}

void NIUserMaskApp::draw()
{
	gl::enableDepthRead();
	gl::enableDepthWrite();
	gl::enableAlphaBlending();
	gl::clear( Color::white() );
	gl::setMatricesWindow( getWindowSize() );

	gl::color( Color::black() );
	mShader.bind();
	mShader.uniform( "WIN_SCALE", Vec2f( getWindowSize() ) );
	mShader.uniform( "MITER_LIMIT", .75f );
	mShader.uniform( "THICKNESS", mOutlineWidth );

	for ( vector< gl::VboMesh >::const_iterator vit = mVboMeshes.begin();
			vit != mVboMeshes.end(); ++vit )
	{
		gl::draw( *vit );
	}

	mShader.unbind();
	gl::disableAlphaBlending();
	gl::disableDepthRead();
	gl::disableDepthWrite();

	mParams.draw();
}

CINDER_APP_BASIC( NIUserMaskApp, RendererGl() )

