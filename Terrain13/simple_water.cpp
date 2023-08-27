/*
    Copyright 2022 Etay Meiri

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "simple_water.h"
#include "texture_config.h"

SimpleWater::SimpleWater()
{
}


SimpleWater::~SimpleWater()
{
}


void SimpleWater::Init(int Size, float WorldScale)
{
    if (!m_waterTech.Init()) {
        printf("Error initializing water tech\n");
        exit(0);
    }

    m_waterTech.Enable();
    m_waterTech.SetWaterHeight(m_waterHeight);

    m_water.CreateTriangleList(2, 2, Size * WorldScale);
}


void SimpleWater::Render(const Matrix4f& WVP)
{
    m_waterTech.Enable();
    m_waterTech.SetWVP(WVP);
    m_waterTech.SetWaterHeight(m_waterHeight);

    m_water.Render();
}
