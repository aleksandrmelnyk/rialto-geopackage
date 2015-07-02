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

#include <rialto/GeoPackageCommon.hpp>

#if WITH_LAZPERF
#include <pdal/Compression.hpp>

static const int MIN_LAZ_POINTS = 20; // TODO
#endif

namespace rialto
{


GpkgMatrixSet::GpkgMatrixSet(const std::string& tileTableName,
                             PointLayoutPtr layout,
                             const std::string& datetime,
                             const SpatialReference& srs,
                             uint32_t numColsAtL0,
                             uint32_t numRowsAtL0,
                             const std::string& description,
                             const std::string& lasMetadata,
                             uint32_t maxLevel)
{
    m_name = tileTableName;

    m_datetime = datetime;
    m_description = description;
    m_lasMetadata = lasMetadata;
    
    m_maxLevel = maxLevel;
    m_numDimensions = layout->dims().size();

    m_wkt = srs.getWKT(SpatialReference::eCompoundOK);
    
    m_tmset_min_x = -180.0; // BUG TODO
    m_tmset_min_y = -90.0;
    m_tmset_max_x = 180.0;
    m_tmset_max_y = 90.0;

    m_data_min_x = 0.0; // TODO
    m_data_min_y = 0.0;
    m_data_max_x = 0.0;
    m_data_max_y = 0.0;

    m_numColsAtL0 = numColsAtL0;
    m_numRowsAtL0 = numRowsAtL0;
    
    GpkgDimension::importVector(layout, m_dimensions);
}


void GpkgMatrixSet::set(const std::string& datetime,
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
                       const std::string& lasMetadata)
{
    m_datetime = datetime;
    m_name = name;
    m_maxLevel = maxLevel;
    m_numDimensions = numDimensions;
    m_wkt = wkt;
    m_data_min_x = data_min_x;
    m_data_min_y = data_min_y;
    m_data_max_x = data_max_x;
    m_data_max_y = data_max_y;
    m_tmset_min_x = tmset_min_x;
    m_tmset_min_y = tmset_min_y;
    m_tmset_max_x = tmset_max_x;
    m_tmset_max_y = tmset_max_y;
    m_numColsAtL0 = numColsAtL0;
    m_numRowsAtL0 = numRowsAtL0;
    m_description = description;
    m_lasMetadata = lasMetadata;
}


uint32_t GpkgMatrixSet::getBytesPerPoint() const
{
    uint32_t numBytes = 0;
    for (auto dim: m_dimensions)
    {
        numBytes += dim.getNumBytes();
    }
    return numBytes;
}


GpkgDimension::GpkgDimension(const std::string& name,
                             uint32_t position,
                             const std::string& dataType,
                             const std::string& description,
                             double minimum,
                             double mean,
                             double maximum) :
    m_name(name),
    m_position(position),
    m_dataType(dataType),
    m_description(description),
    m_minimum(minimum),
    m_mean(mean),
    m_maximum(maximum)
{ }       


void GpkgDimension::importVector(PointLayoutPtr layout,
                                 std::vector<GpkgDimension>& infoList)
{
    const uint32_t numDims = layout->dims().size();

    infoList.clear();

    //log()->get(LogLevel::Debug1) << "num dims: " << infoList.size() << std::endl;

    size_t i = 0;
    for (const auto& dim : layout->dims())
    {
        const std::string name = Dimension::name(dim);
        const std::string description = Dimension::description(dim);
        const std::string& dataTypeName = Dimension::interpretationName(layout->dimType(dim));

        GpkgDimension info(name, i, dataTypeName, description, 0.0, 0.0, 0.0);
        infoList.push_back(info);
        
        ++i;
    }
}


uint32_t GpkgDimension::getNumBytes() const
{
    const Dimension::Type::Enum typeId = Dimension::type(getDataType());
    return Dimension::size(typeId);
}
  

GpkgTile::GpkgTile(PointView* view,
                   uint32_t level, uint32_t column, uint32_t row, uint32_t mask) :
    m_level(level),
    m_column(column),
    m_row(row),
    m_numPoints(0),
    m_mask(mask)
{
    m_blob.clear();

    if (view)
    {
        m_numPoints = view->size();
        importFromPV(*view, m_blob);
    }
}


void GpkgTile::set(uint32_t level,
                   uint32_t column,
                   uint32_t row,
                   uint32_t numPoints,
                   uint32_t mask,
                   const std::vector<char>& blob)
{
    m_level = level;
    m_column = column;
    m_row = row;
    m_numPoints = numPoints;
    m_mask = mask;
    m_blob = blob;
}


void GpkgTile::importFromPV(const PointView& view,
                         std::vector<char>& dest)
{
    const uint32_t pointSize = view.pointSize();
    const uint32_t numPoints = view.size();
    const uint32_t bufLen = pointSize * numPoints;

    dest.resize(bufLen);

    char* p = dest.data();
    const DimTypeList& dtl = view.dimTypes();

    for (size_t i=0; i<numPoints; ++i)
    {
        view.getPackedPoint(dtl, i, p);
        p += pointSize;
    }
    
#if WITH_LAZPERF
    if (numPoints > MIN_LAZ_POINTS)
    {
        std::vector<unsigned char> tmp;
        compressPatch(view, dest, tmp);
        std::vector<unsigned char>& r = tmp;
        dest = (std::vector<char>&)tmp;
    }
#endif
}


// does an append to the PV (does not start at index 0)
void GpkgTile::exportToPV(size_t numPoints, PointViewPtr view,
                          const std::vector<char>& src)
{
#if WITH_LAZPERF
    if (numPoints > MIN_LAZ_POINTS)
    {
        std::vector<char> tmp;
        const std::vector<char>& mv1 = src;
        const std::vector<unsigned char>& mv2 = (const std::vector<unsigned char>&)mv1;
        decompressPatch(numPoints, view, mv2, tmp);
        *(std::vector<char>*)&src = tmp;
    }
#endif
  
    PointId idx = view->size();
    const uint32_t pointSize = view->pointSize();

    const char* p = src.data();
    const DimTypeList& dtl = view->dimTypes();

    for (size_t i=0; i<numPoints; ++i)
    {
        view->setPackedPoint(dtl, idx, p);
        p += pointSize;
        ++idx;
    }
}


#if WITH_LAZPERF
void GpkgTile::compressPatch(const PointView& view,
                          const std::vector<char>& inBuf,
                          std::vector<unsigned char>& outBuf)
{
    outBuf.clear();
    
    const uint32_t pointSize = view.pointSize();
    const uint32_t numPoints = view.size();
    const uint32_t buflen = pointSize * numPoints;

    const char* p = inBuf.data();
    const DimTypeList& dtl = view.dimTypes();

    LazPerfBuf b(outBuf);  // unsigned

    LazPerfCompressor<LazPerfBuf> compressor(b, dtl);
    std::vector<char> tmpbuf(compressor.pointSize());
    assert(compressor.pointSize() == pointSize);/***/
    for (size_t idx=0; idx<numPoints; ++idx)
    {
        compressor.compress(p, compressor.pointSize()); // signed
        p += pointSize;
    }
    compressor.done();
}


void GpkgTile::decompressPatch(size_t numPoints, PointViewPtr view,
                            const std::vector<unsigned char>& inBuf,
                            std::vector<char>& outBuf)
{
    outBuf.clear();
    
    const uint32_t pointSize = view->pointSize();
    const DimTypeList& dtl = view->dimTypes();

    // need to remove const, to satisfy LazPerfBuf ctor
    LazPerfBuf b2((std::vector<unsigned char>&)inBuf);  // unsigned

    LazPerfDecompressor<LazPerfBuf> decompressor(b2, dtl);
    assert(pointSize == decompressor.pointSize());
    size_t outbufSize = decompressor.pointSize() * numPoints;
    outBuf.resize(outbufSize);
    char* q = outBuf.data();
    decompressor.decompress(q, outbufSize); // signed
}
#endif

} // namespace rialto
