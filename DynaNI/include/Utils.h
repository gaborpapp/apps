#pragma once

#include <string>
#include <vector>

#include "cinder/gl/Texture.h"
#include "cinder/Filesystem.h"

namespace cinder {

//! Returns time stamp for current time.
std::string timeStamp();

std::vector< ci::gl::Texture > loadTextures( const ci::fs::path &relativeDir );

} // namespace cinder
