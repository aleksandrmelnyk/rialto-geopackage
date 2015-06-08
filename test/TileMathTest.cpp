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

#include "../src/TileMath.hpp"

#include "gtest/gtest.h"

using namespace rialto;


static void checkMath(const TileMath& tmm,
                      uint32_t e_col, uint32_t e_row, uint32_t e_level,
                      double e_minx, double e_miny, double e_maxx, double e_maxy)
{
    double a_minx, a_miny, a_maxx, a_maxy;
    uint32_t a_col, a_row, a_level;

    const double e_maxx_eps = e_maxx - 0.0001 * (e_maxx - e_minx);
    const double e_maxy_eps = e_maxy - 0.0001 * (e_maxy - e_miny);

    tmm.getTileBounds(e_col, e_row, e_level, a_minx, a_miny, a_maxx, a_maxy);

    EXPECT_DOUBLE_EQ(e_minx, a_minx);
    EXPECT_DOUBLE_EQ(e_miny, a_miny);
    EXPECT_DOUBLE_EQ(e_maxx, a_maxx);
    EXPECT_DOUBLE_EQ(e_maxy, a_maxy);

    tmm.getTileOfPoint(e_minx, e_miny, e_level, a_col, a_row);
    EXPECT_EQ(e_col, a_col);
    EXPECT_EQ(e_row, a_row);

    EXPECT_TRUE(tmm.tileContains(e_col, e_row, e_level, e_minx, e_miny));
    EXPECT_FALSE(tmm.tileContains(e_col, e_row, e_level, e_minx, e_maxy));
    EXPECT_FALSE(tmm.tileContains(e_col, e_row, e_level, e_maxx, e_miny));
    EXPECT_FALSE(tmm.tileContains(e_col, e_row, e_level, e_maxx, e_maxy));

    EXPECT_EQ(TileMath::QuadSW, tmm.getQuadrant(e_col, e_row, e_level, e_minx, e_miny));
    EXPECT_EQ(TileMath::QuadNW, tmm.getQuadrant(e_col, e_row, e_level, e_minx, e_maxy_eps));
    EXPECT_EQ(TileMath::QuadSE, tmm.getQuadrant(e_col, e_row, e_level, e_maxx_eps, e_miny));
    EXPECT_EQ(TileMath::QuadNE, tmm.getQuadrant(e_col, e_row, e_level, e_maxx_eps, e_maxy_eps));
}


static void checkChildren(const TileMath& tmm,
                          uint32_t col, uint32_t row, uint32_t level,
                          uint32_t col_nw, uint32_t row_nw,
                          uint32_t col_ne, uint32_t row_ne,
                          uint32_t col_sw, uint32_t row_sw,
                          uint32_t col_se, uint32_t row_se)
{
    uint32_t a_col, a_row;

    tmm.getChildOfTile(col, row, TileMath::QuadNW, a_col, a_row);
    EXPECT_EQ(a_col, col_nw);
    EXPECT_EQ(a_row, row_nw);

    tmm.getChildOfTile(col, row, TileMath::QuadNE, a_col, a_row);
    EXPECT_EQ(a_col, col_ne);
    EXPECT_EQ(a_row, row_ne);

    tmm.getChildOfTile(col, row, TileMath::QuadSW, a_col, a_row);
    EXPECT_EQ(a_col, col_sw);
    EXPECT_EQ(a_row, row_sw);

    tmm.getChildOfTile(col, row, TileMath::QuadSE, a_col, a_row);
    EXPECT_EQ(a_col, col_se);
    EXPECT_EQ(a_row, row_se);

    tmm.getParentOfTile(col_nw, row_nw, a_col, a_row);
    EXPECT_EQ(a_col, col);
    EXPECT_EQ(a_row, row);

    tmm.getParentOfTile(col_ne, row_ne, a_col, a_row);
    EXPECT_EQ(a_col, col);
    EXPECT_EQ(a_row, row);

    tmm.getParentOfTile(col_sw, row_sw, a_col, a_row);
    EXPECT_EQ(a_col, col);
    EXPECT_EQ(a_row, row);

    tmm.getParentOfTile(col_se, row_se, a_col, a_row);
    EXPECT_EQ(a_col, col);
    EXPECT_EQ(a_row, row);
}


