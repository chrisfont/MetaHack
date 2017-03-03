-- Definition of special functions for the TileContents object type.

require "resources/things/Entity"

TileContents = inheritsFrom(Entity, "TileContents")
TileContents.intrinsics.name = "area"
TileContents.intrinsics.plural = "areas"

TileContents.intrinsics.opacity_red = 0
TileContents.intrinsics.opacity_green = 0
TileContents.intrinsics.opacity_blue = 0
TileContents.intrinsics.inventory_size = -1

function TileContents.get_brief_description()
	return "The things that are held or included in this tile."
end