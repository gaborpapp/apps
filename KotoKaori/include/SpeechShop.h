#pragma once

#include "cinder/Cinder.h"
#include "cinder/gl/Texture.h"
#include "cinder/Text.h"
#include "cinder/Font.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/Filesystem.h"

#include "PParams.h"
#include "Effect.h"

#include "b2cinder/b2cinder.h"

class SpeechShop : public Effect
{
	public:
		SpeechShop( ci::app::App *app );

		void setup();

		void instantiate();

		void resize( ci::app::ResizeEvent event );

		void keyDown( ci::app::KeyEvent event );
		void keyUp( ci::app::KeyEvent event );

		void mouseDown( ci::app::MouseEvent event );
		void mouseDrag( ci::app::MouseEvent event );

		void update();
		void draw();

	private:
		struct Text {
			Text () : wordIndex(0) {};

			string name;
			std::vector<std::string> words;
			unsigned wordIndex;
		};

		// gravity direction
		enum {
			GR_LEFT = 0,
			GR_UP,
			GR_RIGHT,
			GR_DOWN };
		int mGravityDir;
		float mGravity;
		float mMinTextSize;
		float mMaxTextSize;

		void addLetter( ci::Vec2i pos );
		void togglePlug();

		void loadTexts();
		std::vector<Text> mTexts;
		int mTextIndex;
		void initTexts();

		box2d::Sandbox mSandbox;
		std::vector< ci::box2d::BoxElement *> mPlugs;

		ci::params::PInterfaceGl mParams;
		bool mIsPlugged;
		bool mPlugDebounce;
};

