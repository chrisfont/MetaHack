-- Definition of the GameObject object type, Entity and MTUnknown inherit from.

GameObject = inheritsFrom(nil, "GameObject")

function GameObject.get_display_name(id)
    return intrinsics.name
end

function GameObject.get_brief_description(id)
    return "GameObject #" .. id .. ", which has no description associated with it."
end

function GameObject.get_tile_offset(id, frame)
    return 0, 0, LuaType.IntVec2
end

function GameObject.on_create(id)
    --messageLog_add("GameObject.on_create() called, new ID = " .. id)
    return true, LuaType.Boolean
end