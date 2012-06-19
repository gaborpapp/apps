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

#include <vector>
#include <sstream>

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/params/Params.h"
#include "cinder/Rand.h"
#include "cinder/CinderMath.h"
#include "cinder/Surface.h"
#include "cinder/Utilities.h"
#include "cinder/Capture.h"
#include "cinder/ip/Resize.h"
#include "cinder/ImageIo.h"
#include "AntTweakBar.h"

#include "CinderOpenCV.h"
#include "cinderSyphon.h"

#include "Resources.h"

#include "KawaseBloom.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class LiquidApp : public AppBasic
{
	public:
		LiquidApp();

		void prepareSettings(Settings *settings);
		void setup();
		void shutdown();

		void resize(ResizeEvent event);
		void keyDown(KeyEvent event);
		void mouseDown(MouseEvent event);
		void mouseUp(MouseEvent event);
		void mouseDrag(MouseEvent event);

		void simulate();

		void update();
		void draw();

	private:
		params::InterfaceGl mParams;

		struct Particle
		{
			float x;
			float y;
			float u;
			float v;
			float gu;
			float gv;
			float T00;
			float T01;
			float T11;
			int cx;
			int cy;

			float px[3];
			float py[3];
			float gx[3];
			float gy[3];

			Particle() {};
			Particle( float _x, float _y, float _u, float _v)
				: x(_x), y(_y), u(_u), v(_v)
			{}
		};

		struct Node
		{
			float m;
			float gx;
			float gy;
			float u;
			float v;
			float ax;
			float ay;
			bool active;
			int x;
			int y;

			void clear()
			{
				m = gx = gy = u = v = ax = ay = 0;
				active = false;
			}
		};

		static const int gsizeX = 120; //129;
		static const int gsizeY = 90; //97;
		float mMulX;
		float mMulY;

		Node grid[gsizeX][gsizeY];

		vector<Node *> active;
		static const int nx = 100;
		static const int ny = 200;
		int xPoints[ny];
		int yPoints[nx];

		float mDensity;
		float mStiffness;
		float mBulkViscosity;
		float mElasticity;
		float mViscosity;
		float mYieldRate;
		float mGravity;
		float mSmoothing;

		Color mParticleColor;

		float mFps;

		static const int pCount = 10000;
		Particle particles[pCount];

		Vec2i mMousePos;
		Vec2i mMousePrevPos;
		bool mMouseDrag;

		gl::Fbo mFbo;

		gl::ip::KawaseBloom mKawaseBloom;
		float mBloomStrength;

		// capture
		ci::Capture mCapture;
		gl::Texture mCaptTexture;

		vector< ci::Capture > mCaptures;

		static const int CAPTURE_WIDTH = 640;
		static const int CAPTURE_HEIGHT = 480;

		int mCurrentCapture;

		// optflow
		bool mFlip;
		bool mDrawFlow;
		bool mDrawCapture;
		float mFlowMultiplier;

		cv::Mat mPrevFrame;
		cv::Mat mFlow;

		static const int OPTFLOW_WIDTH = 120;
		static const int OPTFLOW_HEIGHT = 90;

		// syphon
		syphonServer mSyphonServer;
};


LiquidApp::LiquidApp()
	: mMouseDrag( false )
{
}

void LiquidApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize(800, 600);
}

