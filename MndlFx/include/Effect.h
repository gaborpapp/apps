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

#include <vector>
#include <string>

#include "cinder/Cinder.h"
#include "cinder/params/Params.h"

#include "Param.h"

namespace mndl { namespace fx {

#define STRINGIFY(x) #x

class Effect
{
	public:
		size_t getParamCount() const
		{
			return mParams.size();
		}

		const std::string &getName() const
		{
			return mName;
		}

		void addToParams( ci::params::InterfaceGl &params )
		{
			for ( size_t i = 0; i < mParams.size(); ++i )
				mParams[ i ]->addToParams( params );
		}

		virtual ci::gl::Texture &process( const ci::gl::Texture &source ) = 0;

		///! Adds parameter to the effect
		void addParam( IParam *prm )
		{
			mParams.push_back( prm );
		}

	protected:
		Effect() {}

		Effect( const std::string &name ) :
			mName( name )
		{}

		std::string mName;
		std::vector< IParam * > mParams;
};

typedef std::shared_ptr< class Effect > EffectRef;

} } // namespace mndl::fx

