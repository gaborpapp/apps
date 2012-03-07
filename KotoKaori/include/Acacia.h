#pragma once

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/Filesystem.h"
#include "cinder/ip/Resize.h"
#include "cinder/Rect.h"

#include "ciMsaFluidSolver.h"
#include "ciMsaFluidDrawerGl.h"

#include "CinderOpenCV.h"

#include "PParams.h"
#include "NI.h"
#include "Leaves.h"
#include "Particles.h"

#include "Effect.h"

class Acacia : public Effect
{
	public:
		Acacia( ci::app::App *app );
		void setup();

		void instantiate();
		void deinstantiate();

		void resize( ci::app::ResizeEvent event);

		void mouseDown( ci::app::MouseEvent event);
		void mouseDrag( ci::app::MouseEvent event);

		void update();
		void draw();

		void setNI( ci::OpenNI aNI ) { mNI = aNI; }

	private:
		int mMaxLeaves;
		int mLeafCount;
		float mAging;
		float mGravity;
		float mVelThres;
		float mVelDiv;
		float mParticleAging;
		float mParticleVelThres;
		float mParticleVelDiv;
		bool mAddLeaves;
		bool mAddParticles;


		bool mDrawAtmosphere;
		bool mDrawCamera;
		bool mDrawFeatures;

		void clearLeaves();
		std::vector< ci::gl::Texture > loadTextures( const ci::fs::path &relativeDir );

		std::vector< ci::gl::Texture > mBWTextures;

		ciMsaFluidSolver mFluidSolver;
		ciMsaFluidDrawerGl mFluidDrawer;
		static const int sFluidSizeX;

		void addToFluid( ci::Vec2f pos, ci::Vec2f vel, bool addLeaves, bool addParticles, bool addForce );

		LeafManager mLeaves;
		ParticleManager mParticles;

		ci::Vec2i mPrevMouse;

		ci::OpenNI mNI;
		ci::gl::Texture mOptFlowTexture;

		void chooseFeatures( cv::Mat currentFrame );
		void trackFeatures( cv::Mat currentFrame );

		cv::Mat mPrevFrame;
		std::vector<cv::Point2f> mPrevFeatures, mFeatures;
		std::vector<uint8_t> mFeatureStatuses;

		static const int MAX_FEATURES = 128;
		#define CAMERA_WIDTH 160
		#define CAMERA_HEIGHT 120
		static const ci::Vec2f CAMERA_SIZE; //( CAMERA_WIDTH, CAMERA_HEIGHT );
};

