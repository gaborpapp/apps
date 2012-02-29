/*
 Copyright (C) 2012 Gabor Papp

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

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"
#include "cinder/Filesystem.h"

#include "PParams.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Acacia : public ci::app::AppBasic
{
	public:
		void prepareSettings(Settings *settings);
		void setup();
		void shutdown();

		void resize(ResizeEvent event);
		void keyDown(KeyEvent event);

		void mouseDown(MouseEvent event);
		void mouseDrag(MouseEvent event);

		void update();
		void draw();

	private:
		ci::params::PInterfaceGl mParams;

		vector< gl::Texture > loadTextures( const fs::path &relativeDir );

		vector< gl::Texture > mBWTextures;
};

void Acacia::prepareSettings(Settings *settings)
{
	settings->setWindowSize(640, 480);
}

void Acacia::setup()
{
	// params
	string paramsXml = getResourcePath().string() + "/params.xml";
	params::PInterfaceGl::load( paramsXml );
	mParams = params::PInterfaceGl("Japan akac", Vec2i(200, 300));
	mParams.addPersistentSizeAndPosition();

	gl::disableVerticalSync();

	mBWTextures = loadTextures( "bw" );

	gl::enableAlphaBlending();
	gl::disableDepthWrite();
	gl::disableDepthRead();
}

void Acacia::shutdown()
{
	params::PInterfaceGl::save();
}

vector< gl::Texture > Acacia::loadTextures( const fs::path &relativeDir )
{
	vector< gl::Texture > textures;

	fs::path dataPath = getAssetPath( relativeDir );

	for (fs::directory_iterator it( dataPath ); it != fs::directory_iterator(); ++it)
	{
		if (fs::is_regular_file(*it) && (it->path().extension().string() == ".png"))
		{
			gl::Texture t = loadImage( loadAsset( relativeDir / it->path().filename() ) );
			textures.push_back( t );
		}
	}

	return textures;
}

void Acacia::resize(ResizeEvent event)
{
}

void Acacia::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
		setFullScreen(!isFullScreen());
}

void Acacia::mouseDrag(MouseEvent event)
{
}

void Acacia::mouseDown(MouseEvent event)
{
}

void Acacia::update()
{
}

void Acacia::draw()
{
	gl::clear(Color(0, 0, 0));
	gl::setMatricesWindow( getWindowSize() );

	params::PInterfaceGl::draw();
}

CINDER_APP_BASIC(Acacia, RendererGl)

