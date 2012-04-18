#pragma once #include <vector>

#include "cinder/Rect.h"
#include "cinder/Area.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"

#include "cinder/Filesystem.h"

#include "PParams.h"

class Gallery
{
	public:
		Gallery( ci::fs::path &folder, int rows = 3, int columns = 4 );

		void resize( int rows, int columns);
		void refreshList();

		void addImage( ci::fs::path imagePath );

		void update();
		void render( const ci::Area &area );

		void paramsShow( bool s = true ) { mParams.show( s ); }
		void paramsHide() { mParams.hide(); }
		bool isParamsVisible() { return mParams.isVisible(); }

		class Picture
		{
			public:
				Picture( Gallery *g );

				void render( const ci::Rectf &rect );

				void flip();
				bool isFlipping() { return flipping; }

			private:
				void setRandomTexture();

				ci::gl::Texture mTexture;
				Gallery *mGallery;

				double flipStart;
				bool flipping;
				bool flipTextureChanged;
				static float sFlipDuration;

				friend class Gallery;
		};

		void setHorizontalMargin( float v ) { mHorizontalMargin = v; }
		void setVerticalMargin( float v ) { mVerticalMargin = v; }
		void setCellHorizontalSpacing( float v ) { mCellHorizontalSpacing = v; }
		void setCellVerticalSpacing( float v ) { mCellVerticalSpacing = v; }

		void setNoiseFreq( float v ) { mRandFreqMultiplier = v; }
		void enableTvLines( bool enable = true ) { mEnableTvLines = enable; }
		void enableVignetting( bool enable = true ) { mEnableVignetting = enable; }

	private:
		ci::params::PInterfaceGl mParams;

		ci::fs::path mGalleryFolder;
		std::vector< ci::fs::path > mFiles;
		std::vector< ci::gl::Texture > mTextures;
		int mRows, mLastRows;
		int mMaxTextures;
		int mColumns, mLastColumns;

		float mHorizontalMargin;
		float mVerticalMargin;
		float mCellHorizontalSpacing;
		float mCellVerticalSpacing;
		float mRandFreqMultiplier;
		bool mEnableTvLines;
		bool mEnableVignetting;
		float mFlipFrequency;

		std::vector< Picture > mPictures;

		ci::gl::GlslProg mGalleryShader;
};

