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

#pragma once

#include "cinder/Cinder.h"
#include "cinder/Rect.h"
#include "cinder/Vector.h"

namespace mndl {

struct Blob
{
	Blob() : mId( -1 ) {}

	int32_t mId;
	ci::Rectf mBbox;
	ci::Vec2f mCentroid;
	ci::Vec2f mPrevCentroid;
};
typedef std::shared_ptr< Blob > BlobRef;

//! Represents a blob event
class BlobEvent
{
	public:
		BlobEvent( BlobRef blobRef ) : mBlobRef( blobRef ) {}

		//! Returns an ID unique for the lifetime of the blob
		int32_t getId() const { return mBlobRef->mId; }
		//! Returns the x position of the blob centroid normalized to the image source width
		float getX() const { return mBlobRef->mCentroid.x; }
		//! Returns the y position of the blob centroid normalized to the image source height
		float getY() const { return mBlobRef->mCentroid.y; }
		//! Returns the position of the blob centroid normalized to the image resolution
		ci::Vec2f getPos() const { return mBlobRef->mCentroid; }
		//! Returns the previous x position of the blob centroid normalized to the image source width
		float getPrevX() const { return mBlobRef->mPrevCentroid.x; }
		//! Returns the previous y position of the blob centroid normalized to the image source height
		float getPrevY() const { return mBlobRef->mPrevCentroid.y; }
		//! Returns the previous position of the blob centroid normalized to the image resolution
		ci::Vec2f getPrevPos() const { return mBlobRef->mPrevCentroid; }

		//! Returns the bounding box of the blob
		ci::Rectf & getBoundingBox() const { return mBlobRef->mBbox; }
	private:
		BlobRef mBlobRef;
};

} // namespace mndl

