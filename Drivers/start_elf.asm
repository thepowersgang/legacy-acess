; Acess Kernel Module
; Assembler Stub

[BITS 32]
ACESSOS_OFFSET	equ 0xC0105000

[extern ModuleLoad]
[extern ModuleUnload]
[extern ModuleIdent]
[extern _Module_Register]

[section .text]
[global start]
;Code
start:
	;Initialise Imported functions
	call __IMPORT_init
	cmp eax, 1
	jz .fastclose
	
	push ebx	;EAX holds base address (a bit of a hack)
	push ModuleIdent
	push ModuleUnload
	mov eax, [ebx+__IMPORT_register]
	call eax
	;call _Module_Register
	add esp, 12	; Restore Stack
	
	call ModuleLoad
.fastclose:
	ret

; ====================
; IMPORTED FUNCTIONS
; - Ported from kmod.c
; ====================

[global memcpy]
[global memcpyd]
[global memcpyda]
[global memset]
[global memsetw]
[global strlen]
[global strcmp]
[global strncmp]
[global strcpy]
[global inb]
[global inw]
[global ind]
[global outb]
[global outw]
[global outd]
[global DevFS_AddDevice]
[global DevFS_DelDevice]
[global DevFS_AddSymlink]
[global DevFS_DelSymlink]
[global VFS_AddFS]
[global VFS_Open]
[global VFS_Close]
[global VFS_Read]
[global VFS_Write]
[global VFS_Seek]
[global VFS_Tell]
[global VFS_ReadDir]
[global VFS_Mount]
[global Inode_InitCache]
[global Inode_ClearCache]
[global Inode_GetCache]
[global Inode_CacheNode]
[global Inode_UncacheNode]
[global now]
[global timestamp]
[global malloc]
[global free]
[global realloc]
[global DMA_SetRW]
[global DMA_ReadData]
[global Time_CreateTimer]
[global Time_RemoveTimer]
[global IRQ_Set]
[global IRQ_Clear]
[global LogF]
[global MM_MapHW]
[global MM_UnmapHW]

__IMPORT_init:
	cmp DWORD [ecx], 0xACE55051
	jz	.init
	mov eax, 1
	ret
.init:
	mov eax, DWORD [ecx+0x04]		;VFS
	mov DWORD [ebx+__IMPORT_vfs], eax
	mov eax, DWORD [ecx+0x08]		;DevFS
	mov DWORD [ebx+__IMPORT_devfs], eax
	mov eax, DWORD [ecx+0x0C]		;Module Register
	mov DWORD [ebx+__IMPORT_register], eax
	mov eax, DWORD [ecx+0x10]		;Misc
	mov DWORD [ebx+__IMPORT_misc], eax
	
	mov eax, 0
	ret

; Aligned 32-Bit Copy
memcpyd:
memcpyda:
	push ebp
	mov ebp, esp
	push edi
	push esi
	
	mov edi, DWORD[ebp+0x8]
	mov esi, DWORD[ebp+0xC]
	mov ecx, DWORD[ebp+0x10]
	
	cld
	rep movsd
	
	pop esi
	pop edi
	pop ebp
	ret

;Memory Copy
memcpy:
	push ebp
	mov ebp, esp
	push edi
	push esi
	
	mov edi, DWORD[ebp+0x8]
	mov esi, DWORD[ebp+0xC]
	mov ecx, DWORD[ebp+0x10]
	
	cld
	rep movsb
	
	pop esi
	pop edi
	pop ebp
	ret

;Memory Set
memset:
	push ebp
	mov ebp, esp
	push edi
	
	mov edi, DWORD[ebp+0x8]
	mov eax, DWORD[ebp+0xC]
	mov ecx, DWORD[ebp+0x10]
	
	cld
	rep stosb
	
	pop edi
	pop ebp
	ret

;Memory Set Word
memsetw:
memsetwa:
	push ebp
	mov ebp, esp
	push edi
	
	mov edi, DWORD[ebp+0x8]
	mov eax, DWORD[ebp+0xC]
	mov ecx, DWORD[ebp+0x10]
	
	cld
	rep stosw
	
	pop edi
	pop ebp
	ret

