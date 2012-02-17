#include "cinder/app/App.h"

#include "NI.h"

using namespace xn;
using namespace std;
using namespace ci;
using namespace ci::app;

namespace cinder {

static inline void checkStatus(XnStatus rc)
{
	if (rc != XN_STATUS_OK)
	{
		console () << "OpenNI Error: " <<  xnGetStatusString(rc) << std::endl;
	}
}

class ImageSourceOpenNIDepth : public ImageSource {
	public:
		ImageSourceOpenNIDepth( uint16_t *buffer, shared_ptr<OpenNI::Obj> ownerObj )
			: ImageSource(), mData( buffer ), mOwnerObj( ownerObj )
		{        setSize( 640, 480 );
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

			for( int32_t row = 0; row < 480; ++row )
				((*this).*func)( target, row, mData + row * 640 );
		}

	protected:
		shared_ptr<OpenNI::Obj>     mOwnerObj;
		uint16_t                    *mData;
};

OpenNI::OpenNI(Device device)
	: mObj( new Obj( device.mIndex ) )
{
}

OpenNI::Obj::Obj( int deviceIndex )
	: mShouldDie( false ),
	  //mColorBuffers(640 * 480 * 3, this),
	  mNewDepthFrame( false )
{
	XnStatus rc = mContext.Init();
	checkStatus(rc);

	// depth
	rc = mDepthGenerator.Create(mContext);
	if (rc != XN_STATUS_OK)
		throw ExcFailedDepthGeneratorInit();
	mDepthGenerator.GetMetaData( mDepthMD );
	mDepthWidth = mDepthMD.XRes();
	mDepthHeight = mDepthMD.YRes();
	mDepthMaxDepth = mDepthGenerator.GetDeviceMaxDepth();

	mDepthBuffers = BufferManager<uint16_t>( mDepthWidth * mDepthHeight, this );

	// start
	rc = mContext.StartGeneratingAll();
	checkStatus(rc);

	console() << "thread running" << endl;
	mThread = shared_ptr<thread>(new thread(threadedFunc, this));
}

OpenNI::Obj::~Obj()
{
	mContext.Shutdown();
	mShouldDie = true;
	console() << "thread should die" << endl;
	if (mThread)
		mThread->join();
}

void OpenNI::Obj::threadedFunc(OpenNI::Obj *obj)
{
	console() << "threadfunc running" << endl;
	while (!obj->mShouldDie)
	{
		XnStatus status = obj->mContext.WaitAndUpdateAll();
		checkStatus(status);

		obj->generateDepth();
	}
	console() << "thread dead" << endl;
}

void OpenNI::Obj::generateDepth()
{
	if (!mDepthGenerator.IsValid())
		return;

	lock_guard<recursive_mutex> lock(mMutex);
	mDepthGenerator.GetMetaData( mDepthMD );

	const uint16_t *depth = reinterpret_cast<const uint16_t*>( mDepthMD.Data() );
	mDepthBuffers.derefActiveBuffer(); // finished with current active buffer
	uint16_t *destPixels = mDepthBuffers.getNewBuffer(); // request a new buffer

	for (size_t p = 0; p < mDepthWidth * mDepthHeight ; ++p )
	{
		uint32_t v = depth[p];
		destPixels[p] = 65535 * v / mDepthMaxDepth;
	}
	mDepthBuffers.setActiveBuffer( destPixels ); // set this new buffer to be the current active buffer
	mNewDepthFrame = true; // flag that there's a new depth frame
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

ImageSourceRef OpenNI::getDepthImage()
{
	// register a reference to the active buffer
	uint16_t *activeDepth = mObj->mDepthBuffers.refActiveBuffer();
	return ImageSourceRef( new ImageSourceOpenNIDepth( activeDepth, this->mObj   ) );
}

