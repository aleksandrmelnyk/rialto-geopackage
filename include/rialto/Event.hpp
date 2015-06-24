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

class Event
{
public:
    // Event e_foo("foo");
    // e.start();
    // ...work...
    // e.stop();
    // e.dump();
    Event(const std::string& name);

    ~Event();

    void start() const;
    void stop() const;
    void dump() const;

    // clock_t start = timerStart();
    // <spin cycles>
    // uint32_t millis = timerStop(start);
    static clock_t timerStart();
    static double timerStop(clock_t start);

private:
     const std::string m_name;

     // these are mutable so that start() and stop() can be const,
     // which we want because we want to do perf checks on const functions
     mutable uint32_t m_count;
     mutable double m_millis;
     mutable clock_t m_start;
     bool m_enabled;
};


class HeartBeat
{
public:
    // normal usage:
    //   HeartBeat hb;
    //   hb.beat();
    //   hb.beat();
    HeartBeat(int totalEvents);
    HeartBeat(int totalEvents, int startPerc, int endPerc);

    ~HeartBeat();
    
    void beat();

private:
    void init();

    const int m_startPerc;
    const int m_endPerc;
    const int m_totalEvents;
    int m_eventNum;
    int m_dotsPrinted;
    bool m_enabled;
};


} // namespace rialto
