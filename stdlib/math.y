function math_min(left, right)
    if left < right then
        return left
    else
        return right
    end
end

function math_max(left, right)
    if left > right then
        return left
    else
        return right
    end
end

function math_clamp(value, low, high)
    if value < low then
        return low
    else
        if value > high then
            return high
        else
            return value
        end
    end
end

function math_sign(value)
    if value < 0 then
        return -1
    else
        if value > 0 then
            return 1
        else
            return 0
        end
    end
end