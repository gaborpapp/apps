#include "cinder/app/App.h"

#include "NI.h"

using namespace xn;
using namespace std;
using namespace ci;
using namespace ci::app;

namespace cinder {

#define CHECK_RC( rc, what ) \
	if (rc != XN_STATUS_OK) \
	{ \
		console () << "OpenNI Error - " << what << " : " << xnGetStatusString(rc) << endl; \
	}

class ImageSourceOpenNIColor : public ImageSource {
	public:
		ImageSourceOpenNIColor( uint8_t *buffer, int w, int h, shared_ptr<OpenNI::Obj> ownerObj )
			: ImageSource(), mData( buffer ), mOwnerObj( ownerObj )
		{
			setSize( w, h );
			setColorModel( ImageIo::CM_RGB );
			setChannelOrder( ImageIo::RGB );
			setDataType( ImageIo::UINT8 );
		}

		~ImageSourceOpenNIColor()
		{
			// let the owner know we are done with the buffer
			mOwnerObj->mColorBuffers.derefBuffer( mData );
		}

		virtual void load( ImageTargetRef target )
		{
			ImageSource::RowFunc func = setupRowFunc( target );

			for( int32_t row = 0; row < mHeight; ++row )
				((*this).*func)( target, row, mData + row * mWidth * 3 );
		}

	protected:
		shared_ptr<OpenNI::Obj>     mOwnerObj;
		uint8_t                     *mData;
};

class ImageSourceOpenNIInfrared : public ImageSource {
	public:
		ImageSourceOpenNIInfrared( uint8_t *buffer, int w, int h, shared_ptr<OpenNI::Obj> ownerObj )
			: ImageSource(), mData( buffer ), mOwnerObj( ownerObj )
		{
			setSize( w, h );
			setColorModel( ImageIo::CM_GRAY );
			setChannelOrder( ImageIo::Y );
			setDataType( ImageIo::UINT8 );
		}

		~ImageSourceOpenNIInfrared()
		{
			// let the owner know we are done with the buffer
			mOwnerObj->mColorBuffers.derefBuffer( mData );
		}

		virtual void load( ImageTargetRef target )
		{
			ImageSource::RowFunc func = setupRowFunc( target );

			for( int32_t row = 0; row < mHeight; ++row )
				((*this).*func)( target, row, mData + row * mWidth );
		}

	protected:
		shared_ptr<OpenNI::Obj>		mOwnerObj;
		uint8_t					*mData;
};

class ImageSourceOpenNIDepth : public ImageSource {
	public:
		ImageSourceOpenNIDepth( uint16_t *buffer, int w, int h, shared_ptr<OpenNI::Obj> ownerObj )
			: ImageSource(), mData( buffer ), mOwnerObj( ownerObj )
		{
			setSize( w, h );
			setColorModel( ImageIo::CM_GRAY );
			setChannelOrder( ImageIo::Y );
			setDataType( ImageIo::UINT16 );
		}

		~ImageSourceOpenNIDepth()
		{
			// let the owner know we are done with the buffer
			mOwnerObj->mDepthBuffers.derefBuffer( mData );
		}

		virtual void load( ImageTargetRef target )
		{
			ImageSource::RowFunc func = setupRowFunc( target );

			for( int32_t row = 0; row < mHeight; ++row )
				((*this).*func)( target, row, mData + row * mWidth );
		}

