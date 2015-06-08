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

#pragma once

#include <pdal/pdal.hpp>
#include <rialto/GeoPackageCommon.hpp>
#include <rialto/GeoPackageManager.hpp>
#include <rialto/GeoPackageReader.hpp>
#include <rialto/GeoPackageWriter.hpp>
#include "gtest/gtest.h"

namespace rialtotest
{
    using namespace pdal;
    using namespace rialto;


class Support
{
public:
    static std::string temppath(const std::string& file)
    {
        return "./temp/" + file;
    }
    static std::string datapath(const std::string& file)
    {
        return "./../data/" + file;
    }
};

class PDAL_DLL RialtoTest
{
public:
    struct Data {
        double x;
        double y;
        double z;
    };

    // always 8 points
    static Data* sampleDataInit(pdal::PointTable&, pdal::PointViewPtr);

    static Data* randomDataInit(pdal::PointTable&, pdal::PointViewPtr, uint32_t numPoints, bool global=true);

    static void populateDatabase(pdal::PointTable& table,
                               pdal::PointViewPtr view,
                               const std::string& filename,
                               uint32_t maxLevel,
                               const std::string& tableName);

    static void createDatabase(pdal::PointTable& table,
                               pdal::PointViewPtr view,
                               const std::string& filename,
                               uint32_t maxLevel,
                               const std::string& tableName="_unnamed_");

    static void verifyPointToData(pdal::PointViewPtr view, pdal::PointId idx, const Data& data);
    static void verifyPointFromBuffer(std::vector<char> const&,
                                      const Data& expectedData);
    static void verifyPointFromBuffer(const GpkgTile& tileInfo,
                                      const Data& expectedData);
    static void verifyPointsInBounds(pdal::PointViewPtr view,
                                     double minx, double miny, double maxx, double maxy);
    static uint32_t countPointsInBounds(Data* xyz, uint32_t numPoints,
                                        double minx, double miny, double maxx, double maxy);
};


} // namespace rialtotest
