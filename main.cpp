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

#include "cxxopts.hpp"

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
    cxxopts::Options options(argv[0], " - example command line options");
        options
          .positional_help("[optional args]")
          .show_positional_help();
        std::string map_xml;
        int width;
        int height;
        double lat0, lon0, lat1, lon1;
        bool raster, cairosvg;
        options
              .allow_unrecognised_options()
              .add_options()
              ("x,xml", "XML mapnik file", cxxopts::value<std::string>(map_xml)->default_value("openstreetmap-carto/project.xml"))
              ("raster", "raster renderer", cxxopts::value<bool>(raster)->default_value("false"))
              ("vector,cairo,svg", "cairo svg renderer", cxxopts::value<bool>(cairosvg)->default_value("true"))
              ("help", "Print help")
              ("w,width", "width in px (raster) points (vector)", cxxopts::value<int>(width)->default_value("2048"))
              ("h,height", "height in px (raster) points (vector)", cxxopts::value<int>(height)->default_value("2048"))
              ("lat0", "min/max latitude", cxxopts::value<double>(lat0)->default_value("25.83"))
              ("lon0", "min/max longitude", cxxopts::value<double>(lon0)->default_value("-80.22"))
              ("lat1", "other min/max latitude", cxxopts::value<double>(lat1)->default_value("25.75"))
              ("lon1", "other min/max longitude", cxxopts::value<double>(lon1)->default_value("-80.11"))
              //("vector", "A list of doubles", cxxopts::value<std::vector<double>>())
              //("option_that_is_too_long_for_the_help", "A very long option")
             ;

    auto result = options.parse(argc, argv);

    mapnik::datasource_cache::instance().register_datasources(MAPNIK_PREFIX "/lib/mapnik/input/");
    mapnik::freetype_engine::register_fonts(MAPNIK_PREFIX "/lib/mapnik/fonts", true);
    mapnik::freetype_engine::register_fonts("/usr/share/fonts", false);
    mapnik::freetype_engine::register_fonts("/usr/local/share/fonts", false);

    mapnik::Map m(width, height, "+init=epsg:3857");
    mapnik::load_map(m, map_xml);
    mapnik::projection proj(mapnik::projection(m.srs().c_str()));

    m.zoom_to_box(coords_to_box(proj, lat0, lon0, lat1, lon1));
    if (raster) {
        mapnik::image_rgba8 im(width, height);
        mapnik::agg_renderer<mapnik::image_rgba8> render(m, im);
        render.apply();
        mapnik::save_to_file(im, "image.png");
    }
    if (cairosvg) {
        mapnik::cairo_surface_ptr surface =
            mapnik::cairo_surface_ptr(cairo_svg_surface_create("output.svg", width, height),
            mapnik::cairo_surface_closer());
        mapnik::cairo_ptr image_context(mapnik::create_context(surface));
        mapnik::cairo_renderer<mapnik::cairo_ptr> svg_render(m, image_context, 1.0);
        svg_render.apply();
    }

    return 0;
}
