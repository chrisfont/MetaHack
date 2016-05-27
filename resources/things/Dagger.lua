-- Definition of special functions for the Dagger object type.

require "resources/things/WeaponStabbing"

Dagger = inheritsFrom(WeaponStabbing, "Dagger")
Dagger.intrinsics.name = "dagger"
Dagger.intrinsics.plural = "daggers"
Dagger.intrinsics.creatable = true

Dagger.intrinsics.physical_mass = 1

function Dagger.can_have_action_rust_done_by(id)
    return true
end

function Dagger.get_description()
    return "A small blade with a hilt."
end