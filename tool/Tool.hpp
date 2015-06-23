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

#include <pdal/pdal.hpp>

using namespace pdal;


class Tool
{
public:
    enum FileType
    {
        TypeInvalid,
        TypeLas,        // rw
        TypeLaz,        // rw
        TypeRialto,     // rw
    };

    Tool();
    virtual ~Tool();

    void processOptions(int argc, char* argv[]);
    virtual void run() = 0;
    
    static void error(const char* p, const char* q=NULL);
    static bool streq(const char* p, const char* q);
    static std::string toString(FileType);

protected:
    virtual bool l_processOptions(int argc, char* argv[]) = 0;
    
    virtual void printUsage() const = 0;
    
    Stage* createReprojector();
    static FileType inferType(const std::string&);
    static Stage* createReader(const std::string& name, FileType type);
    static Stage* createWriter(const std::string& name, FileType type, uint32_t maxLevel);

    static void verify(Stage* readerExpected, Stage* readerActual);

    std::string m_inputName;
    FileType m_inputType;

private:
};
