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


#include "TranslateTool.hpp"

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


TranslateTool::TranslateTool() :
    Tool(),
    m_outputType(TypeInvalid),
    m_doVerify(false),
    m_maxLevel(15),
    m_doReprojection(true)
{
}


TranslateTool::~TranslateTool()
{}


void TranslateTool::printSettings() const
{
    printf("Input file:   %s (%s)\n", m_inputName.c_str(), toString(m_inputType).c_str());
    printf("Output file:  %s (%s)\n", m_outputName.c_str(), toString(m_outputType).c_str());
    printf("Verification: %s\n", m_doVerify ? "true" : "false");
    printf("Reprojection: %s\n", m_doReprojection ? "true" : "false");
    if (m_outputType == TypeRialto) {
        printf("Max level:    %d\n", m_maxLevel);
    }
}


void TranslateTool::run()
{
    printSettings();

    pdal::Stage* reader = createReader(m_inputName, m_inputType);
    pdal::Stage* filter = NULL;
    if (m_doReprojection)
    {
        filter = createReprojector();
    }
    pdal::Stage* writer = createWriter(m_outputName, m_outputType, m_maxLevel);

    if (filter)
    {
        filter->setInput(*reader);
        writer->setInput(*filter);
    }
    else
    {
        writer->setInput(*reader);        
    }

    pdal::PointTable table;
    writer->prepare(table);
    PointViewSet pvs = writer->execute(table);
    
    uint32_t cnt = 0;
    for (auto pv: pvs) {
        cnt += pv->size();
    }
    
    printf("Points processed: %u\n", cnt);

    delete writer;
    delete filter;
    delete reader;
    
    if (m_doVerify)
    {
        pdal::Stage* expectedReader = createReader(m_inputName, m_inputType);
        pdal::Stage* filter = NULL;
        if (m_doReprojection)
        {
            filter = createReprojector();
            filter->setInput(*expectedReader);
            expectedReader = filter;
        }
        
        pdal::Stage* actualReader = createReader(m_outputName, m_outputType);
        
        verify(expectedReader, actualReader);
    }
}


void TranslateTool::printUsage() const
{
    printf("Usage: $ rialto_tool\n");
    printf("           -i infile\n");
    printf("           -o outfile\n");
    printf("           [-maxlevel number]\n");
    printf("           [-noreproj]\n");
    printf("           [-verify]\n");
    printf("where:\n");
    printf("  -i: supports .las, .laz, or .gpkg\n");
    printf("  -o: supports .las, .laz, or .gpkg\n");
    printf("  -noreproj: do not reproject to EPSG:4326\n");
    printf("  -maxlevel: set the maximum resolution level (default: 15)\n");
    printf("  -verify: run verification step\n");
}


bool TranslateTool::l_processOptions(int argc, char* argv[])
{
    int i = 1;

    while (i < argc)
    {
        if (streq(argv[i], "-h"))
        {
            return false;
        }
        
        if (streq(argv[i], "-i"))
        {
            m_inputName = argv[++i];
        }
        else if (streq(argv[i], "-o"))
        {
            m_outputName = argv[++i];
        }
        else if (streq(argv[i], "-noreproj"))
        {
            m_doReprojection = false;
        }
        else if (streq(argv[i], "-maxlevel"))
        {
            m_maxLevel = atoi(argv[++i]);
        }
        else if (streq(argv[i], "-verify"))
        {
            m_doVerify = true;
        }
        else
        {
            error("unrecognized option", argv[i]);
        }

        ++i;
    }
    
    if (m_outputName.empty()) {
        error("output file not specified");
    }
    m_outputType = inferType(m_outputName);
    
    return true;
}