void LiquidApp::setup()
{
	gl::disableVerticalSync();

	// capture

	// list out the capture devices
	vector< Capture::DeviceRef > devices( Capture::getDevices() );
	vector< string > deviceNames;

    for ( vector< Capture::DeviceRef >::const_iterator deviceIt = devices.begin();
			deviceIt != devices.end(); ++deviceIt )
	{
        Capture::DeviceRef device = *deviceIt;
		string deviceName = device->getName() + " " + device->getUniqueId();

        try
		{
            if ( device->checkAvailable() )
			{
                mCaptures.push_back( Capture( CAPTURE_WIDTH, CAPTURE_HEIGHT,
							device ) );
				deviceNames.push_back( deviceName );
            }
            else
			{
                mCaptures.push_back( Capture() );
				deviceNames.push_back( deviceName + " not available" );
			}
        }
        catch ( CaptureExc & )
		{
            console() << "Unable to initialize device: " << device->getName() <<
 endl;
        }
	}

	if ( deviceNames.empty() )
	{
		deviceNames.push_back( "Camera not available" );
		mCaptures.push_back( Capture() );
	}

	mParams = params::InterfaceGl("Parameters", Vec2i( 300, 400 ));

	mFlip = true;
	mParams.addParam( "Flip", &mFlip );
	mDrawFlow = false;
	mParams.addParam( "Draw flow", &mDrawFlow );
	mDrawCapture = true;
	mParams.addParam( "Draw capture", &mDrawCapture );
	mFlowMultiplier = .01;
	mParams.addParam( "Flow multiplier", &mFlowMultiplier, "min=.005 max=2 step=.005" );
	mParams.addSeparator();

	mDensity = 2.0;
	mParams.addParam("Density", &mDensity, "min=0 max=10 step=0.05");
	mStiffness = 1.0;
	mParams.addParam("Stiffness", &mStiffness, "min=0 max=1 step=0.05");
	mBulkViscosity = 1.0;
	mParams.addParam("Bulk Viscosity", &mBulkViscosity, "min=0 max=1 step=0.05");
	mElasticity = .0;
	mParams.addParam("Elasticity", &mElasticity, "min=0 max=1 step=0.05");
	mViscosity = 0.1;
	mParams.addParam("Viscosity", &mViscosity, "min=0 max=1 step=0.05");
	mYieldRate = .0;
	mParams.addParam("Yield Rate", &mYieldRate, "min=0 max=1 step=0.05");
	mGravity = .0;
	mParams.addParam("Gravity", &mGravity, "min=0 max=0.05 step=0.005");
	mSmoothing = .0;
	mParams.addParam("Smoothing", &mSmoothing, "min=0 max=1 step=0.05");

	mParams.addSeparator();
	mParticleColor = Color::hex( 0xffffff );
	mParams.addParam("Particle color", &mParticleColor);

	mBloomStrength = 0.2;
	mParams.addParam("Bloom strength", &mBloomStrength, "min=0 max=1 step=0.05");

	mParams.addSeparator();

	mCurrentCapture = 0;
	mParams.addParam( "Capture", deviceNames, &mCurrentCapture );
	mParams.addParam( "Fps", &mFps, "", true );

	float mul2 = 1.0 / sqrt(mDensity);
	if (mul2 > 0.72)
		mul2 = 0.72;

	int n = 0;
	int pc = (int)sqrt(pCount) + 1;
	for (int j = 0; j < pc; j++)
	{
		for (int i = 0; i < pc; i++)
		{
			if (n < pCount)
				particles[n] = Particle(((i + Rand::randFloat()) * mul2) + 4.0,
										((j + Rand::randFloat()) * mul2) + 4.0, 0.0, 0.0);
			n++;
		}
	}

	gl::Fbo::Format format;
	format.enableDepthBuffer( false );

	mFbo = gl::Fbo( 1024, 768, format );
	mMulX = mFbo.getWidth() / (float)gsizeX;
	mMulY = mFbo.getHeight() / (float)gsizeY;

	mKawaseBloom = gl::ip::KawaseBloom( mFbo.getWidth(), mFbo.getHeight() );

	setFrameRate( 60 );

	// syphon
    mSyphonServer.setName( "Liquid" );

	mParams.show();
	// mParams.setOptions(" TW_HELP ", " visible=false "); // FIXME: not working
	TwDefine(" TW_HELP visible=false ");
}

void LiquidApp::resize(ResizeEvent event)
{
	/*
	mMulX = event.getWidth() / (float)gsizeX;
	mMulY = event.getHeight() / (float)gsizeY;
	*/
}

void LiquidApp::shutdown()
{
	if ( mCapture )
	{
		mCapture.stop();
	}
}

void LiquidApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'f')
	{
		setFullScreen(!isFullScreen());
		if (isFullScreen())
			hideCursor();
		else
			showCursor();
	}
	else
	if (event.getChar() == 's')
	{
		mParams.show( !mParams.isVisible() );
		if ( isFullScreen() )
		{
			if ( !mParams.isVisible() )
				hideCursor();
			else
				showCursor();
		}
	}

	if (event.getCode() == KeyEvent::KEY_ESCAPE)
		quit();
}

void LiquidApp::mouseDown(MouseEvent event)
{
	RectMapping mapping( getWindowBounds(), mFbo.getBounds() );
	mMousePos = mapping.map( event.getPos() );
}

void LiquidApp::mouseDrag(MouseEvent event)
{
	RectMapping mapping( getWindowBounds(), mFbo.getBounds() );
	mMousePrevPos = mMousePos;
	mMousePos = mapping.map( event.getPos() );
	mMouseDrag = true;
}

void LiquidApp::mouseUp(MouseEvent event)
{
	mMouseDrag = false;
}

void LiquidApp::update()
{
	static int lastCapture = -1;

	mFps = getAverageFps();

	if ( lastCapture != mCurrentCapture )
	{
		if ( ( lastCapture >= 0 ) && ( mCaptures[ lastCapture ] ) )
			mCaptures[ lastCapture ].stop();

		if ( mCaptures[ mCurrentCapture ] )
			mCaptures[ mCurrentCapture ].start();

		mCapture = mCaptures[ mCurrentCapture ];
		lastCapture = mCurrentCapture;
	}

	// optical flow
	if ( mCapture && mCapture.checkNewFrame() )
	{
		Surface8u captSurf( Channel8u( mCapture.getSurface() ) );

		Surface8u smallSurface( OPTFLOW_WIDTH, OPTFLOW_HEIGHT, false );
		if ( ( captSurf.getWidth() != OPTFLOW_WIDTH ) ||
			 ( captSurf.getHeight() != OPTFLOW_HEIGHT ) )
		{
			ip::resize( captSurf, &smallSurface );
		}
		else
		{
			smallSurface = captSurf;
		}

		mCaptTexture = gl::Texture( captSurf );

		cv::Mat currentFrame( toOcv( Channel( smallSurface ) ) );
		if ( mFlip )
			cv::flip( currentFrame, currentFrame, 1 );
        if ( mPrevFrame.data )
		{
			cv::calcOpticalFlowFarneback(
					mPrevFrame, currentFrame,
					mFlow,
					.5, 5, 13, 5, 5, 1.1, 0 );
		}
        mPrevFrame = currentFrame;
	}

	simulate();
}

