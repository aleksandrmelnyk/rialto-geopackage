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

#include <rialto/Event.hpp>

namespace rialto
{

static bool streq(const char* p, const char* q)
{
    return (strcmp(p,q)==0);
}
    

static bool getEnvVarSetting(const char* var)
{
    const char* hb = getenv(var);
    
    if (!hb || streq(hb, "true"))
    {
        return true;
    }
    
    if (streq(hb, "false"))
    {
        return false;
    }
    throw pdal_error("Invalid setting for $RIALTO_HEARTBEAT");
}


Event::Event(const std::string& name) :
    m_name(name),
    m_count(0),
    m_millis(0.0),
    m_start(0),
    m_enabled(false)
{
    m_enabled = getEnvVarSetting("RIALTO_EVENTS");
}


Event::~Event()
{
    if (!m_enabled) return;

    if (m_start != 0)
    {
        std::ostringstream oss;
        oss << "timing event not closed: " << m_name;
        throw pdal_error(oss.str());
    }
}


void Event::start() const
{
    if (!m_enabled) return;

    assert(m_start == 0);
    m_start = timerStart();
}


void Event::stop() const
{
    if (!m_enabled) return;

    assert(m_start != 0);
    ++m_count;
    m_millis += timerStop(m_start);
    m_start = 0;
}


void Event::dump() const
{
    if (!m_enabled) return;

    std::cout << "    " << m_name << ":";

    if (m_count)
    {
        std::cout << "  total=" << (int)m_millis << "ms"
                  << "  average=" << (int)(m_millis/(double)m_count) << "ms"
                  << "  (" << m_count << " events)";
    }
    else
    {
        std::cout << " -";
    }

    std::cout << std::endl;
}


clock_t Event::timerStart()
{
     return std::clock();
}


double Event::timerStop(clock_t start)
{
    clock_t stop = std::clock();
    const double secs = (double)(stop - start) / (double)CLOCKS_PER_SEC;
    return secs * 1000.0;
}


HeartBeat::HeartBeat(int totalEvents) :
    m_startPerc(0),
    m_endPerc(100),
    m_dotsPrinted(0),
    m_totalEvents(totalEvents),
    m_eventNum(0),
    m_enabled(true)
{
    m_enabled = getEnvVarSetting("RIALTO_HEARTBEAT");
}


HeartBeat::HeartBeat(int totalEvents, int startPerc, int endPerc) :
    m_startPerc(startPerc),
    m_endPerc(endPerc),
    m_dotsPrinted(0),
    m_totalEvents(totalEvents),
    m_eventNum(0),
    m_enabled(true)
{
    m_enabled = getEnvVarSetting("RIALTO_HEARTBEAT");
}


HeartBeat::~HeartBeat()
{
    if (!m_enabled) return;
    printf("\n");
}


void HeartBeat::beat()
{
    ++m_eventNum;
    
    if (!m_enabled) return;

    assert(m_eventNum > 0);
    assert(m_eventNum <= m_totalEvents);
    
    const double percentComplete =
        ((double)(m_eventNum) / (double)m_totalEvents);

    // how many dots should we have displayed at this point?
    const int dotsWanted = percentComplete * (double)(m_endPerc - m_startPerc);
    
    while (m_dotsPrinted <= dotsWanted)
    {
        printf(".");
        if ((m_dotsPrinted % 5) == 0) {
            printf("%d", m_dotsPrinted);
        }
        ++m_dotsPrinted;
    }
    fflush(stdout);
}


} // namespace rialto
