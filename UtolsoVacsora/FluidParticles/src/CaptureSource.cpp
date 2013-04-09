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

#include "boost/date_time.hpp"

#include "cinder/app/App.h"

#include "CaptureSource.h"

using namespace std;

namespace mndl {

void CaptureSource::setup()
{
	// capture1394
	try
	{
		mCapture1394PParams = Capture1394PParams::create();
	}
	catch( Capture1394Exc &exc )
	{
		ci::app::console() << exc.what() << endl;
		ci::app::App::get()->quit();
	}

	// capture
	// list out the capture devices
	vector< ci::Capture::DeviceRef > devices( ci::Capture::getDevices() );

	for ( auto deviceIt = devices.cbegin(); deviceIt != devices.cend(); ++deviceIt )
	{
		ci::Capture::DeviceRef device = *deviceIt;
		string deviceName = device->getName(); // + " " + device->getUniqueId();

		try
		{
			if ( device->checkAvailable() )
			{
				mCaptures.push_back( ci::Capture::create( 640, 480, device ) );
				mDeviceNames.push_back( deviceName );
			}
			else
			{
				mCaptures.push_back( ci::CaptureRef() );
				mDeviceNames.push_back( deviceName + " not available" );
			}
		}
		catch ( ci::CaptureExc & )
		{
			ci::app::console() << "Unable to initialize device: " << device->getName() << endl;
		}
	}

	if ( mDeviceNames.empty() )
	{
		mDeviceNames.push_back( "Camera not available" );
		mCaptures.push_back( ci::CaptureRef() );
	}

	mParams = mndl::params::PInterfaceGl( "Capture Source", ci::Vec2i( 310, 90 ), ci::Vec2i( 16, 326 ) );
	mParams.addPersistentSizeAndPosition();
	mCaptureParams = mndl::params::PInterfaceGl( "Capture", ci::Vec2i( 310, 90 ), ci::Vec2i( 16, 432 ) );
	mCaptureParams.addPersistentSizeAndPosition();
	mCaptureParams.addPersistentParam( "Camera", mDeviceNames, &mCurrentCapture, 0 );
	if ( mCurrentCapture >= (int)mCaptures.size() )
		mCurrentCapture = 0;
	setupParams();

}

void CaptureSource::setupParams()
{
	mndl::params::PInterfaceGl::save();
	mParams.clear();

	vector< string > enumNames = { "Recording", "Capture", "Capture1394" };
	mParams.addPersistentParam( "Source", enumNames, &mSource, SOURCE_CAPTURE );

	mParams.addSeparator();
	if ( mSource == SOURCE_RECORDING )
	{
		mParams.addButton( "Play video", std::bind( &CaptureSource::playVideoCB, this ) );
	}
	else
	{
		mParams.addButton( "Save video", std::bind( &CaptureSource::saveVideoCB, this ) );
	}
}

void CaptureSource::update()
{
	static int lastCapture = -1;
	static int lastSource = -1;

	// change gui buttons if switched between capture and playback
	if ( lastSource != mSource )
	{
		setupParams();
		lastSource = mSource;
	}

	// capture
	if ( mSource == SOURCE_CAPTURE )
	{
		// stop and start capture devices
		if ( lastCapture != mCurrentCapture )
		{
			if ( ( lastCapture >= 0 ) && ( mCaptures[ lastCapture ] ) )
				mCaptures[ lastCapture ]->stop();

			if ( mCaptures[ mCurrentCapture ] )
				mCaptures[ mCurrentCapture ]->start();

			lastCapture = mCurrentCapture;
		}
	}
	else // SOURCE_RECORDING or SOURCE_CAPTURE1394
	{
		// stop capture device
		if ( ( lastCapture != -1 ) && ( mCaptures[ lastCapture ] ) )
		{
			mCaptures[ lastCapture ]->stop();
			lastCapture = -1;
		}

		if ( mSource == SOURCE_CAPTURE1394 )
		{
			try
			{
				mCapture1394PParams->update();
			}
			catch( Capture1394Exc &exc )
			{
				ci::app::console() << exc.what() << endl;
			}
		}
	}
}

void CaptureSource::shutdown()
{
	if ( ( mSource == SOURCE_CAPTURE ) && ( mCaptures[ mCurrentCapture ] ) )
		mCaptures[ mCurrentCapture ]->stop();
}

bool CaptureSource::isCapturing() const
{
	switch ( mSource )
	{
		case SOURCE_CAPTURE:
			return mCaptures[ mCurrentCapture ].get() != 0;
			break;

		case SOURCE_CAPTURE1394:
			return mCapture1394PParams->getCurrentCaptureRef().get() != 0;
			break;

		case SOURCE_RECORDING:
			return mMovie;
			break;

		default:
			return false;
			break;
	}
}

bool CaptureSource::checkNewFrame()
{
	switch ( mSource )
	{
		case SOURCE_CAPTURE:
			return mCaptures[ mCurrentCapture ]->checkNewFrame();
			break;

		case SOURCE_CAPTURE1394:
			return mCapture1394PParams->getCurrentCaptureRef()->checkNewFrame();
			break;

		case SOURCE_RECORDING:
			return mMovie.checkNewFrame();
			break;

		default:
			return false;
			break;
	}
}

int32_t CaptureSource::getWidth() const
{
	switch ( mSource )
	{
		case SOURCE_CAPTURE:
			return mCaptures[ mCurrentCapture ]->getWidth();
			break;

		case SOURCE_CAPTURE1394:
			return mCapture1394PParams->getCurrentCaptureRef()->getWidth();
			break;

		case SOURCE_RECORDING:
			return mMovie.getWidth();
			break;

		default:
			return 0;
			break;
	}
}

int32_t CaptureSource::getHeight() const
{
	switch ( mSource )
	{
		case SOURCE_CAPTURE:
			return mCaptures[ mCurrentCapture ]->getHeight();
			break;

		case SOURCE_CAPTURE1394:
			return mCapture1394PParams->getCurrentCaptureRef()->getHeight();
			break;

		case SOURCE_RECORDING:
			return mMovie.getHeight();
			break;

		default:
			return 0;
			break;
	}
}

ci::Surface8u CaptureSource::getSurface()
{
	switch ( mSource )
	{
		case SOURCE_CAPTURE:
			return mCaptures[ mCurrentCapture ]->getSurface();
			break;

		case SOURCE_CAPTURE1394:
			return mCapture1394PParams->getCurrentCaptureRef()->getSurface();
			break;

		case SOURCE_RECORDING:
			return mMovie.getSurface();
			break;

		default:
			return ci::Surface8u();
			break;
	}
}

void CaptureSource::saveVideoCB()
{
	if ( mSavingVideo )
	{
		mParams.setOptions( "Save video", "label=`Save video`" );
		mMovieWriter.finish();
	}
	else
	{
		mParams.setOptions( "Save video", "label=`Finish saving`" );

		ci::qtime::MovieWriter::Format format;
		format.setCodec( ci::qtime::MovieWriter::CODEC_H264 );
		format.setQuality( 0.5f );
		format.setDefaultDuration( 1. / 25. );

		ci::fs::path appPath = ci::app::getAppPath();
#ifdef CINDER_MAC
		appPath /= "..";
#endif
		boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
		string timestamp = boost::posix_time::to_iso_string( now );

		ci::Vec2i size;
		if ( mSource == SOURCE_CAPTURE )
		{
			size = mCaptures[ mCurrentCapture ]->getSize();
		}
		else // SOURCE_CAPTURE1394
		{
			size = mCapture1394PParams->getCurrentCaptureRef()->getSize();
		}

		mMovieWriter = ci::qtime::MovieWriter( appPath /
				ci::fs::path( "capture-" + timestamp + ".mov" ),
				size.x, size.y, format );
	}
	mSavingVideo = !mSavingVideo;
}

void CaptureSource::playVideoCB()
{
	ci::fs::path appPath( ci::app::getAppPath() );
#ifdef CINDER_MAC
	appPath /= "..";
#endif
	ci::fs::path moviePath = ci::app::getOpenFilePath( appPath );

	if ( !moviePath.empty() )
	{
		try
		{
			mMovie = ci::qtime::MovieSurface( moviePath );
			mMovie.setLoop();
			mMovie.play();
		}
		catch ( ... )
		{
			ci::app::console() << "Unable to load movie " << moviePath << endl;
			mMovie.reset();
		}
	}
}

} // namespace mndl