;Memory Set DWord
memsetd:
memsetda:
	push ebp
	mov ebp, esp
	push edi
	
	mov edi, DWORD[ebp+0x8]
	mov eax, DWORD[ebp+0xC]
	mov ecx, DWORD[ebp+0x10]
	
	cld
	rep stosd
	
	pop edi
	pop ebp
	ret

; String Length
strlen:
	push ebp
	mov ebp, esp
	push edi
	
	mov edi, DWORD [ebp+0x8]
	xor eax, eax
	xor ecx, ecx
	
	std
	repnz scasb
	mov eax, ecx

	pop edi
	pop ebp
	ret
	
; String Compare
strcmp:
	push ebp
	mov ebp, esp
	push edi
	push esi
	
	mov edi, DWORD [ebp+0x8]	;String 1
	mov esi, DWORD [ebp+0xC]	;String 1
	
	std
.loop:
	scasb
	jne .out
	cmp BYTE [edi-1], 0
	jne .loop
.out:
	
	xor eax, eax
	mov al, [edi]
	sub al, [edi]
	
	pop esi
	pop edi
	pop ebp
	ret

; String Compare (Non case sensitive)
strncmp:
	push ebp
	mov ebp, esp
	push ebx
	push ecx
	mov ebx, DWORD [ebp+0x8]	;String 1
	mov ecx, DWORD [ebp+0xC]	;String 2
.loop:
	mov al, BYTE [ebx]
	sub al, BYTE [ecx]
	jz .loop	;Equal
	cmp al, 0x60	;UC/LC
	jz .loop
	cmp al, (-0x60)	;LC/UC
	jz .loop
.out:
	pop ebp
	ret

; Copy a string
strcpy:
	push ebp
	mov ebp, esp
	push eax
	push ebx
	push ecx
	xor ecx, ecx
	mov eax, DWORD [ebp+0x8]	;Destinaton
	mov ebx, DWORD [ebp+0xC]	;Destinaton
.loop:
	mov cl, BYTE [ebx]
	mov BYTE [eax], cl
	cmp ecx, 0
	jnz .loop
.out:
	pop ecx
	pop ebx
	pop eax
	pop ebp
	ret
	
; Read from a port
inb:
	push edx
	xor eax, eax
	mov dx, WORD [esp+0x8]
	in	al, dx	;IN to `al` from port `dx`
	pop edx
	ret

; Write to a port
outb:
	push edx
	xor eax, eax
	mov dx, WORD [esp+0x8]
	mov al, BYTE [esp+0xC]
	out	dx, al	;OUT from `[esp+0xC]` to port `dx`
	pop edx
	ret
	
; Read from a port
inw:
	push edx
	mov dx, WORD [esp+0x8]
	in	ax, dx	;IN to `eax` from port `dx`
	pop edx
	ret

; Write to a port
outw:
	push edx
	mov dx, WORD [esp+0x8]
	mov ax, WORD [esp+0xC]
	out	dx, ax	;OUT from `eax` to port `dx`
	pop edx
	ret
	
; Read from a port
ind:
	push edx
	mov dx, WORD [esp+0x8]
	in	eax, dx	;IN to `eax` from port `ebx`
	pop edx
	ret

	
; Write to a port
outd:
	push edx
	mov dx, WORD [esp+0x8]
	mov eax, DWORD [esp+0xC]
	out	dx, eax	;OUT from `eax` to port `ebx`
	pop edx
	ret

;======================
;= IMPORTED FUNCTIONS =
;======================
; DevFS
DevFS_AddDevice:
	mov eax, [ebx+__IMPORT_devfs]
	jmp DWORD [eax]
DevFS_DelDevice:
	mov eax, [ebx+__IMPORT_devfs]
	add eax, 0x4
	jmp DWORD [eax]
DevFS_AddSymlink:
	mov eax, [ebx+__IMPORT_devfs]
	add eax, 0x8
	jmp DWORD [eax]