	protected:
		shared_ptr<OpenNI::Obj>     mOwnerObj;
		uint16_t                    *mData;
};

OpenNI::OpenNI(Device device)
	: mObj( new Obj( device.mIndex ) )
{
}

OpenNI::OpenNI( const fs::path &recording )
	: mObj( new Obj( recording ) )
{
}

void OpenNI::start()
{
	mObj->start();
}

OpenNI::Obj::Obj( int deviceIndex )
	: mShouldDie( false ),
	  mNewDepthFrame( false ),
	  mNewVideoFrame( false ),
	  mVideoInfrared( false ),
	  mRecording( false )
{
	XnStatus rc = mContext.Init();
	CHECK_RC(rc, "context");

	// depth
	rc = mDepthGenerator.Create(mContext);
	if (rc != XN_STATUS_OK)
		throw ExcFailedDepthGeneratorInit();
	mDepthGenerator.GetMetaData( mDepthMD );
	mDepthWidth = mDepthMD.FullXRes();
	mDepthHeight = mDepthMD.FullYRes();
	mDepthMaxDepth = mDepthGenerator.GetDeviceMaxDepth();

	mDepthBuffers = BufferManager<uint16_t>( mDepthWidth * mDepthHeight, this );

	// image
	rc = mImageGenerator.Create(mContext);
	if (rc != XN_STATUS_OK)
		throw ExcFailedImageGeneratorInit();
	mImageGenerator.GetMetaData( mImageMD );
	mImageWidth = mImageMD.FullXRes();
	mImageHeight = mImageMD.FullYRes();

	mColorBuffers = BufferManager<uint8_t>( mImageWidth * mImageHeight * 3, this );

	// IR
	rc = mIRGenerator.Create(mContext);
	if (rc != XN_STATUS_OK)
		throw ExcFailedIRGeneratorInit();
	// make new map mode -> default to 640 x 480 @ 30fps
	XnMapOutputMode mapMode;
	mapMode.nXRes = mDepthWidth;
	mapMode.nYRes = mDepthHeight;
	mapMode.nFPS  = 30;
	mIRGenerator.SetMapOutputMode( mapMode );

	mIRGenerator.GetMetaData( mIRMD );
	mIRWidth = mIRMD.FullXRes();
	mIRHeight = mIRMD.FullYRes();

	mLastVideoFrameInfrared = mVideoInfrared;
}

OpenNI::Obj::Obj( const fs::path &recording )
	: mShouldDie( false ),
	  mNewDepthFrame( false ),
	  mNewVideoFrame( false ),
	  mVideoInfrared( false ),
	  mRecording( false )
{
	XnStatus rc = mContext.Init();
	CHECK_RC(rc, "context");

	rc = mContext.OpenFileRecording( recording.c_str() );
	if (rc != XN_STATUS_OK)
	{
		console () << "OpenNI Error - " << "OpenFileRecording" << " : " << xnGetStatusString(rc) << endl; \
		throw ExcFailedOpenFileRecording();
	}

	NodeInfoList list;
	rc = mContext.EnumerateExistingNodes(list);
	if (rc == XN_STATUS_OK)
	{
		for (NodeInfoList::Iterator it = list.Begin(); it != list.End(); ++it)
		{
			switch ((*it).GetDescription().Type)
			{
				case XN_NODE_TYPE_DEPTH:
					(*it).GetInstance(mDepthGenerator);
					break;

				case XN_NODE_TYPE_IMAGE:
					(*it).GetInstance(mImageGenerator);
					break;

				case XN_NODE_TYPE_IR:
					(*it).GetInstance(mIRGenerator);
					break;
			}
		}
	}

	// depth
	if ( mDepthGenerator.IsValid() )
	{
		mDepthGenerator.GetMetaData( mDepthMD );
		mDepthWidth = mDepthMD.FullXRes();
		mDepthHeight = mDepthMD.FullYRes();
		mDepthMaxDepth = mDepthGenerator.GetDeviceMaxDepth();

		mDepthBuffers = BufferManager<uint16_t>( mDepthWidth * mDepthHeight, this );
	}

	// image
	if ( mImageGenerator.IsValid() )
	{
		mImageGenerator.GetMetaData( mImageMD );
		mImageWidth = mImageMD.FullXRes();
		mImageHeight = mImageMD.FullYRes();

		mColorBuffers = BufferManager<uint8_t>( mImageWidth * mImageHeight * 3, this );
		mVideoInfrared = false;
	}

	// IR
	if ( mIRGenerator.IsValid() )
	{
		// make new map mode -> default to 640 x 480 @ 30fps
		XnMapOutputMode mapMode;
		mapMode.nXRes = mDepthWidth;
		mapMode.nYRes = mDepthHeight;
		mapMode.nFPS  = 30;
		mIRGenerator.SetMapOutputMode( mapMode );

		mIRGenerator.GetMetaData( mIRMD );
		mIRWidth = mIRMD.FullXRes();
		mIRHeight = mIRMD.FullYRes();

		mColorBuffers = BufferManager<uint8_t>( mIRWidth * mIRHeight * 3, this );
		mVideoInfrared = true;
	}

	mLastVideoFrameInfrared = mVideoInfrared;
}

OpenNI::Obj::~Obj()
{
	mShouldDie = true;
	if (mThread)
		mThread->join();
	mContext.Shutdown();
}

void OpenNI::Obj::start()
{
	XnStatus rc = mDepthGenerator.StartGenerating();
	CHECK_RC(rc, "DepthGenerator.StartGenerating");

	if (!mVideoInfrared)
		rc = mImageGenerator.StartGenerating();
	else
		rc = mIRGenerator.StartGenerating();
	CHECK_RC(rc, "Video.StartGenerating");

	mThread = shared_ptr<thread>(new thread(threadedFunc, this));
}

void OpenNI::Obj::threadedFunc(OpenNI::Obj *obj)
{
	while (!obj->mShouldDie)
	{
		//XnStatus status = obj->mContext.WaitAndUpdateAll();
		XnStatus status = obj->mContext.WaitOneUpdateAll(obj->mDepthGenerator);
		CHECK_RC(status, "WaitOneUpdateAll");

		obj->generateDepth();
		obj->generateImage();
		obj->generateIR();
	}
}

void OpenNI::Obj::generateDepth()
{
	if (!mDepthGenerator.IsValid())
		return;

	{
		lock_guard<recursive_mutex> lock(mMutex);
		mDepthGenerator.GetMetaData( mDepthMD );

		const uint16_t *depth = reinterpret_cast<const uint16_t*>( mDepthMD.Data() );
		mDepthBuffers.derefActiveBuffer(); // finished with current active buffer
		uint16_t *destPixels = mDepthBuffers.getNewBuffer(); // request a new buffer
		uint32_t depthScale = 0xffff0000 / mDepthMaxDepth;

		for (size_t p = 0; p < mDepthWidth * mDepthHeight; ++p )
		{
			uint32_t v = depth[p];
			destPixels[p] = (depthScale * v) >> 16;
		}
		mDepthBuffers.setActiveBuffer( destPixels ); // set this new buffer to be the current active buffer
		mNewDepthFrame = true; // flag that there's a new depth frame
	}
}

void OpenNI::Obj::generateImage()
{
	if (!mImageGenerator.IsValid() || !mImageGenerator.IsGenerating())
		return;

	{
		lock_guard<recursive_mutex> lock(mMutex);
		mImageGenerator.GetMetaData( mImageMD );

		mColorBuffers.derefActiveBuffer(); // finished with current active buffer
		uint8_t *destPixels = mColorBuffers.getNewBuffer();  // request a new buffer

		const XnRGB24Pixel *src = mImageMD.RGB24Data();
		uint8_t *dst = destPixels + ( mImageMD.XOffset() + mImageMD.YOffset() * mImageWidth ) * 3;
		for (int y = 0; y < mImageMD.YRes(); ++y)
		{
			memcpy(dst, src, mImageMD.XRes() * 3);

			src += mImageMD.XRes(); // FIXME: FullXRes
			dst += mImageWidth * 3;
		}

		mColorBuffers.setActiveBuffer( destPixels ); // set this new buffer to be the current active buffer
		mNewVideoFrame = true; // flag that there's a new color frame
		mLastVideoFrameInfrared = false;
	}
}

void OpenNI::Obj::generateIR()
{
	if (!mIRGenerator.IsValid() || !mIRGenerator.IsGenerating())
		return;

	{
		lock_guard<recursive_mutex> lock(mMutex);

		mColorBuffers.derefActiveBuffer(); // finished with current active buffer
		uint8_t *destPixels = mColorBuffers.getNewBuffer();  // request a new buffer
		const XnIRPixel *src = reinterpret_cast<const XnIRPixel *>( mIRGenerator.GetData() );

		for (size_t p = 0; p < mIRWidth * mIRHeight; ++p )
		{
			destPixels[p] = src[p] / 4;
		}

		mColorBuffers.setActiveBuffer( destPixels ); // set this new buffer to be the current active buffer
		mNewVideoFrame = true; // flag that there's a new color frame
		mLastVideoFrameInfrared = true;
	}
}

// Buffer management
template<typename T>
OpenNI::Obj::BufferManager<T>::~BufferManager()
{
	for( typename map<T*,size_t>::iterator bufIt = mBuffers.begin(); bufIt != mBuffers.end(); ++bufIt ) {
		delete [] bufIt->first;
	}
}

template<typename T>
T* OpenNI::Obj::BufferManager<T>::getNewBuffer()
{
	lock_guard<recursive_mutex> lock( mOpenNIObj->mMutex );

	typename map<T*,size_t>::iterator bufIt;
	for( bufIt = mBuffers.begin(); bufIt != mBuffers.end(); ++bufIt ) {
		if( bufIt->second == 0 ) // 0 means free buffer
			break;
	}
	if( bufIt != mBuffers.end() ) {
		bufIt->second = 1;
		return bufIt->first;
	}
	else { // there were no available buffers - add a new one and return it
		T *newBuffer = new T[mAllocationSize];
		mBuffers[newBuffer] = 1;
		return newBuffer;
	}
}

template<typename T>
void OpenNI::Obj::BufferManager<T>::setActiveBuffer( T *buffer )
{
	lock_guard<recursive_mutex> lock( mOpenNIObj->mMutex );
	// assign new active buffer
	mActiveBuffer = buffer;
}

template<typename T>
T* OpenNI::Obj::BufferManager<T>::refActiveBuffer()
{
	lock_guard<recursive_mutex> lock( mOpenNIObj->mMutex );
	mBuffers[mActiveBuffer]++;
	return mActiveBuffer;
}

template<typename T>
void OpenNI::Obj::BufferManager<T>::derefActiveBuffer()
{
	lock_guard<recursive_mutex> lock( mOpenNIObj->mMutex );
	if( mActiveBuffer )	// decrement use count on current active buffer
		mBuffers[mActiveBuffer]--;
}

template<typename T>
void OpenNI::Obj::BufferManager<T>::derefBuffer( T *buffer )
{
	lock_guard<recursive_mutex> lock( mOpenNIObj->mMutex );
	mBuffers[buffer]--;
}

};

