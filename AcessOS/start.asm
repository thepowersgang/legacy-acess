; Acess OS Kernel
; Entrypoint

[global start]
[global _loader]

KERNEL_VIRTUAL_BASE equ 0xC0000000                  ; 3GB
KERNEL_DIR_NUMBER equ (KERNEL_VIRTUAL_BASE >> 22) 


[BITS 32]
; This part MUST be 4byte aligned, so we solve that issue using 'ALIGN 4'
; ===
; Multiboot Header
; ===
ALIGN 4
mboot:
	MULTIBOOT_INFO_SIZE		equ 11*4
    ; Multiboot macros to make a few lines later more readable
    MULTIBOOT_PAGE_ALIGN	equ 1<<0
    MULTIBOOT_MEMORY_INFO	equ 1<<1
    MULTIBOOT_AOUT_KLUDGE	equ 1<<16
    MULTIBOOT_HEADER_MAGIC	equ 0x1BADB002
    MULTIBOOT_HEADER_FLAGS	equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO ;| MULTIBOOT_AOUT_KLUDGE
    MULTIBOOT_CHECKSUM	equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
    EXTERN code, bss, end

    ; This is the GRUB Multiboot header. A boot signature
    dd MULTIBOOT_HEADER_MAGIC
    dd MULTIBOOT_HEADER_FLAGS
    dd MULTIBOOT_CHECKSUM
	dd mboot - KERNEL_VIRTUAL_BASE	;Location of Multiboot Header
	dd code		;Start of .text
	dd bss		;Start of .bss
	dd end		;End of Kernel
	dd start - KERNEL_VIRTUAL_BASE	;Entrypoint

; === Main Code ===
start:
    ;Copy MBoot Data
	%assign i 0
	%rep MULTIBOOT_INFO_SIZE
	mov cl,	BYTE [ebx+i]
	mov BYTE [mbFlags + i - KERNEL_VIRTUAL_BASE], cl
	%assign i i+1
	%endrep
	
	; Initialize Virtual Memory
	mov ecx, (_page_dir_kernel - KERNEL_VIRTUAL_BASE)
	mov cr3, ecx                                ; Load Page Directory Base Register.
	mov ecx, cr0
	or ecx, 0x80000000                          ; enable paging.
	mov cr0, ecx

	; Start fetching instructions in kernel space.
	lea ecx, [StartInHigherHalf]
	jmp ecx

StartInHigherHalf:
    extern _kmain
    mov esp, 0xEFFFFFFC		; This points to the new stack (Top is saved Process EIP)
    push mbFlags
    push eax
    call _kmain
	hlt
    jmp $

; ================
; = Data Section =
; ================
;SECTION .data

mbFlags:	dd	0	;0
loMem:		dd	0	;4
hiMem:		dd	0	;8
bootDev:	dd	0	;12
cmdPtr:		dd	0	;16
modCount:	dd	0	;20
modPtr:		dd	0	;24
	times 4*4	dd	0
mbFlagsEnd:

; ===
; Page Table
; ===
global _page_table_root
global _page_dir_kernel
global _page_table_kernel
global _page_table_stack
global _a_page_directories

; Page table to map the first 4MB of physical memory
align 0x1000
_page_table_root:
	%assign i 0
	%rep    1024
	dd 0x00000003 + (i * 4096)
	%assign i i+1
	%endrep
	

;KSTACK = 0xEFFF0000
extern _gKernelEnd
;KSTACK_BASE	equ	( _gKernelEnd + 0xFFF)&0xFFFFF000
align 0x1000
_page_table_stack:
	times 1024-24 dd 0
	%assign i 0
	%rep    24
	;dd 0x00200003 + (i * 4096)
	dd _gKernelEnd + 3 + (i * 4096)
	%assign i i+1
	%endrep

; ===
; Page Directory
; ===
; Kernel page directory
align 0x1000
_page_dir_kernel:
    dd 0x00000003 + _page_table_root - KERNEL_VIRTUAL_BASE		;ID Map Lower 2Mb
    times (KERNEL_DIR_NUMBER - 1) dd 0				; Pages before kernel space.
    dd 0x00000003 + _page_table_root - KERNEL_VIRTUAL_BASE		;3 GB (0xC)
	times (190) dd 0	; Padding Between Kernel Image and Kernel Stack
    dd 0x00000003 + _page_table_stack - KERNEL_VIRTUAL_BASE		;3 GB + 767 Mb + 960 Kb (0xEFF)
    times 64 dd 0		; Pages after kernel stack



; ===================
; Memory Manager Code
; ===================
align 0x1000
_page_table_kernel:
	times 1024 dd 0
_a_page_directories:
	times 2048 dd 0

