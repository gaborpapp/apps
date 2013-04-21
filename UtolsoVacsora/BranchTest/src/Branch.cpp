/*
 Copyright (C) 2013 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Vbo.h"
#include "cinder/Rand.h"

#include "Resources.h"
#include "Branch.h"

using namespace ci;

ci::gl::GlslProg Branch::sShader;

Branch::Branch()
{
	if ( sShader )
		return;

	try
	{
		sShader = gl::GlslProg( app::loadResource( RES_BRANCH_VERT ),
				app::loadResource( RES_BRANCH_FRAG ),
				app::loadResource( RES_BRANCH_GEOM ),
				GL_LINES_ADJACENCY_EXT, GL_TRIANGLE_STRIP, 7 );
	}
	catch( const gl::GlslProgCompileExc &e )
	{
		app::console() << e.what() << std::endl;
	}
}

void Branch::setup( const ci::Vec2f &start, const ci::Vec2f &target )
{
	mCurrentPosition = start;
	mTargetPosition = target;
	calcPath();
}

void Branch::calcPath()
{
	// TODO: build this up sequentially
#if 1
	mLength = 0.f;
	for ( int i = 0; i < mMaxIterations; i++ )
	{
		bool lastArc = false;
		Vec2f targetDir = mTargetPosition - mCurrentPosition;
		if ( mCurrentPosition.distanceSquared( mTargetPosition ) < ( mStemLengthMax * mStemLengthMax ) )
		{
			// add the target if it is in the direction range in the next step
			lastArc = true;
		}

		if ( i == 0 )
		{
			addArc( mCurrentPosition );
			if ( lastArc )
				mCurrentPosition = mTargetPosition;
			else
				mCurrentPosition += mStemLengthMin * targetDir.normalized();
			addArc( mCurrentPosition );
			mLength += mStemLengthMin;
		}
		else
		{
			// current direction
			size_t segmentNum = mPath.getNumSegments();
			Vec2f p0 = mPath.getSegmentPosition( segmentNum - 1, 1.f );
			Vec2f currentDir = p0 - mPath.getSegmentPosition( segmentNum - 1, .98f );

			// minimum angle between the current direction and the target
			float angleDir = math< float >::atan2( currentDir.y, currentDir.x );
			float angleTarget = math< float >::atan2( targetDir.y, targetDir.x );
			float angle = angleTarget - angleDir;
			if ( angle > M_PI )
				angle -= 2 * M_PI;
			else
			if ( angle < -M_PI )
				angle += 2 * M_PI;

#if 0
			/* direction range towards the target is the intersection of the bearing angle interval around the target and
			   the current direction, if the intersecion is empty, the maximum angle is used in that direction */
			float angleMin = math< float >::clamp( angle - mStemBearingDelta, -mStemBearingDelta, mStemBearingDelta );
			float angleMax = math< float >::clamp( angle + mStemBearingDelta, -mStemBearingDelta, mStemBearingDelta );
