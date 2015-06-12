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

#include "ViewTileCommon.hpp"
#include "TileMath.hpp"
#include <rialto/Event.hpp>

namespace rialto
{


ViewTileSet::ViewTileSet(
        uint32_t maxLevel,
        double minx, double miny,
        double maxx, double maxy,
        uint32_t numColsAtL0, uint32_t numRowsAtL0,
        LogPtr log) :
    m_sourceView(NULL),
    m_outputSet(NULL),
    m_maxLevel(maxLevel),
    m_log(log),
    m_roots(NULL),
    m_tileId(0)
{
    m_tmm = std::unique_ptr<TileMath>(new TileMath(minx, miny, maxx, maxy, numColsAtL0, numRowsAtL0));

    m_roots = new ViewTile**[numColsAtL0];

    for (uint32_t c=0; c<numColsAtL0; c++)
    {
        m_roots[c] = new ViewTile*[numRowsAtL0];
        for (uint32_t r=0; r<numRowsAtL0; r++)
        {
            m_roots[c][r] = new ViewTile(*this, 0, c, r);
            m_allTiles.push_back(m_roots[c][r]);
        }
    }
}


ViewTileSet::~ViewTileSet()
{
    if (m_roots) {
        const uint32_t numCols = m_tmm->numColsAtLevel(0);
        const uint32_t numRows = m_tmm->numRowsAtLevel(0);
        
        for (uint32_t c=0; c<numCols; c++)
        {
            for (uint32_t r=0; r<numRows; r++)
            {
                delete m_roots[c][r];
            }
            delete[] m_roots[c];
        }
        delete[] m_roots;
    }
}


void ViewTileSet::build(PointViewPtr sourceView, PointViewSet* outputSet)
{
    m_sourceView = sourceView;
    m_outputSet = outputSet;

    const uint32_t numCols = m_tmm->numColsAtLevel(0);
    const uint32_t numRows = m_tmm->numRowsAtLevel(0);

    Heartbeat hb(0);
    
    const uint32_t numPoints = sourceView->size();
    for (PointId idx = 0; idx < numPoints; ++idx)
    {
        const double x = sourceView->getFieldAs<double>(Dimension::Id::X, idx);
        const double y = sourceView->getFieldAs<double>(Dimension::Id::Y, idx);

        assert(m_tmm->matrixContains(x,y));

        // TODO: we could optimize this, since the tile bounds are constant
        // throughout the points loop
        bool added = false;
        for (uint32_t c=0; c<numCols; c++)
        {
            for (uint32_t r=0; r<numRows; r++)
            {
                if (m_tmm->tileContains(c, r, 0, x, y))
                {
                  ViewTile* tile = m_roots[c][r];
                    tile->add(m_sourceView, idx, x, y);
                    added = true;
                    break;
                }
            }
            if (added)
            {
                break;
            }
        }
        assert(added);
        
        hb.beat(idx, numPoints);
    }

    for (uint32_t c=0; c<numCols; c++)
    {
        for (uint32_t r=0; r<numRows; r++)
        {
            ViewTile* tile = m_roots[c][r];
            tile->setMask();
        }
    }
}



PointViewPtr ViewTileSet::createPointView()
{
    PointViewPtr p = m_sourceView->makeNew();
    m_outputSet->insert(p);

    return p;
}


ViewTile::ViewTile(ViewTileSet& tileSet,
        uint32_t level,
        uint32_t column,
        uint32_t row) :
    m_pointView(NULL),
    m_level(level),
    m_column(column),
    m_row(row),
    m_mask(0),
    m_tileSet(tileSet),
    m_children(NULL),
    m_skip(0)
{
    m_id = tileSet.newTileId();

    assert(m_level <= m_tileSet.getMaxLevel());

    //log()->get(LogLevel::Debug1) << "created tb (l=" << m_level
        //<< ", tx=" << m_tileX
        //<< ", ty=" << m_tileY
        //<< ") (slip" << m_skip
        //<< ")  --  w" << m_rect.west()
        //<< " s" << m_rect.south()
        //<< " e" << m_rect.east()
        //<< " n" << m_rect.north() << "\n";

    // level N+1 has 1/4 the points of level N
    //
    // level 3: skip 1
    // level 2: skip 4
    // level 1: skip 16
    // level 0: skip 256

    // max=3, max-level=u
    // 3-3=0  skip 1   4^0
    // 3-2=1  skip 4    4^1
    // 3-1=2  skip 16    4^2
    // 3-0=3  skip 64    4^3
    //
    m_skip = std::pow(4, (m_tileSet.getMaxLevel() - m_level));
    //log()->get(LogLevel::Debug1) << "level=" << m_level
        //<< "  skip=" << m_skip << "\n";
}


ViewTile::~ViewTile()
{
    if (m_children != NULL)
    {
        for (int i=0; i<4; ++i)
        {
            if (m_children[i])
            {
                delete m_children[i];
            }
        }
        delete[] m_children;
    }
}


void ViewTile::setMask()
{
  // child mask
  m_mask = 0x0;
  if (m_children)
  {
      if (m_children[TileMath::QuadSW]) m_mask += 1;
      if (m_children[TileMath::QuadSE]) m_mask += 2;
      if (m_children[TileMath::QuadNE]) m_mask += 4;
      if (m_children[TileMath::QuadNW]) m_mask += 8;
  }

  if (m_children) {
      if (m_children[0]) m_children[0]->setMask();
      if (m_children[1]) m_children[1]->setMask();
      if (m_children[2]) m_children[2]->setMask();
      if (m_children[3]) m_children[3]->setMask();
    }
}


// Add the point to this tile.
//
// If we're not a leaf tile, add the point only if we're a module-N numbered
// point, where N is based on the level.
//
// If we're not a leaf tile, add the node to one of our child tiles.
void ViewTile::add(PointViewPtr sourcePointView, PointId pointNumber, double x, double y)
{
    //log()->get(LogLevel::Debug5) << "-- -- " << pointNumber
        //<< " " << m_skip
        //<< " " << (pointNumber % m_skip == 0) << "\n";

    // put the point into this tile, if we're at the right level of decimation
    if (pointNumber % m_skip == 0)
    {
        if (!m_pointView) {
            m_pointView = m_tileSet.createPointView();
        }
        assert(m_pointView);

        m_pointView->appendPoint(*sourcePointView, pointNumber);
    }

    if (m_level == m_tileSet.getMaxLevel()) return;

    if (!m_children)
    {
        m_children = new ViewTile*[4];
        m_children[0] = NULL;
        m_children[1] = NULL;
        m_children[2] = NULL;
        m_children[3] = NULL;
    }

    //log()->get(LogLevel::Debug5) << "which=" << q << "\n";

    const TileMath& tmm = m_tileSet.tmm();
    const TileMath::Quad q = tmm.getQuadrant(m_column, m_row, m_level, x, y);

    if (!m_children[q])
    {
        uint32_t childCol, childRow;
        tmm.getChildOfTile(m_column, m_row, q, childCol, childRow);
        m_children[q] = new ViewTile(m_tileSet, m_level+1, childCol, childRow);
        m_tileSet.getViewTilesRef().push_back(m_children[q]);
    }

    m_children[q]->add(sourcePointView, pointNumber, x, y);
}

} // namespace rialto
