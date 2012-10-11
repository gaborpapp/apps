#pragma once

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"

#include "Effect.h"

#include "NI.h"

#define TEXTURE_COUNT 128

class DepthMerge : public Effect
{
	public:
		DepthMerge( ci::app::App *app );

		void setup();

		void instantiate();

		void update();
		void draw();

		void setNI( ci::OpenNI aNI ) { mNI = aNI; }

	private:
		ci::OpenNI mNI;

		enum { TYPE_COLOR = 0, TYPE_IR, TYPE_DEPTH };

		//gl::Fbo mDepthTextures[TEXTURE_COUNT];
		ci::gl::Texture mDepthTextures[TEXTURE_COUNT];
		ci::gl::Texture mColorTextures[TEXTURE_COUNT];
		int mCurrentIndex;

		ci::gl::GlslProg mShader;

		ci::params::PInterfaceGl mParams;
		int mType;
		int mStepLog2;
		float mMinDepth;
		float mMaxDepth;
		bool mMirror;
};

