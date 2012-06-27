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
#include <sstream>

#include "cinder/Cinder.h"
#include "cinder/app/App.h"
#include "cinder/params/Params.h"

namespace mndl { namespace fx {

#define STRINGIFY(x) #x

class ParameterBase
{
	public:
		ParameterBase() {}

		ParameterBase( const std::string &name ) :
			mName( name )
		{}

		virtual void addToParams( ci::params::InterfaceGl &params ) = 0;

	protected:
		std::string mName;
};

template< typename T >
class ParameterNumber : public ParameterBase
{
	public:
		ParameterNumber()
		{}

		ParameterNumber( const std::string &name, T def, T min, T max ) :
			mValue( def ), mDef( def ),
			mMin( min ), mMax( max ),
			ParameterBase( name )
		{}

		ParameterNumber( const ParameterNumber< T > &rhs ) // normal copy constructor
			: mValue( rhs.mValue ),
			mDef( rhs.mDef ), mMin( rhs.mMin ), mMax( rhs.mMax ),
			ParameterBase( rhs.mName )
		{}

		operator const T&() const { return mValue; }

		ParameterNumber< T >& operator=( const ParameterNumber< T > &rhs ) // copy assignment
		{
			if ( this != &rhs )
			{
				mName = rhs.mName;
				mValue = rhs.mValue;
				mDef = rhs.mDef;
				mMin = rhs.mMin;
				mMax = rhs.mMax;
			}
			return *this;
		}

		void addToParams( ci::params::InterfaceGl &params ) {
			std::stringstream ss;
			T step = ( mMax - mMin ) / 256.;
			if ( step == 0 )
				step = 1;
			ss << "min=" << mMin << " max=" << mMax << " step=" << step;
			params.addParam( mName, &mValue, ss.str() );
		}

	private:
		T mValue;
		T mDef;
		T mMin;
		T mMax;
};

typedef ParameterNumber< float > ParameterFloat;
typedef ParameterNumber< int > ParameterInt;

class ParameterBool : public ParameterBase
{
	public:
		ParameterBool()
		{}

		ParameterBool( const std::string &name, bool def ) :
			mValue( def ), mDef( def ),
			ParameterBase( name )
		{}

		ParameterBool( const ParameterBool &rhs ) // normal copy constructor
			: mValue( rhs.mValue ),
			mDef( rhs.mDef ),
			ParameterBase( rhs.mName )
		{}

		operator const bool&() const { return mValue; }

		ParameterBool& operator=( const ParameterBool &rhs ) // copy assignment
		{
			if ( this != &rhs )
			{
				mName = rhs.mName;
				mValue = rhs.mValue;
				mDef = rhs.mDef;
			}
			return *this;
		}

		void addToParams( ci::params::InterfaceGl &params )
		{
			params.addParam( mName, &mValue );
		}

	private:
		bool mValue;
		bool mDef;
};

class ParameterColorA : public ParameterBase
{
	public:
		ParameterColorA()
		{}

		ParameterColorA( const std::string &name, ci::ColorA def ) :
			mValue( def ), mDef( def ),
			ParameterBase( name )
		{}

		ParameterColorA( const ParameterColorA &rhs ) // normal copy constructor
			: mValue( rhs.mValue ),
			mDef( rhs.mDef ),
			ParameterBase( rhs.mName )
		{}

		operator const ci::ColorA&() const { return mValue; }

		ParameterColorA& operator=( const ParameterColorA &rhs ) // copy assignment
		{
			if ( this != &rhs )
			{
				mName = rhs.mName;
				mValue = rhs.mValue;
				mDef = rhs.mDef;
			}
			return *this;
		}

		void addToParams( ci::params::InterfaceGl &params )
		{
			params.addParam( mName, &mValue );
		}

	private:
		ci::ColorA mValue;
		ci::ColorA mDef;
};

class Effect
{
	public:
		size_t getParameterCount() const
		{
			return mParams->mParameters.size();
		}

		const std::string &getName() const
		{
			return mName;
		}

		void addToParams( ci::params::InterfaceGl &params )
		{
			for ( size_t i = 0; i < mParams->mParameters.size(); ++i )
				mParams->mParameters[ i ]->addToParams( params );
		}

		virtual ci::gl::Texture &process( const ci::gl::Texture &source ) = 0;

	protected:
		Effect() {}

		Effect( const std::string &name ) :
			mName( name ),
			mParams( new Params )
		{}

		struct Params
		{
			void addParameter( ParameterBase *prm )
			{
				mParameters.push_back( prm );
			}

			std::vector< ParameterBase * > mParameters;
		};
		std::shared_ptr< Params > mParams;

		std::string mName;
};

typedef std::shared_ptr< class Effect > EffectRef;

} }; // namespace mndl::fx

