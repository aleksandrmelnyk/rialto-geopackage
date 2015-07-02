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


#include "InfoTool.hpp"

#include <iomanip>

#include <rialto/GeoPackageCommon.hpp>
#include <rialto/GeoPackageManager.hpp>
#include <rialto/GeoPackageReader.hpp>
#include <rialto/RialtoReader.hpp>
#include <rialto/RialtoWriter.hpp>
#include <pdal/NullWriter.hpp>
#include <pdal/FauxReader.hpp>
#include <pdal/LasReader.hpp>
#include <pdal/LasWriter.hpp>
#include <pdal/ReprojectionFilter.hpp>

#include "../src/TileMath.hpp"

using namespace pdal;
using namespace rialto;


InfoTool::InfoTool() :
    Tool(),
    m_tileInfo(false)
{}


InfoTool::~InfoTool()
{}


void InfoTool::printUsage() const
{
    printf("Usage: $ rialto_info [-h/--help] [--tile level col row] filename\n");
    printf("where:\n");
    printf("  'filename' can be .las, .laz, or .gpkg\n");
}


void InfoTool::printReader(const pdal::Stage& reader) const
{
    const SpatialReference& srs = reader.getSpatialReference();

    std::cout << "SRS WKT:" << std::endl;
    std::cout << "  " << srs.getWKT(SpatialReference::WKTModeFlag::eCompoundOK, true)
              << std::endl;
}


void InfoTool::printRialtoReader(const RialtoReader& reader) const
{
    std::cout << "File type: rialto geopackage" << std::endl;

    const GeoPackageReader& gpkg = reader.getGeoPackageReader();
    const GpkgMatrixSet& matrixSet = reader.getMatrixSet();
    const std::string name = matrixSet.getName();
    const std::vector<GpkgDimension>& dims = matrixSet.getDimensions();
    const uint32_t maxLevel = matrixSet.getMaxLevel();
    
    std::cout << "Name: " << name << std::endl;
    std::cout << "Num dimensions: " << matrixSet.getNumDimensions() << std::endl;
    std::cout << "Max level: " << maxLevel << std::endl;

    std::cout << "Tile min (x,y): "
        << matrixSet.getTmsetMinX()
        << ", " << matrixSet.getTmsetMinY()
        << std::endl;
    std::cout << "Tile max (x,y): "
        << matrixSet.getTmsetMaxX()
        << ", " << matrixSet.getTmsetMaxY()
        << std::endl;
    
    std::cout << "Data min (x,y): "
        << matrixSet.getDataMinX()
        << ", " << matrixSet.getDataMinY()
        << std::endl;
    std::cout << "Data max (x,y): "
        << matrixSet.getDataMaxX()
        << ", " << matrixSet.getDataMaxY()
        << std::endl;

    std::cout << "Level 0 (cols,rows): "
      << matrixSet.getNumColsAtL0()
      << ", " << matrixSet.getNumRowsAtL0()
      << std::endl;

    std::cout << "Dimensions: (position, name, type, min, mean, max)" << std::endl;
    for (auto dim: dims)
    {
        std::cout << "  " << dim.getPosition()
          << ": " << dim.getName()
          << ", " << dim.getDataType()
          << ", " << dim.getMinimum()
          << ", " << dim.getMean()
          << ", " << dim.getMaximum()
          << std::endl;
    }
    
    std::cout << "Num (tiles,points) at level:" << std::endl;
    uint32_t numPointsAtLevelN;
    for (uint32_t i=0; i<=maxLevel; i++)
    {
        uint32_t numTiles, numPoints;
        gpkg.getCountsAtLevel(name, i, numTiles, numPoints);
        std::cout << "  " << i
          << ": " << numTiles
          << ", " << numPoints
          << std::endl;
        numPointsAtLevelN = numPoints;
    }
    
    uint32_t bytesPerPoint = matrixSet.getBytesPerPoint();
    std::cout << "Bytes per point: " << bytesPerPoint << std::endl;
    
    // efficiency of 2.0 means the file is twice the size of the ideal (bloated)
    // efficiency of 0.5 means the file is half the size of the ideal (compressed)
    const uint64_t fileSize = FileUtils::fileSize(m_inputName);
    double efficiency = (double)fileSize / (double)(bytesPerPoint * numPointsAtLevelN);
    efficiency = round(efficiency*100.0)/100.0;
    std::cout << "Encoding efficiency: " << efficiency << std::endl;

    if (m_tileInfo)
    {
        const uint32_t tileId = gpkg.queryForTileId(matrixSet.getName(), m_tileLevel, m_tileColumn, m_tileRow);
        if (tileId == -1)
        {
            error("specified tile not found");
        }
        GpkgTile tileInfo;
        gpkg.readTile(matrixSet.getName(), tileId, false, tileInfo);
        printf("  tile (%u,%u,%u) info:\n", m_tileLevel, m_tileColumn, m_tileRow);
        printf("     num points: %u\n", tileInfo.getNumPoints());
        
        const TileMath tmm(matrixSet.getTmsetMinX(), matrixSet.getTmsetMinY(),
                           matrixSet.getTmsetMaxX(), matrixSet.getTmsetMaxY(),
                           matrixSet.getNumColsAtL0(), matrixSet.getNumRowsAtL0());
        double minx, miny, maxx, maxy;
        tmm.getTileBounds(m_tileLevel, m_tileColumn, m_tileRow,
                          minx, miny, maxx, maxy);
        printf("    tile bounds minx, miny: %f, %f\n", minx, miny);
        printf("                maxx, maxy: %f, %f\n", maxx, maxy);

  }
}


void InfoTool::printLasReader(const LasReader& reader) const
{
    printf("File type: las or laz\n");
  
    const LasHeader& h = reader.header();
    printf("Point count: %llu\n", h.pointCount());
    printf("Point length (bytes): %d\n", h.pointLen());
}


void InfoTool::run()
{
    pdal::Stage* reader = createReader(m_inputName, m_inputType);
    
    pdal::PointTable table;
    reader->prepare(table);
    
    printReader(*reader);
        
    if (m_inputType == TypeLas || m_inputType == TypeLaz)
    {
        LasReader* r = (LasReader*)reader;
        printLasReader(*r);
    }
    else if (m_inputType == TypeRialto)
    {
        RialtoReader* r = (RialtoReader*)reader;    
        printRialtoReader(*r);
    }
    else
    {
        error("unrecognized file type");
    }
    
    delete reader;
}


bool InfoTool::l_processOptions(int argc, char* argv[])
{
    int i = 1;

    while (i < argc)
    {
        if (streq(argv[i], "-h") || streq(argv[i], "--help"))
        {
            return false;
        }
        else if (streq(argv[i], "--tile"))
        {
            m_tileInfo = true;
            m_tileLevel = atoi(argv[++i]);
            m_tileColumn = atoi(argv[++i]);
            m_tileRow = atoi(argv[++i]);
        }
        m_inputName = std::string(argv[i]);

        ++i;
    }

    return true;
}