#endif
			// FIXME: bearing delta .25 breaks continuity?
			// this algorithm tends to keep the target in sight better
			float angleMin, angleMax;
			if ( lastArc )
			{
				// get as close to the target as possible with the last arc
				if ( ( angle > -mStemBearingDelta ) && ( angle < mStemBearingDelta ) )
					angleMin = angleMax = angle;
				else
				if ( angle <= -mStemBearingDelta )
					angleMin = angleMax = -mStemBearingDelta;
				else
				if ( angle >= mStemBearingDelta )
					angleMin = angleMax = mStemBearingDelta;
			}
			else
			if ( angle <= -mStemBearingDelta ) // the angle is the maximum if the target is outside the range
			{
				angleMin = angleMax = -mStemBearingDelta;
			}
			else
			if ( angle >= mStemBearingDelta )
			{
				angleMin = angleMax = mStemBearingDelta;
			}
			else // inside the range on the left handside, allow leftward movements only
			if ( angle < 0.f )
			{
				angleMin = -mStemBearingDelta;
				angleMax = 0.f;
			}
			else // inside the range on the right handside, allow rightward movements only
			if ( angle > 0.f )
			{
				angleMin = 0.f;
				angleMax = mStemBearingDelta;
			}
			else // angle = 0, turn is allowed to either side
			{
				angleMin = -mStemBearingDelta;
				angleMax = mStemBearingDelta;
			}

			float bearingMin = angleDir + angleMin;
			float bearingMax = angleDir + angleMax;

			float stemDistance;
			if ( lastArc )
				stemDistance = targetDir.length();
			else
				stemDistance = Rand::randFloat( mStemLengthMin, mStemLengthMax );
			float stemBearingAngle = Rand::randFloat( bearingMin, bearingMax );

			Vec2f bearing( math< float >::cos( stemBearingAngle ), math< float >::sin( stemBearingAngle ) );
			mCurrentPosition += stemDistance * bearing;
			addArc( mCurrentPosition );
			mLength += stemDistance;
		}
		if ( lastArc )
			break;
	}
#endif
#if 0
	// simple algorithm with big curly movements
	for ( int i = 0; i < mMaxIterations; i++ )
	{
		Vec2f d = mTargetPosition - mCurrentPosition;

		float stemDistance = Rand::randFloat( mStemLengthMin, mStemLengthMax );
		if ( d.length() < stemDistance )
		{
			mCurrentPosition = mTargetPosition;
			addArc( mCurrentPosition );
			break;
		}
		else
		{
			float stemBearingAngle = Rand::randFloat( mStemBearingDelta );
			Vec2f bearing( d.normalized() );
			bearing.rotate( ( ( i & 1 ) ? 1 : -1 ) * stemBearingAngle );
			mCurrentPosition += stemDistance * bearing;
			addArc( mCurrentPosition );
		}
	}
#endif
}

void Branch::loadTextures( const ci::fs::path &textureFolder )
{
	fs::path dataPath = app::getAssetPath( textureFolder );

	for ( fs::directory_iterator it( dataPath ); it != fs::directory_iterator(); ++it )
	{
		if ( fs::is_regular_file( *it ) && ( it->path().extension().string() == ".png" ) )
		{
			std::string basename( it->path().stem().string() );
			gl::Texture t = loadImage( app::loadAsset( textureFolder / it->path().filename() ) );
			if ( basename.find( "stem" ) != std::string::npos )
			{
				mStemTexture = t;
			}
			else
			if ( basename.find( "branch" ) != std::string::npos )
				mBranchTexture = t;
			else
			if ( basename.find( "leaf" ) != std::string::npos )
				mLeafTextures.push_back( t );
			else
			if ( basename.find( "flower" ) != std::string::npos )
				mFlowerTextures.push_back( t );
		}
	}
}

void Branch::start()
{
	mPoints = mPath.subdivide( mApproximationScale );
	mGrowPosition = 0;
	mGrown = false;
	if ( mGrowSpeed != 0.f )
	{
		float duration = mLength / ( 1000.f * mGrowSpeed );
		app::timeline().apply( &mGrowPosition, mPoints.size(), duration ).finishFn( [ & ]() { mGrown = true; } );
	}
	else
	{
		mGrowPosition = mPoints.size();
		mGrown = true;
	}
}

void Branch::update()
{
}

