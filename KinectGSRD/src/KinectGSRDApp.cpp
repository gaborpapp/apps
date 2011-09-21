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

#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "cinder/ImageIo.h"
#include "cinder/CinderMath.h"
#include "cinder/Surface.h"

#include "Resources.h"

#include "KinectGSRDApp.h"

using namespace ci;
using namespace ci::app;
using namespace std;

KinectGSRDApp::KinectGSRDApp() :
	mouse_left(false),
	ping_pong(0)
{
}

void KinectGSRDApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize(640, 480);
	settings->setFrameRate(60.0f);
}

void KinectGSRDApp::setup()
{
	// setup the parameters
	dU = 0.16f;
	dV = 0.08f;
	k = 0.077f;
	f = 0.023f;
	/* .05, .16, .079, .025 */

	params = params::InterfaceGl("Parameters", Vec2i(200, 300));
	params.addParam("dU", &dU, "min=0.0 max=0.4 step=0.01 keyIncr=u keyDecr=U");
	params.addParam("dV", &dV, "min=0.0 max=0.4 step=0.01 keyIncr=v keyDecr=V");
	params.addParam("k", &k, "min=0.0 max=1.0 step=0.001 keyIncr=k keyDecr=K");
	params.addParam("f", &f, "min=0.0 max=1.0 step=0.001 keyIncr=f keyDecr=F");

	params.addSeparator("seed");
	show_seed = false;
	params.addParam("show", &show_seed, "");
	depth_threshold = .7;
	params.addParam("depth threshold", &depth_threshold, "min=0 max=1 step=.01");
	/*
	blur_n = 1;
	params.addParam("blur", &blur_n, "min=0 max=20 step=1");
	*/

	params.addSeparator("");
	show_fps = false;
	params.addParam("fps", &show_fps, "");

	params.addSeparator("");
	show_greyscale = true;
	params.addParam("greyscale", &show_greyscale, "");
	xoffset = 1.0;
	params.addParam("normal xoffset", &xoffset, "min=-10.0 max=10.0 step=0.1");
	yoffset = 1.0;
	params.addParam("normal yoffset", &yoffset, "min=-10.0 max=10.0 step=0.1");

	// fbo
	gl::Fbo::Format format;
	format.enableDepthBuffer(false);
	format.setTarget(GL_TEXTURE_RECTANGLE_ARB);
	format.enableMipmapping(false);
	format.setWrap(GL_CLAMP, GL_CLAMP);
	format.setMinFilter(GL_NEAREST);
	format.setMagFilter(GL_NEAREST);
	format.setColorInternalFormat(GL_RGBA32F_ARB);
	format.enableColorBuffer(true, 2);

	fbo = gl::Fbo(FBO_WIDTH, FBO_HEIGHT, format);

	// shaders
	shader_gsrd = gl::GlslProg(loadResource(RES_PASS_THRU_VERT), loadResource(RES_GSRD_FRAG));
	shader_redlum = gl::GlslProg(loadResource(RES_PASS_THRU_VERT), loadResource(RES_REDLUM_FRAG));
	shader_seed = gl::GlslProg(loadResource(RES_PASS_THRU_VERT), loadResource(RES_SEED_FRAG));

	try
	{
		console() << "There are " << Kinect::getNumDevices() << " Kinects connected." << std::endl;
		kinect = Kinect(Kinect::Device());
	}
	catch (...)
	{
		console() << "Failed to initialize kinect" << std::endl;
		exit(1);
	}

	font = Font("Lucida Grande", 12.0f);
	reset();

	/*
	blur = gl::ip::GaussianBlur(CAPTURE_WIDTH, CAPTURE_HEIGHT);
	*/

	// envmap
	shader_envmap = gl::GlslProg(loadResource(RES_PASS_THRU_VERT), loadResource(RES_ENVMAP_FRAG));
	envmap_tex = gl::Texture(loadImage(loadResource(RES_ENVMAP_TEX)));

	enableVSync(false);
}

void KinectGSRDApp::shutdown()
{
}

