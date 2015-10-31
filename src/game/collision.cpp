/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <math.h>
#include <engine/map.h>
#include <engine/kernel.h>

#include <game/mapitems.h>
#include <game/layers.h>
#include <game/collision.h>

#include <iostream>

CCollision::CCollision()
{
	m_pTiles = 0;
	m_Width = 0;
	m_Height = 0;
	m_pLayers = 0;
}

void CCollision::Init(class CLayers *pLayers)
{
	m_pLayers = pLayers;
	m_Width = m_pLayers->GameLayer()->m_Width;
	m_Height = m_pLayers->GameLayer()->m_Height;
	m_pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));

	for(int i = 0; i < m_Width*m_Height; i++)
	{
		int Index = m_pTiles[i].m_Index;

		if(Index > 128)
			continue;
		m_pTiles[i].m_Index = 0;
		switch(Index)
		{
		case TILE_DEATH:
			m_pTiles[i].m_Index |= COLFLAG_DEATH;
			break;
		case TILE_SOLID:
			m_pTiles[i].m_Index |= COLFLAG_SOLID;
			break;
		case TILE_NOHOOK:
			m_pTiles[i].m_Index |= COLFLAG_SOLID|COLFLAG_NOHOOK;
			break;
		case TILE_RAMP_LEFT:
			m_pTiles[i].m_Index |= COLFLAG_RAMP_LEFT;
			break;
		case TILE_RAMP_RIGHT:
			m_pTiles[i].m_Index |= COLFLAG_RAMP_RIGHT;
			break;
		case TILE_ROOFSLOPE_LEFT:
			m_pTiles[i].m_Index |= COLFLAG_ROOFSLOPE_LEFT;
			break;
		case TILE_ROOFSLOPE_RIGHT:
			m_pTiles[i].m_Index |= COLFLAG_ROOFSLOPE_RIGHT;
			break;
		case TILE_NOHOOK_RAMP_LEFT:
			m_pTiles[i].m_Index |= COLFLAG_RAMP_LEFT|COLFLAG_NOHOOK;
			break;
		case TILE_NOHOOK_RAMP_RIGHT:
			m_pTiles[i].m_Index |= COLFLAG_RAMP_RIGHT|COLFLAG_NOHOOK;
			break;
		case TILE_NOHOOK_ROOFSLOPE_LEFT:
			m_pTiles[i].m_Index |= COLFLAG_ROOFSLOPE_LEFT|COLFLAG_NOHOOK;
			break;
		case TILE_NOHOOK_ROOFSLOPE_RIGHT:
			m_pTiles[i].m_Index |= COLFLAG_ROOFSLOPE_RIGHT|COLFLAG_NOHOOK;
			break;
		default:
			m_pTiles[i].m_Index = 0;
		}
	}
}

int CCollision::GetTile(int x, int y)
{
	int Nx = clamp(x/32, 0, m_Width-1);
	int Ny = clamp(y/32, 0, m_Height-1);

	return m_pTiles[Ny*m_Width+Nx].m_Index > 128 ? 0 : m_pTiles[Ny*m_Width+Nx].m_Index;
}

int CCollision::SolidState(int x, int y, bool* nohook)
{
	unsigned char sol = GetTile(x, y);
	if(nohook) {
		*nohook = sol&COLFLAG_NOHOOK;
	}
	if(sol& COLFLAG_SOLID)
		return true;
	else if(sol&COLFLAG_RAMP_LEFT) {
		//return ((31-x%32) > (31-y%32));
		return ((31-x%32) > (31-y%32) ? SS_COL : ((31-x%32) == (31-y%32) ? SS_COL_RL : SS_NOCOL));
	}
	else if(sol&COLFLAG_RAMP_RIGHT) {
		//return (x%32 > (31-y%32));
		return (x%32 > (31-y%32) ? SS_COL : (x%32 == (31-y%32) ? SS_COL_RR : SS_NOCOL));
	}
	else if(sol&COLFLAG_ROOFSLOPE_LEFT) {
		//return ((31-x%32)> y%32);
		return ((31-x%32) > y%32 ? SS_COL : ((31-x%32) == y%32 ? SS_COL_HL : SS_NOCOL));
	}
	else if(sol&COLFLAG_ROOFSLOPE_RIGHT) {
		return (x%32 > y%32 ? SS_COL : (x%32 == y%32 ? SS_COL_HR : SS_NOCOL));
	}
	else
		return 0;
	//return GetTile(x, y)&COLFLAG_SOLI
}

