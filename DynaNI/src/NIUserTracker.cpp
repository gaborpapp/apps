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
}

void XN_CALLBACK_TYPE UserTracker::lostUserCB( xn::UserGenerator &generator, XnUserID nId, void *pCookie )
{
	app::console() << "lost user " << nId << endl;
}

void XN_CALLBACK_TYPE UserTracker::calibrationStartCB( xn::SkeletonCapability &capability, XnUserID nId, void *pCookie )
{
	app::console() << "calibration start " << nId << endl;
}

void XN_CALLBACK_TYPE UserTracker::calibrationEndCB( xn::SkeletonCapability &capability, XnUserID nId, XnBool bSuccess, void *pCookie )
{
	app::console() << "calibration end " << nId << " status " << bSuccess << endl;
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

	rc = mUserGenerator.Create(mContext);
	if (rc != XN_STATUS_OK)
		throw ExcFailedUserGeneratorInit();

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

