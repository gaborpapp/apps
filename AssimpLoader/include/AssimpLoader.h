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

#pragma once

#include "assimp.hpp"
#include "aiScene.h"

#include "cinder/TriMesh.h"
#include "cinder/Stream.h"
#include "cinder/Exception.h"
#include "cinder/app/App.h"
#include "cinder/Matrix.h"


namespace cinder
{

class AssimpLoader
{
	public:
		/** Constructs and does the parsing of the file **/
		AssimpLoader(DataSourceRef dataSource);
		~AssimpLoader();

		/** Loads all the groups present in the file into a single TriMesh
		 *  \param destTriMesh the destination TriMesh, whose contents are cleared first
		 **/
		void load(TriMesh *destTriMesh);

		//! Base class for AssimpLoader exceptions.
		class Exception : public ci::Exception {};

		//! Exception expressing Assimp importer problems
		class AssimpLoaderExc : public AssimpLoader::Exception
		{
			public:
				AssimpLoaderExc(const std::string &err) throw()
				{
					mMessage = err;
				}
				~AssimpLoaderExc() throw() {};

				virtual const char *what() const throw()
				{
					return mMessage.c_str();
				}

			private:
				std::string mMessage;
		};

	private:
		Buffer mBuffer;

		void load(const std::string &ext = "");
		void recursiveLoad(TriMesh *destTriMesh, const aiScene *sc,
				const aiNode* nd, Matrix44f accTransform = Matrix44f::identity());

		Assimp::Importer importer;
		const aiScene *scene;
};

} // namespace cinder

