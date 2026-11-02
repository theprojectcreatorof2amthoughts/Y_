define input_permit(out, in)
    when in > out doit
        errstate <^ "Input has more characters!"
    else
        errstate <^ "Output has more characters!"
    done
done