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

#include "HoloTest.h"
#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

void HoloTest::prepareSettings(Settings *settings)
{
	settings->setWindowSize(640, 480);
}

void HoloTest::setup()
{
	params = params::InterfaceGl("Parameters", Vec2i(200, 300));
	obj_size = 4.5f;
	params.addParam("Size", &obj_size, "min=1. max=10.0 step=0.25 keyIncr=z keyDecr=Z");
	params.addParam("Rotation", &obj_rotation);
	auto_rotate = false;
	params.addParam("Auto rotate", &auto_rotate);
	obj_color = ColorA(1, 0, 1, 1);
	params.addParam("Color", &obj_color, "");
	cam_fov = 90.0;
	params.addParam("FOV", &cam_fov, "min=45.0 max=90.0 step=1.");
	display_mode = DISPLAY_VIEWS;
	vector<string> modes;
	modes.push_back("main");
	modes.push_back("views");
	modes.push_back("pyramid");
	params.addParam("mode", modes, &display_mode);
	label_views = false;
	params.addParam("View labels", &label_views);

	ObjLoader loader((DataSourceRef)loadResource(RES_MONKEY_OBJ));
	loader.load(&mesh);
	vbo = gl::VboMesh(mesh);

	shader = gl::GlslProg(loadResource(RES_PHONG_VERT), loadResource(RES_PHONG_FRAG));

	gl::Fbo::Format format;
	format.enableDepthBuffer(true);
	format.enableMipmapping(true);
	format.enableColorBuffer(true, 4);
	fbo = gl::Fbo(FBO_WIDTH, FBO_HEIGHT, format);

	Vec3f camera_locations[] = { Vec3f(0, 0, 10), Vec3f(-10, 0, 0),
		Vec3f(0, 0, -10), Vec3f(10, 0, 0),
		Vec3f(0, 0, 10) };

	for (int i = 0; i < 5; i++)
	{
		cams[i].lookAt(camera_locations[i], Vec3f::zero());
		if (i < VIEW_MAIN)
		{
			cams[i].setPerspective(cam_fov, fbo.getAspectRatio(), 1, 1000);
		}
		else
		{
			cams[i].setPerspective(cam_fov, getWindowAspectRatio(), 1, 1000);
		}
	}

	// NOTE:
	// osx 10.5 bug, fonts don't work from a resource
	// http://forum.libcinder.org/#topic/23286000000459027
	//font = Font(loadResource(RES_MUSEOSANS_FONT), 12);
	font = Font("CourierNewPSMT", 20);
}

void HoloTest::shutdown()
{
}

void HoloTest::resize(ResizeEvent event)
{
	cams[VIEW_MAIN].setPerspective(cam_fov, getWindowAspectRatio(), 1, 1000);
}

void HoloTest::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());
	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void HoloTest::keyUp(KeyEvent event)
{
}


void HoloTest::mouseDown(MouseEvent event)
{
}

void HoloTest::mouseDrag(MouseEvent event)
{
}

void HoloTest::update()
{
	static bool last_auto_rotate = false;

	if (auto_rotate)
	{
		static float last_seconds;

		float seconds = getElapsedSeconds();
		if (last_auto_rotate)
			obj_auto_rotation *= Quatf(Vec3f(0, 1, 0), (seconds - last_seconds) * .2);
		last_seconds = seconds;
	}
	last_auto_rotate = auto_rotate;
}

void HoloTest::draw_scene(int cam_index)
{
	gl::pushMatrices();
	gl::clear();

	gl::enableDepthWrite();
	gl::enableDepthRead();

	glDisable(GL_CULL_FACE);

	cams[cam_index].setFov(cam_fov);
	gl::setMatrices(cams[cam_index]);

	Vec4f light_position = Vec4f(-10, -10, 0, 0);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, &light_position.x);

	gl::rotate(obj_rotation * obj_auto_rotation);
	gl::scale(Vec3f(obj_size, obj_size, obj_size));
	gl::color(obj_color);

	shader.bind();
	shader.uniform("eyeDir", cams[cam_index].getViewDirection().normalized());
	gl::draw(vbo);
	shader.unbind();
	gl::color(Color::white());

	gl::popMatrices();

	gl::disableDepthRead();
	gl::disable(GL_LIGHTING);
}

void HoloTest::draw()
{
	fbo.bindFramebuffer();
	gl::setViewport(fbo.getBounds());
	gl::setMatricesWindow(fbo.getSize());
	for (int i = 0; i < 4; i++)
	{
		string camera[] = { "front", "left", "back", "right" };
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT + i);
		draw_scene(i);
		if (label_views)
		{
			gl::drawString(camera[i], Vec2f(10, 10), Color::white(), font);
		}
	}
	fbo.unbindFramebuffer();

	gl::setMatricesWindow(getWindowSize());
	gl::setViewport(getWindowBounds());
	gl::clear(Color(0, 0, 0));

	switch (display_mode)
	{
		case DISPLAY_MAIN:
			draw_scene(VIEW_MAIN);
			if (label_views)
			{
				gl::drawString("front", Vec2f(10, 10), Color::white(), font);
			}
			break;

		case DISPLAY_VIEWS:
			int w = getWindowWidth();
			int h = getWindowHeight();
			gl::pushMatrices();
			gl::translate(Vec3f(0, h, 0));
			gl::scale(Vec3f(1, -1, 1));
			gl::draw(fbo.getTexture(0), Area(0, 0, w/2, h/2));
			gl::draw(fbo.getTexture(1), Area(w/2, 0, w, h/2));
			gl::draw(fbo.getTexture(2), Area(0, h/2, w/2, h));
			gl::draw(fbo.getTexture(3), Area(w/2, h/2, w, h));
			gl::popMatrices();
			break;

		case DISPLAY_PYRAMID:
			break;
	}

	params::InterfaceGl::draw();
}

CINDER_APP_BASIC(HoloTest, RendererGl)