TEST(TilerTest, test_tiler_matrix_math_one)
{
    // 4326
    // two cols, one row
    // (-180,-90) to (180,90)

    const TileMath tmm(-180.0, -90.0, 180.0, 90.0, 2u, 1u);

    EXPECT_DOUBLE_EQ(tmm.minX(), -180.0);
    EXPECT_DOUBLE_EQ(tmm.minY(), -90.0);
    EXPECT_DOUBLE_EQ(tmm.maxX(), 180.0);
    EXPECT_DOUBLE_EQ(tmm.maxY(), 90.0);

    EXPECT_TRUE(tmm.matrixContains(-180.0, -90.0));
    EXPECT_FALSE(tmm.matrixContains(-180.0, 90.0));
    EXPECT_FALSE(tmm.matrixContains(180.0, -90.0));
    EXPECT_FALSE(tmm.matrixContains(180.0, 90.0));

    EXPECT_EQ(tmm.numColsAtLevel(0), 2u);
    EXPECT_EQ(tmm.numRowsAtLevel(0), 1u);
    EXPECT_EQ(tmm.numColsAtLevel(1), 4u);
    EXPECT_EQ(tmm.numRowsAtLevel(1), 2u);
    EXPECT_EQ(tmm.numColsAtLevel(2), 8u);
    EXPECT_EQ(tmm.numRowsAtLevel(2), 4u);

    EXPECT_DOUBLE_EQ(tmm.tileWidthAtLevel(0), 180.0);
    EXPECT_DOUBLE_EQ(tmm.tileHeightAtLevel(0), 180.0);
    EXPECT_DOUBLE_EQ(tmm.tileWidthAtLevel(1), 90.0);
    EXPECT_DOUBLE_EQ(tmm.tileHeightAtLevel(1), 90.0);
    EXPECT_DOUBLE_EQ(tmm.tileWidthAtLevel(2), 45.0);
    EXPECT_DOUBLE_EQ(tmm.tileHeightAtLevel(2), 45.0);

    checkMath(tmm, 0, 0, 0, -180, -90, 0, 90);
    checkMath(tmm, 1, 0, 0, 0, -90, 180, 90);

    checkMath(tmm, 0, 0, 1, -180, 0, -90, 90);
    checkMath(tmm, 1, 0, 1, -90, 0, 0, 90);
    checkMath(tmm, 2, 0, 1, 0, 0, 90, 90);
    checkMath(tmm, 3, 0, 1, 90, 0, 180, 90);
    checkMath(tmm, 0, 1, 1, -180, -90, -90, 0);
    checkMath(tmm, 1, 1, 1, -90, -90, 0, 0);
    checkMath(tmm, 2, 1, 1, -0, -90, 90, 0);
    checkMath(tmm, 3, 1, 1, 90, -90, 180, 0);

    uint32_t c, r;
    double x, y;
    for (c=0, x=-180; c<8; c++, x+=45)
    {
        for (r=0, y=90-45; r<4; r++, y+=-45)
        {
          checkMath(tmm, c, r, 2, x, y, x+45, y+45);
        }
    }

    checkChildren(tmm, 0, 0, 0,
                  0, 0, 1, 0, 0, 1, 1, 1);
    checkChildren(tmm, 1, 0, 0,
                  2, 0, 3, 0, 2, 1, 3, 1);

    checkChildren(tmm, 0, 0, 1,
                  0, 0, 1, 0, 0, 1, 1, 1);
    checkChildren(tmm, 1, 0, 1,
                  2, 0, 3, 0, 2, 1, 3, 1);
    checkChildren(tmm, 2, 0, 1,
                  4, 0, 5, 0, 4, 1, 5, 1);
    checkChildren(tmm, 3, 0, 1,
                  6, 0, 7, 0, 6, 1, 7, 1);
    checkChildren(tmm, 0, 1, 1,
                  0, 2, 1, 2, 0, 3, 1, 3);
    checkChildren(tmm, 1, 1, 1,
                  2, 2, 3, 2, 2, 3, 3, 3);
    checkChildren(tmm, 2, 1, 1,
                  4, 2, 5, 2, 4, 3, 5, 3);
    checkChildren(tmm, 3, 1, 1,
                  6, 2, 7, 2, 6, 3, 7, 3);

}

