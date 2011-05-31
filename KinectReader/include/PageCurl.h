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

#ifndef PAGE_CURL_H
#define PAGE_CURL_H

#include <queue>
#include <vector>

#include "cinder/Cinder.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Area.h"
#include "cinder/Vector.h"
#include "cinder/TriMesh.h"
#include "cinder/Timer.h"
#include "cinder/audio/Output.h"
#include "cinder/audio/Io.h"

class PageCurl
{
	public:
		PageCurl(const std::string dir, const ci::Area& area);
		void resize(const ci::Area& area);

		bool preload(float *progress, int files_per_call = 5);

		void draw();

		void prev();
		void next();
		void set_zoom(float z) { pzoom = z; };
		void set_zoom_pos(ci::Vec2f p) { pzoom_pos = p; };
		float get_zoom() { return pzoom; };

	private:
		void render(const ci::Vec2f& pos, const int i, const int corner);

		ci::Area book_area;
		ci::Rectf bounds;

		bool recalc_bounds_needed;
		void recalc_bounds();

		float book_aspect_ratio;
		ci::Vec2f pdim; // page dimensions
		float pzoom; // page zoom
		ci::Vec2f pzoom_pos; // page zoom position

		void get_pages(const std::string dir);
		void load_page(const std::string filename);

		ci::Vec2f limit_corner_location(const ci::Vec2f& p);
		ci::Vec2f mirror_point(const ci::Vec2f& p, const ci::Vec2f& p0, const ci::Vec2f& p1);
		float project_pnt_scale(const ci::Vec3f& p, const ci::Vec3f& p0, const ci::Vec3f& p1);
		ci::Vec3f point_line_vector(const ci::Vec3f& p, const ci::Vec3f& p0, const ci::Vec3f& p1);
		float point_line_distance(const ci::Vec3f& p, const ci::Vec3f& p0, const ci::Vec3f& p1);
		ci::Vec2f page_curl_points(const ci::Vec2f& p, std::vector<ci::Vec3f>& points, std::vector<ci::Vec2f>& uvs);

		inline ci::Vec2f flipx(ci::Vec2f v)
		{
			return ci::Vec2f(1 - v.x, v.y);
		};

		std::queue<std::string> book_files;
		std::vector<ci::gl::Texture> book;
		ci::gl::Texture page0_shadow;
		ci::gl::Texture page_1_shadow;
		ci::gl::Texture page_shadow;

		int page_idx; // current page index
		int corner; // current corner
		enum // corners
		{
			BOTTOM_LEFT,
			BOTTOM_RIGHT
		};

		int state;
		enum
		{
			STATE_LOADING,
			STATE_IDLE,
			STATE_PREV,
			STATE_NEXT
		};

		float flip_t; // flipping interpolation value
		float flip_speed;
		ci::Timer timer;

		void load_samples();
		// audio samples
		std::vector<ci::audio::SourceRef> samples;
};

#endif

