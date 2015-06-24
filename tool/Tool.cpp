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



Tool::Tool() :
    m_inputType(TypeInvalid)
{
    Utils::random_seed(17);
}


Tool::~Tool()
{}


void Tool::processOptions(int argc, char* argv[])
{
    const bool ok = l_processOptions(argc, argv);
    if (!ok) {
        printUsage();
        error("aborting");
    }
    
    if (m_inputName.empty()) {
        error("input file not specified");
    }
    if (!FileUtils::fileExists(m_inputName)) {
        error("input file does not exist");
    }
    
    m_inputType = inferType(m_inputName);
}

    
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


static bool EQ(double s, double t)
{
    const double delta = abs(t-s) * 0.00001;
    bool ok = (s >= t-delta && s <= t+delta);
    return ok;
}


static void verifyPoints(PointViewPtr viewA, PointViewPtr viewE)
{
    // TODO: yes, this is O(n^2)...
    
    const uint32_t cnt = viewA->size();
    
    double* ax = new double[cnt];
    double* ay = new double[cnt];
    double* az = new double[cnt];
    double* ex = new double[cnt];
    double* ey = new double[cnt];
    double* ez = new double[cnt];

    bool* matched = new bool[cnt];

    HeartBeat hb(cnt);

    for (uint32_t i=0; i<cnt; i++)
    {
        ax[i] = viewA->getFieldAs<double>(Dimension::Id::X, i);
        ay[i] = viewA->getFieldAs<double>(Dimension::Id::Y, i);
        az[i] = viewA->getFieldAs<double>(Dimension::Id::Z, i);

        ex[i] = viewE->getFieldAs<double>(Dimension::Id::X, i);
        ey[i] = viewE->getFieldAs<double>(Dimension::Id::Y, i);
        ez[i] = viewE->getFieldAs<double>(Dimension::Id::Z, i);
        
        matched[i] = false;
    }

    for (uint32_t i=0; i<cnt; i++)
    {
        bool ok = false;
        for (uint32_t j=0; j<cnt; j++)
        {
            if (matched[j]) continue;
            
            if (EQ(ax[i],ex[j]) && EQ(ay[i],ey[j]) && EQ(az[i],ez[j]))
            {
                matched[j] = true; // we've used up this expected value
                ok = true;
                break;
            }
        }
        
        if (!ok)
        {
          printf("verify failed: %i:\n\tax=%f\tay=%f\taz=%f\n",
              i, ax[i], ay[i], az[i]);
        }
        
        hb.beat();
    }

    for (uint32_t i=0; i<cnt; i++)
    {
        assert(matched[i]);
    }
}


void Tool::verify(Stage* readerExpected, Stage* readerActual)
{
    printf("Starting verify...\n");
    
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
