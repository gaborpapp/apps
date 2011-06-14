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

#include <string>

#include "cinder/Utilities.h"

#include "AssimpLoader.h"

using namespace std;
using namespace ci;
using namespace ci::app;

AssimpLoader::AssimpLoader(DataSourceRef dataSource)
	: mBuffer(dataSource->getBuffer())
{
	string ext = getPathExtension(dataSource->getFilePath());
	load(ext);
}

AssimpLoader::~AssimpLoader()
{
}

void AssimpLoader::load(const string &ext)
{
	const void *data = mBuffer.getData();
	size_t data_size = mBuffer.getDataSize();

	scene = importer.ReadFileFromMemory(data, data_size, 0, ext.c_str());
	if (!scene)
		throw AssimpLoaderExc(importer.GetErrorString());
}

void AssimpLoader::load(TriMesh *destTriMesh)
{
	destTriMesh->clear();
	recursiveLoad(destTriMesh, scene, scene->mRootNode);
}

void AssimpLoader::recursiveLoad(TriMesh *destTriMesh, const aiScene *sc,
		const aiNode* nd, Matrix44f accTransform /* = Matrix44f::identity() */)
{
	console() << string(nd->mName.data) << endl;

	aiMatrix4x4 aiTransf = nd->mTransformation;
	aiTransf.Transpose();
	Matrix44f transform(const_cast<const float *>(&aiTransf.a1));

	transform = accTransform * transform;

	for (unsigned n = 0; n < nd->mNumMeshes; n++)
	{
		size_t offset = destTriMesh->getNumVertices();

		const struct aiMesh *mesh = scene->mMeshes[nd->mMeshes[n]];

		/*
		for (unsigned i = 0; i < mesh->mNumBones; i++)
		{
			const aiBone *bone = mesh->mBones[i];
			console() << "bone " + string(bone->mName.data) << endl;
		}
		*/

		bool normals = mesh->HasNormals();
		bool texCoords = mesh->HasTextureCoords(0);

		for (unsigned i = 0; i < mesh->mNumVertices; i++)
		{
			Vec3f v(const_cast<const float *>(&(mesh->mVertices[i].x)));
			Vec3f vt = transform.transformPoint(v);
			destTriMesh->appendVertex(vt);

			if (normals)
				destTriMesh->appendNormal(Vec3f(const_cast<const float *>(&(mesh->mNormals[i].x))));

			if (texCoords)
				destTriMesh->appendTexCoord(Vec2f(mesh->mTextureCoords[0][i].x,
							1.0 - mesh->mTextureCoords[0][i].y));
		}

		for (unsigned t = 0; t < mesh->mNumFaces; t++)
		{
			const struct aiFace *face = &(mesh->mFaces[t]);

			if (face->mNumIndices < 3)
				continue;

			for (unsigned i = 1; i < face->mNumIndices - 1; i++)
			{
				destTriMesh->appendTriangle(offset + face->mIndices[0],
						offset + face->mIndices[i],
						offset + face->mIndices[i + 1]);
			}
		}
	}

	// process all children
	for (unsigned n = 0; n < nd->mNumChildren; n++)
	{
		recursiveLoad(destTriMesh, sc, nd->mChildren[n], transform);
	}
}

