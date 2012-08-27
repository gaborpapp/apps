#include "cinder/app/AppBasic.h"
#include "cinder/Camera.h"
#include "cinder/Cinder.h"
#include "cinder/CinderMath.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Vbo.h"
#include "cinder/TriMesh.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class VboTestApp : public AppBasic
{
	public:
		void setup();
		void draw();

	private:
		TriMesh mMesh0;
		TriMesh mMesh1;
		gl::VboMesh mVboMesh;

		gl::GlslProg mShader;
		static const char *sVertexShader;
		static const char *sFragmentShader;
};

const char *VboTestApp::sVertexShader =
			"attribute vec3 mesh0;\n"
			"attribute vec3 mesh1;\n"
			"uniform float blend;\n"
			"void main()\n {"
			" vec4 vertex = vec4( mix( mesh0, mesh1, blend ), 1. );\n"
			" gl_Position = gl_ModelViewProjectionMatrix * vertex;\n"
			" gl_FrontColor = gl_Color;\n"
			"}\n";

const char *VboTestApp::sFragmentShader =
			"void main() { \n"
			" gl_FragColor = gl_Color;\n"
			"}\n";

void VboTestApp::setup()
{
	mShader = gl::GlslProg( sVertexShader, sFragmentShader );

	GLint numAttribs = 0;
	glGetIntegerv( GL_MAX_VERTEX_ATTRIBS, & numAttribs );
	console() << "num attribs: " << numAttribs << endl;

	// tetrahedron
	Vec3f vertices[] = { Vec3f( 0, 0, 1.732051 ), Vec3f( 1.632993, 0, -0.5773503 ),
		Vec3f( -0.8164966, 1.414214, -0.5773503 ), Vec3f( -0.8164966, -1.414214, -0.5773503 ) };
	uint32_t indices[] = { 0, 1, 2, 0, 2, 3, 0, 3, 1, 1, 3, 2 };

	mMesh0.appendVertices( &vertices[ 0 ], 4 );
	mMesh0.appendIndices( &indices[ 0 ], 12 );

	// half size
	for ( size_t i = 0; i < 4; ++i )
		vertices[ i ] *= .5;
	mMesh1.appendVertices( &vertices[ 0 ], 4 );
	mMesh1.appendIndices( &indices[ 0 ], 12 );

	// setup vbo mesh
	gl::VboMesh::Layout layout;
	layout.setStaticPositions();
	layout.setStaticIndices();

	layout.mCustomStatic.push_back( std::make_pair( gl::VboMesh::Layout::CUSTOM_ATTR_FLOAT3, 0 ) );
	layout.mCustomStatic.push_back( std::make_pair( gl::VboMesh::Layout::CUSTOM_ATTR_FLOAT3, 0 ) );

	mVboMesh = gl::VboMesh( mMesh0.getNumVertices(),
			mMesh0.getNumIndices(), layout, GL_TRIANGLES );

	mVboMesh.bufferPositions( mMesh0.getVertices() ); // this sets gl_Vertex, which is the same as mesh0 now
	mVboMesh.bufferIndices( mMesh0.getIndices() );
	mVboMesh.unbindBuffers();

	mVboMesh.getStaticVbo().bind();
	size_t offset = sizeof( GLfloat ) * 3 * mMesh0.getNumVertices(); // end of the default gl_Vertex data
	mVboMesh.getStaticVbo().bufferSubData( offset,
			sizeof( Vec3f ) * mMesh0.getNumVertices(),
			&mMesh0.getVertices()[ 0 ] );
	mVboMesh.getStaticVbo().bufferSubData( offset + sizeof( GLfloat ) * 3 * mMesh0.getNumVertices(),
			sizeof( Vec3f ) * mMesh1.getNumVertices(),
			&mMesh1.getVertices()[ 0 ] );
	mVboMesh.getStaticVbo().unbind();

	mShader.bind();
	GLint loc0 = mShader.getAttribLocation( "mesh0" );
	mVboMesh.setCustomStaticLocation( 0, loc0 );
	GLint loc1 = mShader.getAttribLocation( "mesh1" );
	mVboMesh.setCustomStaticLocation( 1, loc1 );
	mShader.unbind();
}

void VboTestApp::draw()
{
	gl::clear( Color::black() );

	CameraPersp cam( getWindowWidth(), getWindowHeight(), 60.0f );
	cam.setPerspective( 60, getWindowAspectRatio(), 1, 1000 );
	cam.lookAt( Vec3f( 0, 0, -5 ), Vec3f::zero() );
	gl::setMatrices( cam );
	gl::setViewport( getWindowBounds() );

	gl::color( Color::white() );
	gl::enableWireframe();
	double t = getElapsedSeconds();
	gl::rotate( Vec3f( t * 13, t * 11, 0 ) );
	mShader.bind();
	mShader.uniform( "blend", lmap( math< float >::sin( t ), -1.f, 1.f, 0.f, 1.f ) );
	gl::draw( mVboMesh );
	mShader.unbind();
}

CINDER_APP_BASIC( VboTestApp, RendererGl )

