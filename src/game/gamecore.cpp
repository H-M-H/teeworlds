/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "gamecore.h"

#include <iostream>

const char *CTuningParams::m_apNames[] =
{
	#define MACRO_TUNING_PARAM(Name,ScriptName,Value) #ScriptName,
	#include "tuning.h"
	#undef MACRO_TUNING_PARAM
};


bool CTuningParams::Set(int Index, float Value)
{
	if(Index < 0 || Index >= Num())
		return false;
	((CTuneParam *)this)[Index] = Value;
	return true;
}

bool CTuningParams::Get(int Index, float *pValue) const
{
	if(Index < 0 || Index >= Num())
		return false;
	*pValue = (float)((CTuneParam *)this)[Index];
	return true;
}

bool CTuningParams::Set(const char *pName, float Value)
{
	for(int i = 0; i < Num(); i++)
		if(str_comp_nocase(pName, m_apNames[i]) == 0)
			return Set(i, Value);
	return false;
}

bool CTuningParams::Get(const char *pName, float *pValue) const
{
	for(int i = 0; i < Num(); i++)
		if(str_comp_nocase(pName, m_apNames[i]) == 0)
			return Get(i, pValue);

	return false;
}

float HermiteBasis1(float v)
{
	return 2*v*v*v - 3*v*v+1;
}

float VelocityRamp(float Value, float Start, float Range, float Curvature)
{
	if(Value < Start)
		return 1.0f;
	return 1.0f/powf(Curvature, (Value-Start)/Range);
}

void CCharacterCore::Init(CWorldCore *pWorld, CCollision *pCollision)
{
	m_pWorld = pWorld;
	m_pCollision = pCollision;
}

void CCharacterCore::Reset()
{
	m_Pos = vec2(0,0);
	m_Vel = vec2(0,0);
	m_HookPos = vec2(0,0);
	m_HookDir = vec2(0,0);
	m_HookTick = 0;
	m_HookState = HOOK_IDLE;
	m_HookedPlayer = -1;
	m_Jumped = 0;
	m_Sliding = 0;
	m_TriggeredEvents = 0;
}

bool CCharacterCore::IsGrounded() {
	float PhysSize = 28.0f;

	for(int i = -PhysSize/2; i <= PhysSize/2; i++) {
		if(m_pCollision->CheckPoint(m_Pos.x+i, m_Pos.y+PhysSize/2+5)) {
			return true;
		}
	}

	return false;
}

int CCharacterCore::SlopeState(bool* nohook) {
	float PhysSize = 28.0f;

	int tmp = 0;
	int height_left = 0;
	bool is_nohook = false;
	bool tmp_nohook = false;
	for(int x = -1; x <= -1; x++)
		for(int y = 0; y <= 4; y++)
			if ( (tmp = m_pCollision->CheckPointNoHook(m_Pos.x-PhysSize/2+x, m_Pos.y+PhysSize/2+y, &tmp_nohook)) > height_left) {
				height_left = tmp;
				is_nohook = tmp_nohook;
			}
	if(nohook)
		*nohook = is_nohook;


	int height_right = 0;
	for(int x = 1; x <= 1; x++)
		for(int y = 0; y <= 4; y++)
			if ( (tmp = m_pCollision->CheckPointNoHook(m_Pos.x+PhysSize/2+x, m_Pos.y+PhysSize/2+y, &tmp_nohook)) > height_right) {
				height_right = tmp;
				is_nohook = tmp_nohook;
			}

	if(nohook && !*nohook)
		*nohook = is_nohook;
// 	if ( (tmp = m_pCollision->CheckPoint(m_Pos.x-PhysSize/2-1, m_Pos.y+PhysSize/2+1)) > height_left)
// 		height_left = tmp;
// 	if ( (tmp = m_pCollision->CheckPoint(m_Pos.x-PhysSize/2, m_Pos.y+PhysSize/2+1)) > height_left)
// 		height_left = tmp;
//
// 	int height_right = max(m_pCollision->CheckPoint(m_Pos.x+PhysSize/2+1, m_Pos.y+PhysSize/2), m_pCollision->CheckPoint(m_Pos.x+PhysSize/2, m_Pos.y+PhysSize/2));
// 	if ( (tmp = m_pCollision->CheckPoint(m_Pos.x+PhysSize/2, m_Pos.y+PhysSize/2)) > height_right)
// 		height_right = tmp;
// 	if ( (tmp = m_pCollision->CheckPoint(m_Pos.x+PhysSize/2+1, m_Pos.y+PhysSize/2+1)) > height_right)
// 		height_right = tmp;
// 	if ( (tmp = m_pCollision->CheckPoint(m_Pos.x+PhysSize/2, m_Pos.y+PhysSize/2+1)) > height_right)
// 		height_right = tmp;
	//left hand side
	/*for(int j = 0; j <= 28; j++) {
		if(m_pCollision->CheckPoint(m_Pos.x-PhysSize/2-1, m_Pos.y+PhysSize/2+j)) {
			height_left = j;
			break;
		}
	}

	for(int j = 0; j <=28; j++) {
		if(m_pCollision->CheckPoint(m_Pos.x+PhysSize/2+1, m_Pos.y+PhysSize/2+j)) {
			height_right = j;
			break;
		}
	}*/
	//return 1;
	//std::cerr << "HEIGHTS: " << height_left << " - " << height_right << std::endl;
	if(height_left == CCollision::SS_COL_RL) {
		//std::cerr << "RET 1" << std::endl;
		return 1;
	}
	else if (height_right == CCollision::SS_COL_RR) {
		//std::cerr << "RET -1" << std::endl;
		return -1;
	}
	//std::cerr << "RET 0" << std::endl;
	return 0;
}

