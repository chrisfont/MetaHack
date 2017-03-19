-- Definition of the GameObject object type, Entity and MTUnknown inherit from.

GameObject = inheritsFrom(nil, "GameObject")
GameObject.intrinsics = {}
GameObject.intrinsics.name = "[GameObject]"
GameObject.intrinsics.plural = "[GameObjects]"

GameObject.intrinsics.inventory_size = 0
GameObject.intrinsics.living = false
GameObject.intrinsics.passable = false
GameObject.intrinsics.opacity_red = 255
GameObject.intrinsics.opacity_green = 255
GameObject.intrinsics.opacity_blue = 255
GameObject.intrinsics.tile_character_code = 0x003f   -- question mark
GameObject.intrinsics.tile_bg_color_red = 0          -- black
GameObject.intrinsics.tile_bg_color_green = 0
GameObject.intrinsics.tile_bg_color_blue = 0
GameObject.intrinsics.tile_fg_color_red = 128        -- brown
GameObject.intrinsics.tile_fg_color_green = 64
GameObject.intrinsics.tile_fg_color_blue = 0

GameObject.intrinsics.inventory_size_is_integer = true


function GameObject.get_display_name(id)
    return intrinsics.name
end

function GameObject.get_brief_description(id)
    return "GameObject #" .. id .. ", which has no description associated with it."
end

function GameObject.get_tile_offset(id, frame)
    return 0, 0, PropertyType.IntVec2
end

function GameObject:get_tile_character_code(id, frame)
    return intrinsics.tile_character_code
end

function GameObject.get_tile_bg_color(id, frame)
    return intrinsics.tile_bg_color_red,
           intrinsics.tile_bg_color_green,
           intrinsics.tile_bg_color_blue,
           255
end

function GameObject.get_tile_fg_color(id, frame)
    return intrinsics.tile_fg_color_red,
           intrinsics.tile_fg_color_green,
           intrinsics.tile_fg_color_blue,
           255
end

function GameObject.on_create(id)
    --messageLog_add("GameObject.on_create() called, new ID = " .. id)
    return ActionResult.Success
end

-- Glue code to return values to C++, so the C++ code doesn't have to parse
-- tables. Probably not the most efficient way to do things, but eh...
function GameObject:get_intrinsic(key)
    --print(self.typeName .. ":get_intrinsic(" .. tostring(key) .. ")")

    local values, key_type = self:get_intrinsic_(key)

    if (key_type == PropertyType.IntVec2) then
        return values.x, values.y, key_type
    end
            
    return values, key_type
end

function GameObject:get_intrinsic_(key)
    --print(self.typeName .. ":get_intrinsic_(" .. tostring(key) .. ")")

    if (PropertyType == nil or type(PropertyType) == "nil") then
        error("PropertyType enum doesn't exist")
    end

    local value = (self.intrinsics ~= nil) and self.intrinsics[key] or nil

    if (value == nil) then
        local superclass = self:superClass()
        if (superclass ~= nil) then
            local superclass_values, superclass_type = superclass:get_intrinsic_(key)
            return superclass_values, superclass_type
        else
            --print(" -- No superclass, and key does not exist, returning nil")
            return nil, PropertyType.Null
        end
    else
        --print(" -- Value is " .. tostring(value))

        -- Check the intrinsic type.
        if (type(value) == "boolean") then
            --print(" Boolean = " .. tostring(value));
            return value, PropertyType.Boolean
        elseif (type(value) == "string") then
            --print(" String = " .. value);
            return value, PropertyType.String
        elseif (type(value) == "number") then
            local key_int = key .. "_is_integer"
            local value_is_integer, _ = self:get_intrinsic(key_int)
            if (value_is_integer == true) then
                --print(" Integer = " .. tostring(value));
                return value, PropertyType.Integer
            else
                --print(" Number = " .. tostring(value));
                return value, PropertyType.Number
            end
        elseif (type(value) == "table") then
            --print(" -- Table: " .. value.type)
            if (value.type == "range") then
                --print("Range type found, returning " .. tostring(value.min) .. " and " .. tostring(value.max))
                return math.random(value.min, value.max), PropertyType.Integer
            elseif (value.type == "vector2") then
                return { x = value.x, y = value.y }, PropertyType.IntVec2
            else
                return value.type, PropertyType.Unknown
            end
        else
            --print(" -- Unknown: " .. type(value))
            return type(value), PropertyType.Unknown
        end
    end
end

function GameObject:set_intrinsic(name, value)
    self.intrinsics[name] = value
end
