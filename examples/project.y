!#libinc "stdlib/math.y"
!#libinc "stdlib/strings.y"

function describe_score(name, score)
    local safe_score = math_clamp(score, 0, 100)
    writescrn >^ string_join(name, " scored ")
    writescrn >^ tostring(safe_score)
    return safe_score
end

local adjusted = describe_score("Ada", 108)
local repetitions = string_repeat("Y_", 3)
!$buffer_size = 12

memdesig !$buffer, !$buffer_size
memwrite(!$buffer, 0, 89)
memwrite(!$buffer, 1, 95)

writescrn >^ repetitions
writescrn >^ math_sign(adjusted - 50)
writescrn >^ memread(!$buffer, 0)
writescrn >^ memread(!$buffer, 1)