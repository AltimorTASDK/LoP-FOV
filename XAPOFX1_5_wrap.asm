_TEXT SEGMENT

EXTERN orig_CreateFX: QWORD
PUBLIC CreateFX
CreateFX PROC
	jmp [orig_CreateFX]
CreateFX ENDP

_TEXT ENDS

END