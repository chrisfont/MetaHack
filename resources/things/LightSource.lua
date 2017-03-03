-- Definition of special functions for the LightSource object type.

require "resources/things/Entity"

LightSource = inheritsFrom(Entity, "LightSource")
LightSource.intrinsics.name = "[LightSource]"
LightSource.intrinsics.plural = "[LightSources]"

LightSource.intrinsics.lit = true
LightSource.intrinsics.light_color_red = 64
LightSource.intrinsics.light_color_green = 64
LightSource.intrinsics.light_color_blue = 64
LightSource.intrinsics.light_strength = 64

function LightSource.get_brief_description()
    return "A source of illumination."
end

function LightSource.modify_property_attribute_strength(id, old_value)
    new_value = old_value + 1
    return new_value
end

function LightSource.can_have_action_use_done_by(id)
    -- This SHOULD check if a creature is sentient.
    -- For now just return true.
    return true
end

function LightSource.be_object_of_action_use(object, subject)
    local name = thing_get_intrinsic_string(object, "name")
    local is_lit = thing_get_modified_property_flag(object, "lit")
    is_lit = not is_lit
    if is_lit then
        messageLog_add("You light the " .. name .. ".")
        thing_add_property_modifier(subject, "attribute_strength", object)
    else
        messageLog_add("You extinguish the " .. name .. ".")
        thing_remove_property_modifier(subject, "attribute_strength", object)
    end
    thing_set_base_property_flag(id, "lit", is_lit)
    return ActionResult.Success
end