void CCharacterCore::Tick(bool UseInput)
{
	float PhysSize = 28.0f;
	m_TriggeredEvents = 0;

	// get ground state
	bool Grounded = IsGrounded();
	bool nohook;
	int slope = SlopeState(&nohook);

	//std::cerr << "Nohook " << nohook << std::endl;


	/*if(m_pCollision->CheckPoint(m_Pos.x+PhysSize/2, m_Pos.y+PhysSize/2+5))
		Grounded = true;
	if(m_pCollision->CheckPoint(m_Pos.x-PhysSize/2, m_Pos.y+PhysSize/2+5))
		Grounded = true;*/

	vec2 TargetDirection = normalize(vec2(m_Input.m_TargetX, m_Input.m_TargetY));

	m_Vel.y += m_pWorld->m_Tuning.m_Gravity;

	float MaxSpeed = Grounded ? m_pWorld->m_Tuning.m_GroundControlSpeed : m_pWorld->m_Tuning.m_AirControlSpeed;

	float Accel = Grounded ? m_pWorld->m_Tuning.m_GroundControlAccel : m_pWorld->m_Tuning.m_AirControlAccel;
	float Friction = Grounded ? m_pWorld->m_Tuning.m_GroundFriction : m_pWorld->m_Tuning.m_AirFriction;
	float SlideFriction = m_pWorld->m_Tuning.m_SlideFriction;

	float SlideSlopeAcceleration = m_pWorld->m_Tuning.m_SlideSlopeAcceleration;
	float SlopeDeceleration = m_pWorld->m_Tuning.m_SlopeDeceleration;
	float SlopeAscendingControlSpeed = m_pWorld->m_Tuning.m_SlopeAscendingControlSpeed*invsqrt2;
	float SlopeDescendingControlSpeed = m_pWorld->m_Tuning.m_SlopeDescendingControlSpeed*invsqrt2;
	float SlideControlSpeed = m_pWorld->m_Tuning.m_SlideControlSpeed*invsqrt2;

	float SlideActivationSpeed = m_pWorld->m_Tuning.m_SlideActivationSpeed;
	// handle input
	if(UseInput)
	{
		m_Direction = m_Input.m_Direction;

		// setup angle
		float a = 0;
		if(m_Input.m_TargetX == 0)
			a = atanf((float)m_Input.m_TargetY);
		else
			a = atanf((float)m_Input.m_TargetY/(float)m_Input.m_TargetX);

		if(m_Input.m_TargetX < 0)
			a = a+pi;

		m_Angle = (int)(a*256.0f);

		// handle jump
		if(m_Input.m_Jump)
		{
			if(!(m_Jumped&1))
			{
				if(Grounded)
				{
					m_TriggeredEvents |= COREEVENTFLAG_GROUND_JUMP;
					if(slope == 0)
						m_Vel.y = -m_pWorld->m_Tuning.m_GroundJumpImpulse;
					else {
						m_Vel.y = -m_pWorld->m_Tuning.m_GroundJumpImpulse*invsqrt2;
					}

					m_Jumped |= 1;
				}
				else if(!(m_Jumped&2))
				{
					m_TriggeredEvents |= COREEVENTFLAG_AIR_JUMP;
					m_Vel.y = -m_pWorld->m_Tuning.m_AirJumpImpulse;
					m_Jumped |= 3;
				}
			}
		}
		else
			m_Jumped &= ~1;

		// handle hook
		if(m_Input.m_Hook)
		{
			if(m_HookState == HOOK_IDLE)
			{
				m_HookState = HOOK_FLYING;
				m_HookPos = m_Pos+TargetDirection*PhysSize*1.5f;
				m_HookDir = TargetDirection;
				m_HookedPlayer = -1;
				m_HookTick = 0;
				//m_TriggeredEvents |= COREEVENTFLAG_HOOK_LAUNCH;
			}
		}
		else
		{
			m_HookedPlayer = -1;
			m_HookState = HOOK_IDLE;
			m_HookPos = vec2(0,0);
			m_HookDir = vec2(0,0);
			m_HookTick = 0;
		}

		m_Sliding = m_Input.m_Slide;

	}

	if(nohook && slope != 0 && m_pWorld->m_Tuning.m_NoHookAutoSlide)
		m_Sliding = true;

	if( (m_Vel.x > SlideActivationSpeed && slope == 1) || (m_Vel.x < -SlideActivationSpeed && slope == -1)) {
		m_Sliding = true;
	}

	if(slope != 0 && m_Direction == slope && !m_Sliding)
		MaxSpeed = SlopeDescendingControlSpeed;
	else if(slope != 0 && m_Direction == -slope && !m_Sliding) {
		MaxSpeed = SlopeAscendingControlSpeed;
		float diff = SlopeDeceleration*fabs(m_Vel.x - MaxSpeed);
		/*if(m_Vel.x > MaxSpeed)
			m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Vel.x, -SlopeDeceleration);
		if(m_Vel.x < -MaxSpeed)
			m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Vel.x, SlopeDeceleration);*/
		if(m_Vel.x > MaxSpeed)
			m_Vel.x -= diff;
		if(m_Vel.x < -MaxSpeed)
			m_Vel.x += diff;
	}
	// add the speed modification according to players wanted direction
	if(m_Direction < 0 && (!m_Sliding || !Grounded)) {
		m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Vel.x, -Accel);
	}
	if(m_Direction > 0 && (!m_Sliding || !Grounded)) {

		m_Vel.x = SaturatedAdd(-MaxSpeed, MaxSpeed, m_Vel.x, Accel);
	}


	if(m_Sliding && slope != 0) {
		//std::cerr << "Fric1..." << m_Pos.x << std::endl;
		m_Vel.x = SaturatedAdd(-SlideControlSpeed, SlideControlSpeed, m_Vel.x, slope*SlideSlopeAcceleration);
		//m_Vel.y = slope*SaturatedAdd(-SlideControlSpeed, SlideControlSpeed, m_Vel.y, slope*SlideSlopeAcceleration);
	}
	else if(m_Sliding && Grounded) {
		//std::cerr << "Fric2..." << m_Pos.x << ", " << SlideFriction << std::endl;
		m_Vel.x *= SlideFriction;
	}
	else if(m_Direction == 0) {
		//std::cerr << "Fric3..." << m_Pos.x << std::endl;
		if(slope != 0 && !m_Jumped) {
			m_Vel.x *= Friction;// /invsqrt2;
			m_Vel.y *= Friction;// /invsqrt2;
		} else {
			m_Vel.x *= Friction;
		}
	}



	/*if(Slope == -1 && m_Vel.x < 1) {
		//std::cerr << "SLOPE -1" << std::endl;
		m_Vel.x -= m_pWorld->m_Tuning.m_Gravity;
		m_Vel.y += m_pWorld->m_Tuning.m_Gravity;
	}
	else if(Slope == 1 && m_Vel.x > -1) {
		//std::cerr << "SLOPE 1" << std::endl;
		m_Vel.x += m_pWorld->m_Tuning.m_Gravity;
		m_Vel.y += m_pWorld->m_Tuning.m_Gravity;
	}*/
	// handle jumping
	// 1 bit = to keep track if a jump has been made on this input
	// 2 bit = to keep track if a air-jump has been made
	if(Grounded)
		m_Jumped &= ~2;

	// do hook
	if(m_HookState == HOOK_IDLE)
	{
		m_HookedPlayer = -1;
		m_HookState = HOOK_IDLE;
		m_HookPos = m_Pos;
	}
	else if(m_HookState >= HOOK_RETRACT_START && m_HookState < HOOK_RETRACT_END)
	{
		m_HookState++;
	}
	else if(m_HookState == HOOK_RETRACT_END)
	{
		m_HookState = HOOK_RETRACTED;
		//m_TriggeredEvents |= COREEVENTFLAG_HOOK_RETRACT;
		m_HookState = HOOK_RETRACTED;
	}
	else if(m_HookState == HOOK_FLYING)
	{
		vec2 NewPos = m_HookPos+m_HookDir*m_pWorld->m_Tuning.m_HookFireSpeed;
		if(distance(m_Pos, NewPos) > m_pWorld->m_Tuning.m_HookLength)
		{
			m_HookState = HOOK_RETRACT_START;
			NewPos = m_Pos + normalize(NewPos-m_Pos) * m_pWorld->m_Tuning.m_HookLength;
		}

		// make sure that the hook doesn't go though the ground
		bool GoingToHitGround = false;
		bool GoingToRetract = false;
		int Hit = m_pCollision->IntersectLine(m_HookPos, NewPos, &NewPos, 0);
		if(Hit)
		{
			if(Hit&CCollision::COLFLAG_NOHOOK)
				GoingToRetract = true;
			else
				GoingToHitGround = true;
		}

		// Check against other players first
		if(m_pWorld && m_pWorld->m_Tuning.m_PlayerHooking)
		{
			float Distance = 0.0f;
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				CCharacterCore *pCharCore = m_pWorld->m_apCharacters[i];
				if(!pCharCore || pCharCore == this)
					continue;

				vec2 ClosestPoint = closest_point_on_line(m_HookPos, NewPos, pCharCore->m_Pos);
				if(distance(pCharCore->m_Pos, ClosestPoint) < PhysSize+2.0f)
				{
					if (m_HookedPlayer == -1 || distance(m_HookPos, pCharCore->m_Pos) < Distance)
					{
						m_TriggeredEvents |= COREEVENTFLAG_HOOK_ATTACH_PLAYER;
						m_HookState = HOOK_GRABBED;
						m_HookedPlayer = i;
						Distance = distance(m_HookPos, pCharCore->m_Pos);
					}
				}
			}
		}

		if(m_HookState == HOOK_FLYING)
		{
			// check against ground
			if(GoingToHitGround)
			{
				m_TriggeredEvents |= COREEVENTFLAG_HOOK_ATTACH_GROUND;
				m_HookState = HOOK_GRABBED;
			}
			else if(GoingToRetract)
			{
				m_TriggeredEvents |= COREEVENTFLAG_HOOK_HIT_NOHOOK;
				m_HookState = HOOK_RETRACT_START;
			}

			m_HookPos = NewPos;
		}
	}

	if(m_HookState == HOOK_GRABBED)
	{
		if(m_HookedPlayer != -1)
		{
			CCharacterCore *pCharCore = m_pWorld->m_apCharacters[m_HookedPlayer];
			if(pCharCore)
				m_HookPos = pCharCore->m_Pos;
			else
			{
				// release hook
				m_HookedPlayer = -1;
				m_HookState = HOOK_RETRACTED;
				m_HookPos = m_Pos;
			}

			// keep players hooked for a max of 1.5sec
			//if(Server()->Tick() > hook_tick+(Server()->TickSpeed()*3)/2)
				//release_hooked();
		}

		// don't do this hook rutine when we are hook to a player
		if(m_HookedPlayer == -1 && distance(m_HookPos, m_Pos) > 46.0f)
		{
			vec2 HookVel = normalize(m_HookPos-m_Pos)*m_pWorld->m_Tuning.m_HookDragAccel;
			// the hook as more power to drag you up then down.
			// this makes it easier to get on top of an platform
			if(HookVel.y > 0)
				HookVel.y *= 0.3f;

			// the hook will boost it's power if the player wants to move
			// in that direction. otherwise it will dampen everything abit
			if((HookVel.x < 0 && m_Direction < 0) || (HookVel.x > 0 && m_Direction > 0))
				HookVel.x *= 0.95f;
			else
				HookVel.x *= 0.75f;

			vec2 NewVel = m_Vel+HookVel;

			// check if we are under the legal limit for the hook
			if(length(NewVel) < m_pWorld->m_Tuning.m_HookDragSpeed || length(NewVel) < length(m_Vel))
				m_Vel = NewVel; // no problem. apply

		}

		// release hook (max hook time is 1.25
		m_HookTick++;
		if(m_HookedPlayer != -1 && (m_HookTick > SERVER_TICK_SPEED+SERVER_TICK_SPEED/5 || !m_pWorld->m_apCharacters[m_HookedPlayer]))
		{
			m_HookedPlayer = -1;
			m_HookState = HOOK_RETRACTED;
			m_HookPos = m_Pos;
		}
	}

	if(m_pWorld)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			CCharacterCore *pCharCore = m_pWorld->m_apCharacters[i];
			if(!pCharCore)
				continue;

			//player *p = (player*)ent;
			if(pCharCore == this) // || !(p->flags&FLAG_ALIVE)
				continue; // make sure that we don't nudge our self

			// handle player <-> player collision
			float Distance = distance(m_Pos, pCharCore->m_Pos);
			vec2 Dir = normalize(m_Pos - pCharCore->m_Pos);
			if(m_pWorld->m_Tuning.m_PlayerCollision && Distance < PhysSize*1.25f && Distance > 0.0f)
			{
				float a = (PhysSize*1.45f - Distance);
				float Velocity = 0.5f;

				// make sure that we don't add excess force by checking the
				// direction against the current velocity. if not zero.
				if (length(m_Vel) > 0.0001)
					Velocity = 1-(dot(normalize(m_Vel), Dir)+1)/2;

				m_Vel += Dir*a*(Velocity*0.75f);
				m_Vel *= 0.85f;
			}

			// handle hook influence
			if(m_HookedPlayer == i && m_pWorld->m_Tuning.m_PlayerHooking)
			{
				if(Distance > PhysSize*1.50f) // TODO: fix tweakable variable
				{
					float Accel = m_pWorld->m_Tuning.m_HookDragAccel * (Distance/m_pWorld->m_Tuning.m_HookLength);
					float DragSpeed = m_pWorld->m_Tuning.m_HookDragSpeed;

					// add force to the hooked player
					pCharCore->m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, pCharCore->m_Vel.x, Accel*Dir.x*1.5f);
					pCharCore->m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, pCharCore->m_Vel.y, Accel*Dir.y*1.5f);

					// add a little bit force to the guy who has the grip
					m_Vel.x = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.x, -Accel*Dir.x*0.25f);
					m_Vel.y = SaturatedAdd(-DragSpeed, DragSpeed, m_Vel.y, -Accel*Dir.y*0.25f);
				}
			}
		}
	}

	// clamp the velocity to something sane
	if(length(m_Vel) > 6000)
		m_Vel = normalize(m_Vel) * 6000;
}

