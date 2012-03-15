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
#include <assert.h>

#include "cinder/app/App.h"
#include "cinder/ImageIo.h"
#include "cinder/Utilities.h"
#include "cinder/Filesystem.h"
#include "cinder/CinderMath.h"
#include "cinder/Rand.h"

#include "PageCurl.h"

#include "Resources.h"

using namespace ci;
using namespace ci::app;
using namespace std;

PageCurl::PageCurl(const string dir, const Area& area) :
	page_idx(0),
	corner(BOTTOM_RIGHT),
	state(STATE_IDLE),
	flip_t(1.0),
	flip_speed(1.5)
{
	load_textures(dir);

	set_bounds(area);

	page0_shadow = loadImage(loadResource(RES_PAGE0_SHADOW));
	page_1_shadow = loadImage(loadResource(RES_PAGE_1_SHADOW));
	page_shadow = loadImage(loadResource(RES_PAGE_SHADOW));

	load_samples();

	timer.start();
}

void PageCurl::resize(const Area& area)
{
	set_bounds(area);
}

void PageCurl::set_bounds(const Area& area)
{
	bounds = area;
	pdim = Vec2f(area.getWidth() / 2.0, area.getHeight());
}

void PageCurl::load_textures(string dir)
{
	string data_path = getAppPath().string();
#ifdef CINDER_MAC
	data_path += "/Contents/Resources/";
#endif
	data_path += dir + "/";

	book.clear();

	fs::path p(data_path);
	for (fs::directory_iterator it(p); it != fs::directory_iterator(); ++it)
	{
		if (fs::is_regular_file(*it))
		{
#ifdef DEBUG
			console() << "   " << it->path().filename() << endl;
#endif
			book.push_back(gl::Texture(loadImage(data_path + it->path().filename().string())));
		}
	}
}

void PageCurl::load_samples()
{
	string data_path = getAppPath().string();
#ifdef CINDER_MAC
	data_path += "/Contents/Resources/";
#endif
	data_path += "sfx/";

	samples.clear();

	fs::path p(data_path);
	for (fs::directory_iterator it(p); it != fs::directory_iterator(); ++it)
	{
		if (fs::is_regular_file(*it) && (it->path().extension().string() == ".mp3"))
		{
#ifdef DEBUG
			console() << "   " << it->path().filename() << endl;
#endif
			samples.push_back(audio::load(data_path + it->path().filename().string()));
		}
	}
}
void PageCurl::prev()
{
	if ((state == STATE_IDLE) && (page_idx > 0))
	{
		state = STATE_PREV;

		audio::Output::play(samples[Rand::randInt(samples.size())]);
	}
}

void PageCurl::next()
{
	if ((state == STATE_IDLE) &&
		((!(book.size() & 1) && (page_idx < static_cast<int>(book.size() - 1))) ||
		 ((book.size() & 1) && (page_idx < static_cast<int>(book.size() - 2)))))
	{
		state = STATE_NEXT;

		audio::Output::play(samples[Rand::randInt(samples.size())]);
	}
}

// Hermite Interpolation on the unit interval (0, 1) given a starting point
// p1 at s = 0 and an ending point p2 at s = 1 with starting tangent t1 at s = 0
// and ending tangent t2 at s = 1
template<typename T, typename L>
T hermiteInterp(T p1, T t1, T p2, T t2, L s)
{
	L s2 = s * s;
	L s3 = s * s * s;
	L h1 = 2 * s3 - 3 * s2 + 1;
	L h2 = -2 * s3 + 3 * s2;
	L h3 = s3 - 2 * s2 + s;
	L h4 = s3 - s2;
	return h1 * p1 + h2 * p2 + h3 * t1 + h4 * t2;
}

template<typename T, typename L>
T hermiteInterpRef(const T &p1, const T &t1, const T &p2, const T &t2, L s)
{
	L s2 = s * s;
	L s3 = s * s * s;
	L h1 = 2 * s3 - 3 * s2 + 1;
	L h2 = -2 * s3 + 3 * s2;
	L h3 = s3 - 2 * s2 + s;
	L h4 = s3 - s2;
	return h1 * p1 + h2 * p2 + h3 * t1 + h4 * t2;
}

