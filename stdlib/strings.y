function string_join(left, right)
    return left + right
end

function string_repeat(text, count)
    local output = ""
    local index = 0
    while index < count do
        output = output + text
        index = index + 1
    end
    return output
end

function string_is_empty(text)
    return text == ""
end