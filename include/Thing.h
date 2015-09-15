#ifndef THING_H
#define THING_H

#include "Action.h"
#include "ActionResult.h"
#include "AttributeSet.h"
#include "BodyPart.h"
#include "Direction.h"
#include "ErrorHandler.h"
#include "GameObject.h"
#include "Gender.h"
#include "MapId.h"
#include "MapTileType.h"
#include "ThingMetadata.h"

#include <memory>
#include <string>
#include <set>
#include <boost/dynamic_bitset.hpp>
#include <boost/optional.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/serialization/strong_typedef.hpp>
#include <SFML/Graphics.hpp>

// Forward declarations
class AIStrategy;
class Entity;
class MapTile;
class Inventory;
class ThingFactory;
class ThingRef;
class Thing;

// Using Declarations

/// Typedef for the factory method.
using ThingCreator = std::shared_ptr<Thing>(*)(void);

// Thing is any object in the game, animate or not.
class Thing :
  public GameObject,
  public std::enable_shared_from_this<Thing>
{
  friend class boost::object_pool < Thing >;
  friend class AIStrategy;
  friend class ThingFactory;
  friend class ThingManager;

  public:
    virtual ~Thing();

    /// Queue an action for this Entity to perform.
    void queue_action(Action action);

    /// Return whether this is an action pending for this Entity.
    bool pending_action() const;

    /// Returns true if this thing is the current player.
    /// By default, returns false. Overridden by Entity class.
    virtual bool is_player() const;

    std::string const& get_type() const;

    /// Set the AI strategy associated with this Thing.
    /// The Thing assumes responsibility for maintenance of the new object.
    /// Any old strategy in use will be discarded.
    /// @note While it is possible to use the same AIStrategy instance with
    ///       more than one Thing, doing this with probably not result in the
    ///       desired behavior, as the AIStrategy may maintain state information
    ///       which would be shared across multiple Things.  However, this
    ///       MIGHT be desirable if dealing with an Thing type with a hive
    ///       mind (thus sharing all of its knowledge across the species).
    /// @param[in] strategy_ptr Pointer to a newly created AIStrategy object.
    /// @return True if strategy was set; false if not (e.g. if the pointer
    ///         passed in was not valid).
    bool set_ai_strategy(AIStrategy* strategy_ptr);

    /// Return whether a Thing is wielded by this Entity.
    /// This is used by InventoryArea to show wielded status.
    /// @param[in] thing Thing to check
    /// @return true if the Thing is wielded by the Entity.
    bool is_wielding(ThingRef thing);

    /// Return whether a Thing is wielded by this Entity.
    /// This is used by InventoryArea to show wielded status.
    /// @param[in] thing Thing to check
    /// @param[out] number Hand number it is wielded in.
    /// @return true if the Thing is wielded by the Entity.
    bool is_wielding(ThingRef thing, unsigned int& number);

    /// Return whether a Thing is equipped (worn) by this Entity.
    /// @param[in] thing Thing to check
    /// @return true if the Thing is being worn.
    bool has_equipped(ThingRef thing);

    /// Return whether a Thing is being worn by this Entity.
    /// @param[in] thing Thing to check
    /// @param[out] location of the worn Thing, if worn
    /// @return true if the Thing is being worn.
    bool has_equipped(ThingRef thing, WearLocation& location);

    /// Return whether a Thing is within reach of the Entity.
    /// @param[in] thing Thing to check
    /// @return true if the Thing is in the Entity's inventory or is at the
    ///         same location as the Entity, false otherwise.
    bool can_reach(ThingRef thing);

    /// Attempt to attack a thing.
    /// @param[in] thing Thing to attack.
    /// @param[out] action_time The time it took to attack it.
    /// @return true if attack was performed (whether it succeeded or not),
    ///         false if it wasn't performed
    bool do_attack(ThingRef thing, unsigned int& action_time);

    /// Return whether the Entity can drink the requested Thing.
    /// The base method checks to make sure the Thing is a liquid, but beyond
    /// that, assumes it can drink anything.
    /// It is recommended that any derived classes first call the base method
    /// before proceeding with their own checks.
    /// @param[in] thing Thing to try to drink
    /// @param[out] action_time The time it will take to drink it
    /// @return ActionResult indicating what happened.
    virtual ActionResult can_drink(ThingRef thing, unsigned int& action_time);

    /// Attempt to drink a thing.
    /// @param[in] thing Thing to try to drink
    /// @param[out] action_time The time it took to drink it.
    /// @return true if object is drank, false if not
    bool do_drink(ThingRef thing, unsigned int& action_time);

    /// Return whether the Entity can drop the requested Thing.
    /// The base method checks to make sure the Entity is actually holding the
    /// thing in its Inventory.
    /// It is recommended that any derived classes first call the base method
    /// before proceeding with their own checks.
    /// @param[in] thing Thing to try to drop
    /// @param[out] action_time The time it will take to drop it
    /// @return ActionResult indicating what happened.
    virtual ActionResult can_drop(ThingRef thing, unsigned int& action_time);

    /// Attampt to drop a thing.
    /// @param[in] thing Thing to try to drop
    /// @param[out] action_time The time it took to drop it.
    /// @return true if object is dropped, false if not
    bool do_drop(ThingRef thing, unsigned int& action_time);

    /// Return whether the Entity can eat the requested Thing.
    /// It is recommended that any derived classes first call the base method
    /// before proceeding with their own checks.
    /// @param[in] thing Thing to try to eat
    /// @param[out] action_time The time it will take to eat it
    /// @return ActionResult indicating what happened.
    virtual ActionResult can_eat(ThingRef thing, unsigned int& action_time);

    /// Attempt to eat a thing.
    bool do_eat(ThingRef thing, unsigned int& action_time);

    /// Return whether the Entity can mix these two Things.
    /// It is recommended that any derived classes first call the base method
    /// before proceeding with their own checks.
    /// @param[in] thing1 First thing to mix.
    /// @param[in] thing2 Second thing to mix.
    /// @param[out] action_time The time it will take to mix.
    /// @return ActionResult indicating what happened.
    virtual ActionResult can_mix(ThingRef thing1, ThingRef thing2, unsigned int& action_time);

    /// Attempt to mix two things.
    bool do_mix(ThingRef thing1, ThingRef thing2, unsigned int& action_time);

    /// Attempt to move in a particular direction.
    /// @param[in] direction Direction to move in
    /// @param[out] action_time The time it took to move
    /// @return true on success; false if the move was prevented.
    virtual bool do_move(Direction direction, unsigned int& action_time);

    /// Return whether the Entity can put thing into container.
    /// It is recommended that any derived classes first call the base method
    /// before proceeding with their own checks.
    /// @param[in] thing Thing to put in.
    /// @param[in] container Thing to put it into.
    /// @return ActionResult indicating what happened.
    virtual ActionResult can_put_into(ThingRef thing, ThingRef container, unsigned int& action_time);

    /// Attempt to put a thing into a container.
    bool do_put_into(ThingRef thing, ThingRef container, unsigned int& action_time);

    /// @todo The next three might be better as protected methods.
    FlagsMap const& get_property_flags() const;
    ValuesMap const& get_property_values() const;
    StringsMap const& get_property_strings() const;


    bool get_property_flag(std::string name, bool default_value = false) const;
    int get_property_value(std::string name, int default_value = 0) const;
    std::string get_property_string(std::string name, std::string default_value = "") const;
    bool get_intrinsic_flag(std::string name, bool default_value = false) const;
    int get_intrinsic_value(std::string name, int default_value = 0) const;
    std::string get_intrinsic_string(std::string name, std::string default_value = "") const;
    void set_property_flag(std::string name, bool value);
    void set_property_value(std::string name, int value);
    void set_property_string(std::string name, std::string value);

    /// Get the quantity this thing represents.
    virtual unsigned int get_quantity() const;

    /// Set the quantity this thing represents.
    void set_quantity(unsigned int quantity);

    /// Return a reference to this thing.
    ThingRef get_ref() const;

    /// Return the location of this thing.
    ThingRef get_location() const;

    /// Get the number of game cycles until this Entity can process a new
    /// command.
    int get_busy_counter() const;

    /// Traverse the line of sight to a map tile, setting visibility
    /// for the tiles between.
    /// @warning Assumes entity is on a map, and that the ending coordinates
    ///          are valid for the map the entity is on!
    void traverseLineOfSightTo(int xEnd, int yEnd);

    /// Return whether the Entity can see the requested Thing.
    bool can_see(ThingRef thing);

    /// Return whether the Entity can see the requested tile.
    bool can_see(sf::Vector2i coords);

    /// Return whether the Entity can see the requested tile.
    bool can_see(int x, int y);

    /// Find out which tiles on the map can be seen by this Entity.
    /// This method uses a recursive raycasting algorithm to figure out what
    /// can be seen at a particular position.
    void find_seen_tiles();

    /// Get the remembered tile type at the specified coordinates.
    MapTileType get_memory_at(int x, int y) const;

    /// Get the remembered tile type at the specified coordinates.
    MapTileType get_memory_at(sf::Vector2i coords) const;

    /// Add the memory of a particular tile to a VertexArray.
    void add_memory_vertices_to(sf::VertexArray& vertices, int x, int y);

    /// Check if the Entity can move in the specified direction.
    virtual bool can_move(Direction direction);

    /// Return whether the Entity can pick up the requested Thing.
    /// The base method checks to make sure the Thing is at the same location
    /// as the Entity, and that the Entity's inventory can contain it.
    /// It is recommended that any derived classes first call the base method
    /// before proceeding with their own checks.
    /// @param[in] thing Thing to try to pick up
    /// @param[out] action_time The time it will take to pick it up
    /// @return ActionResult indicating what happened.
    virtual ActionResult can_pick_up(ThingRef thing, unsigned int& action_time);

    /// Attempt to pick a thing up.
    /// @param[in] thing Thing to try to pick up
    /// @param[out] action_time The time it took to pick up
    /// @return true if object is picked up, false if not
    bool do_pick_up(ThingRef thing, unsigned int& action_time);

    /// Return whether the Entity can take a thing out of its container.
    /// It is recommended that any derived classes first call the base method
    /// before proceeding with their own checks.
    /// @param[in] thing Thing to take out.
    /// @return ActionResult indicating what happened.
    virtual ActionResult can_take_out(ThingRef thing, unsigned int& action_time);

    /// Attempt to take a thing out of its container.
    bool do_take_out(ThingRef thing_id, unsigned int& action_time);

    virtual ActionResult can_read(ThingRef thing_id, unsigned int& action_time);

    bool do_read(ThingRef thing_id, unsigned int& action_time);

    virtual ActionResult can_throw(ThingRef thing_id, unsigned int& action_time);

    /// Attempt to toss/throw a thing in a particular direction.
    bool do_throw(ThingRef thing_id, Direction& direction, unsigned int& action_time);

    virtual ActionResult can_deequip(ThingRef thing_id, unsigned int& action_time);

    /// Attempt to de-equip (remove) a thing.
    bool do_deequip(ThingRef thing_id, unsigned int& action_time);

    virtual ActionResult can_equip(ThingRef thing_id, unsigned int& action_time);

    /// Attempt to equip (wear) a thing.
    bool do_equip(ThingRef thing_id, unsigned int& action_time);

    virtual ActionResult can_wield(ThingRef thing_id, unsigned int hand, unsigned int& action_time);

    /// Attempt to wield a thing.
    /// @param[in] thing Thing to wield, or empty ptr if unwielding everything.
    /// @param[in] hand Hand to wield it in.
    /// @param[out] action_time Time it takes to wield.
    bool do_wield(ThingRef thing_id, unsigned int hand, unsigned int& action_time);

    /// Return whether this Entity can currently see.
    /// @todo Implement blindness counter, blindness due to wearing blindfold,
    ///       et cetera.
    virtual bool can_currently_see() const;

    /// Return whether this Entity can currently move.
    /// @todo Implement paralysis counter, and/or other reasons to be immobile.
    virtual bool can_currently_move() const;

    void set_gender(Gender gender);

    Gender get_true_gender() const;

    Gender get_gender() const;

    /// Get the number of a particular body part the Entity has.
    virtual unsigned int get_bodypart_number(BodyPart part) const;

    /// Get the appropriate body part name for the Entity.
    virtual std::string get_bodypart_name(BodyPart part) const;

    /// Get the appropriate body part plural for the Entity.
    virtual std::string get_bodypart_plural(BodyPart part) const;

    /// Get the appropriate description for a body part.
    /// This takes the body part name and the number referencing the particular
    /// part and comes up with a description.
    /// For example, for most creatures with two hands, hand #0 will be the
    /// "right hand" and hand #1 will be the "left hand".
    /// In most cases the default implementation here will work, but if a
    /// creature has (for example) a strange configuration of limbs this can be
    /// overridden.
    virtual std::string get_bodypart_description(BodyPart part, unsigned int number);

    /// Return the attribute set for this Entity.
    virtual AttributeSet& get_attributes();

    /// Return the attribute set for this Entity.
    virtual AttributeSet const& get_attributes() const;

    /// Returns a reference to the inventory.
    Inventory& get_inventory();

    /// Returns true if this thing is inside another Thing.
    bool is_inside_another_thing() const;

    /// Get the MapTile this thing is on, or nullptr if not on a map.
    MapTile* get_maptile() const;

    /// Return the MapId this Thing is currently on, or 0 if not on a map.
    MapId get_map_id() const;

    /// Set the direction the thing is facing.
    void set_facing_direction(Direction d);

    /// Get the direction the thing is facing.
    Direction get_facing_direction() const;

    /// Return this thing's description.
    /// Adds adjective qualifiers (such as "fireproof", "waterproof", etc.)
    /// @todo Add adjective qualifiers.s
    virtual std::string get_pretty_name() const override final;

    /// Return this object's plural.
    std::string get_pretty_plural() const;

    /// Get the thing's proper name (if any).
    std::string get_proper_name() const;

    /// Set this thing's proper name.
    void set_proper_name(std::string name);

    /// Return a string that identifies this thing.
    /// Returns "the/a/an" and a description of the thing, such as
    /// "the chair".
    /// If it is carried by the player, it'll return "your (thing)".
    /// If it IS the player, it'll return "you".
    /// Likewise, if it is carried by another Entity it'll return
    /// "(Entity)'s (thing)".
    /// @param definite   If true, uses definite articles. 
    ///                   If false, uses indefinite articles.
    ///                   Defaults to true.
    std::string get_identifying_string(bool definite = true) const;

    /// Return a string that identifies this thing without using possessives.
    /// The same as get_identifying_string, but without using any possessives.
    /// @param definite   If true, uses definite articles. 
    ///                   If false, uses indefinite articles.
    ///                   Defaults to true.
    std::string get_identifying_string_without_possessives(bool definite = true) const;

    /// Choose the proper possessive form
    /// For a Thing, this is simply "the", as Things cannot own things.
    /// This function checks to see if this Thing is currently designated as
    /// the player (in the ThingFactory).  If so, it returns "your".  If not,
    /// it returns get_name() + "'s".
    /// @note If you want a possessive pronoun like his/her/its/etc., use
    /// get_possessive_adjective().
    virtual std::string get_possessive() const;


    /// Choose which verb form to use based on first/second/third person.
    /// This function checks to see if this Thing is currently designated as
    /// the player (in the ThingFactory).  If so, it returns the string passed
    /// as verb2; otherwise, it returns the string passed as verb3.
    /// @param verb2 The second person verb form, such as "shake"
    /// @param verb3 The third person verb form, such as "shakes"
    std::string const& choose_verb(std::string const& verb2,
                                   std::string const& verb3) const;

    /// Return this thing's mass.
    virtual int get_mass() const;

    /// Get the appropriate subject pronoun for the Thing.
    virtual std::string const& get_subject_pronoun() const;

    /// Get the appropriate object pronoun for the Thing.
    virtual std::string const& get_object_pronoun() const;

    /// Get the appropriate reflexive pronoun for the Thing.
    virtual std::string const& get_reflexive_pronoun() const;

    /// Get the appropriate possessive adjective for the Thing.
    virtual std::string const& get_possessive_adjective() const;

    /// Get the appropriate possessive pronoun for the Thing.
    virtual std::string const& get_possessive_pronoun() const;

    /// Return the coordinates of the tile representing the thing.
    virtual sf::Vector2u get_tile_sheet_coords(int frame) const;

    /// Add this Thing to a VertexArray to be drawn.
    /// @param vertices Array to add vertices to.
    /// @param use_lighting If true, calculate lighting when adding.
    ///                     If false, store directly w/white bg color.
    /// @param frame Animation frame number.
    virtual void add_vertices_to(sf::VertexArray& vertices,
                                 bool use_lighting = true,
                                 int frame = 0) override;

    /// Draw this Thing onto a RenderTexture, at the specified coordinates.
    /// @param target Texture to draw onto.
    /// @param target_coords Coordinates to draw the Thing at.
    /// @param target_size Target size of thing, in pixels.
    /// @param use_lighting If true, calculate lighting when adding.
    ///                     If false, store directly w/white bg color.
    /// @param frame Animation frame number.
    void draw_to(sf::RenderTexture& target,
                 sf::Vector2f target_coords,
                 unsigned int target_size = 0,
                 bool use_lighting = true,
                 int frame = 0);

    /// Simple check to see if a Thing is opaque.
    virtual bool is_opaque() const;

    /// Provide light to this Thing's surroundings.
    /// If Thing is not opaque, calls light_up_surroundings() for each Thing
    /// in its inventory.
    virtual void light_up_surroundings();

    /// Receive light from the specified light source.
    /// The default behavior is to pass the light source to the location if
    /// this Thing is opaque.
    virtual void be_lit_by(ThingRef light);

    /// Attempt to destroy this Thing.
    virtual void destroy();

    /// Attempt to move this Thing into a location.
    virtual bool move_into(ThingRef new_location);

    /// Return whether or not this thing can move from its current location.
    /// The default behavior for this is to return true.
    virtual bool is_movable() const;

    /// Return whether or not this thing can be activated by this Entity.
    /// The default behavior for this is to return false.
    virtual bool is_usable_by(ThingRef thing) const;

    /// Return whether or not this thing can be drank by this Entity.
    /// The default behavior for this is to return false.
    virtual bool is_drinkable_by(ThingRef thing) const;

    /// Return whether or not this thing can be eaten by this Entity.
    /// The default behavior for this is to return false.
    virtual bool is_edible_by(ThingRef thing) const;

    /// Return whether or not this thing can be read by this Entity.
    /// The default behavior for this is to return false.
    virtual bool is_readable_by(ThingRef thing) const;

    /// Return whether or not this thing can be mixed with another Thing.
    /// The default behavior for this is to return false.
    virtual bool is_miscible_with(ThingRef thing) const;

    /// Return the body part this thing is equippable on.
    /// If thing is not equippable, return BodyPart::Count.
    virtual BodyPart is_equippable_on() const;

    /// Process this Thing and its inventory for a single tick.
    bool process();

    /// Perform an action when this thing is activated.
    /// If this function returns false, the action is aborted.
    /// The default behavior is to do nothing and return false.
    bool perform_action_activated_by(ThingRef thing);

    /// Perform an action when this thing collides with another thing.
    void perform_action_collided_with(ThingRef thing);

    /// Perform an action when this thing is eaten.
    /// If this function returns false, the action is aborted.
    /// The default behavior is to do nothing and return false.
    bool perform_action_drank_by(ThingRef thing);

    /// Perform an action when this thing is dropped.
    /// If this function returns false, the action is aborted.
    /// The default behavior is to do nothing and return true.
    bool perform_action_dropped_by(ThingRef thing);

    /// Perform an action when this thing is eaten.
    /// If this function returns false, the action is aborted.
    /// The default behavior is to do nothing and return false.
    bool perform_action_eaten_by(ThingRef thing);

    /// Perform an action when this thing is picked up.
    /// If this function returns false, the action is aborted.
    /// The default behavior is to do nothing and return true.
    bool perform_action_picked_up_by(ThingRef thing);

    /// Perform an action when this thing is put into another thing.
    /// If this function returns false, the action is aborted.
    /// The default behavior is to do nothing and return true.
    bool perform_action_put_into(ThingRef container);

    /// Perform an action when this thing is taken out its container.
    /// If this function returns false, the action is aborted.
    /// The default behavior is to do nothing and return true.
    bool perform_action_take_out();

    /// Perform an action when this thing is read.
    /// If this function returns Failure, the action is aborted.
    /// The default behavior is to do nothing and return Failure.
    ActionResult perform_action_read_by(ThingRef thing);

    /// Perform an action when this thing hits an entity.
    /// This action executes when the thing is wielded by an entity, and an
    /// attack successfully hits its target.  It is a side-effect in addition
    /// to the damage done by Entity::attack(entity).
    /// The default behavior is to do nothing.
    /// @see Entity::attack
    void perform_action_attack_hits(ThingRef thing);

    /// Perform an action when this thing is thrown.
    /// If this function returns false, the action is aborted.
    /// The default behavior is to do nothing and return true.
    bool perform_action_thrown_by(ThingRef thing, Direction direction);

    /// Perform an action when this thing is de-equipped (taken off).
    /// If this function returns false, the action is aborted.
    /// The default behavior is to do nothing and return true.
    bool perform_action_deequipped_by(ThingRef thing, WearLocation& location);

    /// Perform an action when this thing is equipped.
    /// If this function returns false, the action is aborted.
    /// The default behavior is to do nothing and return false.
    bool perform_action_equipped_by(ThingRef thing, WearLocation& location);

    /// Perform an action when this thing is unwielded.
    /// If this function returns false, the action is aborted.
    /// The default behavior is to do nothing and return true.
    bool perform_action_unwielded_by(ThingRef thing);

    /// Perform an action when this thing is wielded.
    /// If this function returns false, the action is aborted.
    /// The default behavior is to do nothing and return true.
    bool perform_action_wielded_by(ThingRef thing);

    /// Perform an action when this thing is fired.
    /// If this function returns false, the action is aborted.
    /// The default behavior is to do nothing and return false.
    bool perform_action_fired_by(ThingRef thing, Direction direction);

    /// Returns whether the Thing can merge with another Thing.
    /// Calls an overridden subclass function.
    bool can_merge_with(ThingRef other) const;

    /// Returns whether the Thing can hold a certain thing.
    /// If Thing's inventory size is 0, returns
    /// ActionResult::FailureTargetNotAContainer.
    /// Otherwise, calls private virtual method _can_contain().
    /// @param thing Thing to check.
    /// @return ActionResult specifying whether the thing can be held here.
    ActionResult can_contain(ThingRef thing) const;

    /// Return the type of this thing.
    //virtual char const* get_thing_type() const final;

    /// Returns whether this Thing can hold a liquid.
    virtual bool is_liquid_carrier() const;

    /// Returns whether this Thing is flammable.
    virtual bool is_flammable() const;

    /// Returns whether this Thing is corrodible.
    virtual bool is_corrodible() const;

    /// Returns whether this Thing is shatterable.
    virtual bool is_shatterable() const;

  protected:
    /// Named Constructor
    Thing(std::string type, ThingRef ref);

    /// Floor Constructor
    Thing(MapTile* map_tile, ThingRef ref);

    /// Copy Constructor
    Thing(const Thing& original);

    struct Impl;
    std::unique_ptr<Impl> pImpl;

    /// Do any subclass-specific processing; called by _process().
    /// This function is particularly useful if the subclass is able to do
    /// specialized actions such as rise from the dead after a time (as in
    /// NetHack trolls).
    /// @warning In order to support the aforementioned rising from the dead,
    ///          this function is called <i>regardless of the Entity's HP</>!
    ///          Keep this in mind when implementing specialized behavior.
    virtual void _process_specific();

    /// Decrement the busy counter if greater than 0.
    /// Returns true if the counter has reached 0, false otherwise.
    bool dec_busy_counter();

    /// Set the busy counter to a value.
    void set_busy_counter(int value);

    /// Get a reference to this Entity's map memory.
    std::vector<MapTileType>& get_map_memory();

    /// Perform the recursive visibility scan for an octant.
    /// Used by find_seen_tiles.
    void do_recursive_visibility(int octant,
      int depth,
      double slope_A,
      double slope_B);

    /// Set the location of this thing.
    /// Does no checks, nor does it update the source/target inventories.
    /// Those are the responsibility of the caller.
    void set_location(ThingRef target);

    /// Static method initializing font sizes.
    static void initialize_font_sizes();

    /// Does the actual call to light surroundings.
    /// Default behavior is to do nothing.
    virtual void _light_up_surroundings();

    /// Receive light from the specified light source.
    /// The default behavior is to do nothing.
    virtual void _be_lit_by(ThingRef light);

    /// Ratio of desired height to font size.
    static float font_line_to_point_ratio_;

    /// Outline color for walls when drawing on-screen.
    static sf::Color const wall_outline_color_;

  private:
    /// Returns whether the Thing can hold a certain thing.
    virtual ActionResult _can_contain(ThingRef thing) const;

    /// Gets this location's maptile.
    virtual MapTile* _get_maptile() const;

    virtual bool _perform_action_activated_by(ThingRef thing);
    virtual void _perform_action_collided_with(ThingRef thing);
    virtual bool _perform_action_drank_by(ThingRef thing);
    virtual bool _perform_action_dropped_by(ThingRef thing);
    virtual bool _perform_action_eaten_by(ThingRef thing);
    virtual bool _perform_action_picked_up_by(ThingRef thing);
    virtual bool _perform_action_put_into(ThingRef container);
    virtual bool _perform_action_take_out();
    virtual ActionResult _perform_action_read_by(ThingRef thing);
    virtual void _perform_action_attack_hits(ThingRef thing);
    virtual bool _perform_action_thrown_by(ThingRef thing, Direction direction);
    virtual bool _perform_action_deequipped_by(ThingRef thing,
                                               WearLocation& location);
    virtual bool _perform_action_equipped_by(ThingRef thing,
                                             WearLocation& location);
    virtual bool _perform_action_unwielded_by(ThingRef thing);
    virtual bool _perform_action_wielded_by(ThingRef thing);
    virtual bool _perform_action_fired_by(ThingRef thing, Direction direction);

    /// Process this Thing for a single tick.
    /// By default, does nothing and returns true.
    /// The function returns false to indicate to its parent that it no longer
    /// exists and should be deleted.
    /// @return true if the Thing continues to exist after the tick;
    ///         false if the Thing ceases to exist.
    virtual bool _process();

    // Static Lua functions.
    // @todo (Maybe these should be part of ThingManager instead?)
    static int LUA_get_intrinsic_flag(lua_State* L);
    static int LUA_get_intrinsic_value(lua_State* L);
    static int LUA_get_intrinsic_string(lua_State* L);
    static int LUA_get_property_flag(lua_State* L);
    static int LUA_get_property_value(lua_State* L);
    static int LUA_get_property_string(lua_State* L);
    static int LUA_set_property_flag(lua_State* L);
    static int LUA_set_property_value(lua_State* L);
    static int LUA_set_property_string(lua_State* L);
};

#endif // THING_H
