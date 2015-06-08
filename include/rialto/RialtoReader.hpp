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

#include <pdal/Reader.hpp>


namespace rialto
{
    using namespace pdal;

class GpkgTile;
class GpkgMatrixSet;
class GeoPackageReader;
class TileMath;

class PDAL_DLL RialtoReader : public Reader
{
public:
    RialtoReader();
    ~RialtoReader();

    static void * create();
    static int32_t destroy(void *);
    std::string getName() const;

    Options getDefaultOptions();

    const GpkgMatrixSet& getMatrixSet() const { return *m_matrixSet; }
    
private:
    void processOptions(const Options& options);
    void initialize();
    void addDimensions(PointLayoutPtr layout);
    void ready(PointTableRef table);
    
    point_count_t read(PointViewPtr view, point_count_t /*not used*/);
    void doQuery(const TileMath&, const GpkgTile&, PointViewPtr,
                 double qMinX, double qMinY, double qMaxX, double qMaxY);
    void setQueryParams();

    GeoPackageReader* m_gpkg;
    std::unique_ptr<GpkgMatrixSet> m_matrixSet;
    std::string m_matrixSetName;

    uint32_t m_queryLevel;
    BOX3D m_queryBox;

    RialtoReader& operator=(const RialtoReader&); // not implemented
    RialtoReader(const RialtoReader&); // not implemented
};

} // rialto namespace
