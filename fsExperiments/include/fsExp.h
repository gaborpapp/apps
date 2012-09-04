/*
 Copyright (C) 2012 Gabor Papp

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

#pragma once

#include <string>

#include "cinder/Cinder.h"

namespace mndl
{

class fsExp
{
	public:
		virtual void setup() {}
		virtual void setupParams() {}

		virtual void update() {}

		virtual void draw() = 0;

		const std::string &getName() const
		{
			return mName;
		}

	protected:
		fsExp() {}

		fsExp( const std::string &name ) :
			mName( name )
		{}

		std::string mName;
};

typedef std::shared_ptr< fsExp > fsExpRef;

} // namespace mndl