void KinectGSRDApp::update()
{
	const int ITERATIONS = 20;

	// set orthographic projection with lower-left origin
	gl::setMatricesWindow(fbo.getSize(), false);
	gl::setViewport(fbo.getBounds());

	if (kinect.checkNewDepthFrame())
	{
		kinect_depth = kinect.getDepthImage();
	}

	if (kinect_depth)
	{
		gl::enableAlphaBlending();
		fbo.bindFramebuffer();
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT + ping_pong);
		shader_seed.bind();
		shader_seed.uniform("tex", 0);
		shader_seed.uniform("threshold", depth_threshold);
		//gl::color(Color(.5, 0, .25));
		gl::color(Color(1., 0, .5));
		gl::draw(kinect_depth, fbo.getBounds());
		gl::color(Color::white());
		shader_seed.unbind();
		fbo.unbindFramebuffer();
		gl::disableAlphaBlending();
	}

	// simulation
	fbo.bindFramebuffer();
	for (int i = 0; i < ITERATIONS; i++)
	{
		int other = ping_pong;
		ping_pong ^= 1;

		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT + ping_pong);

		fbo.bindTexture(0, other);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		shader_gsrd.bind();
		shader_gsrd.uniform("tex", 0);
		shader_gsrd.uniform("dU", dU);
		shader_gsrd.uniform("dV", dV);
		shader_gsrd.uniform("k", k);
		shader_gsrd.uniform("f", f);

		gl::enable(GL_TEXTURE_RECTANGLE_ARB);
		gl::drawSolidRect(fbo.getBounds(), true);

		shader_gsrd.unbind();

		if (mouse_left)
		{
			gl::disable(GL_TEXTURE_RECTANGLE_ARB);
			gl::color(Color(.5, 0, 0.25));
			RectMapping window2fbo(getWindowBounds(), fbo.getBounds());
			// FIXME: getMousePos() gives different coordinates in fullscreen than
			// event.getPos()
			//Vec2i mpos = window2fbo.map(getMousePos());
			Vec2i mpos = window2fbo.map(mouse_pos);
			gl::drawSolidRect(Rectf(mpos - Vec2f(5, 5),
									mpos + Vec2f(5, 5)));
		}
		gl::color(Color::white());
	}
	fbo.unbindFramebuffer();

	gl::disable(GL_TEXTURE_RECTANGLE_ARB);
}

void KinectGSRDApp::draw()
{
	gl::clear(ColorA(0, 0, 0, 0));
	gl::setMatricesWindow(getWindowSize());
	gl::setViewport(getWindowBounds());

	if (show_greyscale)
	{
		shader_redlum.bind();
		shader_redlum.uniform("tex", 0);
		gl::draw(fbo.getTexture(ping_pong), getWindowBounds());
		shader_redlum.unbind();
	}
	else
	{
		shader_envmap.bind();
		fbo.getTexture(ping_pong).bind(0);
		envmap_tex.bind(1);
		shader_envmap.uniform("hmap", 0);
		shader_envmap.uniform("envmap", 1);
		shader_envmap.uniform("xoffset", xoffset);
		shader_envmap.uniform("yoffset", yoffset);
		gl::draw(fbo.getTexture(ping_pong), getWindowBounds());
		shader_envmap.unbind();
	}

	if (show_seed && seed_tex)
	{
		int w = getWindowWidth();
		gl::draw(seed_tex, Rectf(w - 160, 0, w, 120));
	}

	if (show_fps)
		gl::drawString("FPS: " + toString(getAverageFps()), Vec2f(10.0f, 10.0f), Color::white(), font);

	params::InterfaceGl::draw();
}

void KinectGSRDApp::reset()
{
	gl::setMatricesWindow(fbo.getSize(), false);
	gl::setViewport(fbo.getBounds());

	gl::enableAlphaBlending();
	for (int i = 0; i < 2; i++)
	{
		fbo.bindFramebuffer();
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT + i);
		gl::clear(ColorA(1.0, 0, 0, 1.0));
		fbo.unbindFramebuffer();
	}
	gl::disableAlphaBlending();
}

void KinectGSRDApp::enableVSync(bool vs)
{
#if defined(CINDER_MAC)
	GLint vsync = vs;
	CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &vsync);
#endif
}


void KinectGSRDApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'r')
		reset();
	else
	if (event.getChar() == ' ')
		setFullScreen(!isFullScreen());

	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void KinectGSRDApp::keyUp(KeyEvent event)
{
}

void KinectGSRDApp::mouseDown(MouseEvent event)
{
	if (event.isLeft())
		mouse_left = true;
}

void KinectGSRDApp::mouseUp(MouseEvent event)
{
	if (event.isLeft())
		mouse_left = false;
}

void KinectGSRDApp::mouseMove(MouseEvent event)
{
	mouse_pos = event.getPos();
}

void KinectGSRDApp::mouseDrag(MouseEvent event)
{
	mouse_pos = event.getPos();
}

CINDER_APP_BASIC(KinectGSRDApp, RendererGl(0))