bool OpenNI::checkNewDepthFrame()
{
	lock_guard<recursive_mutex> lock( mObj->mMutex );
	bool oldValue = mObj->mNewDepthFrame;
	mObj->mNewDepthFrame = false;
	return oldValue;
}

bool OpenNI::checkNewVideoFrame()
{
	lock_guard<recursive_mutex> lock( mObj->mMutex );
	bool oldValue = mObj->mNewVideoFrame;
	mObj->mNewVideoFrame = false;
	return oldValue;
}

ImageSourceRef OpenNI::getDepthImage()
{
	// register a reference to the active buffer
	uint16_t *activeDepth = mObj->mDepthBuffers.refActiveBuffer();
	return ImageSourceRef( new ImageSourceOpenNIDepth( activeDepth, mObj->mDepthWidth, mObj->mDepthHeight, this->mObj ) );
}

ImageSourceRef OpenNI::getVideoImage()
{
	uint8_t *activeColor = mObj->mColorBuffers.refActiveBuffer();
	if (mObj->mLastVideoFrameInfrared)
	{
		return ImageSourceRef( new ImageSourceOpenNIInfrared( activeColor, mObj->mIRWidth, mObj->mIRHeight, this->mObj ) );
	}
	else
	{
		return ImageSourceRef( new ImageSourceOpenNIColor( activeColor, mObj->mImageWidth, mObj->mImageHeight, this->mObj ) );
	}
}

