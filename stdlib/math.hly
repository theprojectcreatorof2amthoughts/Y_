define math_min(left, right)
    when left < right doit
        give left
    else
        give right
    done
done

define math_max(left, right)
    when left > right doit
        give left
    else
        give right
    done
done

define math_clamp(value, low, high)
    when value < low doit
        give low
    else
        when value > high doit
            give high
        else
            give value
        done
    done
done

define math_sign(value)
    when value < 0 doit
        give -1
    else
        when value > 0 doit
            give 1
        else
            give 0
        done
    done
done