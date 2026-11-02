!#libinc "examples/greeting.y"

function countdown(start)
    local count = start
    while count > 0 do
        writescrn >^ count
        count = count - 1
    end
    return "done"
end

!$message = "Y_ is running"
announce(!$message)
writescrn >^ countdown(3)

memdesig !$memory, 8
memwrite(!$memory, 0, 89)
memwrite(!$memory, 1, 95)
writescrn >^ memread(!$memory, 0)
writescrn >^ memread(!$memory, 1)