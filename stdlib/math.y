define math_min(left, right)
    when left < right doit
        give left
    otherwise
        give right
    done
done

define math_max(left, right)
    when left > right doit
        give left
    otherwise
        give right
    done
done

define math_clamp(value, low, high)
    when value < low doit
        give low
    otherwise
        when value > high doit
            give high
        otherwise
            give value
        done
    done
done

define math_sign(value)
    when value < 0 doit
        give -1
    otherwise
        when value > 0 doit
            give 1
        otherwise
            give 0
        done
    done
done