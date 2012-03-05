#pragma once

#include <string>

#include "cinder/app/App.h"

#include "PParams.h"

class Effect {
	public:
		Effect( ci::app::App *app = NULL ) : mApp( app ) {}

		virtual void setup() {};

		virtual void instantiate() {}
		virtual void deinstantiate() {}

		virtual void resize( ci::app::ResizeEvent event ) {}

		virtual void mouseDown( ci::app::MouseEvent event ) {}
		virtual void mouseDrag( ci::app::MouseEvent event ) {}

		virtual void keyDown( ci::app::KeyEvent event ) {}

		virtual void update() {};
		virtual void draw() {};

	protected:
		ci::app::App *mApp;

		ci::params::PInterfaceGl mParams;

};