void CCharacterCore::Move()
{
	//int slope = SlopeState();


	if(!m_pWorld)
		return;

	float RampValue = VelocityRamp(length(m_Vel)*50, m_pWorld->m_Tuning.m_VelrampStart, m_pWorld->m_Tuning.m_VelrampRange, m_pWorld->m_Tuning.m_VelrampCurvature);
	//if( (slope == -1 && m_Vel.x < 0) || (slope == 1 && m_Vel.x > 0))
	//	RampValue = VelocityRamp(length(m_Vel)*20, m_pWorld->m_Tuning.m_VelrampStart, m_pWorld->m_Tuning.m_VelrampRange, m_pWorld->m_Tuning.m_VelrampCurvature);

	//std::cerr << "->VelX: " << m_Vel.x << " - " << m_Vel.y << std::endl;

	m_Vel.x = m_Vel.x*RampValue;

	vec2 NewPos = m_Pos;

	//bool switched = true;
	/*if(slope == -1 && m_Vel.x < 0 && m_Vel.y >= 0) {
		m_Vel.x *= invsqrt2;
		m_Vel.y = -m_Vel.x;
	}
	else if(slope == 1 && m_Vel.x > 0 && m_Vel.y >= 0) {
		m_Vel.x *= invsqrt2;
		m_Vel.y = m_Vel.x;
	}
	else if(slope == -1 && m_Vel.x > 0 && m_Vel.y >= 0) {
		m_Vel.x *= invsqrt2;
		m_Vel.y = -m_Vel.x;
	}
	else if(slope == 1 && m_Vel.x < 0 && m_Vel.y >= 0) {
		m_Vel.x *= invsqrt2;
		m_Vel.y = m_Vel.x;
	}
	else*/
	//	switched = false;

	m_pCollision->MoveBox(&NewPos, &m_Vel, vec2(28.0f, 28.0f), 0, !m_Sliding);

	//std::cerr << "ColX: " << m_Vel.x << " - " << m_Vel.y << std::endl;
	m_Vel.x = m_Vel.x*(1.0f/RampValue);
	//std::cerr << "RamX: " << m_Vel.x << " - " << m_Vel.y << std::endl;

	if(m_pWorld->m_Tuning.m_PlayerCollision)
	{
		// check player collision
		float Distance = distance(m_Pos, NewPos);
		int End = Distance+1;
		vec2 LastPos = m_Pos;
		for(int i = 0; i < End; i++)
		{
			float a = i/Distance;
			vec2 Pos = mix(m_Pos, NewPos, a);
			for(int p = 0; p < MAX_CLIENTS; p++)
			{
				CCharacterCore *pCharCore = m_pWorld->m_apCharacters[p];
				if(!pCharCore || pCharCore == this)
					continue;
				float D = distance(Pos, pCharCore->m_Pos);
				if(D < 28.0f && D > 0.0f)
				{
					if(a > 0.0f)
						m_Pos = LastPos;
					else if(distance(NewPos, pCharCore->m_Pos) > D)
						m_Pos = NewPos;
					return;
				}
			}
			LastPos = Pos;
		}
	}

	m_Pos = NewPos;

	//slope = SlopeState();
	//if(switched) {
	//	m_Vel.x /= invsqrt2;
	//	m_Vel.y = 0;
	//}
	/*if(slope == -1 && m_Vel.x < 0 && m_Vel.y >= 0) {
		m_Vel.x /= 0.7071067811865475244f;
		m_Vel.y = 0;
	}
	else if(slope == 1 && m_Vel.x > 0 && m_Vel.y >= 0) {
		m_Vel.x /= 0.7071067811865475244f;
		m_Vel.y = 0;
	}
	else if(slope == -1 && m_Vel.x > 0 && m_Vel.y >= 0) {
		m_Vel.x /= 0.7071067811865475244f;
		m_Vel.y = 0;

	}
	else if(slope == 1 && m_Vel.x < 0 && m_Vel.y >= 0) {
		m_Vel.x /= 0.7071067811865475244f;
		m_Vel.y = 0;
	}*/



	//std::cerr << "EndX: " << m_Vel.x << " - " << m_Vel.y << std::endl;
}