void LiquidApp::simulate()
{
	float mdx = 0.0;
	float mdy = 0.0;
	if (mMouseDrag)
	{
		mdx = (mMousePos.x - mMousePrevPos.x) / mMulX;
		mdy = (mMousePos.y - mMousePrevPos.y) / mMulY;
	}

	for (vector<Node *>::iterator i = active.begin(); i != active.end(); ++i)
	{
		(*i)->clear();
	}
	active.clear();

	for (int k = 0; k < pCount; k++)
	{
		Particle *p = &particles[k];
		p->cx = (int)(p->x - 0.5);
		if (p->cx > gsizeX - 4)
			p->cx = gsizeX - 4;
		else
		if (p->cx < 0)
			p->cx = 0;

		p->cy = (int)(p->y - 0.5);
		if (p->cy > gsizeY - 4)
			p->cy = gsizeY - 4;
		else
		if (p->cy < 0)
			p->cy = 0;

		float x = p->cx - p->x;
		p->px[0] = (0.5 * x * x + 1.5 * x + 1.125);
		p->gx[0] = (x + 1.5);
		x += 1.0;
		p->px[1] = (-x * x + 0.75);
		p->gx[1] = (-2.0 * x);
		x += 1.0;
		p->px[2] = (0.5 * x * x - 1.5 * x + 1.125);
		p->gx[2] = (x - 1.5);

		float y = p->cy - p->y;
		p->py[0] = (0.5 * y * y + 1.5 * y + 1.125);
		p->gy[0] = (y + 1.5);
		y += 1.0;
		p->py[1] = (-y * y + 0.75);
		p->gy[1] = (-2.0 * y);
		y += 1.0;
		p->py[2] = (0.5 * y * y - 1.5F * y + 1.125);
		p->gy[2] = (y - 1.5);

		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				int cxi = p->cx + i;
				int cyj = p->cy + j;
				Node *n = &grid[cxi][cyj];
				if (!n->active)
				{
					active.push_back(n);
					n->active = true;
				}
				float phi = p->px[i] * p->py[j];
				n->m += phi;
				float dx = p->gx[i] * p->py[j];
				float dy = p->px[i] * p->gy[j];
				n->gx += dx;
				n->gy += dy;
				n->u += phi * p->u;
				n->v += phi * p->v;
			}
		}
	}

	for (vector<Node *>::iterator i = active.begin(); i != active.end(); ++i)
	{
		Node *n = *i;
		if (n->m > 0)
		{
			n->u /= n->m;
			n->v /= n->m;
		}

	}

	for (int k = 0; k < pCount; k++)
	{
		Particle *p = &particles[k];

		float dudx = 0.0;
		float dudy = 0.0;
		float dvdx = 0.0;
		float dvdy = 0.0;
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				Node *n = &grid[(p->cx + i)][(p->cy + j)];
				float gx = p->gx[i] * p->py[j];
				float gy = p->px[i] * p->gy[j];
				dudx += n->u * gx;
				dudy += n->u * gy;
				dvdx += n->v * gx;
				dvdy += n->v * gy;
			}
		}

		float w1 = dudy - dvdx;
		float wT0 = w1 * p->T01;
		float wT1 = 0.5F * w1 * (p->T00 - p->T11);
		float D00 = dudx;
		float D01 = 0.5F * (dudy + dvdx);
		float D11 = dvdy;
		float trace = 0.5F * (D00 + D11);
		D00 -= trace;
		D11 -= trace;
		p->T00 += -wT0 + D00 - mYieldRate * p->T00;
		p->T01 += wT1 + D01 - mYieldRate * p->T01;
		p->T11 += wT0 + D11 - mYieldRate * p->T11;

		/*
		float norm = p->T00 * p->T00 + 2.0 * p->T01 * p->T01 + p->T11 * p->T11;
		if ((this.mode > -1) || (norm > 5.0)) {
			p->T00 = (p->T01 = p->T11 = 0.0);
		}
		*/
		float norm = p->T00 * p->T00 + 2.0 * p->T01 * p->T01 + p->T11 * p->T11;
		if (norm > 5.0)
			p->T00 = p->T01 = p->T11 = 0.0;

		int cx = (int)p->x;
		int cy = (int)p->y;
		if (cx > gsizeX - 5)
			cx = gsizeX - 5;
		else
		if (cx < 0)
			cx = 0;
		if (cy > gsizeY - 5)
			cy = gsizeY - 5;
		else
		if (cy < 0)
			cy = 0;
		int cxi = cx + 1;
		int cyi = cy + 1;

		float p00 = grid[cx][cy].m;
		float x00 = grid[cx][cy].gx;
		float y00 = grid[cx][cy].gy;
		float p01 = grid[cx][cyi].m;
		float x01 = grid[cx][cyi].gx;
		float y01 = grid[cx][cyi].gy;
		float p10 = grid[cxi][cy].m;
		float x10 = grid[cxi][cy].gx;
		float y10 = grid[cxi][cy].gy;
		float p11 = grid[cxi][cyi].m;
		float x11 = grid[cxi][cyi].gx;
		float y11 = grid[cxi][cyi].gy;

		float pdx = p10 - p00;
		float pdy = p01 - p00;
		float C20 = 3.0 * pdx - x10 - 2.0 * x00;
		float C02 = 3.0 * pdy - y01 - 2.0 * y00;
		float C30 = -2.0 * pdx + x10 + x00;
		float C03 = -2.0 * pdy + y01 + y00;
		float csum1 = p00 + y00 + C02 + C03;
		float csum2 = p00 + x00 + C20 + C30;
		float C21 = 3.0 * p11 - 2.0 * x01 - x11 - 3.0 * csum1 - C20;
		float C31 = -2.0 * p11 + x01 + x11 + 2.0 * csum1 - C30;
		float C12 = 3.0 * p11 - 2.0 * y10 - y11 - 3.0 * csum2 - C02;
		float C13 = -2.0 * p11 + y10 + y11 + 2.0 * csum2 - C03;
		float C11 = x01 - C13 - C12 - x00;

		float u = p->x - cx;
		float u2 = u * u;
		float u3 = u * u2;
		float v = p->y - cy;
		float v2 = v * v;
		float v3 = v * v2;
		float density = p00 + x00 * u + y00 * v + C20 * u2 + C02 * v2 +
			C30 * u3 + C03 * v3 + C21 * u2 * v + C31 * u3 * v + C12 *
			u * v2 + C13 * u * v3 + C11 * u * v;

		float pressure = mStiffness / max(1.0f, mDensity) * (density - mDensity);
		if (pressure > 2.0) {
			pressure = 2.0;
		}

		float fx = 0.0;
		float fy = 0.0;
		if (p->x < 3.0)
			fx += 3.0 - p->x;
		else
		if (p->x > gsizeX - 4)
			fx += gsizeX - 4 - p->x;

		if (p->y < 3.0F)
			fy += 3.0F - p->y;
		else
		if (p->y > gsizeY - 4)
			fy += gsizeY - 4 - p->y;

		trace *= mStiffness;
		float T00 = mElasticity * p->T00 + mViscosity * D00 + pressure + mBulkViscosity * trace;
		float T01 = mElasticity * p->T01 + mViscosity * D01;
		float T11 = mElasticity * p->T11 + mViscosity * D11 + pressure + mBulkViscosity * trace;

		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				Node *n = &grid[(p->cx + i)][(p->cy + j)];
				float phi = p->px[i] * p->py[j];
				float dx = p->gx[i] * p->py[j];
				float dy = p->px[i] * p->gy[j];

				n->ax += -(dx * T00 + dy * T01) + fx * phi;
				n->ay += -(dx * T01 + dy * T11) + fy * phi;
			}
		}
	}

	for (vector<Node *>::iterator i = active.begin(); i != active.end(); ++i)
	{
		Node *n = *i;
		if (n->m > 0)
		{
			n->ax /= n->m;
			n->ay /= n->m;
			n->u = 0;
			n->v = 0;
		}

	}


	for (int k = 0; k < pCount; k++)
	{
		Particle *p = &particles[k];
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				Node *n = &grid[(p->cx + i)][(p->cy + j)];
				float phi = p->px[i] * p->py[j];
				p->u += phi * n->ax;
				p->v += phi * n->ay;
			}
		}
		p->v += mGravity;

		if (mMouseDrag)
		{
			float vx = abs(p->x - mMousePos.x / mMulX);
			float vy = abs(p->y - mMousePos.y / mMulY);
			if ((vx < 10.0) && (vy < 10.0))
			{
				float weight = (1.0 - vx / 10.0) * (1.0 - vy / 10.0);
				p->u += weight * (mdx - p->u);
				p->v += weight * (mdy - p->v);
			}
		}

		// optical flow
		if ( mFlow.data )
		{
			cv::Point2f v = mFlow.at< cv::Point2f >( static_cast< int >( p->y ),
					static_cast< int >( p->x ) );
			p->u += mFlowMultiplier * v.x;
			p->v += mFlowMultiplier * v.y;
		}

		float x = p->x + p->u;
		float y = p->y + p->v;
		if (x < 2.0)
			p->u += 2.0 - x + Rand::randFloat() * 0.01;
		else
		if (x > gsizeX - 3)
			p->u += gsizeX - 3 - x - Rand::randFloat() * 0.01;

		if (y < 2.0)
			p->v += 2.0 - y + Rand::randFloat() * 0.01;
		else
		if (y > gsizeY - 3)
			p->v += gsizeY - 3 - y - Rand::randFloat() * 0.01;

		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				Node *n = &grid[(p->cx + i)][(p->cy + j)];
				float phi = p->px[i] * p->py[j];
				n->u += phi * p->u;
				n->v += phi * p->v;
			}
		}
	}

	for (vector<Node *>::iterator i = active.begin(); i != active.end(); ++i)
	{
		Node *n = *i;
		if (n->m > 0)
		{
			n->u /= n->m;
			n->v /= n->m;
		}

	}

	for (int k = 0; k < pCount; k++)
	{
		Particle *p = &particles[k];
		float gu = 0.0;
		float gv = 0.0;
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				Node *n = &grid[(p->cx + i)][(p->cy + j)];
				float phi = p->px[i] * p->py[j];
				gu += phi * n->u;
				gv += phi * n->v;
			}
		}

		p->gu = gu;
		p->gv = gv;

		p->x += gu;
		p->y += gv;
		p->u += mSmoothing * (gu - p->u);
		p->v += mSmoothing * (gv - p->v);
	}
}