TEST(TilerTest, test_tiler_matrix_math_two)
{
    // one col by one row
    // (10,2000) to (100,3000)

    const TileMath tmm(10.0, 2000.0, 100.0, 3000.0, 1u, 1u);

    EXPECT_DOUBLE_EQ(tmm.minX(), 10.0);
    EXPECT_DOUBLE_EQ(tmm.minY(), 2000.0);
    EXPECT_DOUBLE_EQ(tmm.maxX(), 100.0);
    EXPECT_DOUBLE_EQ(tmm.maxY(), 3000.0);

    EXPECT_TRUE(tmm.matrixContains(10.0, 2000.0));
    EXPECT_FALSE(tmm.matrixContains(10.0, 3000.0));
    EXPECT_FALSE(tmm.matrixContains(100.0, 2000.0));
    EXPECT_FALSE(tmm.matrixContains(100.0, 3000.0));

    EXPECT_EQ(tmm.numColsAtLevel(0), 1u);
    EXPECT_EQ(tmm.numRowsAtLevel(0), 1u);
    EXPECT_EQ(tmm.numColsAtLevel(1), 2u);
    EXPECT_EQ(tmm.numRowsAtLevel(1), 2u);
    EXPECT_EQ(tmm.numColsAtLevel(2), 4u);
    EXPECT_EQ(tmm.numRowsAtLevel(2), 4u);

    EXPECT_DOUBLE_EQ(tmm.tileWidthAtLevel(0), 90.0);
    EXPECT_DOUBLE_EQ(tmm.tileHeightAtLevel(0), 1000.0);
    EXPECT_DOUBLE_EQ(tmm.tileWidthAtLevel(1), 45.0);
    EXPECT_DOUBLE_EQ(tmm.tileHeightAtLevel(1), 500.0);
    EXPECT_DOUBLE_EQ(tmm.tileWidthAtLevel(2), 22.5);
    EXPECT_DOUBLE_EQ(tmm.tileHeightAtLevel(2), 250.0);

    checkMath(tmm, 0, 0, 0, 10, 2000, 100, 3000);

    checkMath(tmm, 0, 0, 1, 10, 2500, 55, 3000);
    checkMath(tmm, 1, 0, 1, 55, 2500, 100, 3000);
    checkMath(tmm, 0, 1, 1, 10, 2000, 55, 2500);
    checkMath(tmm, 1, 1, 1, 55, 2000, 100, 2500);

    uint32_t c, r;
    double x, y;
    for (c=0, x=10; c<4; c++, x+=22.5)
    {
        for (r=0, y=3000-250; r<4; r++, y+=-250)
        {
          checkMath(tmm, c, r, 2, x, y, x+22.5, y+250);
        }
    }

    checkChildren(tmm, 0, 0, 0,
                  0, 0, 1, 0, 0, 1, 1, 1);
    checkChildren(tmm, 1, 0, 0,
                  2, 0, 3, 0, 2, 1, 3, 1);

    checkChildren(tmm, 0, 0, 1,
                  0, 0, 1, 0, 0, 1, 1, 1);
    checkChildren(tmm, 1, 0, 1,
                  2, 0, 3, 0, 2, 1, 3, 1);
    checkChildren(tmm, 2, 0, 1,
                  4, 0, 5, 0, 4, 1, 5, 1);
    checkChildren(tmm, 3, 0, 1,
                  6, 0, 7, 0, 6, 1, 7, 1);
    checkChildren(tmm, 0, 1, 1,
                  0, 2, 1, 2, 0, 3, 1, 3);
    checkChildren(tmm, 1, 1, 1,
                  2, 2, 3, 2, 2, 3, 3, 3);
    checkChildren(tmm, 2, 1, 1,
                  4, 2, 5, 2, 4, 3, 5, 3);
    checkChildren(tmm, 3, 1, 1,
                  6, 2, 7, 2, 6, 3, 7, 3);

}

