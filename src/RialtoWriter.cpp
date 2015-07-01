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

#include <rialto/RialtoWriter.hpp>
#include <rialto/GeoPackageWriter.hpp>
#include <rialto/GeoPackageCommon.hpp>
#include "WritableTileCommon.hpp"

#include <boost/filesystem.hpp>


static PluginInfo const s_info = PluginInfo(
    "writers.rialto",
    "Rialto Writer",
    "http://pdal.io/stages/writers.rialto.html" );

//CREATE_SHARED_PLUGIN(1, 0, rialto::RialtoWriter, Writer, s_info)

namespace rialto
{


void RialtoWriter::ready(PointTableRef table)
{
    log()->get(LogLevel::Debug) << "RialtoWriter::localStart()" << std::endl;

    assert(FileUtils::fileExists(m_filename));

    // init database
    {
        m_gpkg = new GeoPackageWriter(m_filename, log());
        m_gpkg->open();

        if (m_gpkg->doesTableExist(m_dataset))
        {
            throw pdal_error("point cloud table already exists in database: " + m_dataset);
        }
    }
    
    // init SRS
    const SpatialReference& srs = getSpatialReference().empty() ?
        table.spatialRef() : getSpatialReference();
    setSpatialReference(srs);

    // get LAS metadata
    std::string lasMetadata;
    {
        std::string lasMinorVersion = "?";
        std::string lasMajorVersion = "?";
        std::string lasProject = "none";

        MetadataNode lasNode = table.metadata().findChild("readers.las");    
        if (lasNode.valid())
        {
            lasMinorVersion = lasNode.findChild("minor_version").value();
            lasMajorVersion = lasNode.findChild("major_version").value();
            lasProject = lasNode.findChild("project_id").value();
        }
        std::ostringstream oss;
        oss << "LAS: " << lasMajorVersion
            << "." << lasMinorVersion
            << ", Project: " << lasProject;
        lasMetadata = oss.str();
    }

    initStats(table.layout());

    // write tile matrix set table
    {
        const GpkgMatrixSet info(m_dataset, table.layout(), m_timestamp, srs,
                                 m_numColsAtL0, m_numRowsAtL0, m_description,
                                 lasMetadata, m_maxLevel);

        m_gpkg->writeTileTable(info);
    }
}


void RialtoWriter::initStats(PointLayoutPtr layout)
{
    for (auto i: layout->dims())
    {
        m_mins[i] = (std::numeric_limits<double>::max)();
        m_maxes[i] = (std::numeric_limits<double>::lowest)();
        m_means[i] = 0.0;
    }
    m_numPoints = 0;
}

void RialtoWriter::collectStats(PointView* pv)
{
    for (uint32_t pt=0; pt < pv->size(); pt++)
    {
        for (auto dim: pv->dims())
        {
            const double v = pv->getFieldAs<double>(dim, pt);
            m_mins[dim] = std::min(m_mins[dim], v);
            m_maxes[dim] = std::max(m_maxes[dim], v);
            m_means[dim] += v;
            //printf("%d: %f %f\n", dim, v, m_means[dim]);
        }
    }
    m_numPoints += pv->size();
}


void RialtoWriter::write(const PointViewPtr inView)
{
    // TODO: doesn't support being called more than once, e.g.
    // for a set of PointViews
                  
    WritableTileSet tileSet(m_maxLevel, 
                            m_tms_minx, m_tms_miny, m_tms_maxx, m_tms_maxy,
                            m_numColsAtL0, m_numRowsAtL0,
                            log());

    PointViewSet outViews;
    tileSet.build(inView, &outViews);

    m_gpkg->beginTransaction();
    
    writeAllTiles(tileSet);
    
    m_gpkg->commitTransaction();
}


void RialtoWriter::writeAllTiles(WritableTileSet& tileSet)
{
    const int numTiles = tileSet.getTiles().size();

    HeartBeat hb(numTiles, 50, 100);

    for (auto tile: tileSet.getTiles())
    {
        assert(tile != NULL);
        PointView* pv = tile->getPointView().get();
        if (pv)
        {
            writeTile(pv, tile->getLevel(), tile->getColumn(), tile->getRow(), tile->getMask());

            if (tile->getLevel() == m_maxLevel)
            {
                collectStats(pv);
            }
        }
        
        hb.beat();
    }
}


void RialtoWriter::done(PointTableRef table)
{  
    log()->get(LogLevel::Debug) << "RialtoWriter::localFinish()" << std::endl;

    updateDimensionStats(table.layout());

    m_gpkg->close();
    delete m_gpkg;
    m_gpkg = NULL;
}


std::string RialtoWriter::getName() const
{
    return s_info.name;
}


void RialtoWriter::processOptions(const Options& options)
{
    m_dataset = options.getValueOrDefault<std::string>("dataset", "");
    m_numColsAtL0 = options.getValueOrThrow<uint32_t>("numColsAtL0");
    m_numRowsAtL0 = options.getValueOrThrow<uint32_t>("numRowsAtL0");
    m_description = options.getValueOrThrow<std::string>("description");
    m_timestamp = options.getValueOrThrow<std::string>("timestamp");
    m_maxLevel = options.getValueOrThrow<uint32_t>("maxLevel");
    m_tms_minx = options.getValueOrThrow<double>("tms_minx");
    m_tms_miny = options.getValueOrThrow<double>("tms_miny");
    m_tms_maxx = options.getValueOrThrow<double>("tms_maxx");
    m_tms_maxy = options.getValueOrThrow<double>("tms_maxy");

    if (m_tms_minx >= m_tms_maxx || m_tms_miny >= m_tms_maxy)
    {
        throw pdal_error("TilerFilter: invalid matrix bounding box");
    }
    
    if (m_numColsAtL0 == 0 || m_numRowsAtL0 == 0)
    {
        throw pdal_error("TilerFilter: invalid matrix dimensions");
    }

    if (m_dataset == "")
    {
        m_dataset = boost::filesystem::path(m_filename).stem().string() + "_tiles";
    }
}


Options RialtoWriter::getDefaultOptions()
{
    Options options;
    return options;
}


void RialtoWriter::writeTile(PointView* view, uint32_t level, uint32_t col, uint32_t row, uint32_t mask)
{
    if (view && view->size() > 0)
    {
        const GpkgTile tile(view, level, col, row, mask);
        m_gpkg->writeTile(m_dataset, tile);
    }
}


void RialtoWriter::updateDimensionStats(PointLayoutPtr layout)
{
    for (auto dim: layout->dims())
    {
        const std::string& name = Dimension::name(dim);
        const double mean = m_means[dim] / (double)m_numPoints;
        m_gpkg->updateDimensionStats(m_dataset, name, m_mins[dim], mean, m_maxes[dim]);
    }
}


} // namespace rialto
