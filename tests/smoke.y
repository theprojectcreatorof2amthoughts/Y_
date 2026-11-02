!#inclib %math.yl%
!#inclib %strings.yl%

## Verify Y_ truth words plus loop escape.
stash limited = math_clamp(18, 0, 10)
stash repeated = string_repeat("ab", 2)
stash truth = yes both invert no
stash !${score-level@v2} = 7
stash !${score level+@v\}2} = 8
memdesig !$buffer, 2
memwrite(!$buffer, 1, 42)

writescrn >^ limited
writescrn >^ repeated
writescrn >^ memread(!$buffer, 1)
when truth either no doit
	writescrn >^ truth
else
	writescrn >^ no
done
repeatwhile yes execute
	writescrn >^ "escaped"
	escape
done
writescrn >^ void
	when !${score-level@v2} == 7 doit
		writescrn >^ "symbol-name-ok"
	else
		writescrn >^ "symbol-name-failed"
	done
	when !${score level+@v\}2} == 8 doit
		writescrn >^ "escaped-symbol-name-ok"
	else
		writescrn >^ "escaped-symbol-name-failed"
	done
	stash inclibr
	when inclibr define = yes doit
		writescrn >^ "declaration-check-ok"
	else
		writescrn >^ "declaration-check-failed"
	done