void LiquidApp::draw()
{
	mFbo.bindFramebuffer();
	gl::setMatricesWindow( mFbo.getSize(), false );
	gl::setViewport( mFbo.getBounds() );

	gl::clear( Color::black() );

	gl::color( mParticleColor );
	Vec2f mul( mMulX, mMulY );
	gl::enableAdditiveBlending();
	gl::disableDepthRead();
	gl::disableDepthWrite();

	for (int i = 0; i < pCount; i++)
	{
		Particle *p = &particles[i];

		gl::drawLine( mul * Vec2f(p->x - 1.0, p->y),
					  mul * Vec2f(p->x - 1.0 - p->gu, p->y - p->gv));
	}
	gl::disableAlphaBlending();

	mFbo.unbindFramebuffer();

	gl::Texture output = mKawaseBloom.process( mFbo.getTexture(), 8, mBloomStrength );

	// draw output to window
	gl::clear( Color::black() );

	gl::setMatricesWindow( getWindowSize() );
	gl::setViewport( getWindowBounds() );
	gl::color( Color::white() );

	if ( mDrawCapture && mCaptTexture )
	{
		gl::enableAdditiveBlending();
		gl::color( ColorA( 1, 1, 1, .4 ) );
		mCaptTexture.enableAndBind();

		gl::pushModelView();
		if ( mFlip )
		{
			gl::translate( getWindowWidth(), 0 );
			gl::scale( -1, 1 );
		}
		gl::drawSolidRect( getWindowBounds() );
		gl::popModelView();
		mCaptTexture.unbind();
		gl::color( Color::white() );
	}
	else
	{
		gl::enableAlphaBlending();
	}

	output.bind();
	gl::drawSolidRect( getWindowBounds() );
	output.unbind();

	mSyphonServer.publishTexture( &output );

	// flow vectors
	gl::disable( GL_TEXTURE_2D );
	if ( mDrawFlow && mFlow.data )
	{
		RectMapping ofToWin( Area( 0, 0, mFlow.cols, mFlow.rows ),
			getWindowBounds() );
		float ofScale = mFlowMultiplier * getWindowWidth() / (float)OPTFLOW_WIDTH;
		gl::color( Color::white() );
		for ( int y = 0; y < mFlow.rows; y++ )
		{
			for ( int x = 0; x < mFlow.cols; x++ )
			{
				Vec2f v = fromOcv( mFlow.at< cv::Point2f >( y, x ) );
				Vec2f p( x + .5, y + .5 );
				gl::drawLine( ofToWin.map( p ),
							  ofToWin.map( p + ofScale * v ) );
			}
		}
	}

	params::InterfaceGl::draw();
}

CINDER_APP_BASIC(LiquidApp, RendererGl( RendererGl::AA_NONE ))

