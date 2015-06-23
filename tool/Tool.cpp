/******************************************************************************
* Copyright (c) 2015, RadiantBlue Technologies, Inc.
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following
* conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in
*       the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of Hobu, Inc. or Flaxen Geo Consulting nor the
*       names of its contributors may be used to endorse or promote
*       products derived from this software without specific prior
*       written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
* OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
****************************************************************************/


#include "Tool.hpp"

#include <rialto/GeoPackageCommon.hpp>
#include <rialto/GeoPackageManager.hpp>
#include <rialto/RialtoReader.hpp>
#include <rialto/RialtoWriter.hpp>
#include <pdal/NullWriter.hpp>
#include <pdal/FauxReader.hpp>
#include <pdal/LasReader.hpp>
#include <pdal/LasWriter.hpp>
#include <pdal/ReprojectionFilter.hpp>

using namespace pdal;
using namespace rialto;



Tool::Tool()
{
    Utils::random_seed(17);
}


Tool::~Tool()
{}

    
void Tool::error(const char* p, const char* q)
{
    fprintf(stderr, "ERROR: %s", p);
    if (q)
    {
        fprintf(stderr, ": %s", q);
    }
    fprintf(stderr, "\n");
    exit(1);
}


bool Tool::streq(const char* p, const char* q)
{
    return (strcmp(p,q)==0);
}


std::string Tool::toString(FileType type)
{
    switch (type)
    {
        case TypeLas:       return "las";
        case TypeLaz:       return "laz";
        case TypeRialto:    return "geopackage";
        default: break;
    }
    return "INVALID";
}


Tool::FileType Tool::inferType(const std::string& p)
{
    const char* ext = strrchr(p.c_str(), '.');
    if (streq(ext, ".las")) return TypeLas;
    if (streq(ext, ".laz")) return TypeLaz;
    if (streq(ext, ".gpkg")) return TypeRialto;

    return TypeInvalid;
}


Stage* Tool::createReprojector()
{
    Stage* filter = new ReprojectionFilter();
    
    const SpatialReference srs("EPSG:4326");
    Option opt("out_srs", srs.getWKT());
    
    Options options;
    options.add(opt);

    filter->setOptions(options);

    return filter;
}


Stage* Tool::createReader(const std::string& name, FileType type)
{
    Options opts;
    Reader* reader = NULL;

    switch (type)
    {
        case TypeLas:
            opts.add("filename", name);
            reader = new LasReader();
            break;
        case TypeLaz:
            opts.add("filename", name);
            reader = new LasReader();
            break;
        case TypeRialto:
            opts.add("filename", name);
            reader = new rialto::RialtoReader();
            break;
        default:
            assert(0);
            break;
    }

    reader->setOptions(opts);
    return reader;
}


Stage* Tool::createWriter(const std::string& name, FileType type, uint32_t maxLevel)
{
    FileUtils::deleteFile(name);

    Options opts;
    Writer* writer = NULL;

    switch (type)
    {
        case TypeLas:
            opts.add("filename", name);
            writer = new LasWriter();
            opts.add("compressed", false);
            break;
        case TypeLaz:
            opts.add("filename", name);
            writer = new LasWriter();
            opts.add("compressed", true);
            break;
        case TypeRialto:
            {
                LogPtr log(new Log("rialtowritertest", "stdout"));
                GeoPackageManager db(name, log);
                db.open();
                db.close();
            }
            opts.add("filename", name);
            opts.add("name", "mytablename");
            opts.add("numColsAtL0", 2);
            opts.add("numRowsAtL0", 1);
            opts.add("timestamp", "mytimestamp");
            opts.add("description", "mydescription");
            opts.add("maxLevel", maxLevel);
            opts.add("tms_minx", -180.0);
            opts.add("tms_miny", -90.0);
            opts.add("tms_maxx", 180.0);
            opts.add("tms_maxy", 90.0);
            writer = new rialto::RialtoWriter();
            break;
        default:
            assert(0);
            break;
    }

    writer->setOptions(opts);
    return writer;
}


static void verifyPoints(PointViewPtr viewA, PointViewPtr viewE)
{
    for (uint32_t i=0; i<viewA->size(); i++)
    {
        const double xA = viewA->getFieldAs<double>(Dimension::Id::X, i);
        const double yA = viewA->getFieldAs<double>(Dimension::Id::Y, i);
        const double zA = viewA->getFieldAs<double>(Dimension::Id::Z, i);

        const double xE = viewE->getFieldAs<double>(Dimension::Id::X, i);
        const double yE = viewE->getFieldAs<double>(Dimension::Id::Y, i);
        const double zE = viewE->getFieldAs<double>(Dimension::Id::Z, i);

        if (xA != xE || yA != yE || zA != zE)
        {
          char buf[1024];
          sprintf(buf, "%i:\n\txA=%f\txE=%f\n\tyA=%f\tyE=%f\n\tzA=%f\tzE=%f\n",
              i, xA, xE, yA, yE, zA, zE);
          Tool::error("verify failed", buf);
        }
    }
}


void Tool::verify(Stage* readerExpected, Stage* readerActual)
{
    BOX3D unusedBox;
    uint32_t unusedCount;

//    Stage* readerExpected = createReader(m_inputName, m_inputType, m_rBounds, m_rCount);
//    Stage* readerActual = createReader(m_outputName, m_outputType, unusedBox, unusedCount);

    PointViewSet viewsActual;
    PointViewPtr viewActual;
    PointViewSet viewsExpected;
    PointViewPtr viewExpected;

    pdal::PointTable tableActual;
    readerActual->prepare(tableActual);
    viewsActual = readerActual->execute(tableActual);

    pdal::PointTable tableExpected;
    readerExpected->prepare(tableExpected);
    viewsExpected = readerExpected->execute(tableExpected);

    if (viewsActual.size() != viewsExpected.size() || viewsActual.size() != 1)
    {
        error("verify failed", "unequal view set sizes");
    }

    viewActual = *(viewsActual.begin());
    viewExpected = *(viewsExpected.begin());

    // if the sizes are equal, we'll assume a one-way compare is okay
    if (viewActual->size() != viewExpected->size())
    {
        error("verify failed", "unequal view sizes");
    }

    verifyPoints(viewActual, viewExpected);
    printf("verify passed\n");
}