void OpenNI::setVideoInfrared(bool infrared)
{
	if (mObj->mVideoInfrared == infrared)
		return;

	{
		lock_guard<recursive_mutex> lock( mObj->mMutex );

		mObj->mVideoInfrared = infrared;
		if (mObj->mVideoInfrared)
		{
			XnStatus rc = mObj->mImageGenerator.StopGenerating();
			CHECK_RC(rc, "ImageGenerater.StopGenerating");
			rc = mObj->mIRGenerator.StartGenerating();
			CHECK_RC(rc, "IRGenerater.StartGenerating");
		}
		else
		{
			XnStatus rc = mObj->mIRGenerator.StopGenerating();
			CHECK_RC(rc, "IRGenerater.StopGenerating");
			rc = mObj->mImageGenerator.StartGenerating();
			CHECK_RC(rc, "ImageGenerater.StartGenerating");
		}
	}
}

void OpenNI::setDepthAligned(bool aligned)
{
	if (mObj->mDepthAligned == aligned)
		return;

    if (!mObj->mImageGenerator.IsValid())
        return;

	{
		lock_guard<recursive_mutex> lock( mObj->mMutex );
		if (mObj->mDepthGenerator.IsCapabilitySupported(XN_CAPABILITY_ALTERNATIVE_VIEW_POINT))
		{
			XnStatus rc = XN_STATUS_OK;

			if (aligned)
				rc = mObj->mDepthGenerator.GetAlternativeViewPointCap().SetViewPoint( mObj->mImageGenerator );
			else
				rc = mObj->mDepthGenerator.GetAlternativeViewPointCap().ResetViewPoint();
			CHECK_RC(rc, "DepthGenerator.GetAlternativeViewPointCap");

			if (rc == XN_STATUS_OK)
				mObj->mDepthAligned = aligned;
		}
		else
		{
			console() << "DepthGenerator alternative view point not supported. " << endl;
		}
	}
}