void CCharacterCore::Write(CNetObj_CharacterCore *pObjCore)
{
	pObjCore->m_X = round_to_int(m_Pos.x);
	pObjCore->m_Y = round_to_int(m_Pos.y);

	pObjCore->m_VelX = round_to_int(m_Vel.x*256.0f);
	pObjCore->m_VelY = round_to_int(m_Vel.y*256.0f);
	pObjCore->m_HookState = m_HookState;
	pObjCore->m_HookTick = m_HookTick;
	pObjCore->m_HookX = round_to_int(m_HookPos.x);
	pObjCore->m_HookY = round_to_int(m_HookPos.y);
	pObjCore->m_HookedPlayer = m_HookedPlayer;
	pObjCore->m_Jumped = m_Jumped;
	pObjCore->m_Direction = m_Direction;
	pObjCore->m_Angle = m_Angle;
	pObjCore->m_Sliding = m_Sliding;
	pObjCore->m_Grounded = IsGrounded();
	pObjCore->m_Slope = SlopeState();
}

void CCharacterCore::Read(const CNetObj_CharacterCore *pObjCore)
{
	m_Pos.x = pObjCore->m_X;
	m_Pos.y = pObjCore->m_Y;
	m_Vel.x = pObjCore->m_VelX/256.0f;
	m_Vel.y = pObjCore->m_VelY/256.0f;
	m_HookState = pObjCore->m_HookState;
	m_HookTick = pObjCore->m_HookTick;
	m_HookPos.x = pObjCore->m_HookX;
	m_HookPos.y = pObjCore->m_HookY;
	m_HookDir = normalize(m_HookPos-m_Pos);
	m_HookedPlayer = pObjCore->m_HookedPlayer;
	m_Jumped = pObjCore->m_Jumped;
	m_Direction = pObjCore->m_Direction;
	m_Angle = pObjCore->m_Angle;
	m_Sliding = pObjCore->m_Sliding;
}

void CCharacterCore::Quantize()
{
	CNetObj_CharacterCore Core;
	Write(&Core);
	Read(&Core);
}