DevFS_DelSymlink:
	mov eax, [ebx+__IMPORT_devfs]
	add eax, 0xC
	jmp DWORD [eax]

; VFS
VFS_AddFS:
	mov eax, [__IMPORT_vfs]
	add eax, 0x0
	jmp DWORD [eax]
VFS_Open:
	mov eax, [__IMPORT_vfs]
	add eax, 0x4
	jmp DWORD [eax]
VFS_Close:
	mov eax, [__IMPORT_vfs]
	add eax, 0x8
	jmp DWORD [eax]
VFS_Read:
	mov eax, [__IMPORT_vfs]
	add eax, 0xC
	jmp DWORD [eax]
VFS_Write:
	mov eax, [__IMPORT_vfs]
	add eax, 0x10
	jmp DWORD [eax]
VFS_Seek:
	mov eax, [__IMPORT_vfs]
	add eax, 0x14
	jmp DWORD [eax]
VFS_Tell:
	mov eax, [__IMPORT_vfs]
	add eax, 0x18
	jmp DWORD [eax]
VFS_ReadDir:
	mov eax, [__IMPORT_vfs]
	add eax, 0x1C
	jmp DWORD [eax]
VFS_Mount:
	mov eax, [__IMPORT_vfs]
	add eax, 0x20
	jmp DWORD [eax]
Inode_InitCache:
	mov eax, [__IMPORT_vfs]
	add eax, 0x24
	jmp DWORD [eax]
Inode_ClearCache:
	mov eax, [__IMPORT_vfs]
	add eax, 0x28
	jmp DWORD [eax]
Inode_GetCache:
	mov eax, [__IMPORT_vfs]
	add eax, 0x2C
	jmp DWORD [eax]
Inode_CacheNode:
	mov eax, [__IMPORT_vfs]
	add eax, 0x30
	jmp DWORD [eax]
Inode_UncacheNode:
	mov eax, [__IMPORT_vfs]
	add eax, 0x34
	jmp DWORD [eax]

;Misc Functions
now:
	mov eax, [__IMPORT_misc]
	add eax, 0x0
	jmp DWORD [eax]
timestamp:
	mov eax, [__IMPORT_misc]
	add eax, 0x4
	jmp DWORD [eax]
malloc:
	mov eax, [__IMPORT_misc]
	add eax, 0x8
	jmp DWORD [eax]
free:
	mov eax, [__IMPORT_misc]
	add eax, 0xC
	jmp DWORD [eax]
realloc:
	mov eax, [__IMPORT_misc]
	add eax, 0x10
	jmp DWORD [eax]
DMA_SetRW:
	mov eax, [__IMPORT_misc]
	add eax, 0x14
	jmp DWORD [eax]
DMA_ReadData:
	mov eax, [__IMPORT_misc]
	add eax, 0x18
	jmp DWORD [eax]
Time_CreateTimer:
	mov eax, [__IMPORT_misc]
	add eax, 0x1C
	jmp DWORD [eax]
Time_RemoveTimer:
	mov eax, [__IMPORT_misc]
	add eax, 0x20
	jmp DWORD [eax]
IRQ_Set:
	mov eax, [__IMPORT_misc]
	add eax, 0x24
	jmp DWORD [eax]
IRQ_Clear:
	mov eax, [__IMPORT_misc]
	add eax, 0x28
	jmp DWORD [eax]
LogF:
	mov eax, [__IMPORT_misc]
	add eax, 0x2C
	jmp DWORD [eax]
MM_MapHW:
	mov eax, [__IMPORT_misc]
	add eax, 0x30
	jmp DWORD [eax]
MM_UnmapHW:
	mov eax, [__IMPORT_misc]
	add eax, 0x34
	jmp DWORD [eax]


; DATA - Note: In .text to allow PC relative addressing
__IMPORT_vfs:	dd	0
__IMPORT_devfs:	dd	0
__IMPORT_register:	dd	0
__IMPORT_misc:	dd	0