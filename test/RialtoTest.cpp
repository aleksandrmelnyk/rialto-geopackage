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

#include "RialtoTest.hpp"

#include <cstdint>
#include <string>

#include <pdal/BufferReader.hpp>

#include <rialto/GeoPackage.hpp>
#include <rialto/GeoPackageCommon.hpp>
#include <rialto/GeoPackageManager.hpp>
#include <rialto/GeoPackageReader.hpp>
#include <rialto/GeoPackageWriter.hpp>
#include <rialto/RialtoWriter.hpp>

#include "gtest/gtest.h"

namespace pdal
{
namespace rialto
{


RialtoTest::Data* RialtoTest::sampleDataInit(pdal::PointTable& table, pdal::PointViewPtr view)
{
    Data* data = new Data[8];
    data[0] = { -179.0, 89.0, 0.0};
    data[1] = { -1.0, 89.0, 11.0};
    data[2] = { -179.0, -89.0, 22.0};
    data[3] = { -1.0, -89.0, 33.0};
    data[4] = { 89.0, 1.0, 44.0};
    data[5] = { 91.0, 1.0, 55.0};
    data[6] = { 89.0, -1.0, 66.0};
    data[7] = { 91.0, -1.0, 77.0};

    table.layout()->registerDim(Dimension::Id::X);
    table.layout()->registerDim(Dimension::Id::Y);
    table.layout()->registerDim(Dimension::Id::Z);

    for (int i=0; i<8; i++)
    {
        view->setField(Dimension::Id::X, i, data[i].x);
        view->setField(Dimension::Id::Y, i, data[i].y);
        view->setField(Dimension::Id::Z, i, data[i].z);
    }

    return data;
}


RialtoTest::Data* RialtoTest::randomDataInit(pdal::PointTable& table, pdal::PointViewPtr view, uint32_t numPoints, bool global)
{
    Utils::random_seed(17);

    Data* data = new Data[numPoints];

    table.layout()->registerDim(Dimension::Id::X);
    table.layout()->registerDim(Dimension::Id::Y);
    table.layout()->registerDim(Dimension::Id::Z);

    double minx = global ? -179.9 : 12.3;
    double miny = global ? -89.9 : 12.4;
    double maxx = global ? 179.9 : 45.6;
    double maxy = global ? 89.9 : 45.7;

    for (uint32_t i=0; i<numPoints; i++)
    {
        data[i].x = Utils::random(minx, maxx);
        data[i].y = Utils::random(miny, maxy);
        data[i].z = i;
    }

    for (uint32_t i=0; i<numPoints; i++)
    {
        view->setField(Dimension::Id::X, i, data[i].x);
        view->setField(Dimension::Id::Y, i, data[i].y);
        view->setField(Dimension::Id::Z, i, data[i].z);
    }

    return data;
}


void RialtoTest::populateDatabase(pdal::PointTable& table,
                                pdal::PointViewPtr view,
                                const std::string& filename,
                                uint32_t maxLevel,
                                const std::string& tableName)
{
    assert(FileUtils::fileExists(filename));

    {
        time_t now;
        time(&now);
        char buf[sizeof("yyyy-mm-ddThh:mm:ss.sssZ")+1];
        // TODO: this produces "ss", not "ss.sss" as the gpkg spec implies is required
        strftime(buf, sizeof(buf), "%FT%TZ", gmtime(&now));
        const std::string timestamp(buf);

        pdal::Options readerOptions;
        pdal::BufferReader reader;
        reader.setOptions(readerOptions);
        reader.addView(view);
        reader.setSpatialReference(SpatialReference("EPSG:4326"));

        pdal::StageFactory f;
        pdal::Options writerOptions;
        writerOptions.add("filename", filename);
        writerOptions.add("name", tableName);
        writerOptions.add("numColsAtL0", 2);
        writerOptions.add("numRowsAtL0", 1);
        writerOptions.add("timestamp", timestamp);
        writerOptions.add("description", "");
        writerOptions.add("maxLevel", maxLevel);
        writerOptions.add("tms_minx", -180.0);
        writerOptions.add("tms_miny", -90.0);
        writerOptions.add("tms_maxx", 180.0);
        writerOptions.add("tms_maxy", 90.0);

        //writerOptions.add("overwrite", true);
        //writerOptions.add("verbose", LogLevel::Debug);
        pdal::Stage* writer = new RialtoWriter();
        writer->setOptions(writerOptions);
        writer->setInput(reader);

        // execution: write to database
        writer->prepare(table);
        PointViewSet outputViews = writer->execute(table);
        delete writer;
    }

    assert(FileUtils::fileExists(filename));
}


void RialtoTest::createDatabase(pdal::PointTable& table,
                                pdal::PointViewPtr view,
                                const std::string& filename,
                                uint32_t maxLevel,
                                const std::string& tableName)
{
    assert(!FileUtils::fileExists(filename));
    {
        LogPtr log(new Log("rialtodbwritertest", "stdout"));
        GeoPackageManager db(filename, log);
        db.open();
        db.close();
    }
    assert(FileUtils::fileExists(filename));

    populateDatabase(table, view, filename, maxLevel, tableName);
}


void RialtoTest::verifyPointToData(pdal::PointViewPtr view, pdal::PointId idx, const RialtoTest::Data& data)
{
    const double x = view->getFieldAs<double>(pdal::Dimension::Id::X, idx);
    const double y = view->getFieldAs<double>(pdal::Dimension::Id::Y, idx);
    const double z = view->getFieldAs<double>(pdal::Dimension::Id::Z, idx);

    EXPECT_FLOAT_EQ(x, data.x);
    EXPECT_FLOAT_EQ(y, data.y);
    EXPECT_FLOAT_EQ(z, data.z);
}


void RialtoTest::verifyPointFromBuffer(std::vector<char> const& vec,
                                       const RialtoTest::Data& expectedData)
{
    const char* p = vec.data();

    union U {
      char buf[24];
      struct S {
        double x;
        double y;
        double z;
      } s;
    } u;

    for (int i=0; i<24; i++)
      u.buf[i] = vec[i];

    EXPECT_EQ(u.s.x, expectedData.x);
    EXPECT_EQ(u.s.y, expectedData.y);
    EXPECT_EQ(u.s.z, expectedData.z);
}


void RialtoTest::verifyPointFromBuffer(const GpkgTile& tileInfo,
                                       const RialtoTest::Data& expectedData)
{
    std::vector<char> const& vec = tileInfo.getBlob();
    verifyPointFromBuffer(vec, expectedData);
}


void RialtoTest::verifyPointsInBounds(PointViewPtr view,
                                      double minx, double miny, double maxx, double maxy)
{
    for (uint32_t i=0; i<view->size(); i++)
    {
        const double x = view->getFieldAs<double>(Dimension::Id::X, i);
        const double y = view->getFieldAs<double>(Dimension::Id::Y, i);

        if (!(x >= minx && x <= maxx))
        {
            printf("query x:  min=%lf  max=%lf  x=%lf\n", minx, maxx, x);
            printf("query y:  min=%lf  max=%lf  y=%lf\n", miny, maxy, y);
        }
        if (!(y >= miny && y <= maxy))
        {
            printf("query y:  min=%lf  max=%lf  y=%lf\n", miny, maxy, y);
        }
        EXPECT_TRUE(x >= minx && x <= maxx);
        EXPECT_TRUE(y >= miny && y <= maxy);
    }
}

uint32_t RialtoTest::countPointsInBounds(Data* xyz, uint32_t numPoints,
                   double minx, double miny, double maxx, double maxy)
{
    uint32_t cnt = 0;

    for (uint32_t i=0; i<numPoints; i++)
    {
        if (xyz[i].x >= minx && xyz[i].x <= maxx && xyz[i].y >= miny && xyz[i].y <= maxy)
        {
            cnt++;
        }
    }

    return cnt;
}


} // namespace rialto
} // namespace pdal
