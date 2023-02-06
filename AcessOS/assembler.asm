; AcessOS 0.1
;Assembler Code
;
[BITS 32]
[section .text]

%include "../CommonCode/commonasm.inc.asm"

; This will set up our new segment registers. We need to do
; something special in order to set CS. We do what is called a
; far jump. A jump that includes a segment as well as an offset.
; This is declared in C as 'extern void gdt_flush();'
[global _gdt_flush]
[extern _gp]
_gdt_flush:
    lgdt [_gp]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    jmp 0x08:flush2
flush2:
    ret

; Loads the IDT defined in '_idtp' into the processor.
; This is declared in C as 'extern void idt_load();'
[global _idt_load]
[extern _idtp]
_idt_load:
    lidt [_idtp]
    ret

; Marco to define the ISRs
; Supports ones that provide an error code and ones that don't
%macro  ISR_ERRNO	1
	[global _isr%1]
_isr%1:
;	xchg bx, bx
    cli
    push byte %1
    jmp ISR_Common_Stub
%endmacro
%macro  ISR_NOERRNO	1
	[global _isr%1]
_isr%1:
;	xchg bx, bx
    cli
	push byte 0
    push byte %1
    jmp ISR_Common_Stub
%endmacro
%macro  ISR_NOERRNOND	1
	[global _isr%1]
_isr%1:
    cli
	push byte 0
    push byte %1
    jmp ISR_Common_Stub
%endmacro

ISR_NOERRNO	0;  0: Divide By Zero Exception
ISR_NOERRNOND	1;  1: Debug Exception
ISR_NOERRNO	2;  2: Non Maskable Interrupt Exception
ISR_NOERRNO	3;  3: Int 3 Exception
ISR_NOERRNO	4;  4: INTO Exception
ISR_NOERRNO	5;  5: Out of Bounds Exception
ISR_NOERRNO	6;  6: Invalid Opcode Exception
ISR_NOERRNO	7;  7: Coprocessor Not Available Exception
ISR_ERRNO	8;  8: Double Fault Exception (With Error Code!)
ISR_NOERRNO	9;  9: Coprocessor Segment Overrun Exception
ISR_ERRNO	10; 10: Bad TSS Exception (With Error Code!)
ISR_ERRNO	11; 11: Segment Not Present Exception (With Error Code!)
ISR_ERRNO	12; 12: Stack Fault Exception (With Error Code!)
ISR_ERRNO	13; 13: General Protection Fault Exception (With Error Code!)
ISR_ERRNO	14; 14: Page Fault Exception (With Error Code!)
ISR_NOERRNO	15; 15: Reserved Exception
ISR_NOERRNO	16; 16: Floating Point Exception
ISR_NOERRNO	17; 17: Alignment Check Exception
ISR_NOERRNO	18; 18: Machine Check Exception
ISR_NOERRNO	19; 19: Reserved
ISR_NOERRNO	20; 20: Reserved
ISR_NOERRNO	21; 21: Reserved
ISR_NOERRNO	22; 22: Reserved
ISR_NOERRNO	23; 23: Reserved
ISR_NOERRNO	24; 24: Reserved
ISR_NOERRNO	25; 25: Reserved
ISR_NOERRNO	26; 26: Reserved
ISR_NOERRNO	27; 27: Reserved
ISR_NOERRNO	28; 28: Reserved
ISR_NOERRNO	29; 29: Reserved
ISR_NOERRNO	30; 30: Reserved
ISR_NOERRNO	31; 31: Reserved


; ISR Common - Saves CPU state and calls the handler
[extern _Fault_Handler]
[global ISR_Common_Stub]
ISR_Common_Stub:
	;xchg bx, bx	; Magic Breakpoint
    pusha
	
	; Change Stack
;	pushf
;	cli
;	SHIFT_STACK	_Proc_Stack_Top, 0xF0000000
;	popf
	
    push ds
    push es
    push fs
    push gs
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp
    push eax
    call _Fault_Handler
    pop eax
    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8	; Error Number and argument
    iret

%macro  IRQ	1
	[global _irq%1]
_irq%1:
;	xchg bx,bx
    cli
	push byte 0
    push byte %1
    jmp irq_common_stub
%endmacro
IRQ 0	; PIT Interrupt
IRQ 1	; Keyboard
IRQ 2	; 
IRQ 3	; 
IRQ 4	; 
IRQ 5	; 
IRQ 6	; FDC
IRQ 7	; 
IRQ 8	; 
IRQ 9	; 
IRQ 10	; 
IRQ 11	; 
IRQ 12	; 
IRQ 13	; 
IRQ 14	; 
IRQ 15	; 

[extern _irq_handler]
[global irq_common_stub]

irq_common_stub:
    pusha
	
	; Change Stack
;	pushf
;	cli
;	SHIFT_STACK	_Proc_Stack_Top, 0xF0000000
;	popf
	
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp

    push eax
    call _irq_handler
    pop eax

    pop gs
    pop fs
    pop es
    pop ds
    popa
    add esp, 8
    iret

	
; MEMORY MANAGING FUNCTIONS
[global _read_cr0]
[global _write_cr0]
[global _read_cr3]
[global _write_cr3]
_read_cr0:
	mov eax, cr0
	retn
_write_cr0:
	push ebp
	mov ebp, esp
	mov eax, [ebp+8]
	mov cr0,  eax
	pop ebp
	retn
