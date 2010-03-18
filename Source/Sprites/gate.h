/*
 * Filename      : gate.h
 * Author(s)     : Matt Zweig
 * Date Created  : Tuesday, March 16, 2010
 * Last Modified : Tuesday, March 16, 2010
 * Purpose       : Sprite SubClass for Warp Gates
 * Notes         : A gate is a two-part Sprite that ships can move through
 */

#ifndef __H_EFFECT__
#define __H_EFFECT__

#include "Graphics/animation.h"
#include "Sprites/sprite.h"
#include "Graphics/image.h"
#include "includes.h"

#define GATE_RADIUS 20000

class Gate : public Sprite {
	public:
		Gate(Coordinate pos);

		virtual int GetDrawOrder( void ) {
			return( top? DRAW_ORDER_GATE_TOP : DRAW_ORDER_GATE_BOTTOM );
		}

		// These override the normal Sprite Setters
		// They modify both this and the partner Gate at the same time
		void SetAngle(float angle);
		void SetWorldPosition(Coordinate pos);

		Gate* GetTop();

		void Update();
	private:
		bool top;
		int partnerID;

		void SendToRandomLocation(Sprite* ship);
		void SendRandomDistance(Sprite* ship);

		Gate(int topID);
		// These setters modify this and only this Gate (not the partner)
		void _SetAngle(float angle) { Sprite::SetAngle(angle); }
		void _SetWorldPosition(Coordinate c) { Sprite::SetWorldPosition(c); }
		Gate* GetPartner();
};

#endif // __H_EFFECT__
