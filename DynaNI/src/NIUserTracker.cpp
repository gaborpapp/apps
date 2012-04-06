#include "cinder/app/App.h"

#include "NIUserTracker.h"

using namespace ci;
using namespace std;

#define CHECK_RC( rc, what ) \
	if (rc != XN_STATUS_OK) \
	{ \
		app::console() << "OpenNI Error - " << what << " : " << xnGetStatusString(rc) << endl; \
	}

void XN_CALLBACK_TYPE UserTracker::newUserCB( xn::UserGenerator &generator, XnUserID nId, void *pCookie )
{
	app::console() << "new user " << nId << endl;
	UserTracker::Obj *obj = static_cast<UserTracker::Obj *>(pCookie);

	if (Obj::sNeedPose)
	{
		obj->mUserGenerator.GetPoseDetectionCap().StartPoseDetection(Obj::sCalibrationPose, nId);
	}
	else
	{
		obj->mUserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
	}

	for ( list< Listener *>::const_iterator i = obj->mListeners.begin();
			i != obj->mListeners.end(); ++i )
	{
		(*i)->newUser( UserEvent( nId ) );
	}
}

void XN_CALLBACK_TYPE UserTracker::lostUserCB( xn::UserGenerator &generator, XnUserID nId, void *pCookie )
{
	app::console() << "lost user " << nId << endl;
	UserTracker::Obj *obj = static_cast<UserTracker::Obj *>(pCookie);
	for ( list< Listener *>::const_iterator i = obj->mListeners.begin();
			i != obj->mListeners.end(); ++i )
	{
		(*i)->lostUser( UserEvent( nId ) );
	}
}

void XN_CALLBACK_TYPE UserTracker::calibrationStartCB( xn::SkeletonCapability &capability, XnUserID nId, void *pCookie )
{
	app::console() << "calibration start " << nId << endl;

	UserTracker::Obj *obj = static_cast<UserTracker::Obj *>(pCookie);
	for ( list< Listener *>::const_iterator i = obj->mListeners.begin();
			i != obj->mListeners.end(); ++i )
	{
		(*i)->calibrationStart( UserEvent( nId ) );
	}
}

void XN_CALLBACK_TYPE UserTracker::calibrationEndCB( xn::SkeletonCapability &capability, XnUserID nId, XnBool bSuccess, void *pCookie )
{
	app::console() << "calibration end " << nId << " status " << bSuccess << endl;
	UserTracker::Obj *obj = static_cast<UserTracker::Obj *>(pCookie);

	if (bSuccess)
	{
		// Calibration succeeded
		app::console() << "calibration complete for user " << nId << endl;
		obj->mUserGenerator.GetSkeletonCap().StartTracking(nId);
	}
	else
	{
		// Calibration failed
		app::console() << "calibration failed for user " << nId << endl;
		if (Obj::sNeedPose)
		{
			obj->mUserGenerator.GetPoseDetectionCap().StartPoseDetection(Obj::sCalibrationPose, nId);
		}
		else
		{
			obj->mUserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
		}
	}

	for ( list< Listener *>::const_iterator i = obj->mListeners.begin();
			i != obj->mListeners.end(); ++i )
	{
		(*i)->calibrationEnd( UserEvent( nId ) );
	}
}

void XN_CALLBACK_TYPE UserTracker::userPoseDetectedCB( xn::PoseDetectionCapability &capability, const XnChar *strPose, XnUserID nId, void *pCookie )
{
	app::console() << "pose " << strPose << " detected for user " << nId << endl;

	/*
	mUserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
	mUserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
	*/
}


UserTracker::UserTracker( xn::Context context )
	: mObj( new Obj( context ) )
{
}


bool UserTracker::Obj::sNeedPose = false;
XnChar UserTracker::Obj::sCalibrationPose[20] = "";