void PageCurl::draw()
{
	static float last_time = 0;

	float time = timer.getSeconds();
	float delta = time - last_time;
	last_time = time;

	switch (state)
	{
		case STATE_NEXT:
			if (corner == BOTTOM_LEFT)
			{
				corner = BOTTOM_RIGHT;
				flip_t = 1;
				page_idx += 1;
			}

			if (flip_t > 0)
			{
				flip_t -= flip_speed * delta;
			}
			else
			{
				state = STATE_IDLE;
				flip_t = 0;
				page_idx += 1;
				corner = BOTTOM_LEFT;
			}
			break;

		case STATE_PREV:
			if (corner == BOTTOM_RIGHT)
			{
				corner = BOTTOM_LEFT;
				flip_t = 0;
				page_idx -= 1;
			}

			if (flip_t < 1)
			{
				flip_t += flip_speed * delta;
			}
			else
			{
				state = STATE_IDLE;
				flip_t = 1;
				page_idx -= 1;
				corner = BOTTOM_RIGHT;
			}
			break;
	}

	Vec2f p0 = Vec2f(0, pdim.y);
	Vec2f t0 = Vec2f(pdim.x * .5, -pdim.y * .5);
	Vec2f p1 = Vec2f(pdim.x * 2, pdim.y);
	Vec2f t1 = Vec2f(pdim.x * .5, pdim.y * .5);

	Vec2f pos = hermiteInterpRef<Vec2f, float>(p0, t0, p1, t1, flip_t);

	render(pos, page_idx, corner);
}

Vec2f flip(const Vec2f& v)
{
	return Vec2f(1 - v.x, v.y);
}

Vec2f noflip(const Vec2f& v)
{
	return Vec2f(v.x, v.y);
}

