#include <iostream>
#include <string>
#include <functional>

#include <mapnik/map.hpp>
#include <mapnik/load_map.hpp>
#include <mapnik/agg_renderer.hpp>
#include <mapnik/svg/svg_renderer_agg.hpp>
#include <mapnik/cairo/cairo_renderer.hpp>
#include <mapnik/image.hpp>
#include <mapnik/image_util.hpp>
#include <mapnik/datasource_cache.hpp>
#include <mapnik/projection.hpp>

#include <cairo.h>
#include <cairo-svg.h>

#include "argparse.hpp"

#define MAPNIK_PREFIX "/usr/local"
#define PROGRAM_NAME "minemapnik"

void projforw(const mapnik::projection &proj, double x, double y, double *u, double *v)
{
    proj.forward(x, y);
    if (u != nullptr) {
        *u = x;
    }
    if (v != nullptr) {
        *v = y;
    }
}

/*
    Conversion of OSM URL parameters into approximate rectangle expressed by
    mapnik 2D box.
*/
/*
mapnik::box2d<double> *osmurl2box(double lati, double longi, unsigned zoom, unsigned pixwidth, unsigned pixheight, mapnik::projection *proj)
{
    double halfwidth, halfheight, canvassize= 180.0, dummy= 0.0;
    unsigned tilesize= 256;

    proj->forward(canvassize, dummy);
    canvassize *= 2;
    // half window size in projection coordinates:
    halfwidth= canvassize / (1 << zoom) / tilesize * pixwidth / 2;
    halfheight= canvassize / (1 << zoom) / tilesize * pixheight / 2;
    proj->forward(longi, lati);
    return new mapnik::box2d<double>(longi - halfwidth, lati - halfheight, longi + halfwidth, lati + halfheight);
}
*/

// Compute a rectangle in projected coordinates that contain all
// corners of the given latitude/longitude rectangle:
mapnik::box2d<double> coords_to_box(const mapnik::projection &proj, double lat0, double lon0, double lat1, double lon1) {
    double x[4], y[4], x0, y0, x1, y1;
    projforw(proj, lon0, lat0, &x[0], &y[0]);
    projforw(proj, lon1, lat0, &x[1], &y[1]);
    projforw(proj, lon1, lat1, &x[2], &y[2]);
    projforw(proj, lon0, lat1, &x[3], &y[3]);
    x0 = x1 = x[0];
    y0 = y1 = y[0];
    for (int i= 1; i < 4; ++i) {
        if (x[i] < x0) {
            x0 = x[i];
        }
        if (y[i] < y0) {
            y0 = y[i];
        }
        if (x[i] > x1) {
            x1 = x[i];
        }
        if (y[i] > y1) {
            y1 = y[i];
        }
    }

    return mapnik::box2d<double>(x0, y0, x1, y1);
}

int main(int argc, char **argv)
{
    constexpr auto stoi_fun = [](const std::string& value) { return std::stoi(value); };
    constexpr auto stod_fun = [](const std::string& value) { return std::stod(value); };
    argparse::ArgumentParser argparse(PROGRAM_NAME);
    argparse.add_argument("width")
            .help("width in px (raster) points (vector)")
            .default_value(2048)
            .action(stoi_fun);
    argparse.add_argument("height")
            .help("height in px (raster) points (vector)")
            .default_value(2048)
            .action(stoi_fun);
    argparse.add_argument("lat0")
            .help("min/max latitude")
            .default_value(25.83 /*26.67*/)
            .action(stod_fun);
    argparse.add_argument("lon0")
            .help("min/max longitude")
            .default_value(-80.22 /*-81.90*/)
            .action(stod_fun);
    argparse.add_argument("lat1")
            .help("other latitude")
            .default_value(25.75 /*26.58*/)
            .action(stod_fun);
    argparse.add_argument("lon1")
            .help("other longitude")
            .default_value(-80.11 /*-81.80*/)
            .action(stod_fun);
    try {
        argparse.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << argparse;
        return 0;
    }

    auto width = argparse.get<int>("width");
    auto height = argparse.get<int>("height");
    auto lat0 = argparse.get<double>("lat0");
    auto lon0 = argparse.get<double>("lon0");
    auto lat1 = argparse.get<double>("lat1");
    auto lon1 = argparse.get<double>("lon1");

    mapnik::datasource_cache::instance().register_datasources(MAPNIK_PREFIX "/lib/mapnik/input/");
    mapnik::freetype_engine::register_fonts(MAPNIK_PREFIX "/lib/mapnik/fonts");
    mapnik::freetype_engine::register_fonts("/usr/share/fonts");
    mapnik::freetype_engine::register_fonts("/usr/local/share/fonts");

    mapnik::Map m(width, height, "+init=epsg:3857");
    mapnik::load_map(m, "/home/paulyc/development/openstreetmap-carto/project.xml");
    mapnik::projection proj(mapnik::projection(m.srs().c_str()));

    m.zoom_to_box(coords_to_box(proj, lat0, lon0, lat1, lon1));
#ifdef RENDER_RASTER
    mapnik::image_rgba8 im(width, height);
    mapnik::agg_renderer<mapnik::image_rgba8> render(m, im);
    render.apply();
    mapnik::save_to_file(im, "image.png");
#endif
    mapnik::cairo_surface_ptr surface =
        mapnik::cairo_surface_ptr(cairo_svg_surface_create("output.svg", width, height),
        mapnik::cairo_surface_closer());
    mapnik::cairo_ptr image_context(mapnik::create_context(surface));
    mapnik::cairo_renderer<mapnik::cairo_ptr> svg_render(m, image_context, 1.0);
    svg_render.apply();

    return 0;
}
