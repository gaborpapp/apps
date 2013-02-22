#include "cinder/Cinder.h"
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"

#include "DualParaboloidShadowMap.h"

using namespace ci;

void DualParaboloidShadowMap::setup()
{
	mParams = mndl::kit::params::PInterfaceGl( "DPSM", Vec2i( 200, 300 ), Vec2i( 220, 16 ) );
	mParams.addPersistentSizeAndPosition();

	mParams.addPersistentParam( "Draw maps", &mDrawMaps, false );

	// NOTE: although the color buffer would not be necessary without it the app hangs on OSX 10.8 with some Geforce cards
	// https://forum.libcinder.org/topic/anyone-else-experiencing-shadow-mapping-problems-with-the-new-xcode
	gl::Fbo::Format format;
	format.setColorInternalFormat( GL_RGB32F_ARB );
	format.enableDepthBuffer();
	mFboDepthForward = gl::Fbo( SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION, format );
	mFboDepthBackward = gl::Fbo( SHADOW_MAP_RESOLUTION, SHADOW_MAP_RESOLUTION, format );

	/*
	mFboDepthForward.bindDepthTexture();
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	mFboDepthForward.unbindTexture();
	mFboDepthBackward.bindDepthTexture();
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	mFboDepthBackward.unbindTexture();
	*/
	mFboDepthForward.bindTexture();
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	mFboDepthForward.unbindTexture();
	mFboDepthBackward.bindTexture();
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	mFboDepthBackward.unbindTexture();

	try
	{
		mShaderDepth = gl::GlslProg( app::loadAsset( "shaders/DualParaboloidDepth.vert" ),
									 app::loadAsset( "shaders/DualParaboloidDepth.frag" ) );
	}
	catch( const std::exception &e )
	{
		app::console() << "Could not compile shader:" << e.what() << std::endl;
	}

	try
	{
		mShaderShadow = gl::GlslProg( app::loadAsset( "shaders/DualParaboloidShadow.vert" ),
									  app::loadAsset( "shaders/DualParaboloidShadow.frag" ) );
	}
	catch( const std::exception &e )
	{
		app::console() << "Could not compile shader:" << e.what() << std::endl;
	}

	mLightRef = std::shared_ptr< gl::Light >( new gl::Light( gl::Light::POINT, 0 ) );
	mLightRef->setShadowParams( 90.f, .1f, 16.f );
}

void DualParaboloidShadowMap::update( const CameraPersp &cam )
{
	mLightRef->update( cam );

	// calculate shadow matrix without the projection transform
	const CameraPersp &shadowCam = mLightRef->getShadowCamera();
	mShadowMatrix = shadowCam.getModelViewMatrix();
	// FIXME: test readings are needed to properly cache matrices?!?
	// wasn't this cinder bug fixed in commit 3455e767?
	Matrix44f test = cam.getModelViewMatrix();
	mShadowMatrix *= cam.getInverseModelViewMatrix();
}

void DualParaboloidShadowMap::draw()
{
	if ( !mDrawMaps )
		return;

	gl::color( Color::white() );
	gl::draw( mFboDepthForward.getTexture(), Rectf( 0, 256, 256, 0 ) );
	gl::draw( mFboDepthBackward.getTexture(), Rectf( 256, 256, 512, 0 ) );
}

void DualParaboloidShadowMap::bindDepth( int direction )
{
	// store the current OpenGL state, so we can restore it when done
	glPushAttrib( GL_VIEWPORT_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT );

	// bind the shadow map Fbo
	if ( direction > 0 )
		mFboDepthForward.bindFramebuffer();
	else
		mFboDepthBackward.bindFramebuffer();

	// set the viewport to the correct dimensions and clear the Fbo
	gl::setViewport( mFboDepthForward.getBounds() );
	gl::clear();

	gl::disable( GL_CULL_FACE );
	gl::enableDepthRead();
	gl::enableDepthWrite();

	// point light has no direction, but it is needed for a proper
	// render matrix, +z, -z will be the two directions the paraboloid
	// maps are facing
	mLightRef->setDirection( Vec3f( 0.f, 0.f, 1.f ) );
	mLightRef->setShadowRenderMatrices();

	const CameraPersp &shadowCam = mLightRef->getShadowCamera();
	if ( mShaderDepth )
	{
		mShaderDepth.bind();
		mShaderDepth.uniform( "hemisphereDirection", (float)direction );
		mShaderDepth.uniform( "nearClip", shadowCam.getNearClip() );
		mShaderDepth.uniform( "farClip", shadowCam.getFarClip() );
	}
}

void DualParaboloidShadowMap::unbindDepth()
{
	if ( mShaderDepth )
		mShaderDepth.unbind();

	gl::popMatrices();

	mFboDepthForward.unbindFramebuffer();

	glPopAttrib();
}

void DualParaboloidShadowMap::bindShadow()
{
	if ( mShaderShadow )
	{
		const CameraPersp &shadowCam = mLightRef->getShadowCamera();
		mShaderShadow.bind();
		mShaderShadow.uniform( "shadowMatrix", mShadowMatrix );
		// TODO: these are not really needed
		mShaderShadow.uniform( "shadowNearClip", shadowCam.getNearClip() );
		mShaderShadow.uniform( "shadowFarClip", shadowCam.getFarClip() );
		mShaderShadow.uniform( "shadowTextureForward", 0 );
		mShaderShadow.uniform( "shadowTextureBackward", 1 );
		/*
		mFboDepthForward.bindDepthTexture( 0 );
		mFboDepthBackward.bindDepthTexture( 1 );
		*/
		mFboDepthForward.bindTexture( 0 );
		mFboDepthBackward.bindTexture( 1 );
	}
}

void DualParaboloidShadowMap::unbindShadow()
{
	if ( mShaderShadow )
	{
		mShaderShadow.unbind();
		/*
		mFboDepthForward.getDepthTexture().unbind( 0 );
		mFboDepthBackward.getDepthTexture().unbind( 1 );
		*/
		mFboDepthForward.getTexture().unbind( 0 );
		mFboDepthBackward.getTexture().unbind( 1 );
	}
}

