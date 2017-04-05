-- Definition of the Entity object type, which all others inherit from.

function Entity_get_display_name(id)
    local quantity = thing_get_modified_property(id, "quantity")
    if quantity > 1 then
        return intrinsics.plural, LuaType.String
    else
        return intrinsics.name, LuaType.String
    end
end

function Entity_get_brief_description(id)
    return "Entity #" .. id .. ", which has no description associated with it.", LuaType.String
end

function Entity_can_contain(id)
    -- By default a Entity can only contain solid objects.
    if thing_get_intrinsic(id, "liquid") == true then
        return false, LuaType.Boolean
    end

    return true, LuaType.Boolean
end

function Entity_on_lit_by(id)
    return true, LuaType.Boolean
end

function Entity_process()
    return true, LuaType.Boolean
end