TEST(TilerTest, test_tiler_matrix_math_three)
{
    // two cols by three rows
    // (0,0) to (20,30)

    const TileMath tmm(0.0, 0.0, 20.0, 30.0, 2u, 3u);

    EXPECT_DOUBLE_EQ(tmm.minX(), 0.0);
    EXPECT_DOUBLE_EQ(tmm.minY(), 0.0);
    EXPECT_DOUBLE_EQ(tmm.maxX(), 20.0);
    EXPECT_DOUBLE_EQ(tmm.maxY(), 30.0);

    EXPECT_TRUE(tmm.matrixContains(0.0, 0.0));
    EXPECT_FALSE(tmm.matrixContains(0.0, 30.0));
    EXPECT_FALSE(tmm.matrixContains(20.0, 0.0));
    EXPECT_FALSE(tmm.matrixContains(20.0, 30.0));

    EXPECT_EQ(tmm.numColsAtLevel(0), 2u);
    EXPECT_EQ(tmm.numRowsAtLevel(0), 3u);
    EXPECT_EQ(tmm.numColsAtLevel(1), 4u);
    EXPECT_EQ(tmm.numRowsAtLevel(1), 6u);
    EXPECT_EQ(tmm.numColsAtLevel(2), 8u);
    EXPECT_EQ(tmm.numRowsAtLevel(2), 12u);

    EXPECT_DOUBLE_EQ(tmm.tileWidthAtLevel(0), 10.0);
    EXPECT_DOUBLE_EQ(tmm.tileHeightAtLevel(0), 10.0);
    EXPECT_DOUBLE_EQ(tmm.tileWidthAtLevel(1), 5.0);
    EXPECT_DOUBLE_EQ(tmm.tileHeightAtLevel(1), 5.0);

    checkMath(tmm, 0, 0, 0, 0, 20, 10, 30);
    checkMath(tmm, 1, 0, 0, 10, 20, 20, 30);
    checkMath(tmm, 0, 1, 0, 0, 10, 10, 20);
    checkMath(tmm, 1, 1, 0, 10, 10, 20, 20);
    checkMath(tmm, 0, 2, 0, 0, 0, 10, 10);
    checkMath(tmm, 1, 2, 0, 10, 0, 20, 10);

    checkMath(tmm, 1, 2, 1, 5, 15, 10, 20);
    checkMath(tmm, 3, 5, 1, 15, 0, 20, 5);
    checkMath(tmm, 0, 5, 1, 0, 0, 5, 5);
    checkMath(tmm, 3, 0, 1, 15, 25, 20, 30);

    checkChildren(tmm, 0, 2, 0,
                  0, 4, 1, 4, 0, 5, 1, 5);

}

TEST(TilerTest, test_tiler_matrix_rect_contains_rect)
{
    // A completely contains B
    EXPECT_TRUE(TileMath::rectContainsRect(1.0, 1.0, 2.0, 2.0, 1.4, 1.4, 1.6, 1.6));
    
    // A is completely outside B
    EXPECT_FALSE(TileMath::rectContainsRect(1.4, 1.4, 1.6, 1.6, 1.0, 1.0, 2.0, 2.0));
    
    // A completely contains B only along x-axis
    EXPECT_FALSE(TileMath::rectContainsRect(1.0, 1.4, 2.0, 1.6, 1.4, 1.0, 1.6, 2.0));

    // A completely contains B only along y-axis
    EXPECT_FALSE(TileMath::rectContainsRect(1.4, 1.0, 1.6, 2.0, 1.0, 1.4, 2.0, 1.6));
}
