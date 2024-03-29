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

#include <rialto/GeoPackageReader.hpp>

#include <rialto/GeoPackageCommon.hpp>
#include "SQLiteCommon.hpp"
#include "TileMath.hpp"

namespace rialto
{

GeoPackageReader::GeoPackageReader(const std::string& connection, LogPtr mylog) :
    GeoPackage(connection, mylog),
    m_srid(4326),
    e_tilesRead("tilesRead"),
    e_tileTablesRead("tileTablesRead"),
    e_queries("queries"),
    m_numPointsRead(0)
{
    log()->get(LogLevel::Debug) << "GeoPackageReader::GeoPackageReader" << std::endl;
}


GeoPackageReader::~GeoPackageReader()
{
    log()->get(LogLevel::Debug) << "GeoPackageReader::~GeoPackageReader" << std::endl;
}


void GeoPackageReader::open()
{
    log()->get(LogLevel::Debug) << "GeoPackageReader::open" << std::endl;

    internalOpen(false);

    verifyTableExists("gpkg_spatial_ref_sys");
    verifyTableExists("gpkg_contents");
    verifyTableExists("gpkg_pctile_matrix");
    verifyTableExists("gpkg_pctile_matrix_set");
    verifyTableExists("gpkg_extensions");
    verifyTableExists("gpkg_metadata");
    verifyTableExists("gpkg_metadata_reference");
}


void GeoPackageReader::close()
{
    log()->get(LogLevel::Debug) << "GeoPackageReader::close" << std::endl;

    internalClose();
    //dumpStats();
}


void GeoPackageReader::setupLayout(const GpkgMatrixSet& tileTableInfo, PointLayoutPtr layout)
{
    for (uint32_t i=0; i<tileTableInfo.getNumDimensions(); i++)
    {
        const GpkgDimension& dimInfo = tileTableInfo.getDimensions()[i];

        const Dimension::Id::Enum nameId = Dimension::id(dimInfo.getName());
        const Dimension::Type::Enum typeId = Dimension::type(dimInfo.getDataType());

        layout->registerDim(nameId, typeId);
    }
}


void GeoPackageReader::readTile(std::string const& name, uint32_t tileId, bool withPoints, GpkgTile& info) const
{
    if (!m_sqlite)
    {
        throw pdal_error("RialtoDB: invalid state (session does exist)");
    }

    e_tilesRead.start();

    std::ostringstream oss;
    oss << "SELECT zoom_level,tile_column,tile_row,num_points,child_mask"
        << (withPoints ? ",tile_data " : " ")
        << "FROM '" << name << "'"
        << "WHERE id=" << tileId;

    //log()->get(LogLevel::Debug) << "SELECT for tile" << std::endl;

    m_sqlite->query(oss.str());

    // should get exactly one row back
    const row* r = m_sqlite->get();
    assert(r);

    const uint32_t level = boost::lexical_cast<uint32_t>(r->at(0).data);
    const uint32_t column = boost::lexical_cast<uint32_t>(r->at(1).data);
    const uint32_t row = boost::lexical_cast<uint32_t>(r->at(2).data);
    const uint32_t numPoints = boost::lexical_cast<uint32_t>(r->at(3).data);
    const uint32_t mask = boost::lexical_cast<uint32_t>(r->at(4).data);

    if (withPoints)
    {
        const std::vector<char>& v = (const std::vector<char>&)(r->at(5).blobBuf);
        info.set(level, column, row, numPoints, mask, v);
        ++m_numPointsRead;
    }
    else
    {
        info.set(level, column, row, numPoints, mask, std::vector<char>());
    }
    
    e_tilesRead.stop();

    assert(!m_sqlite->next());
}

void GeoPackageReader::getCountsAtLevel(std::string const& name, uint32_t level,
                                        uint32_t& numTiles, uint32_t& numPoints) const
{
    if (!m_sqlite)
    {
        throw pdal_error("RialtoDB: invalid state (session does exist)");
    }

    std::ostringstream oss;
    oss << "SELECT count(num_points),sum(num_points) FROM '" << name << "'"
        << " WHERE zoom_level=" << level;

    log()->get(LogLevel::Debug) << "SELECT for tile ids at level: " << level << std::endl;

    m_sqlite->query(oss.str());

    numTiles = 0;
    numPoints = 0;
    
    do {
        const row* r = m_sqlite->get();
        if (!r) break;

        numTiles += boost::lexical_cast<uint32_t>(r->at(0).data);
        numPoints += boost::lexical_cast<uint32_t>(r->at(1).data);
        
        //log()->get(LogLevel::Debug) << "  got tile id=" << id << std::endl;
    } while (m_sqlite->next());
}


void GeoPackageReader::readTileIdsAtLevel(std::string const& name, uint32_t level, std::vector<uint32_t>& ids) const
{
    if (!m_sqlite)
    {
        throw pdal_error("RialtoDB: invalid state (session does exist)");
    }

    ids.clear();

    std::ostringstream oss;
    oss << "SELECT id FROM '" << name << "'"
        << " WHERE zoom_level=" << level;

    log()->get(LogLevel::Debug) << "SELECT for tile ids at level: " << level << std::endl;

    m_sqlite->query(oss.str());

    do {
        const row* r = m_sqlite->get();
        if (!r) break;

        uint32_t id = boost::lexical_cast<uint32_t>(r->at(0).data);

        //log()->get(LogLevel::Debug) << "  got tile id=" << id << std::endl;
        ids.push_back(id);
    } while (m_sqlite->next());
}


uint32_t GeoPackageReader::queryForTileId(std::string const& name,
                                          uint32_t levelNum,
                                          uint32_t columnNum,
                                          uint32_t rowNum) const
{
    if (!m_sqlite)
    {
        throw pdal_error("RialtoDB: invalid state (session does exist)");
    }

    log()->get(LogLevel::Debug) << "Querying tile set " << name
                                << " for a tile id" << std::endl;
    std::ostringstream oss;
    oss << "SELECT id FROM '" << name << "'"
        << " WHERE zoom_level=" << levelNum
        << " AND tile_column = " << columnNum
        << " AND tile_row <= " << rowNum;

    m_sqlite->query(oss.str());

    const row* r = m_sqlite->get();
    if (!r) return -1;

    uint32_t id = boost::lexical_cast<uint32_t>(r->at(0).data);
    log()->get(LogLevel::Debug) << "  got tile id=" << id << std::endl;

    return id;
}


void GeoPackageReader::queryForTileIds(std::string const& name,
                               double minx, double miny,
                               double maxx, double maxy,
                               uint32_t level,
                               std::vector<uint32_t>& ids) const
{
    if (!m_sqlite)
    {
        throw pdal_error("RialtoDB: invalid state (session does exist)");
    }

    log()->get(LogLevel::Debug) << "Querying tile set " << name
                                << " for some tile ids" << std::endl;

    ids.clear();

    e_queries.start();

    assert(minx <= maxx);
    assert(miny <= maxy);

    GpkgMatrixSet info;
    readMatrixSet(name, info); // TODO: should cache this

    const TileMath tmm(info.getTmsetMinX(), info.getTmsetMinY(),
                                          info.getTmsetMaxX(), info.getTmsetMaxY(),
                                          info.getNumColsAtL0(), info.getNumRowsAtL0());
    uint32_t mincol, minrow, maxcol, maxrow;
    // we use mincol/maxrow and maxcol/minrow because the tile matrix has (0,0) at upper-left
    tmm.getTileOfPoint(minx, miny, level, mincol, minrow);
    tmm.getTileOfPoint(maxx, maxy, level, maxcol, maxrow);

    // because grid starts upper-left
    std::swap(minrow, maxrow);

    assert(mincol <= maxcol);
    assert(minrow <= maxrow);

    std::ostringstream oss;
    oss << "SELECT id FROM '" << name << "'"
        << " WHERE zoom_level=" << level
        << " AND tile_column >= " << mincol
        << " AND tile_column <= " << maxcol
        << " AND tile_row >= " << minrow
        << " AND tile_row <= " << maxrow;

    m_sqlite->query(oss.str());

    do {
        const row* r = m_sqlite->get();
        if (!r) break;

        uint32_t id = boost::lexical_cast<uint32_t>(r->at(0).data);
        log()->get(LogLevel::Debug) << "  got tile id=" << id << std::endl;
        ids.push_back(id);
    } while (m_sqlite->next());

    e_queries.stop();
}


void GeoPackageReader::queryForTiles_begin(std::string const& name,
                                   double minx, double miny,
                                   double maxx, double maxy,
                                   uint32_t level)
{
    if (!m_sqlite)
    {
        throw pdal_error("RialtoDB: invalid state (session does exist)");
    }

    e_tilesRead.start();

    log()->get(LogLevel::Debug) << "Querying tile set " << name
                                << " for some tile infos" << std::endl;

    assert(minx <= maxx);
    assert(miny <= maxy);

    GpkgMatrixSet info;
    readMatrixSet(name, info); // TODO: should cache this

    const TileMath tmm(info.getTmsetMinX(), info.getTmsetMinY(),
                                          info.getTmsetMaxX(), info.getTmsetMaxY(),
                                          info.getNumColsAtL0(), info.getNumRowsAtL0());
    uint32_t mincol, minrow, maxcol, maxrow;
    // we use mincol/maxrow and maxcol/minrow because the tile matrix has (0,0) at upper-left
    tmm.getTileOfPoint(minx, miny, level, mincol, maxrow);
    tmm.getTileOfPoint(maxx, maxy, level, maxcol, minrow);

    assert(mincol <= maxcol);
    assert(minrow <= maxrow);

    std::ostringstream oss;
    oss << "SELECT zoom_level,tile_column,tile_row,num_points,child_mask,tile_data"
        << " FROM '" << name << "'"
        << " WHERE zoom_level=" << level
        << " AND tile_column >= " << mincol
        << " AND tile_column <= " << maxcol
        << " AND tile_row >= " << minrow
        << " AND tile_row <= " << maxrow;

    m_sqlite->query(oss.str());

    e_tilesRead.stop();
}


bool GeoPackageReader::queryForTiles_step(GpkgTile& info)
{
    e_tilesRead.start();

    const row* r = m_sqlite->get();

    if (!r)
    {
        e_tilesRead.stop();
        return false;
    }

    const uint32_t level = boost::lexical_cast<uint32_t>(r->at(0).data);
    const uint32_t column = boost::lexical_cast<uint32_t>(r->at(1).data);
    const uint32_t row = boost::lexical_cast<uint32_t>(r->at(2).data);
    const uint32_t numPoints = boost::lexical_cast<uint32_t>(r->at(3).data);
    const uint32_t mask = boost::lexical_cast<uint32_t>(r->at(4).data);

    // this query always reads the points
    const std::vector<char>& v = (const std::vector<char>&)(r->at(5).blobBuf);

    info.set(level, column, row, numPoints, mask, v);

    e_tilesRead.stop();

    m_numPointsRead += info.getNumPoints();

    return true;
}


bool GeoPackageReader::queryForTiles_next()
{
    return m_sqlite->next();
}



void GeoPackageReader::childDumpStats() const
{
    std::cout << "GeoPackageReader stats" << std::endl;

    e_tilesRead.dump();
    e_tileTablesRead.dump();
    e_queries.dump();

    std::cout << "    pointsRead: ";

    if (m_numPointsRead)
    {
        std::cout << m_numPointsRead;
    }
    else
    {
        std::cout << "-";
    }

    std::cout << std::endl;
}


} // namespace rialto
