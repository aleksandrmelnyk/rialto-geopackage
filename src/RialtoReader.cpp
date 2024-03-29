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

#include <rialto/RialtoReader.hpp>
#include <rialto/GeoPackageReader.hpp>
#include <rialto/GeoPackageCommon.hpp>
#include "WritableTileCommon.hpp"
#include "TileMath.hpp"

#include <boost/filesystem.hpp>


static PluginInfo const s_info = PluginInfo(
    "readers.rialto",
    "Read data from a Rialto DB",
    "" );

//CREATE_SHARED_PLUGIN(1, 0, rialto::RialtoReader, Reader, s_info)

namespace rialto
{


std::string RialtoReader::getName() const { return s_info.name; }


RialtoReader::RialtoReader() :
    Reader(),
    m_gpkg(NULL)
{}


RialtoReader::~RialtoReader()
{
    if (m_gpkg)
    {
      m_gpkg->close();
        delete m_gpkg;
    }
}


void RialtoReader::initialize()
{
    log()->get(LogLevel::Debug) << "RialtoReader::initialize()" << std::endl;

    if (!m_gpkg)
    {
      m_gpkg = new GeoPackageReader(m_filename, log());
      m_gpkg->open();

        m_matrixSet = std::unique_ptr<GpkgMatrixSet>(new GpkgMatrixSet());

        m_gpkg->readMatrixSet(m_dataset, *m_matrixSet);
        
        const SpatialReference srs(m_matrixSet->getWkt());
        setSpatialReference(srs);
    }
}


Options RialtoReader::getDefaultOptions()
{
    log()->get(LogLevel::Debug) << "RialtoReader::getDefaultOptions()" << std::endl;

    Options options;

    return options;
}


void RialtoReader::processOptions(const Options& options)
{
    log()->get(LogLevel::Debug) << "RialtoReader::processOptions()" << std::endl;

    if (!m_gpkg)
    {
        // you can't change the filename or dataset name once we've opened the DB
        m_filename = options.getValueOrThrow<std::string>("filename");
        m_dataset = options.getValueOrDefault<std::string>("dataset", "");
    }

    if (m_dataset == "")
    {
        m_dataset = boost::filesystem::path(m_filename).stem().string() + "_tiles";
    }
    
    m_queryBox = options.getValueOrDefault<BOX3D>("bounds", BOX3D());
    m_queryLevel = options.getValueOrDefault<uint32_t>("level", 0xffff);

    log()->get(LogLevel::Debug) << "process options: bounds=" << m_queryBox << std::endl;
}


void RialtoReader::addDimensions(PointLayoutPtr layout)
{
    log()->get(LogLevel::Debug) << "RialtoReader::addDimensions()" << std::endl;

    m_gpkg->setupLayout(*m_matrixSet, layout);
}


void RialtoReader::ready(PointTableRef table)
{
    log()->get(LogLevel::Debug) << "RialtoReader::ready()" << std::endl;
}


void RialtoReader::setQueryParams()
{
    if (m_queryBox.empty())
    {
        m_queryBox.minx = m_matrixSet->getDataMinX();
        m_queryBox.miny = m_matrixSet->getDataMinY();
        m_queryBox.maxx = m_matrixSet->getDataMaxX();
        m_queryBox.maxy = m_matrixSet->getDataMaxY();
    }
            
    if (m_queryLevel == 0xffff)
    {
        m_queryLevel = m_matrixSet->getMaxLevel();
    }
    else if (m_queryLevel > m_matrixSet->getMaxLevel())
    {
        throw pdal_error("Zoom level set higher than data set allows");
    }
}


point_count_t RialtoReader::read(PointViewPtr view, point_count_t /*not used*/)
{
    // TODO: okay to ignore point count parameter?
    
    log()->get(LogLevel::Debug) << "RialtoReader::read()" << std::endl;

    const TileMath tmm(m_matrixSet->getTmsetMinX(), m_matrixSet->getTmsetMinY(),
                       m_matrixSet->getTmsetMaxX(), m_matrixSet->getTmsetMaxY(),
                       m_matrixSet->getNumColsAtL0(), m_matrixSet->getNumRowsAtL0());

    setQueryParams();
    
    const double qMinX = m_queryBox.minx;
    const double qMinY = m_queryBox.miny;
    const double qMaxX = m_queryBox.maxx;
    const double qMaxY = m_queryBox.maxy;

    const uint32_t level = m_queryLevel;

    m_gpkg->queryForTiles_begin(m_dataset, qMinX, qMinY, qMaxX, qMaxY, level);

    GpkgTile info;

    do {
        bool ok = m_gpkg->queryForTiles_step(info);
        if (!ok) break;

        doQuery(tmm, info, view, qMinX, qMinY, qMaxX, qMaxY);
        
        log()->get(LogLevel::Debug) << "  resulting view now has "
            << view->size() << " points" << std::endl;
    } while (m_gpkg->queryForTiles_next());

    return view->size();
}


void RialtoReader::doQuery(const TileMath& tmm,
                           const GpkgTile& tile,
                           PointViewPtr view,
                           double qMinX, double qMinY, double qMaxX, double qMaxY)
{
    const uint32_t level = tile.getLevel();
    const uint32_t column = tile.getColumn();
    const uint32_t row = tile.getRow();
    const uint32_t numPoints = tile.getNumPoints();
    
    // if this tile is entirely inside the query box, then
    // we won't need to check each point
    double tileMinX, tileMinY, tileMaxX, tileMaxY;
    tmm.getTileBounds(column, row, level,
                      tileMinX, tileMinY, tileMaxX, tileMaxY);
    const bool tileEntirelyInsideQueryBox =
        tmm.rectContainsRect(qMinX, qMinY, qMaxX, qMaxY,
                             tileMinX, tileMinY, tileMaxX, tileMaxY);

    log()->get(LogLevel::Debug) << "  intersecting tile "
        << "(" << level << "," << column << "," << row << ")" 
        << " contains " << numPoints << " points" << std::endl;

    PointViewPtr tempView = view->makeNew();

    GpkgTile::exportToPV(numPoints, tempView, tile.getBlob());

    for (uint32_t i=0; i<tempView->size(); i++) {
        const double x = tempView->getFieldAs<double>(Dimension::Id::X, i);
        const double y = tempView->getFieldAs<double>(Dimension::Id::Y, i);
        if (tileEntirelyInsideQueryBox || (x >= qMinX && x <= qMaxX && y >= qMinY && y <= qMaxY))
        {
            view->appendPoint(*tempView, i);
        }
    }
}


} // namespace rialto

namespace pdal
{
void Options::remove(const std::string& name)
{
    m_options.erase(name);
}
}