void PageCurl::render(const Vec2f& pos, const int page0_i, const int corner)
{
	gl::disableDepthRead();
	gl::enableAlphaBlending();

	gl::enable(GL_TEXTURE_2D);
	gl::disable(GL_CULL_FACE);

	Vec2f corner_pos(pos);

	Vec2f (*tex_coord_func)(const Vec2f&);
	if (corner == BOTTOM_LEFT)
	{
		tex_coord_func = &noflip;
	}
	else
	{
		tex_coord_func = &flip;
	}

	if (corner == BOTTOM_RIGHT)
	{
		corner_pos.x = 2 * pdim.x - pos.x;
	}

	// page curl points and texture coordinates
	vector<Vec3f> points;
	vector<Vec2f> uvs;
	Vec2f clamp_pos;
	clamp_pos = page_curl_points(corner_pos, points, uvs);
	//clamp_pos = page_curl_points(pos - bounds.getUpperLeft(), points, uvs);

	// page texture indices
	int page_2_i, page_1_i, page1_i;
	switch (corner)
	{
		case BOTTOM_LEFT:
			page_2_i = page0_i - 2;
			page_1_i = page0_i - 1;
			page1_i = page0_i + 1;
			break;
		case BOTTOM_RIGHT:
			page_2_i = page0_i + 2;
			page_1_i = page0_i + 1;
			page1_i = page0_i - 1;
			break;
		default:
			assert(0);
			break;
	}

	// shadow opacity
	float d = clamp_pos.distance(Vec2f(2 * pdim.x, pdim.y));
	float md = pdim.x / 4;
	float shadow_opac = d > md ? 1 : (1 - (md - d) / md);

	gl::color(Color::white());

	gl::pushModelView();
	gl::translate(bounds.getUpperLeft());

	if (corner == BOTTOM_RIGHT)
	{
		glTranslatef(pdim.x, pdim.y / 2, 0);
		glScalef(-1, 1, 1);
		glTranslatef(-pdim.x, -pdim.y / 2, 0);
	}

	// draw pages in display order
	TriMesh page, shadow;
	// next page after flipped one
	page.appendVertex(Vec3f(pdim.x, 0, 0));
	page.appendVertex(Vec3f(pdim));
	page.appendVertex(Vec3f(2 * pdim.x, pdim.y, 0));
	page.appendVertex(Vec3f(2 * pdim.x, 0, 0));

	page.appendTexCoord(tex_coord_func(Vec2f(0, 0)));
	page.appendTexCoord(tex_coord_func(Vec2f(0, 1)));
	page.appendTexCoord(tex_coord_func(Vec2f(1, 1)));
	page.appendTexCoord(tex_coord_func(Vec2f(1, 0)));

	page.appendTriangle(0, 1, 2);
	page.appendTriangle(2, 3, 0);

	if ((page1_i >= 0) && (page1_i < static_cast<int>(book.size())))
	{
		book[page1_i].bind();
		gl::draw(page);
		book[page1_i].unbind();
	}

	// frontside of flipped page
	page.clear();
	unsigned n = 8 - points.size();
	if (n == 4)
	{
		page.appendVertex(points[0]);
		page.appendVertex(points[1]);
		page.appendVertex(Vec3f(pdim));
		page.appendVertex(Vec3f(pdim.x, 0, 0));

		page.appendTexCoord(tex_coord_func(flipx(uvs[0])));
		page.appendTexCoord(tex_coord_func(flipx(uvs[1])));
		page.appendTexCoord(tex_coord_func(Vec2f(1, 1)));
		page.appendTexCoord(tex_coord_func(Vec2f(1, 0)));

		page.appendTriangle(0, 1, 2);
		page.appendTriangle(2, 3, 0);
	}
	else
	{
		page.appendVertex(Vec3f(0, 0, 0));
		page.appendVertex(points[0]);
		page.appendVertex(points[1]);
		page.appendVertex(Vec3f(pdim));
		page.appendVertex(Vec3f(pdim.x, 0, 0));

		page.appendTexCoord(tex_coord_func(Vec2f(0, 0)));
		page.appendTexCoord(tex_coord_func(flipx(uvs[0])));
		page.appendTexCoord(tex_coord_func(flipx(uvs[1])));
		page.appendTexCoord(tex_coord_func(Vec2f(1, 1)));
		page.appendTexCoord(tex_coord_func(Vec2f(1, 0)));

		page.appendTriangle(0, 1, 2);
		page.appendTriangle(2, 3, 0);
		page.appendTriangle(3, 4, 0);
	}
	if ((page0_i >= 0) && (page0_i < static_cast<int>(book.size())))
	{
		book[page0_i].bind();
		gl::draw(page);
		book[page0_i].unbind();

		// shadow
		page_shadow.bind();
		Vec3f p0(points[0]);
		Vec3f p1(points[1]);
		Vec3f vd(p1 - p0);
		Vec3f nd = point_line_vector(points[2], p0, p1);

		shadow.appendVertex(p0 - vd);
		shadow.appendVertex(p1 + vd);
		shadow.appendVertex(p1 + vd + nd);
		shadow.appendVertex(p0 - vd + nd);

		shadow.appendTexCoord(Vec2f(0.03, 1.0));
		shadow.appendTexCoord(Vec2f(0.03, 0.0));
		shadow.appendTexCoord(Vec2f(.97, 0.0));
		shadow.appendTexCoord(Vec2f(.97, 1.0));

		shadow.appendTriangle(0, 1, 2);
		shadow.appendTriangle(0, 2, 3);
		gl::draw(shadow);
		page_shadow.unbind();
	}

	// page below flipped page
	page.clear();
	n = points.size();
	if (n == 3)
	{
		page.appendVertex(points[0]);
		page.appendVertex(Vec3f(0, pdim.y, 0));
		page.appendVertex(points[1]);

		page.appendTexCoord(tex_coord_func(flipx(uvs[0])));
		page.appendTexCoord(tex_coord_func(Vec2f(0, 1)));
		page.appendTexCoord(tex_coord_func(flipx(uvs[1])));

		page.appendTriangle(0, 1, 2);
	}
	else
	{
		page.appendVertex(Vec3f(0, 0, 0));
		page.appendVertex(Vec3f(0, pdim.y, 0));
		page.appendVertex(points[1]);
		page.appendVertex(points[0]);

		page.appendTexCoord(tex_coord_func(Vec2f(0, 0)));
		page.appendTexCoord(tex_coord_func(Vec2f(0, 1)));
		page.appendTexCoord(tex_coord_func(flipx(uvs[1])));
		page.appendTexCoord(tex_coord_func(flipx(uvs[0])));

		page.appendTriangle(0, 1, 2);
		page.appendTriangle(2, 3, 0);
	}

	if ((page_2_i >= 0) && (page_2_i < static_cast<int>(book.size())))
	{
		book[page_2_i].bind();
		gl::draw(page);
		book[page_2_i].unbind();

		// shadow
		shadow.clear();
		vector<Vec3f> pts = page.getVertices();
		for (unsigned i = 0; i < n; i++)
		{
			shadow.appendVertex(pts[i]);
		}
		vector<uint32_t> indices = page.getIndices();
		for (unsigned i = 0; i < indices.size(); i += 3)
		{
			shadow.appendTriangle(indices[i], indices[i + 1], indices[i + 2]);
		}

		if (n == 3)
		{
			shadow.appendTexCoord(Vec2f(1, 1));
			shadow.appendTexCoord(Vec2f(0, project_pnt_scale(pts[1], pts[0], pts[2])));
			shadow.appendTexCoord(Vec2f(1, 0));
		}
		else
		{
			float s2 = project_pnt_scale(pts[1], pts[3], pts[2]);
			float s3 = project_pnt_scale(pts[0], pts[3], pts[2]);
			float d2 = point_line_distance(pts[1], pts[3], pts[2]);
			float d3 = point_line_distance(pts[0], pts[3], pts[2]);
			if (d2 > d3)
			{
				shadow.appendTexCoord(Vec2f(1 - d3 / d2, 1));
				shadow.appendTexCoord(Vec2f(0, s2));
				shadow.appendTexCoord(Vec2f(1, 0));
				shadow.appendTexCoord(Vec2f(1, 1 + s3));
			}
			else
			{
				shadow.appendTexCoord(Vec2f(0, s3));
				shadow.appendTexCoord(Vec2f(1 - d2 / d3, 0));
				shadow.appendTexCoord(Vec2f(1, 1 - s2));
				shadow.appendTexCoord(Vec2f(1, 1));
			}
		}
		gl::color(ColorA(1, 1, 1, shadow_opac));
		page_1_shadow.bind();
		gl::draw(shadow);
		page_1_shadow.unbind();
		gl::color(Color::white());
	}

	// backside of flipped page
	page.clear();
	n = points.size();
	for (unsigned i = 0; i < n; i++)
	{
		page.appendVertex(points[i]);
		page.appendTexCoord(tex_coord_func(uvs[i]));
	}
	for (unsigned i = 1; i < n - 1; i++)
	{
		page.appendTriangle(i, i + 1, 0);
	}

	if ((page_1_i >= 0) && (page_1_i < static_cast<int>(book.size())))
	{
		book[page_1_i].bind();
		gl::draw(page);
		book[page_1_i].unbind();

		// shadow
		shadow.clear();
		vector<Vec3f> pts = page.getVertices();
		for (unsigned i = 0; i < n; i++)
		{
			shadow.appendVertex(pts[i]);
		}
		vector<uint32_t> indices = page.getIndices();
		for (unsigned i = 0; i < indices.size(); i += 3)
		{
			shadow.appendTriangle(indices[i], indices[i + 1], indices[i + 2]);
		}

		if (n == 3)
		{
			shadow.appendTexCoord(Vec2f(1, 1));
			shadow.appendTexCoord(Vec2f(1, 0));
			shadow.appendTexCoord(Vec2f(0, project_pnt_scale(pts[2], pts[0], pts[1])));
		}
		else
		{
			float s2 = project_pnt_scale(pts[2], pts[0], pts[1]);
			float s3 = project_pnt_scale(pts[3], pts[0], pts[1]);
			float d2 = point_line_distance(pts[2], pts[0], pts[1]);
			float d3 = point_line_distance(pts[3], pts[0], pts[1]);
			if (d2 > d3)
			{
				shadow.appendTexCoord(Vec2f(1, 1 + s3));
				shadow.appendTexCoord(Vec2f(1, 0));
				shadow.appendTexCoord(Vec2f(0, 1 - s2));
				shadow.appendTexCoord(Vec2f(1 - d3 / d2, 1));
			}
			else
			{
				shadow.appendTexCoord(Vec2f(1, 1));
				shadow.appendTexCoord(Vec2f(1, s2 - 1));
				shadow.appendTexCoord(Vec2f(1 - d2 / d3, 0));
				shadow.appendTexCoord(Vec2f(0, s3));
			}
		}
		gl::color(ColorA(1, 1, 1, shadow_opac));
		page0_shadow.bind();
		gl::draw(shadow);
		page0_shadow.unbind();
		gl::color(Color::white());
	}

	gl::popModelView();

#if 0
	gl::pushModelView();
	gl::translate(bounds.getUpperLeft());

	gl::color(Color(1, 1, 1));
	gl::drawSolidCircle(corner_pos, 4);

	Vec2f lpos = limit_corner_location(corner_pos);
	gl::color(ColorA(.1, .1, 1, .2));
	gl::drawSolidCircle(lpos, 6);

	gl::popModelView();
#endif
}

