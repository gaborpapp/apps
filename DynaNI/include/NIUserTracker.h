#pragma once

#include <vector>
#include <list>

#include "cinder/Cinder.h"
#include "cinder/Vector.h"
#include "cinder/Exception.h"

#include <XnOpenNI.h>
#include <XnCppWrapper.h>
#include <XnHash.h>
#include <XnLog.h>

namespace cinder {

class UserTracker
{
	public:
		UserTracker() {}
		UserTracker( xn::Context context );

		struct UserEvent
		{
			UserEvent( unsigned aId ) : id( aId ) {}

			unsigned id;
		};

		class Listener
		{
			public:
				virtual void newUser( UserEvent event ) {}
				virtual void lostUser( UserEvent event ) {}
				virtual void calibrationStart( UserEvent event ) {}
				virtual void calibrationEnd( UserEvent event ) {}
		};

		void start();

		std::vector< unsigned > getUsers();

		ci::Vec2f getJoint2d( XnUserID userId, XnSkeletonJoint jointId );
		ci::Vec3f getJoint3d( XnUserID userId, XnSkeletonJoint jointId );
		float getJointConfidance( XnUserID userId, XnSkeletonJoint jointId );

		ci::Vec3f getUserCenter( XnUserID userId );

		void addListener( Listener *listener );

	protected:
		struct Obj {
			Obj( xn::Context context );
			//~Obj();

			xn::Context mContext;

			xn::UserGenerator mUserGenerator;
			xn::DepthGenerator mDepthGenerator;
			std::list< Listener * > mListeners;

			static bool sNeedPose;
			static XnChar sCalibrationPose[20];
		};
		std::shared_ptr<Obj> mObj;

		static void XN_CALLBACK_TYPE newUserCB( xn::UserGenerator &generator, XnUserID nId, void *pCookie );
		static void XN_CALLBACK_TYPE lostUserCB( xn::UserGenerator &generator, XnUserID nId, void *pCookie );
		static void XN_CALLBACK_TYPE calibrationStartCB( xn::SkeletonCapability &capability, XnUserID nId, void *pCookie );
		static void XN_CALLBACK_TYPE calibrationEndCB( xn::SkeletonCapability &capability, XnUserID nId, XnBool bSuccess, void *pCookie );
		static void XN_CALLBACK_TYPE userPoseDetectedCB( xn::PoseDetectionCapability &capability, const XnChar *strPose, XnUserID nId, void *pCookie );

		//! Parent class for all exceptions
		class Exc : cinder::Exception {};

		//! Exception thrown from a failure to create an user generator
		class ExcFailedUserGeneratorInit : public Exc {};

		//! Exception thrown if skeleton capability is not supported
		class ExcNoSkeletonCapability : public Exc {};

		//! Exception thrown if pose detection is required but not supported
		class ExcNoPoseDetection : public Exc {};
};

} // namespace cinder

