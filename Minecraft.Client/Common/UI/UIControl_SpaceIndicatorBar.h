#pragma once

#include "UIControl_Base.h"

class UIControl_SpaceIndicatorBar : public UIControl_Base
{
  private:
    IggyName m_setSaveSizeFunc, m_setTotalSizeFunc, m_setSaveGameOffsetFunc;
    std::int64_t m_min;
    std::int64_t m_max;
    std::int64_t m_currentSave, m_currentTotal;
    float m_currentOffset;

    vector<pair<std::int64_t, float>> m_sizeAndOffsets;

  public:
    UIControl_SpaceIndicatorBar();

    virtual bool setupControl(UIScene *scene, IggyValuePath *parent, const string &controlName);

    void init(const wstring &label, int id, std::int64_t min, std::int64_t max);
    virtual void ReInit();
    void reset();

    void addSave(std::int64_t size);
    void selectSave(int index);

  private:
    void setSaveSize(std::int64_t size);
    void setTotalSize(std::int64_t totalSize);
    void setSaveGameOffset(float offset);
};