UserTracker::Obj::Obj( xn::Context context )
	: mContext( context )
{
	XnStatus rc;

	rc = mContext.FindExistingNode(XN_NODE_TYPE_DEPTH, mDepthGenerator);
	CHECK_RC(rc, "Retrieving depth generator");

	rc = mContext.FindExistingNode(XN_NODE_TYPE_USER, mUserGenerator);
	if (rc != XN_STATUS_OK)
	{
		rc = mUserGenerator.Create(mContext);
		if (rc != XN_STATUS_OK)
			throw ExcFailedUserGeneratorInit();
	}

	if (!mUserGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON))
		throw ExcNoSkeletonCapability();

	// user callbacks
	XnCallbackHandle userCallbacks;
	rc = mUserGenerator.RegisterUserCallbacks(newUserCB, lostUserCB,
			this, userCallbacks);
	CHECK_RC(rc, "Register user callbacks");

	// calibration callbacks
	XnCallbackHandle calibrationCallbacks;
	rc = mUserGenerator.GetSkeletonCap().RegisterCalibrationCallbacks(
		calibrationStartCB, calibrationEndCB,
		this, calibrationCallbacks);
	CHECK_RC(rc, "Register calibration callbacks");

	if (mUserGenerator.GetSkeletonCap().NeedPoseForCalibration())
	{
		sNeedPose = true;
		if (!mUserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION))
			throw ExcNoPoseDetection();

		XnCallbackHandle poseDetected;
		rc = mUserGenerator.GetPoseDetectionCap().RegisterToPoseDetected(
				userPoseDetectedCB, this, poseDetected);
		CHECK_RC(rc, "Register pose detected callback");
		mUserGenerator.GetSkeletonCap().GetCalibrationPose(sCalibrationPose);
	}

	mUserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);

	/*
	XnCallbackHandle calibrationInProgress;
	rc = mUserGenerator.GetSkeletonCap().RegisterToCalibrationInProgress(
			calibrationInProgressCB, this, calibrationInProgress);
	CHECK_RC(rc, "Register calibration in progress callback");

	XnCallbackHandle poseInProgress;
	rc = mUserGenerator.GetPoseDetectionCap().RegisterToPoseInProgress(
			userPoseInProgressCB, this, poseInProgress);
	CHECK_RC(nRetVal, "Register pose in progress callback");
	*/
}

void UserTracker::start()
{
	XnStatus rc;
	rc = mObj->mUserGenerator.StartGenerating();
	CHECK_RC(rc, "UserGenerator.StartGenerating");
}

void UserTracker::addListener( Listener *listener )
{
	mObj->mListeners.push_back( listener );
}

vector< unsigned > UserTracker::getUsers()
{
	XnUserID aUsers[20];
	XnUInt16 nUsers = 20;
	mObj->mUserGenerator.GetUsers( aUsers, nUsers );
	vector< unsigned > users;
	for ( unsigned i = 0; i < nUsers; i++)
		users.push_back( aUsers[i] );

	return users;
}

Vec2f UserTracker::getJoint2d( XnUserID userId, XnSkeletonJoint jointId )
{
	if (mObj->mUserGenerator.GetSkeletonCap().IsTracking( userId ))
	{
		XnSkeletonJointPosition joint;
		mObj->mUserGenerator.GetSkeletonCap().GetSkeletonJointPosition( userId, jointId, joint );

	   if (joint.fConfidence <= 0.)
		   return Vec2f();

		XnPoint3D pt[1];
		pt[0] = joint.position;

		mObj->mDepthGenerator.ConvertRealWorldToProjective( 1, pt, pt );

		return Vec2f( pt[0].X, pt[0].Y );
	}
	else
	{
		return Vec2f();
	}
}

Vec3f UserTracker::getJoint3d( XnUserID userId, XnSkeletonJoint jointId )
{
	if (mObj->mUserGenerator.GetSkeletonCap().IsTracking( userId ))
	{
		XnSkeletonJointPosition joint;
		mObj->mUserGenerator.GetSkeletonCap().GetSkeletonJointPosition( userId, jointId, joint );

		if (joint.fConfidence <= 0.)
			return Vec3f();

		return Vec3f( joint.position.X, joint.position.Y, joint.position.Z );
	}
	else
	{
		return Vec3f();
	}
}

float UserTracker::getJointConfidance( XnUserID userId, XnSkeletonJoint jointId )
{
	if (mObj->mUserGenerator.GetSkeletonCap().IsTracking( userId ))
	{
		XnSkeletonJointPosition joint;
		mObj->mUserGenerator.GetSkeletonCap().GetSkeletonJointPosition( userId, jointId, joint );

		return joint.fConfidence;
	}
	else
	{
		return 0;
	}
}

void UserTracker::setSmoothing( float s )
{
	mObj->mUserGenerator.GetSkeletonCap().SetSmoothing( s );
}

Vec3f UserTracker::getUserCenter( XnUserID userId )
{
	XnPoint3D center;
	mObj->mUserGenerator.GetCoM( userId, center );
	return Vec3f( center.X, center.Y, center.Z );
}

