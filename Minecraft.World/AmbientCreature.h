#pragma once

#include "Creature.h"
#include "Mob.h"

class AmbientCreature : public Mob, public Creature
{

  public:
    AmbientCreature(Level *level);

    virtual bool canBeLeashed();

  protected:
    virtual bool mobInteract(shared_ptr<Player> player);
};