Vec2f PageCurl::limit_corner_location(const Vec2f& p)
{
	const float pw = pdim.x;
	const float ph = pdim.y;

	if ((p.x < 0) && (p.y >= ph)) // below bottom-left corner
	{
		return Vec2f(0, ph);
	}
	else
	if ((p.x >= 2 * pw) && (p.y >= ph)) // below bottom-right corner
	{
		return Vec2f(2 * pw, ph);
	}
	else
	if (p.y < ph) // above bottom line
	{
		Vec2f dv = p - pdim;
		float d = dv.length();
		float dmax = pw;
		if (d < dmax)
		{
			return Vec2f(p);
		}
		else
		{
			return pdim + (dv / d) * dmax;
		}

	}
	else // below bottom line
	{
		Vec2f dv = Vec2f(p.x - pw, p.y);
		float d = dv.length();
		float dmax = pdim.length();
		if (d < dmax)
		{
			return Vec2f(p);
		}
		else
		{
			return Vec2f(pw, 0) + (dv / d) * dmax;
		}
	}
}

Vec2f PageCurl::mirror_point(const Vec2f& p, const Vec2f& p0, const Vec2f& p1)
{
	Vec2f v = p - p0; // vector of p from p0
	Vec2f u = p1 - p0; // line direction
	float s = v.dot(u) / u.lengthSquared();
	Vec2f pp = p0 + s * u; // p projected to line
	return 2 * pp - p;
}


