/*
 Copyright (c) 2010, The Barbarian Group
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

	* Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
	* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "cinder/params/Params.h"
#include "cinder/Xml.h"

#include <vector>
#include <string>

#include <boost/bind.hpp>
#include <boost/function.hpp>

namespace cinder { namespace params {

class PInterfaceGl : public InterfaceGl {
 public:
	PInterfaceGl() {}
	PInterfaceGl( const std::string &title, const Vec2i &size, const ColorA colorA = ColorA( 0.3f, 0.3f, 0.3f, 0.4f ) )
		: InterfaceGl( title, size, colorA ), m_id( name2id( title ) ) {}

	/** Add a persistent parameter for the window size, position and iconified status
	 * Persistent parameter will be initialized with saved value if found, or with
	 * supplied default otherwise
	 */
	void addPersistentSizeAndPosition();

	/** Add a persistent parameter to the window.  Persistent parameter will be
	 * initialized with saved value if found, or with supplied default
	 * otherwise
	 */
	template<typename T, typename TVAL>
	void addPersistentParam(const std::string& name, T* var, TVAL defVal,
			const std::string& optionsStr="", bool readOnly=false)
	{
		addParam(name,var,optionsStr,readOnly);
		std::string id = name2id(name);
		*var = getXml().hasChild(id)
			? getXml().getChild(id).getValue((T)defVal)
			: (T)defVal;
		persistCallbacks().push_back(
				boost::bind( &PInterfaceGl::persistParam<T>, this, var, id ) );
	}

	//! Adds enumerated persistent parameter. The value corresponds to the indices of \a enumNames.
	void addPersistentParam(const std::string& name, std::vector<std::string> &enumNames, int* var, int defVal,
			const std::string& optionsStr="", bool readOnly=false)
	{
		addParam(name, enumNames, var, optionsStr, readOnly);
		std::string id = name2id(name);
		*var = getXml().hasChild(id)
			? getXml().getChild(id).getValue(defVal)
			: defVal;
		persistCallbacks().push_back(
				boost::bind( &PInterfaceGl::persistParam<int>, this, var, id ) );
	}

	/** Load persistent params from file. At the moment this only works when
	 * called at application start up, before creating persistent parameteres.
	 * Will remember the filename for saving later.
	 */
	static void load(const std::string& path);

	/** Save persistent params (to the path passed to load before). */
	static void save();

protected:
	std::string m_id;

	// "manager"
	struct Manager {
		std::vector< boost::function< void() > > persistCallbacks;
		XmlTree root;
		std::string filename;

		Manager() {
			root = XmlTree::createDoc();
		}
	};

	static Manager& manager() {
		static Manager * m = new Manager();
		return *m;
	}

	static std::vector< boost::function< void() > >& persistCallbacks()
	{
		return manager().persistCallbacks;
	}
	static XmlTree& root()
	{
		return manager().root;
	}
	static std::string& filename()
	{
		return manager().filename;
	}

	// save current size, position and iconified status value into an xml tree
	void persistSizeAndPosition();

	// save current parameter value into an xml tree
	template<typename T>
	void persistParam(T * var, const std::string& paramId)
	{
		if (!getXml().hasChild(paramId))
			getXml().push_back(XmlTree(paramId,""));
		getXml().getChild(paramId).setValue(*var);
	}

	XmlTree& getXml() {
		if (!root().hasChild(m_id))
			root().push_back(XmlTree(m_id,""));
		return root().getChild(m_id);
	}


	// convert "some title" to SomeTitle so it can be used as XML tag
	static std::string name2id( const std::string& name );
};

} } // namespace cinder::params
