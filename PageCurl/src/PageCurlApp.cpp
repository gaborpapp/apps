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

#include "PageCurlApp.h"

using namespace ci;
using namespace ci::app;
using namespace std;

void PageCurlApp::setup()
{
	Area book_area = getWindowBounds();
	int margin = static_cast<int>(getWindowHeight() * .1);
	book_area.expand(-margin, -margin);
	pc = new PageCurl("book", book_area);
	mouse_pos = Vec2i(0, 0);
}

void PageCurlApp::shutdown()
{
	delete pc;
}

void PageCurlApp::resize(ResizeEvent event)
{
	Area book_area(0, 0, event.getWidth(), event.getHeight());
	int margin = static_cast<int>(event.getHeight() * .1);
	book_area.expand(-margin, -margin);
	pc->resize(book_area);
}

void PageCurlApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());
	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void PageCurlApp::keyUp(KeyEvent event)
{
	if (event.getCode() == KeyEvent::KEY_LEFT)
		pc->prev();
	else
	if (event.getCode() == KeyEvent::KEY_RIGHT)
		pc->next();
}

void PageCurlApp::mouseMove(MouseEvent event)
{
	mouse_pos = event.getPos();
}

void PageCurlApp::update()
{
}

void PageCurlApp::draw()
{
	gl::clear(Color(0, 0, 0));
	gl::setMatricesWindow(getWindowSize());

	pc->draw();
}

CINDER_APP_BASIC(PageCurlApp, RendererGl)