// Projects point p onto line p0, p1
// and returns scaling factor
float PageCurl::project_pnt_scale(const Vec3f& p, const Vec3f& p0, const Vec3f& p1)
{
	Vec3f v = p - p0;
	Vec3f u = p1 - p0;
	return v.dot(u) / u.dot(u);
}

Vec3f PageCurl::point_line_vector(const Vec3f& p, const Vec3f& p0, const Vec3f& p1)
{
	Vec3f u = p1 - p0;
	float s = project_pnt_scale(p, p0, p1);
	Vec3f pp = p0 + s * u; // p projected to line
	return p - pp;
}

float PageCurl::point_line_distance(const Vec3f& p, const Vec3f& p0, const Vec3f& p1)
{
	Vec3f u = p1 - p0;
	float s = project_pnt_scale(p, p0, p1);
	Vec3f pp = p0 + s * u; // p projected to line
	return pp.distance(p);
}

Vec2f PageCurl::page_curl_points(const Vec2f& p, vector<Vec3f>& points, vector<Vec2f>& uvs)
{
	const float pw = pdim.x;
	const float ph = pdim.y;

	Vec2f clp = limit_corner_location(p);

	Vec2f pc(0, ph); // page corner
	Vec2f c((pc + clp) / 2);
	Vec2f v(clp - pc);

	float sqv = v.length();
	float y0 = 0; // bisection intersection at x = 0
	float xpcy = 0; // bisection intersection at y = pc.y
	float x0 = 0; // bisection intersecion at y = 0
	Vec2f n(0, 0); // bisection normal;

	if (v.length() > 0)
	{
		n = Vec2f(v.y / sqv, -v.x / sqv);
		y0 = c.y - n.y * (c.x / n.x);
		xpcy = c.x - n.x * ((c.y - pc.y) / n.y);
		x0 = c.x - n.x * (c.y / n.y);
	}

	if (x0 > 0)
	{
		points.push_back(Vec3f(x0, 0, 0));
		points.push_back(Vec3f(xpcy, pc.y, 0));
		points.push_back(Vec3f(clp));
		points.push_back(Vec3f(mirror_point(Vec2f(0, 0), c, c + n)));

		uvs.push_back(Vec2f(1 - x0 / pw, 0));
		uvs.push_back(Vec2f(1 - xpcy / pw, 1));
		uvs.push_back(Vec2f(1, 1));
		uvs.push_back(Vec2f(1, 0));
	}
	else
	{
		points.push_back(Vec3f(0, y0, 0));
		points.push_back(Vec3f(xpcy, pc.y, 0));
		points.push_back(Vec3f(clp));

		uvs.push_back(Vec2f(1, y0 / ph));
		uvs.push_back(Vec2f(1 - xpcy / pw, 1));
		uvs.push_back(Vec2f(1, 1));
	}

#if 0
	gl::pushModelView();
	gl::translate(bounds.getUpperLeft());

	gl::color(Color(1, 1, 1));
	gl::drawLine(clp, pc);
	gl::drawLine(c, c + 50 * n);
	gl::color(Color(1, 0, 0));
	gl::drawSolidCircle(c, 3);
	gl::color(Color(0, 0, 1));
	gl::drawSolidCircle(Vec2f(0, y0), 3);
	gl::drawSolidCircle(Vec2f(x0, 0), 3);
	gl::drawSolidCircle(Vec2f(xpcy, pc.y), 3);

	gl::popModelView();
#endif
	return clp;
}