_read_cr3:
	mov eax, cr3
	retn
_write_cr3:
	push ebp
	mov ebp, esp
	mov eax, [ebp+8]
	mov cr3, eax
	pop ebp
	retn

[global _getEip]
_getEip:
	pop eax
	jmp eax

[global _getEipSafe]
_getEipSafe:
	pushad
	mov eax, DWORD [esp+36]	; Argument 1
	mov DWORD [eax], esp
	call _getEip
	push eax
	add esp, 4	; Rewind by 1
	popad	;32
	mov eax, DWORD [esp-36]
	ret
	
[extern _syscall_handler]
[global _syscall_stub]
_syscall_stub:	
	pushad
	
;	pushf
;	cli
;	SHIFT_STACK	_Proc_Stack_Top, 0xF0000000
;	popf
	
	push ds
	push es
	push fs
	push gs
	
	mov eax, esp
	push eax
	call _syscall_handler
	mov DWORD [tmpRet], eax
	add esp, 4
	
	pop gs
	pop fs
	pop es
	pop ds
	popad
	
	mov eax, DWORD [tmpRet]
	iret

[extern _syscall_linuxhandler]
[global _syscall_linuxstub]
_syscall_linuxstub:
	iret
	

;==== Read the RTC ====
; void readRTC(char *years, char *month, char *day, char *hrs, char *min, char *sec);
[global _readRTC]
%define RTCaddress	0x70
%define RTCdata		0x71
_readRTC:
	push ebx
	mov ebx, esp
	sub ebx, 4
	push eax
.l1:	mov al,10			;Get RTC register A
	out RTCaddress,al
	in al,RTCdata
	test al,0x80			;Is update in progress?
	jne .l1				; yes, wait

	mov al,0			;Get seconds (00 to 59)
	out RTCaddress,al
	in al,RTCdata
	mov [ebx-8],al

	mov al,0x02			;Get minutes (00 to 59)
	out RTCaddress,al
	in al,RTCdata
	mov [ebx-12],al

	mov al,0x04			;Get hours (see notes)
	out RTCaddress,al
	in al,RTCdata
	mov [ebx-16],al

	mov al,0x07			;Get day of month (01 to 31)
	out RTCaddress,al
	in al,RTCdata
	mov [ebx-20],al

	mov al,0x08			;Get month (01 to 12)
	out RTCaddress,al
	in al,RTCdata
	mov [ebx-24],al

	mov al,0x09			;Get year (00 to 99)
	out RTCaddress,al
	in al,RTCdata
	mov [ebx-28],al

	pop eax
	pop ebx
	ret

; ============================
; = Memory Manager Functions =
; ============================
[extern _paging_pid]
[extern _page_table_root]
[extern _a_page_directories]
; void MM_SetPid(int pid)
[global _MM_SetPid]
_MM_SetPid:
	mov eax, DWORD [esp+4]
	cmp eax, 1024
	jg	.end
	mov DWORD [_paging_pid], eax
	shl eax, 2
	mov eax, DWORD [_a_page_directories + eax]
	and eax, 0xFFFFF000
	mov cr3, eax
.end:
	ret

; =========================
; = Scheduler IRQ Handler =
; =========================
[extern _proc_scheduler]
[global _sched]
_sched:
	cli
	
	pushad	; Save All GP Registers
	
;	SHIFT_STACK	0xF0000000, _Proc_Stack_Top
	
;	xchg bx, bx
	push ds
	push es
	push fs
	push gs

	; Set Segment Registers
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
	
	mov ebx, esp	; Save to ebx to preserve it
	mov esp, _Proc_Stack_Top	; Change Stack to common one, (this cannot be interrupted)
	mov DWORD [0xEFFFFFFC], ebx
	push ebx		; Push stack pointer
	call _proc_scheduler
	;xchg bx, bx
	add esp, 4		; Clear argument from stack
	mov esp, DWORD [0xEFFFFFFC]	; Restore Process Stack
	
	pop gs
	pop fs
	pop es
	pop ds
	
	; ACK timer interrupt
	mov	al, 0x20
	out	0x20, al
	
	popad
	iret

; void Proc_ProcessStart(Uint *stack)
[global _Proc_ProcessStart]
_Proc_ProcessStart:
	cli
	mov eax, DWORD [esp+0x4]
	mov esp, eax
	
	pop gs
	pop fs
	pop es
	pop ds
	popad
	
	
	iret

[global _Proc_DropToUsermode]
_Proc_DropToUsermode:
	cli
	
	mov ax, 0x23
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	mov eax, esp
	push 0x23	;SS 0x20 | 3
	push eax	;ESP
	pushf		;ELFAGS
	pop eax
	or eax, 0x200	;0x200 - Interrupt Flag
	push eax	;EFLAGS
	push 0x1B	;CS 0x18 | 3
	push .usermode
	iret
.usermode:
	ret

;[global _Proc_RestoreStack]
;_Proc_RestoreStack:
	push ebp
	mov ebp, esp
	push edi	; oldesp
	push esi	; pointer
	mov esi, [ebp-8]
	
	
	pop edi
	pop ebp
	ret
	
[section .data]
tmpRet:	dd	0
	
[section .bss]
_Proc_Stack:
	resb	1024	; Small Stack for Process Switch
[global _Proc_Stack_Top]
_Proc_Stack_Top:
