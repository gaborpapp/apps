#pragma once

#include "cinder/Camera.h"
#include "cinder/Cinder.h"
#include "cinder/Matrix.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Light.h"

class DualParaboloidShadowMap
{
	public:
		void setup();
		void update( const ci::CameraPersp &cam );
		void draw();

		void bindDepth( int direction );
		void unbindDepth();

		void bindShadow();
		void unbindShadow();

		std::shared_ptr< ci::gl::Light > getLightRef() { return mLightRef; }

	private:
		static const int SHADOW_MAP_RESOLUTION = 2048;
		ci::gl::Fbo mFboDepthForward; // forward hemisphere
		ci::gl::Fbo mFboDepthBackward; // backward hemisphere

		ci::gl::GlslProg mShaderDepth;
		ci::gl::GlslProg mShaderShadow;

		std::shared_ptr< ci::gl::Light > mLightRef;

		ci::Matrix44f mShadowMatrix;
};

