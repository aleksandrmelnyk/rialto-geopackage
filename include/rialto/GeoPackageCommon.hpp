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

namespace rialto
{
    using namespace pdal;


class GpkgDimension
{
public:
    GpkgDimension(const std::string& name,
                  uint32_t position,
                  const std::string& dataType,
                  const std::string& description,
                  double minimum,
                  double mean,
                  double maximum);

    static void importVector(PointLayoutPtr layout,
                             std::vector<GpkgDimension>& infoList);

    const std::string& getName() const { return m_name; }
    uint32_t getPosition() const { return m_position; }
    const std::string& getDataType() const { return m_dataType; };
    const std::string& getDescription() const { return m_description; }
    double getMinimum() const { return m_minimum; }
    double getMean() const { return m_mean; }
    double getMaximum() const { return m_maximum; }

    // helpers
    uint32_t getNumBytes() const;

private:
    std::string m_name;
    uint32_t m_position;
    std::string m_dataType;
    std::string m_description;
    double m_minimum;
    double m_mean;
    double m_maximum;
};


class GpkgMatrixSet
{
public:
    GpkgMatrixSet() {}

    GpkgMatrixSet(const std::string& tileTableName,
                PointLayoutPtr layout,
                const std::string& datetime,
                const SpatialReference& srs,
                uint32_t numColsAtL0,
                uint32_t numRowsAtL0,
                const std::string& description,
                const std::string& lasMetadata,
                uint32_t maxLevel);

    void set(const std::string& datetime,
             const std::string& name,
             uint32_t maxLevel,
             uint32_t numDimensions,
             const std::string& wkt,
             double data_min_x,
             double data_min_y,
             double data_max_x,
             double data_max_y,
             double tmset_min_x,
             double tmset_min_y,
             double tmset_max_x,
             double tmset_max_y,
             uint32_t numColsAtL0,
             uint32_t numRowsAtL0,
             const std::string& description,
             const std::string& lasMetadata);

    std::string getDateTime() const { return m_datetime; }
    std::string getName() const { return m_name; } // aka filename
    uint32_t getMaxLevel() const { return m_maxLevel; }
    uint32_t getNumDimensions() const { return m_numDimensions; }
    const std::vector<GpkgDimension>& getDimensions() const { return m_dimensions; };
    std::vector<GpkgDimension>& getDimensionsRef() { return m_dimensions; };
    const std::string getWkt() const { return m_wkt; }
    const std::string getDescription() const { return m_description; }
    const std::string getLasMetadata() const { return m_lasMetadata; }

    double getDataMinX() const { return m_data_min_x; } // data extents
    double getDataMinY() const { return m_data_min_y; }
    double getDataMaxX() const { return m_data_max_x; }
    double getDataMaxY() const { return m_data_max_y; }

    double getTmsetMinX() const { return m_tmset_min_x; }
    double getTmsetMinY() const { return m_tmset_min_y; }
    double getTmsetMaxX() const { return m_tmset_max_x; }
    double getTmsetMaxY() const { return m_tmset_max_y; }

    uint32_t getNumColsAtL0() const { return m_numColsAtL0; }
    uint32_t getNumRowsAtL0() const { return m_numRowsAtL0; }

    // helpers
    uint32_t getBytesPerPoint() const; // helper method

private:
    std::string m_datetime;
    std::string m_name; // aka filename
    uint32_t m_maxLevel;
    uint32_t m_numDimensions;
    std::vector<GpkgDimension> m_dimensions;
    std::string m_wkt; // the srs
    double m_data_min_x; // data extents
    double m_data_min_y;
    double m_data_max_x;
    double m_data_max_y;
    double m_tmset_min_x; // tile extents
    double m_tmset_min_y;
    double m_tmset_max_x;
    double m_tmset_max_y;
    uint32_t m_numColsAtL0;
    uint32_t m_numRowsAtL0;
    std::string m_description;
    std::string m_lasMetadata;
};


class GpkgTile
{
public:
    GpkgTile() {}

    GpkgTile(PointView* view, uint32_t level, uint32_t column, uint32_t row, uint32_t mask);

    void set(uint32_t level,
            uint32_t column,
            uint32_t row,
            uint32_t numPoints,
            uint32_t mask,
            const std::vector<char>& blob);

    uint32_t getLevel() const { return m_level; }
    uint32_t getColumn() const { return m_column; }
    uint32_t getRow() const { return m_row; }
    uint32_t getNumPoints() const { return m_numPoints; }
    uint32_t getMask() const { return m_mask; }

    const std::vector<char>& getBlob() const { return m_blob; }
    
    // does an append to the PV (does not start at index 0)
    static void exportToPV(size_t numPoints, PointViewPtr view,
                           const std::vector<char>& src);

private:
    static void importFromPV(const PointView& view,
                             std::vector<char>& dest);
    static void compressPatch(const PointView& view,
                              const std::vector<char>& inBuf,
                              std::vector<unsigned char>& outBuf);
    static void decompressPatch(size_t numPoints, PointViewPtr view,
                                const std::vector<unsigned char>& inBuf,
                                std::vector<char>& outBuf);

    uint32_t m_level;
    uint32_t m_column;
    uint32_t m_row;
    uint32_t m_numPoints;
    uint32_t m_mask;
    std::vector<char> m_blob;
};


} // namespace rialto
