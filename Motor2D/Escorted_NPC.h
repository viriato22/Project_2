#ifndef _ESCORTED_NPC_H_
#define _ESCORTED_NPC_H_

#include "Unit.h"
#define	COLLISION_ADJUSTMENT iPoint(0, 0)

enum entity_type;

class EscortedNPC : public Unit
{
public:
	EscortedNPC(entity_type _type);
	~EscortedNPC();

	// Load animations
	bool LoadEntity(iPoint pos, entity_name name);

public:
	iPoint mouse_position;
};

#endif