void Branch::draw()
{
	//gl::color( Color::white() );
	//gl::color( Color( 0, 1, 0 ) );
	//gl::draw( mPath );

	/*
	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 2, GL_FLOAT, 0, &( mPoints[ 0 ] ) );
	glDrawArrays( GL_LINE_STRIP, 0, mGrowPosition );
	glDisableClientState( GL_VERTEX_ARRAY );
	*/

	if ( mPoints.size() < 2 )
		return;

	// create a new vector that can contain 3D vertices
	std::vector< Vec3f > vertices;

	// to improve performance, make room for the vertices + 2 adjacency vertices
	vertices.reserve( mGrowPosition + 2 );

	// first, add an adjacency vertex at the beginning
	vertices.push_back( 2.0f * Vec3f( mPoints[ 0 ] ) - Vec3f( mPoints[ 1 ] ) );

	// next, add all 2D points as 3D vertices
	size_t endIndex = 0;
	float currentLength = 0;
	for ( size_t i = 0; i < mGrowPosition; i++ )
	{
		float d = 0;
		if ( !vertices.empty() )
			d = vertices.back().distance( Vec3f( mPoints[ i ] ) );
		//	vertices.push_back( Vec3f( mPoints[ i ] ) );
		if ( vertices.empty() || d > 5.f )
		{
			vertices.push_back( Vec3f( mPoints[ i ] ) );
			endIndex = i;
			currentLength += d;
		}
	}

	// next, add an adjacency vertex at the end
	size_t n = mGrowPosition;
	vertices.push_back( 2.0f * Vec3f( mPoints[ n - 1 ] ) - Vec3f( mPoints[ n - 2 ] ) );

	// now that we have a list of vertices, create the index buffer
	n = vertices.size() - 2;
	std::vector< uint32_t > indices;

	indices.reserve( n * 4 );
	for ( size_t i = 1 ; i < vertices.size() - 2; ++i )
	{
		indices.push_back( i - 1 );
		indices.push_back( i );
		indices.push_back( i + 1 );
		indices.push_back( i + 2 );
	}

	std::vector< Vec2f > texCoords;
	n = vertices.size();
	texCoords.reserve( n );
	Vec2f uv( Vec2f::zero() );
	Vec2f step( 0.0f, 1.f / n );
	for ( size_t i = 0 ; i < n; ++i )
	{
		texCoords.push_back( uv );
		uv += step;
	}

	// finally, create the mesh
	gl::VboMesh::Layout layout;
	layout.setStaticPositions();
	layout.setStaticIndices();
	layout.setStaticTexCoords2d();

	gl::VboMesh mVboMesh = gl::VboMesh( vertices.size(), indices.size(), layout, GL_LINES_ADJACENCY_EXT );
	mVboMesh.bufferPositions( &(vertices.front()), vertices.size() );
	mVboMesh.bufferIndices( indices );
	mVboMesh.bufferTexCoords2d( 0, texCoords );

	// FIXME: the first line segment is not drawn
	if ( sShader )
	{
		sShader.bind();

		sShader.uniform( "WIN_SCALE", mWindowSize );
		sShader.uniform( "MITER_LIMIT", mLimit );
		sShader.uniform( "THICKNESS", mThickness );
		sShader.uniform( "tex0", 0 );
	}
	gl::color( mColor );

	if ( mStemTexture )
		mStemTexture.bind();

	gl::draw( mVboMesh );
	if ( mStemTexture )
		mStemTexture.unbind();

	if ( sShader )
		sShader.unbind();

	// current direction
	Vec2f dir;
	if ( endIndex == 0 )
	{
		dir = mPoints[ 1 ] - mPoints[ 0 ];
	}
	else
	if ( endIndex < mPoints.size() - 1 )
	{
		dir = mPoints[ endIndex ] - mPoints[ endIndex - 1 ];
	}
	else
	{
		size_t n = mPoints.size() - 1;
		dir = mPoints[ n ] - mPoints[ n - 1 ];
	}
	/*
	dir.normalize();
	gl::color( Color( 1, 0, 0 ) );
	gl::drawLine( mPoints[ endIndex ], mPoints[ endIndex ] + 20 * dir );
	*/

	if ( ( currentLength - mLastItemLength ) > mSpawnInterval )
	{
		mLastItemLength = currentLength;
		dir.rotate( mBranchAngle * mCurrentItemSide );
		if ( Rand::randInt( 2 ) == 0 )
			mBranchItems.push_back( BranchItem( mLeafTextures[ Rand::randInt( mLeafTextures.size() ) ],
						mPoints[ endIndex ], dir ) );
		else
			mBranchItems.push_back( BranchItem( mFlowerTextures[ Rand::randInt( mFlowerTextures.size() ) ],
						mPoints[ endIndex ], dir ) );
		mBranchItems.back().mScale = mItemScale;
		mCurrentItemSide *= -1.f;
	}

	gl::color( mColor );
	for ( auto &item : mBranchItems )
	{
		item.draw();
	}

#if 0
	//const std::vector< Vec2f > &points = mPath.subdivide( .001f );
	const std::vector< Vec2f > &points = mPath.getPoints();
	gl::color( Color( 1, 0, 0 ) );
	for ( auto p : points )
	{
		gl::drawSolidCircle( p, 2 );
	}
#endif
}

