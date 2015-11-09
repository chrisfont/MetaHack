-- Definition of special functions for the Coin object type.

require "resources/things/Thing"

Coin = inheritsFrom(Thing, "Coin")
Coin.name = "[Coin]"

Coin.intrinsics.physical_mass = 1

function Coin.get_description()
	return "A flat, typically round piece of metal with an official stamp, used as money."
end