bool CCollision::IsTileSolid(int x, int y)
{
	return SolidState(x,y) != SS_NOCOL;
}

// TODO: rewrite this smarter!
int CCollision::IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision)
{
	float Distance = distance(Pos0, Pos1);
	int End(Distance+1);
	vec2 Last = Pos0;

	for(int i = 0; i < End; i++)
	{
		float a = i/Distance;
		vec2 Pos = mix(Pos0, Pos1, a);
		if(CheckPoint(Pos.x, Pos.y))
		{
			if(pOutCollision)
				*pOutCollision = Pos;
			if(pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			return GetCollisionAt(Pos.x, Pos.y);
		}
		Last = Pos;
	}
	if(pOutCollision)
		*pOutCollision = Pos1;
	if(pOutBeforeCollision)
		*pOutBeforeCollision = Pos1;
	return 0;
}

// TODO: OPT: rewrite this smarter!
void CCollision::MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces)
{
	if(pBounces)
		*pBounces = 0;

	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;
	if(CheckPoint(Pos + Vel))
	{
		int Affected = 0;
		if(CheckPoint(Pos.x + Vel.x, Pos.y))
		{
			pInoutVel->x *= -Elasticity;
			if(pBounces)
				(*pBounces)++;
			Affected++;
		}

		if(CheckPoint(Pos.x, Pos.y + Vel.y))
		{
			pInoutVel->y *= -Elasticity;
			if(pBounces)
				(*pBounces)++;
			Affected++;
		}

		if(Affected == 0)
		{
			pInoutVel->x *= -Elasticity;
			pInoutVel->y *= -Elasticity;
		}
	}
	else
	{
		*pInoutPos = Pos + Vel;
	}
}

int CCollision::TestBox(vec2 Pos, vec2 Size)
{
	Size *= 0.5f;
	int r;
	for(int x = 0; x <= Size.x; x++) {
		if( (r = CheckPoint(Pos.x+x, Pos.y-Size.y)) )
			return r;
		if( (r = CheckPoint(Pos.x+x, Pos.y+Size.y)) )
			return r;
		
		if( (r = CheckPoint(Pos.x-x, Pos.y-Size.y)) )
			return r;
		if( (r = CheckPoint(Pos.x-x, Pos.y+Size.y)) )
			return r;
	}
	
	for(int y = 0; y <= Size.y; y++) {
		int r;
		if( (r = CheckPoint(Pos.x-Size.x, Pos.y+y)) )
			return r;
		if( (r = CheckPoint(Pos.x+Size.x, Pos.y+y)) )
			return r;
		
		if( (r = CheckPoint(Pos.x-Size.x, Pos.y-y)) )
			return r;
		if( (r = CheckPoint(Pos.x+Size.x, Pos.y-y)) )
			return r;
	}
			
	/*if(CheckPoint(Pos.x-Size.x, Pos.y-Size.y))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y-Size.y))
		return true;
	if(CheckPoint(Pos.x-Size.x, Pos.y+Size.y))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y+Size.y))
		return true;*/
	return 0;
}

