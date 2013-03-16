/*
 Copyright (C) 2012-2013 Gabor Papp

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

 Thanks to Community Core Vision, http://nuicode.com/projects/tbeta
 and Patricio Gonzalez Vivo for ofxBlobTracker,
 https://github.com/patriciogonzalezvivo/ofxBlobTracker
*/

#include <list>
#include <map>

#include "boost/date_time.hpp"

#include "cinder/app/App.h"
#include "cinder/Area.h"
#include "cinder/Utilities.h"

#include "NIBlobTracker.h"
#include "CinderOpenCV.h"
//#include "Utils.h"

using namespace ci;
using namespace std;

namespace mndl {

void NIBlobTracker::setup()
{
	// OpenNI
	mKinectProgress = "Connecting...\0\0\0\0\0\0\0\0\0";
	mKinectThread = thread( bind( &NIBlobTracker::openKinect, this, fs::path() ) );

	mParams = mndl::kit::params::PInterfaceGl( "Tracker", Vec2i( 350, 400 ) );
	mParams.addPersistentSizeAndPosition();

	setupGui();
}

void NIBlobTracker::setupGui()
{
	// params
	mndl::kit::params::PInterfaceGl::save();
	mParams.clear();

	vector< string > enumNames{ "Recording", "Kinect" };
	mParams.addPersistentParam( "Source", enumNames, &mSource, SOURCE_CAMERA );
	mParams.addParam( "Kinect", &mKinectProgress, "", true );

	mParams.addSeparator();
	if ( mSource == SOURCE_CAMERA )
	{
		mParams.addButton( "Save video", std::bind( &NIBlobTracker::saveVideoCB, this ) );
	}
	else
	{
	    mParams.addButton( "Play video", std::bind( &NIBlobTracker::playVideoCB, this ) );
	}

	mParams.addSeparator();

	mParams.addText( "Tracking parameters" );
	mParams.addPersistentParam( "Flip", &mFlip, true );

	mParams.addPersistentParam( "Threshold", &mThreshold, 150, "min=0 max=255");
	mParams.addPersistentParam( "Blur size", &mBlurSize, 10, "min=1 max=15" );
	mParams.addPersistentParam( "Min area", &mMinArea, 0.0001f, "min=0.0 max=1.0 step=0.0001" );
	mParams.addPersistentParam( "Max area", &mMaxArea, 0.2f, "min=0.0 max=1.0 step=0.001" );

	mParams.addSeparator();
	mParams.addText( "Debug" );

	vector< string > enumNamesDraw{ "None", "Original", "Blurred", "Thresholded" };
	mParams.addPersistentParam( "Draw capture", enumNamesDraw, &mDrawCapture, 0, "key='c'" );
}

void NIBlobTracker::openKinect( const fs::path &path )
{
	try
	{
		mndl::ni::OpenNI kinect;
		mndl::ni::OpenNI::Options options;
		options.enableImage( false ).enableIR( false ).enableUserTracker( false );

		if ( path.empty() )
			kinect = mndl::ni::OpenNI( mndl::ni::OpenNI::Device(), options );
		else
			kinect = mndl::ni::OpenNI( path, options );
		{
			std::lock_guard< std::mutex > lock( mKinectMutex );
			mNI = kinect;
		}
	}
	catch ( const mndl::ni::OpenNIExc &exc )
	{
		if ( path.empty() )
			mKinectProgress = "No device detected";
		else
			mKinectProgress = "Recording not found";
		return;
	}

	if ( path.empty() )
		mKinectProgress = "Connected";
	else
		mKinectProgress = "Recording loaded";

	mNI.start();
}

void NIBlobTracker::update()
{
	static int lastSource = mSource;

	// select source
	bool processFrame = false;
	// TODO: make this work with Surface16u
	Surface8u inputSurface;

	if ( mSource == SOURCE_CAMERA )
	{
		// start kinect
		if ( lastSource == SOURCE_RECORDING )
		{
			mNI.start();
			mKinectProgress = "Started";
		}

		{
			std::lock_guard< std::mutex > lock( mKinectMutex );
			// normalized camera coordinates mapping
			mNormMapping = RectMapping( Rectf( 0.0f, 0.0f, (float)CAPTURE_WIDTH, (float)CAPTURE_HEIGHT ),
					Rectf( 0.0f, 0.0f, 1.0f, 1.0f ) );

			processFrame = mNI && mNI.checkNewDepthFrame();
			if ( processFrame )
			{
				inputSurface = mNI.getDepthImage();

				Surface::Iter iter = inputSurface.getIter();
				while ( iter.line() )
				{
					while( iter.pixel() )
					{
						// leave self-shadow pixels and invert the rest
						if ( iter.r() != 0 )
						{
							iter.r() = 255 - iter.r();
							iter.g() = 255 - iter.g();
							iter.b() = 255 - iter.b();
						}
					}
				}
			}
		}
	}
	else // SOURCE_RECORDING
	{
		// stop kinect
		if ( lastSource == SOURCE_CAMERA )
		{
			mNI.stop();
			mKinectProgress = "Stopped";
		}

		if ( mMovie )
		{
			// normalized movie coordinates mapping
			mNormMapping = RectMapping( Rectf( 0.0f, 0.0f, (float)mMovie.getWidth(), (float)mMovie.getHeight() ),
					Rectf( 0.0f, 0.0f, 1.0f, 1.0f ) );

			processFrame = mMovie.checkNewFrame();
			if ( processFrame )
				inputSurface = mMovie.getSurface();
		}
	}

	// process source image
	if ( processFrame )
	{
		if ( mSavingVideo )
			mMovieWriter.addFrame( inputSurface );

		// opencv
		cv::Mat input( toOcv( Channel8u( inputSurface ) ) );
		if ( mFlip )
			cv::flip( input, input, 1 );

		cv::Mat blurred, thresholded;

		cv::blur( input, blurred, cv::Size( mBlurSize, mBlurSize ) );
		cv::threshold( blurred, thresholded, mThreshold, 255, CV_THRESH_BINARY );

		mTextureOrig = gl::Texture( fromOcv( input ) );
		mTextureBlurred = gl::Texture( fromOcv( blurred ) );
		mTextureThresholded = gl::Texture( fromOcv( thresholded ) );

		vector< vector< cv::Point > > contours;
		cv::findContours( thresholded, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE );

		float surfArea = inputSurface.getWidth() * inputSurface.getHeight();
		float minAreaLimit = surfArea * mMinArea;
		float maxAreaLimit = surfArea * mMaxArea;

		vector< BlobRef > newBlobs;
		for ( vector< vector< cv::Point > >::iterator cit = contours.begin(); cit < contours.end(); ++cit )
		{
			BlobRef b = BlobRef( new Blob() );
			cv::Mat pmat = cv::Mat( *cit );
			cv::Rect cvRect = cv::boundingRect( pmat );
			b->mBbox = Rectf( cvRect.x, cvRect.y,
							cvRect.x + cvRect.width, cvRect.y + cvRect.height );
			float area = b->mBbox.calcArea();
			if ( ( minAreaLimit <= area ) && ( area < maxAreaLimit ) )
			{
				cv::Moments m = cv::moments( pmat );
				b->mCentroid = Vec2f( m.m10 / m.m00, m.m01 / m.m00 );

				b->mBbox = mNormMapping.map( b->mBbox );
				b->mCentroid = b->mPrevCentroid = mNormMapping.map( b->mCentroid );
				newBlobs.push_back( b );
			}
		}

		trackBlobs( newBlobs );
	}

	// change gui buttons if switched between capture and playback
	if ( lastSource != mSource )
	{
		setupGui();
		lastSource = mSource;
	}

}

void NIBlobTracker::trackBlobs( vector< BlobRef > newBlobs )
{
	// all new blob id's initialized with -1

	// step 1: match new blobs with existing nearest ones
	for ( size_t i = 0; i < mBlobs.size(); i++ )
	{
		int32_t winner = findClosestBlobKnn( newBlobs, mBlobs[ i ], 3, 0 );

		if ( winner == -1 ) // track has died
		{
			mBlobsEndedSig( BlobEvent( mBlobs[ i ] ) );
			mBlobs[ i ]->mId = -1; // marked for deletion
		}
		else
		{
			// if winning new blob was labeled winner by another track
			// then compare with this track to see which is closer
			if ( newBlobs[ winner ]->mId != -1 )
			{
				// find the currently assigned blob
				int j; // j will be the index of it
				for ( j = 0; j < mBlobs.size(); j++ )
				{
					if ( mBlobs[ j ]->mId == newBlobs[ winner ]->mId )
						break;
				}

				if ( j == mBlobs.size() ) // got to end without finding it
				{
					newBlobs[ winner ]->mId = mBlobs[ i ]->mId;
					mBlobs[ i ] = newBlobs[ winner ];
				}
				else // found it, compare with current blob
				{
					Vec2f p = newBlobs[ winner ]->mCentroid;
					Vec2f pOld = mBlobs[ j ]->mCentroid;
					Vec2f pNew = mBlobs[ i ]->mCentroid;
					float distOld = p.distanceSquared( pOld );
					float distNew = p.distanceSquared( pNew );

					// if this track is closer, update the Id of the blob
					// otherwise delete this track.. it's dead
					if ( distNew < distOld ) // update
					{
						newBlobs[ winner ]->mId = mBlobs[ i ]->mId;
						/* TODO
						   now the old winning blob has lost the win.
						   I should also probably go through all the newBlobs
						   at the end of this loop and if there are ones without
						   any winning matches, check if they are close to this
						   one. Right now I'm not doing that to prevent a
						   recursive mess. It'll just be a new track.
						 */
						mBlobsEndedSig( BlobEvent( mBlobs[ j ] ) );
						// mark the blob for deletion
						mBlobs[ j ]->mId = -1;
					}
					else // delete
					{
						mBlobsEndedSig( BlobEvent( mBlobs[ i ] ) );
						// mark the blob for deletion
						mBlobs[ i ]->mId = -1;
					}
				}
			}
			else // no conflicts, so simply update
			{
				newBlobs[ winner ]->mId = mBlobs[ i ]->mId;
			}
		}
	}

	// step 2: blob update
	//
	// update all current tracks
	// remove every track labeled as dead, id = -1
	// find every track that's alive and copy its data from newBlobs
	for ( size_t i = 0; i < mBlobs.size(); i++ )
	{
		if ( mBlobs[ i ]->mId == -1 ) // dead
		{
			// erase track
			mBlobs.erase( mBlobs.begin() + i, mBlobs.begin() + i + 1 );
			i--; // decrement one since we removed an element
		}
		else // living, so update its data
		{
			for ( int j = 0; j < newBlobs.size(); j++ )
			{
				if ( mBlobs[ i ]->mId == newBlobs[ j ]->mId )
				{
					// update track
					// store the last centroid
					newBlobs[ j ]->mPrevCentroid = mBlobs[ i ]->mCentroid;
					mBlobs[ i ] = newBlobs[ j ];

					Vec2f tD = mBlobs[ i ]->mCentroid - mBlobs[ i ]->mPrevCentroid;

					// calculate the acceleration
					float posDelta = tD.length();
					if ( posDelta > 0.001 )
					{
						mBlobsMovedSig( BlobEvent( mBlobs[ i ] ) );
					}

					// TODO: add other blob features
				}
			}
		}
	}

	// step 3: add tracked blobs to touchevents
	// -- add new living tracks
	// now every new blob should be either labeled with a tracked id or
	// have id of -1. if the id is -1, we need to make a new track.
	for ( size_t i = 0; i < newBlobs.size(); i++ )
	{
		if ( newBlobs[ i ]->mId == -1 )
		{
			// add new track
			newBlobs[ i ]->mId = mIdCounter;
			mIdCounter++;

			mBlobs.push_back( newBlobs[ i ] );

			mBlobsBeganSig( BlobEvent( newBlobs[ i ] ) );
		}
	}
}

/** Finds the blob in newBlobs that is closest to the blob \a track.
 * \param newBlobs list of blobs detected in the last frame
 * \param track current blob
 * \param k number of nearest neighbours, must be odd number (1, 3, 5 are common)
 * \param thres optimization threshold
 * Returns the closest blob id if found or -1
 */
int32_t NIBlobTracker::findClosestBlobKnn( const vector< BlobRef > &newBlobs, BlobRef track,
		int k, double thresh )
{
	int32_t winner = -1;
	if ( thresh > 0. )
		thresh *= thresh;

	// list of neighbour point index and respective distances
	list< pair< size_t, double > > nbors;
	list< pair< size_t, double > >::iterator iter;

	// find 'k' closest neighbors of testpoint
	Vec2f p;
	Vec2f pT = track->mCentroid;
	float distSquared;

	// search for blobs
	for ( size_t i = 0; i < newBlobs.size(); i++ )
	{
		p = newBlobs[ i ]->mCentroid;
		distSquared = p.distanceSquared( pT );

		if ( distSquared <= thresh )
		{
			winner = i;
			return winner;
		}

		// check if this blob is closer to the point than what we've seen
		// so far and add it to the index/distance list if positive

		// search the list for the first point with a longer distance
		for ( iter = nbors.begin();
				iter != nbors.end() && distSquared >= iter->second; ++iter );

		if ( ( iter != nbors.end() ) || ( nbors.size() < k ) )
		{
			nbors.insert( iter, 1, std::pair< size_t, double >( i, distSquared ));
			// too many items in list, get rid of farthest neighbor
			if ( nbors.size() > k )
				nbors.pop_back();
		}
	}


	/********************************************************************
	 * we now have k nearest neighbors who cast a vote, and the majority
	 * wins. we use each class average distance to the target to break any
	 * possible ties.
	 *********************************************************************/

	// a mapping from labels (IDs) to count/distance
	map< int32_t, pair< size_t, double > > votes;

	// remember:
	// iter->first = index of newBlob
	// iter->second = distance of newBlob to current tracked blob
	for ( iter = nbors.begin(); iter != nbors.end(); ++iter )
	{
		// add up how many counts each neighbor got
		size_t count = ++( votes[ iter->first ].first );
		double dist = ( votes[ iter->first ].second += iter->second );

		// check for a possible tie and break with distance
		if ( ( count > votes[ winner ].first ) ||
				( ( count == votes[ winner ].first ) &&
				  ( dist < votes[ winner ].second ) ) )
		{
			winner = iter->first;
		}
	}

	return winner;
}

size_t NIBlobTracker::getBlobNum() const
{
	return mBlobs.size();
}

Rectf NIBlobTracker::getBlobBoundingRect( size_t i ) const
{
	return mBlobs[ i ]->mBbox;
}

Vec2f NIBlobTracker::getBlobCentroid( size_t i ) const
{
	return mBlobs[ i ]->mCentroid;
}

void NIBlobTracker::draw()
{
	gl::pushMatrices();
	gl::setMatricesWindow( app::getWindowSize() );
	gl::setViewport( app::getWindowBounds() );

	gl::enableAlphaBlending();
	gl::disableDepthRead();
	gl::disableDepthWrite();

	if ( ( mDrawCapture > 0 ) && mTextureOrig )
	{
		gl::color( ColorA::gray( 1.f, .5f ) );

		Area outputArea = app::getWindowBounds();
		/*
			Area::proportionalFit( mTextureOrig.getBounds(),
					app::getWindowBounds(), true, true );
		*/

		Rectf captureDrawRect = Rectf( outputArea );

		gl::Texture txt;
		switch ( mDrawCapture )
		{
			case DRAW_ORIGINAL:
				txt = mTextureOrig;
				break;

			case DRAW_BLURRED:
				txt = mTextureBlurred;
				break;

			case DRAW_THRESHOLDED:
				txt = mTextureThresholded;
				break;

			default:
				break;
		}

		gl::draw( txt, captureDrawRect );

		RectMapping blobMapping( Rectf( 0, 0, 1, 1 ), captureDrawRect );
		for ( size_t i = 0; i < mBlobs.size(); ++i )
		{
			Vec2f pos = blobMapping.map( getBlobCentroid( i ) );
			gl::color( ColorA( 1, 0, 0, .5 ) );
			gl::drawStrokedRect( blobMapping.map( getBlobBoundingRect( i ) ) );
			gl::drawSolidCircle( pos, 2 );
			gl::drawString( toString< int32_t >( mBlobs[ i ]->mId ), pos + Vec2f( 3, -3 ),
					ColorA( 1, 0, 0, .9 ) );
		}
	}

	gl::color( Color::white() );
	gl::disableAlphaBlending();
	gl::popMatrices();
}

void NIBlobTracker::playVideoCB()
{
	fs::path appPath( app::getAppPath() );
#ifdef CINDER_MAC
	appPath /= "..";
#endif
	fs::path moviePath = app::getOpenFilePath( appPath );

	if ( !moviePath.empty() )
	{
		try
		{
			mMovie = qtime::MovieSurface( moviePath );
			mMovie.setLoop();
			mMovie.play();
		}
		catch (...)
		{
			app::console() << "Unable to load movie " << moviePath << endl;
			mMovie.reset();
		}
	}
}

void NIBlobTracker::saveVideoCB()
{
	if ( mSavingVideo )
	{
		mParams.setOptions( "Save video", "label=`Save video`" );
		mMovieWriter.finish();
	}
	else
	{
		mParams.setOptions( "Save video", "label=`Finish saving`" );

		qtime::MovieWriter::Format format;
		format.setCodec( qtime::MovieWriter::CODEC_H264 );
		format.setQuality( 0.5f );
		format.setDefaultDuration( 1. / 25. );

		fs::path appPath = app::getAppPath();
#ifdef CINDER_MAC
		appPath /= "..";
#endif
		boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
		string timestamp = boost::posix_time::to_iso_string( now );

		mMovieWriter = qtime::MovieWriter( appPath /
				fs::path( "capture-" + timestamp + ".mov" ),
				CAPTURE_WIDTH, CAPTURE_HEIGHT, format );
	}
	mSavingVideo = !mSavingVideo;
}

void NIBlobTracker::shutdown()
{
	mKinectThread.join();

	if ( mNI )
	{
		std::lock_guard< std::mutex > lock( mKinectMutex );
		mNI.stop();
	}
}

} // namspace mndl