void Branch::addArc( const Vec2f &p1 )
{
	if ( mPath.empty() )
	{
		mPath.moveTo( p1 );
		return;
	}

	size_t segmentNum = mPath.getNumSegments();
	if ( segmentNum == 0 )
	{
		mPath.lineTo( p1 );
		return;
	}

	// last point
	Vec2f p0 = mPath.getSegmentPosition( segmentNum - 1, 1.f );
	// same point as the last one -> skip
	if ( p0.distanceSquared( p1 ) < EPSILON )
		return;

	// segment tangent - direction of the last segment
	Vec2f tangent = p0 - mPath.getSegmentPosition( segmentNum - 1, .98f );
	Vec2f n( -tangent.y, tangent.x ); // segment normal

	Vec2f d( p1 - p0 ); // distance of the arc center and the last path point
	d *= .5f;
	Vec2f b( -d.y, d.x ); // arc bisector

	// parallel vectors, p1 lies on the tangent
	float dot = b.dot( n );
	float sqrLength = b.lengthSquared() * n.lengthSquared();
	// does not seem to be a very precise test, hence the .1f limit instead of EPSILON
	if ( math< float >::abs( dot * dot - sqrLength ) < .1f )
	{
		mPath.lineTo( p1 );
		return;
	}

	// the arc center is the intersection of the segment normal and the bisector
	float s;
	if ( n.x == 0.f )
	{
		if ( b.x == 0.f ) // parallel
		{
			mPath.lineTo( p1 );
			return;
		}
		s = -d.x / b.x;
	}
	else
	{
		float den = b.x * n.y / n.x - b.y;
		if ( den == 0.f ) // parallel
		{
			mPath.lineTo( p1 );
			return;
		}
		s = ( d.y - d.x * n.y / n.x ) / ( b.x * n.y / n.x - b.y );
	}
	Vec2f c( ( p0 + p1 ) *.5f + s * b ); // arc center

	Vec2f d0( p0 - c );
	float a0 = math< float >::atan2( d0.y, d0.x );
	Vec2f d1( p1 - c );
	float a1 = math< float >::atan2( d1.y, d1.x );

	// segment normal line and segment bisector distance
	tangent.normalize();
	float C = tangent.dot( p0 );
	float dist = tangent.dot( ( p0 + p1 ) * .5f ) - C;

	// arc direction depends on the quadrant, in which the new point resides
	bool forward = ( s > 0.f ) ^ ( dist < 0.f );
	mPath.arc( c, d0.length(), a0, a1, forward );
}

Branch::BranchItem::BranchItem( gl::Texture texture, const Vec2f &position, const Vec2f &direction ) :
	mTexture( texture ), mPosition( position )
{
	mRotation = toDegrees( math< float >::atan2( direction.y, direction.x ) ) + 45.f;
}

void Branch::BranchItem::BranchItem::draw()
{
	gl::pushModelView();
	gl::translate( mPosition );
	gl::rotate( mRotation );
	gl::scale( Vec2f( mScale, mScale ) );
	gl::translate( Vec2f( 0, -mTexture.getHeight() ) );
	gl::draw( mTexture );
	gl::popModelView();
}

