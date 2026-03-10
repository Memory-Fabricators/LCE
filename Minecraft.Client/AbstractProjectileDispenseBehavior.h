#pragma once

#include "BlockSource.h"
#include "DefaultDispenseItemBehavior.h"
#include "ItemInstance.h"
#include "Minecraft.World/DefaultDispenseItemBehavior.h"

class Projectile;

class AbstractProjectileDispenseBehavior : public DefaultDispenseItemBehavior
{
  public:
    std::shared_ptr<ItemInstance> execute(BlockSource *source, std::shared_ptr<ItemInstance> dispensed);

  protected:
    virtual void playSound(BlockSource *source);
    virtual std::shared_ptr<Projectile> getProjectile(Level *world, Position *position) = 0;
    virtual float getUncertainty();
    virtual float getPower();
};
