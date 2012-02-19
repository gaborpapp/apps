#pragma once

#include "cinder/Cinder.h"
#include "cinder/Thread.h"
#include "cinder/Vector.h"
#include "cinder/Area.h"
#include "cinder/Exception.h"
#include "cinder/ImageIo.h"

#include "cinder/Surface.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Function.h"

#include <map>

#include <XnOpenNI.h>
#include <XnCppWrapper.h>
#include <XnHash.h>
#include <XnLog.h>

namespace cinder {

class OpenNI
{
	public:
		//! Represents the identifier for a particular Kinect
		struct Device {
			Device( int index = 0 )
				: mIndex( index )
			{}

			int     mIndex;
		};

		//! Default constructor - creates an uninitialized instance
		OpenNI() {}

		//! Creates a new Kinect based on Device # \a device. 0 is the typical value for \a deviceIndex.
		OpenNI(Device device);

		void start();

		//! Returns whether there is a new depth frame available since the last call to checkNewDepthFrame(). Call getDepthImage() to retrieve it.
		bool			checkNewDepthFrame();

		//! Returns whether there is a new video frame available since the last call to checkNewVideoFrame(). Call getVideoImage() to retrieve it.
		bool			checkNewVideoFrame();

		//! Returns latest depth frame.
		ImageSourceRef	getDepthImage();

		//! Returns latest video frame.
		ImageSourceRef	getVideoImage();

		//! Sets the video image returned by getVideoImage() and getVideoData() to be infrared when \a infrared is true, color when it's false (the default)
		void			setVideoInfrared( bool infrared = true );

		//! Returns whether the video image returned by getVideoImage() and getVideoData() is infrared when \c true, or color when it's \c false (the default)
		bool			isVideoInfrared() const { return mObj->mVideoInfrared; }

		//! Calibrates depth to video frame.
		void			setDepthAligned( bool aligned = true );
		bool			isDepthAligned() const { return mObj->mDepthAligned; }

		void			setMirrored( bool mirror = true );
		bool			isMirrored() const { return mObj->mMirrored; }

	protected:
		struct Obj {
			Obj( int deviceIndex );
			~Obj();

			void start();

			template<typename T>
			struct BufferManager {
				BufferManager() {};
				BufferManager( size_t allocationSize, Obj *openNIObj )
					: mAllocationSize( allocationSize ), mOpenNIObj( openNIObj ), mActiveBuffer( 0 )
				{}
				~BufferManager();

				T*			getNewBuffer();
				void		setActiveBuffer( T *buffer );
				void		derefActiveBuffer();
				T*			refActiveBuffer();
				void		derefBuffer( T *buffer );

				Obj						*mOpenNIObj;
				size_t					mAllocationSize;
				// map from pointer to reference count
				std::map<T*,size_t>		mBuffers;
				T						*mActiveBuffer;
			};

			BufferManager<uint8_t> mColorBuffers;
			BufferManager<uint16_t> mDepthBuffers;

			static void threadedFunc(struct OpenNI::Obj *arg);

			xn::Context mContext;

			xn::DepthGenerator mDepthGenerator;
			xn::DepthMetaData mDepthMD;
			int mDepthWidth;
			int mDepthHeight;
			int mDepthMaxDepth;

			xn::ImageGenerator mImageGenerator;
			xn::ImageMetaData mImageMD;
			int mImageWidth;
			int mImageHeight;

			xn::IRGenerator mIRGenerator;
			xn::IRMetaData mIRMD;
			int mIRWidth;
			int mIRHeight;

			void generateDepth();
			void generateImage();
			void generateIR();

			std::shared_ptr<std::thread> mThread;
			std::recursive_mutex mMutex;

			volatile bool mShouldDie;
			volatile bool mNewDepthFrame, mNewVideoFrame;
			volatile bool mVideoInfrared;
			volatile bool mLastVideoFrameInfrared;

			volatile bool mDepthAligned;
			volatile bool mMirrored;
		};

		friend class ImageSourceOpenNIColor;
		friend class ImageSourceOpenNIInfrared;
		friend class ImageSourceOpenNIDepth;

		std::shared_ptr<Obj> mObj;

		//! Parent class for all OpenNI exceptions
		class Exc : cinder::Exception {};

		//! Exception thrown from a failure to create a depth generator
		class ExcFailedDepthGeneratorInit : public Exc {};

		//! Exception thrown from a failure to create an image generator
		class ExcFailedImageGeneratorInit : public Exc {};

		//! Exception thrown from a failure to create an IR generator
		class ExcFailedIRGeneratorInit : public Exc {};
};

}

