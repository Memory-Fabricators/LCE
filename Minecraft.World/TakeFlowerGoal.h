#pragma once

#include "Goal.h"

class Villager;
class VillagerGolem;

class TakeFlowerGoal : public Goal
{
  private:
    Villager *villager;
    weak_ptr<VillagerGolem> golem;
    int pickupTick;
    bool takeFlower;

  public:
    TakeFlowerGoal(Villager *villager);

    bool canUse();
    bool canContinueToUse();
    void start();
    void stop();
    void tick();
};
