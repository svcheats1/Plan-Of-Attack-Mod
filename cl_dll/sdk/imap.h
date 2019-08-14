#ifndef I_MAP_H
#define I_MAP_H
#ifdef _WIN32
#pragma once
#endif

#include <vector.h>
#include <vector2d.h>

/**
* Declaration of an interface for an in-game map
**/
class IMap : public IGameEventListener2
{
public:
	virtual	~IMap(void) {};

	// accessors
	virtual void SetZoom(float fZoom) = 0; // set zoom
	virtual void SetTime(float fTime) = 0; // set game time
	virtual void SetAngle(float fAngle) = 0; // set map orientation
	virtual void SetFollowEntity(bool bState) = 0; // if true, map follows with spectators view
	virtual void SetCenter(Vector2D &vecMapPos) = 0; // set map pos in center of panel
	virtual void SetPlayerPosition(int iIndex, const Vector &vecPos, const QAngle &qaAngle) = 0; // update player position
	virtual float GetZoom(void)= 0;

	// helpers
	virtual Vector2D WorldToMap(const Vector &vecWorldPos) = 0; // convert 3d world to 2d map pos

	// events
	virtual void FireGameEvent(IGameEvent *pEvent) = 0;

protected:
	static IMap *s_pMap;
};

#endif