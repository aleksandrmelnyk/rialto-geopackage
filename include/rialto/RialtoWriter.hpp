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

#include <pdal/Writer.hpp>
#include <rialto/ViewTileCommon.hpp>


extern "C" int32_t RialtoWriter_ExitFunc();
extern "C" PF_ExitFunc RialtoWriter_InitPlugin();


namespace pdal
{
namespace rialto
{
    class GeoPackageWriter;
    

class PDAL_DLL RialtoWriter : public Writer
{
public:
    RialtoWriter()
    {}

    static void * create();
    static int32_t destroy(void *);
    std::string getName() const;

    Options getDefaultOptions();

    void ready(PointTableRef table);
    void write(const PointViewPtr viewPtr);
    void done(PointTableRef table);

private:
    void startWrite(PointTableRef table, const SpatialReference& srs);
    void writeAllTiles(ViewTileSet& viewTileSet);
    void writeTile(PointView*,
                   uint32_t level, uint32_t col, uint32_t row,
                   uint32_t mask);
    void initStats(PointLayoutPtr layout);
    void collectStats(PointView* pv);
    void updateDimensionStats(PointLayoutPtr layout);
    void processOptions(const Options& options);

    std::string m_matrixSetName;
    uint32_t m_numColsAtL0;
    uint32_t m_numRowsAtL0;
    std::string m_description;
    std::string m_timestamp;
    GeoPackageWriter* m_gpkg;
    uint32_t m_maxLevel;
    double m_tms_minx, m_tms_miny, m_tms_maxx, m_tms_maxy;
    
    std::map<uint32_t,double> m_mins;
    std::map<uint32_t,double> m_means;
    std::map<uint32_t,double> m_maxes;
    uint32_t m_numPoints;

    RialtoWriter& operator=(const RialtoWriter&); // not implemented
    RialtoWriter(const RialtoWriter&); // not implemented
};

} // namespace rialto
} // namespace pdal
