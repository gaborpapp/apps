#pragma once

#include "cinder/gl/Vbo.h"
#include "cinder/gl/Texture.h"
#include "cinder/Quaternion.h"

#include "PParams.h"
#include "ciFaceShift.h"

namespace mndl {

class GlobalData
{
	private:
		//! Singleton implementation
		GlobalData() {}
		~GlobalData() {};

	public:
		static GlobalData& get() { static GlobalData data; return data; }

		ci::gl::VboMesh mVboMesh;
		ci::gl::Texture mBlendshapeTexture;
		ci::gl::Texture mHeadTexture;

		mndl::faceshift::ciFaceShift mFaceShift;

		ci::Quatf mHeadRotation;
		ci::Quatf mLeftEyeRotation;
		ci::Quatf mRightEyeRotation;
		ci::params::PInterfaceGl mParams;
};

} // namespace mndl