#ifndef GUILABEL_H
#define GUILABEL_H

#include "stdafx.h"

#include "GUIObject.h"

namespace metagui
{
  class Label :
    public Object, public ObjectVisitable<Label>
  {
  public:
    explicit Label(std::string name, IntegerVec2 location = IntegerVec2(0, 0));
    virtual ~Label();

  protected:
    virtual void drawPreChildren_(sf::RenderTexture& texture, int frame) override final;

  private:
  };
}; // end namespace metagui

#endif // GUILABEL_H