void OpenNI::setMirrored(bool mirror)
{
	if (mObj->mMirrored == mirror)
		return;
	XnStatus rc = mObj->mContext.SetGlobalMirror(mirror);
	CHECK_RC(rc, "Context.SetGlobalMirror");

	if (rc == XN_STATUS_OK)
		mObj->mMirrored = mirror;
}

void OpenNI::startRecording( const fs::path &filename )
{
	if ( !mObj->mRecording )
	{
		XnStatus rc;
		rc = mObj->mContext.FindExistingNode(XN_NODE_TYPE_RECORDER, mObj->mRecorder);
		if (rc != XN_STATUS_OK)
		{
			rc = mObj->mRecorder.Create( mObj->mContext );
			CHECK_RC(rc, "Recorder.Create");
			if (rc != XN_STATUS_OK)
				return;
		}
		rc = mObj->mRecorder.SetDestination(XN_RECORD_MEDIUM_FILE, filename.c_str() );
		CHECK_RC(rc, "Recorder.SetDestination");

		if ( mObj->mDepthGenerator.IsValid() )
		{
			rc = mObj->mRecorder.AddNodeToRecording( mObj->mDepthGenerator, XN_CODEC_16Z_EMB_TABLES );
			CHECK_RC(rc, "Recorder.AddNodeToRecording depth");
		}

		if ( mObj->mImageGenerator.IsValid() && !mObj->mVideoInfrared )
		{
			rc = mObj->mRecorder.AddNodeToRecording( mObj->mImageGenerator, XN_CODEC_JPEG );
			CHECK_RC(rc, "Recorder.AddNodeToRecording image");
		}

		if ( mObj->mIRGenerator.IsValid() && mObj->mVideoInfrared )
		{
			rc = mObj->mRecorder.AddNodeToRecording( mObj->mIRGenerator, XN_CODEC_NULL );
			CHECK_RC(rc, "Recorder.AddNodeToRecording image");
		}
		mObj->mRecording = true;
	}
}

void OpenNI::stopRecording()
{
	if ( mObj->mRecording )
	{
		mObj->mRecorder.Release();
		mObj->mRecording = false;
	}
}

