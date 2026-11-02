!#libinc "stdlib/math.y"
!#libinc "stdlib/strings.y"

local limited = math_clamp(18, 0, 10)
local repeated = string_repeat("ab", 2)
memdesig !$buffer, 2
memwrite(!$buffer, 1, 42)

writescrn >^ limited
writescrn >^ repeated
writescrn >^ memread(!$buffer, 1)