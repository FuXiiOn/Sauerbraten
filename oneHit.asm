PUBLIC oneHit
.code

EXTERN bOnehit:BYTE
EXTERN jmpBackOneHit:QWORD

oneHit PROC
	cmp byte ptr [bOnehit],0
	je normalFlow
	cmp dword ptr [rcx+0],0
	jne kill
	add [rcx+340h],edx
	mov dword ptr [rcx+348h],eax
	jmp jmpBackOneHit
kill:
	mov dword ptr [rcx+340h],0
	mov dword ptr [rcx+348h],eax
	jmp jmpBackOneHit
normalFlow:
	add [rcx+340h],edx
	mov dword ptr [rcx+348h],eax
	jmp jmpBackOneHit
oneHit ENDP

END