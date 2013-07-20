#ifndef SCONCE_H
#define SCONCE_H

#include "LightSource.h"
#include "CreatableThing.h"

/// A mounted torch.
class Sconce :
  public LightSource,
  public CreatableThing<Sconce>
{
  friend class CreatableThing;

  public:
    virtual ~Sconce();

    // Thing overrides
    virtual std::string get_description() const override;
    virtual sf::Vector2u get_tile_sheet_coords(int frame) const override;

    virtual bool do_process() override;

  protected:
    Sconce();

  private:
    static Sconce prototype;
};

#endif // LIGHTORB_H
