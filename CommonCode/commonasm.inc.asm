[BITS 32]

;[extern _Proc_Stack_Top]
	
%macro SHIFT_STACK	2
	mov ecx, %1-4
	sub ecx, esp
	mov edi, %2-4
	sub edi, eax
	mov esi, esp
	rep movsb
	
	mov ecx, %1-4
	sub ecx, esp
	mov esp, %2-4
	sub esp, ecx
%endmacro