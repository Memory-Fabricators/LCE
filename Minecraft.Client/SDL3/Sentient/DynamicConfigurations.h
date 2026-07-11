#pragma once

#define DYNAMIC_CONFIG_TRIAL_ID 0
#define DYNAMIC_CONFIG_TRIAL_VERSION 1
#define DYNAMIC_CONFIG_DEFAULT_TRIAL_TIME 2400

class MinecraftDynamicConfigurations
{
  public:
    static DWORD GetTrialTime();
};
