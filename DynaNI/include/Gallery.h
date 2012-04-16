#pragma once #include <vector>

#include "cinder/Rect.h"
#include "cinder/Area.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/GlslProg.h"

#include "cinder/Filesystem.h"

class Gallery
{
	public:
		Gallery() {}
		Gallery( ci::fs::path &folder, int rows = 3, int columns = 4 );

		void resize( int rows, int columns);
		void refreshList();

		void update();
		void render( const ci::Area &area );

		class Picture
		{
			public:
				Picture( Gallery *g );

				void render( const ci::Rectf &rect );

			private:
				ci::gl::Texture mTexture;
				Gallery *mGallery;
		};

		void setHorizontalMargin( float v ) { mHorizontalMargin = v; }
		void setVerticalMargin( float v ) { mVerticalMargin = v; }
		void setCellHorizontalSpacing( float v ) { mCellHorizontalSpacing = v; }
		void setCellVerticalSpacing( float v ) { mCellVerticalSpacing = v; }
		void setNoiseFreq( float v ) { mRandFreqMultiplier = v; }

		void enableTvLines( bool enable = true ) { mEnableTvLines = enable; }
		void enableVignetting( bool enable = true ) { mEnableVignetting = enable; }

	private:
		ci::fs::path mGalleryFolder;
		std::vector< ci::fs::path > mFiles;
		int mRows;
		int mColumns;
		float mHorizontalMargin;
		float mVerticalMargin;
		float mCellHorizontalSpacing;
		float mCellVerticalSpacing;
		float mRandFreqMultiplier;
		bool mEnableTvLines;
		bool mEnableVignetting;

		std::vector< Picture > mPictures;

		ci::gl::GlslProg mGalleryShader;
};

