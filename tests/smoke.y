!#libinc "stdlib/math.y"
!#libinc "stdlib/strings.y"

## Verify Y_ truth words plus loop escape.
stash limited = math_clamp(18, 0, 10)
stash repeated = string_repeat("ab", 2)
stash truth = yes both invert no
memdesig !$buffer, 2
memwrite(!$buffer, 1, 42)

writescrn >^ limited
writescrn >^ repeated
writescrn >^ memread(!$buffer, 1)
when truth either no doit
	writescrn >^ truth
otherwise
	writescrn >^ no
done
repeatwhile yes execute
	writescrn >^ "escaped"
	escape
done
writescrn >^ void