void CCollision::MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity, bool check_speed)
{
	// do the move
	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;

	float Distance = length(Vel);
	int Max = (int)Distance;

	if(Distance > 0.00001f)
	{
		//vec2 old_pos = pos;
		float Fraction = 1.0f/(float)(Max+1);
		for(int i = 0; i <= Max; i++)
		{
			//float amount = i/(float)max;
			//if(max == 0)
				//amount = 0;

			vec2 NewPos = Pos + Vel*Fraction; // TODO: this row is not nice
			int rr = TestBox(vec2(NewPos.x, NewPos.y), Size);
			
			/*if (rr == SS_COL_RL || rr == SS_COL_RR) {
				std::cerr << "COL: " << rr << std::endl;
				int r = 0;
				if(rr == SS_COL_RL) {
					//Vel.x *= invsqrt2;
					Vel.y = Vel.x;
				}
				else if(rr == SS_COL_RR) {
					//Vel.x *= invsqrt2;
					Vel.y = -Vel.x;
				}
				//NewPos = Pos;
			}
			else*/ if(rr)
			{
				int Hits = 0;
				int r = 0;
				
				if( (r = TestBox(vec2(Pos.x, NewPos.y), Size)) )
				{
					//bool taken_care = false;
					NewPos.y = Pos.y;
					if(r == SS_COL_RR && Vel.x >= -Vel.y && (!check_speed || fabs(Vel.x) > 0.005f)) {
						float new_force = Vel.x * invsqrt2 - Vel.y * invsqrt2; 
						//if(new_force/Distance < 0.95f) {
							Vel.y = -new_force * invsqrt2;
							Vel.x = new_force  * invsqrt2;
							//std::cerr << "C1 " << new_force/Distance << std::endl;
							//taken_care = true;
						//}
					}
					else if(r == SS_COL_RL && Vel.x <= Vel.y && (!check_speed || fabs(Vel.x) > 0.005f)) {
						float new_force = -Vel.x * invsqrt2 - Vel.y * invsqrt2;
						//std::cerr << "C2pre " << Vel.x << ", " << check_speed << std::endl;
						//if(new_force/Distance < 0.95f) {
							Vel.y = -new_force * invsqrt2;
							Vel.x = -new_force * invsqrt2;
							//std::cerr << "C2 " << new_force/Distance << std::endl;
							//taken_care = true;
						//}
					}
					else
						Vel.y *= -Elasticity;
					Hits++;
					//Vel.y *= -Elasticity;
					//NewPos.y = Pos.y;
				}

				if( (r = TestBox(vec2(NewPos.x, Pos.y), Size)) )
				{
					
					/*bool climbing = false;
					//std::cerr << "Oh" << std::endl;
					for(int y = 1; y <= 2; y++) {
						if(!TestBox(vec2(NewPos.x, NewPos.y-y), Size)) {
							//std::cerr << "WUI " << y << std::endl;
							NewPos = vec2(NewPos.x, NewPos.y-y);
							climbing = true;
							break;
						}
					}
					if(!climbing) {*/
					//bool taken_care = false;
					NewPos.x = Pos.x;
					if(r == SS_COL_RR && Vel.x >= -Vel.y && (!check_speed || fabs(Vel.x) > 0.005f)) {
						float new_force = Vel.x * invsqrt2 - Vel.y * invsqrt2; 
						//if(new_force/Distance < 0.95f) {
							Vel.y = -new_force * invsqrt2;
							Vel.x = new_force  * invsqrt2;
							//std::cerr << "D1 " << new_force/Distance << std::endl;
							//taken_care = true;
						//}
					}
					else if(r == SS_COL_RL && Vel.x <= Vel.y && (!check_speed || fabs(Vel.x) > 0.005f)) {
						float new_force = -Vel.x * invsqrt2 - Vel.y * invsqrt2;
						//if(new_force/Distance < 0.95f) {
							Vel.y = -new_force * invsqrt2;
							Vel.x = -new_force * invsqrt2;
							//std::cerr << "D2 " << new_force/Distance << std::endl;
							//taken_care = true;
						//}
					}
					else
						Vel.x *= -Elasticity;
							
					//} else {
						//Vel.x *= 0.85f;
						//float newvely = -abs(Vel.x/2.0f);
						
						//if(Vel.y > newvely)
						//	Vel.y = newvely;
					//}
					
					Hits++;
					//Vel.x *= -Elasticity;
					//NewPos.x = Pos.x;
				}

				// neither of the tests got a collision.
				// this is a real _corner case_!
				if(Hits == 0)
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
				}
			}
			/*else if(rr != 0) {
				NewPos.y = Pos.y;
				Vel.y *= -Elasticity;
				NewPos.x = Pos.x;
				Vel.x *= -Elasticity;
			}*/
			
			Pos = NewPos;
		}
	}
	
	/*if(Vel.y >= 0) {
		bool was_hitting = false;
		for(int y = 3; y >= 0; y--) {
			bool hitting = TestBox(vec2(Pos.x, Pos.y+y), Size);
			if(!hitting && was_hitting) {
				Pos.y+=y;
				break;
			}
			else if (hitting) {
				was_hitting = true;
			}
		}
	}*/

	*pInoutPos = Pos;
	*pInoutVel = Vel;
}
