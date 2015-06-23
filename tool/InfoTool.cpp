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

#include <rialto/GeoPackageCommon.hpp>
#include <rialto/GeoPackageManager.hpp>
#include <rialto/RialtoReader.hpp>
#include <rialto/RialtoWriter.hpp>
#include <pdal/NullWriter.hpp>
#include <pdal/FauxReader.hpp>
#include <pdal/LasReader.hpp>
#include <pdal/LasWriter.hpp>
#include <pdal/ReprojectionFilter.hpp>

using namespace pdal;
using namespace rialto;


InfoTool::InfoTool() :
    Tool(),
    m_inputType(TypeInvalid)
{}


InfoTool::~InfoTool()
{}


void InfoTool::printUsage() const
{
    printf("Usage: $ rialto_tool filename\n");
    printf("where:\n");
    printf("  'filename' can be .las, .laz, or .gpkg\n");
}

void InfoTool::run()
{
    pdal::Stage* reader = createReader(m_inputName, m_inputType);
    
    pdal::PointTable table;
    reader->prepare(table);

    const SpatialReference& srs = reader->getSpatialReference();
    
    printf("SRS name: %s\n", srs.getName().c_str());
    //printf("SRS description: %s\n", srs.getDescription().c_str());
    printf("SRS WKT: %s\n", srs.getWKT(SpatialReference::WKTModeFlag::eCompoundOK, true).c_str());
    
    if (m_inputType == TypeLas || m_inputType == TypeLaz)
    {
        printf("File type: las or laz\n");
        
        LasReader* r = (LasReader*)reader;
        const LasHeader& h = r->header();
        printf("Point count: %llu\n", h.pointCount());
        printf("Point length (bytes): %d\n", h.pointLen());
    }
    else if (m_inputType == TypeRialto)
    {
        printf("Filt type: rialto geopackage\n");
        
        RialtoReader* r = (RialtoReader*)reader;
        const GpkgMatrixSet& ms = r->getMatrixSet();
        printf("Num dimensions: %d\n", ms.getNumDimensions());
    }
    else
    {
        error("unrecognized file type");
    }
    
    delete reader;
}


void InfoTool::processOptions(int argc, char* argv[])
{
    int i = 1;

    while (i < argc)
    {
        if (streq(argv[i], "-h"))
        {
            printUsage();
            error("exiting");
        }
        
        m_inputName = std::string(argv[i]);

        ++i;
    }

    if (m_inputName.empty()) error("input file not specified");
    m_inputType = inferType(m_inputName);
}
