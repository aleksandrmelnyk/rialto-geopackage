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

#include <pdal/pdal_types.hpp>
#include <pdal/Writer.hpp>

#include <cassert>
#include <cstdint>
#include <vector>

namespace pdal
{

namespace rialto
{


class ViewTile;
class TileMath;

// the tile set holds all the tiles that make up the tile matrix
class ViewTileSet
{
  public:
      ViewTileSet(uint32_t maxLevel,
              double minx, double miny,
              double maxx, double maxy,
              uint32_t numColsAtL0, uint32_t numRowsAtL0,
              LogPtr log);
      ~ViewTileSet();

      void build(PointViewPtr sourceView, PointViewSet* outputSet);

      uint32_t getMaxLevel() const { return m_maxLevel; }

      PointViewPtr createPointView();
      LogPtr log() { return m_log; }

      uint32_t newTileId() { uint32_t t = m_tileId; ++m_tileId; return t; }

      const TileMath& tmm() const { return *m_tmm; }

      const std::vector<ViewTile*>& getViewTiles() const { return m_allTiles; }
      std::vector<ViewTile*>& getViewTilesRef() { return m_allTiles; }

  private:
      PointViewPtr m_sourceView;
      PointViewSet* m_outputSet;
      uint32_t m_maxLevel;
      LogPtr m_log;
      ViewTile*** m_roots;
      uint32_t m_tileId;
      std::unique_ptr<TileMath> m_tmm;
      std::vector<ViewTile*> m_allTiles;
};


// A node of the tile tree
//
// Tiles are identified by level, column (x), and row (y)
// A tile may contain points; if so, m_pointView will be set.
// A tile may have 0..3 children.
class ViewTile
{
public:
    ViewTile(ViewTileSet& tileSet, uint32_t level, uint32_t column, uint32_t row);
    ~ViewTile();

    void add(PointViewPtr pointView, PointId pointNumber, double lon, double lat);
    void setMask();

    PointViewPtr getPointView() { return m_pointView; }
    uint32_t getLevel() const { return m_level; }
    uint32_t getColumn() const { return m_column; }
    uint32_t getRow() const { return m_row; }
    uint32_t getMask() const { return m_mask; }

private:
    LogPtr log() { return m_tileSet.log(); }
    char* getPointData(const PointView& buf, PointId& idx) const;
    
    PointViewPtr m_pointView;
    uint32_t m_level;
    uint32_t m_column;
    uint32_t m_row;
    uint32_t m_mask;
    uint32_t m_id;
    ViewTileSet& m_tileSet;
    ViewTile** m_children;
    uint64_t m_skip;
};

} // namespace rialto
} // namespace pdal
