/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>

class CCollision
{
	class CTile *m_pTiles;
	int m_Width;
	int m_Height;
	class CLayers *m_pLayers;

	int SolidState(int x, int y, bool* nohook = NULL) const;
	bool IsTileSolid(int x, int y) const;
	int GetTile(int x, int y) const;

public:
	enum
	{
		COLFLAG_SOLID=1,
		COLFLAG_DEATH=2,
		COLFLAG_NOHOOK=4,

		COLFLAG_RAMP_LEFT=8,
		COLFLAG_RAMP_RIGHT=16,
		COLFLAG_ROOFSLOPE_LEFT=32,
		COLFLAG_ROOFSLOPE_RIGHT=64,
	};

	enum
	{
		SS_NOCOL=0,
		SS_COL=1,
		SS_COL_RL=2,
		SS_COL_RR=3,
		SS_COL_HL=4,
		SS_COL_HR=5,
	};

	CCollision();
	void Init(class CLayers *pLayers);
	int CheckPoint(float x, float y) const { return SolidState(round(x), round(y)); }
	int CheckPoint(vec2 Pos) const { return CheckPoint(Pos.x, Pos.y); }
	int CheckPointNoHook(float x, float y, bool* nohook) const { return SolidState(round(x), round(y), nohook); }
	int CheckPointNoHook(vec2 Pos, bool* nohook) const { return CheckPointNoHook(Pos.x, Pos.y, nohook); }

	int GetCollisionAt(float x, float y) const { return GetTile(round(x), round(y)); }
	int GetWidth() const { return m_Width; };
	int GetHeight() const { return m_Height; };
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision) const;
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces) const;
	void MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity, bool check_speed) const;
	int TestBox(vec2 Pos, vec2 Size) const;
};

#endif
