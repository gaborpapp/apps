/*
 Copyright (C) 2011 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"

#include "FilterApp.h"

using namespace ci;
using namespace ci::app;
using namespace std;

void FilterApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize(WIDTH, HEIGHT);
}

void FilterApp::setup()
{
	boxblur = gl::ip::BoxBlur(WIDTH, HEIGHT);
	blur = gl::ip::GaussianBlur(WIDTH, HEIGHT);
	sobel = gl::ip::Sobel(WIDTH, HEIGHT);
	frei_chen = gl::ip::FreiChen(WIDTH, HEIGHT);

	params = params::InterfaceGl("Parameters", Vec2i(200, 300));
	boxblur_radius = 1.0;
	apply_boxblur = false;
	params.addParam("Box Blur", &apply_boxblur);
	params.addParam(" radius", &boxblur_radius, "min=0.0 max=10.0 step=.5 keyIncr=q keyDecr=w");
	params.addSeparator();

	blur_n = 1;
	apply_blur = false;
	params.addParam("Gaussian Blur", &apply_blur);
	params.addParam(" n", &blur_n, "min=0 max=100 step=1 keyIncr=s keyDecr=a");
	params.addSeparator();

	apply_sobel = false;
	params.addParam("Sobel", &apply_sobel);
	params.addSeparator();

	apply_frei_chen = false;
	params.addParam("Frei-Chen", &apply_frei_chen);

	// capture
	try
	{
		capture = Capture(640, 480);
		capture.start();
	}
	catch (...)
	{
		console() << "Failed to initialize capture" << std::endl;
	}

	font = Font("Arial", 12.0f);
	enableVSync(false);
}

void FilterApp::shutdown()
{
	if (capture)
	{
		capture.stop();
	}
}

void FilterApp::enableVSync(bool vs)
{
#if defined(CINDER_MAC)
	GLint vsync = vs;
	CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &vsync);
#endif
}

void FilterApp::update()
{
}

void FilterApp::draw()
{
	static gl::Texture source;

	gl::clear(Color(0, 0, 0));

	bool new_frame = (capture && capture.checkNewFrame());

	if (new_frame)
	{
		Surface8u capt_surf = capture.getSurface();
		source = gl::Texture(capt_surf);
	}

	gl::setMatricesWindow(getWindowSize());
	gl::setViewport(getWindowBounds());

	if (new_frame && apply_boxblur)
		source = boxblur.process(source, boxblur_radius);

	if (new_frame && apply_blur)
		source = blur.process(source, blur_n);

	if (new_frame && apply_sobel)
		source = sobel.process(source);

	if (new_frame && apply_frei_chen)
		source = frei_chen.process(source);

	if (source)
	{
		gl::draw(source, getWindowBounds());
	}

	gl::drawString("FPS: " + toString(getAverageFps()), Vec2f(10.0f, 10.0f), Color::white(), font);
	params::InterfaceGl::draw();
}

void FilterApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());
	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

CINDER_APP_BASIC(FilterApp, RendererGl)

