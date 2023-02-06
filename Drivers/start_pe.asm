; Acess Kernel Module
; Assembler Stub

[BITS 32]
ACESSOS_OFFSET	equ 0xC0105000

[extern _ModuleLoad]
[extern _ModuleUnload]
[extern _ModuleIdent]
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
	push _ModuleIdent
	push _ModuleUnload
	mov eax, [__IMPORT_register]
	call eax
	add esp, 12	; Restore Stack
	
	call _ModuleLoad
.fastclose:
	ret

; ====================
; IMPORTED FUNCTIONS
; - Ported from kmod.c
; ====================

[global _memcpy]
[global _memcpyd]
[global _memcpyda]
[global _memset]
[global _memsetw]
[global _memsetd]
[global _memsetda]
[global _strlen]
[global _strcmp]
[global _strncmp]
[global _strcpy]
[global _inb]
[global _inw]
[global _ind]
[global _outb]
[global _outw]
[global _outd]
[global _DevFS_AddDevice]
[global _DevFS_DelDevice]
[global _DevFS_AddSymlink]
[global _DevFS_DelSymlink]
[global _VFS_AddFS]
[global _VFS_Open]
[global _VFS_Close]
[global _VFS_Read]
[global _VFS_Write]
[global _VFS_Seek]
[global _VFS_Tell]
[global _VFS_ReadDir]
[global _VFS_Mount]
[global _Inode_InitCache]
[global _Inode_ClearCache]
[global _Inode_GetCache]
[global _Inode_CacheNode]
[global _Inode_UncacheNode]
[global _now]
[global _timestamp]
[global _malloc]
[global _free]
[global _realloc]
[global _DMA_SetRW]
[global _DMA_ReadData]
[global _Time_CreateTimer]
[global _Time_RemoveTimer]
[global _IRQ_Set]
[global _IRQ_Clear]
[global _LogF]
[global _MM_MapHW]
[global _MM_UnmapHW]


__IMPORT_vfs	dd	0
__IMPORT_devfs	dd	0
__IMPORT_register	dd	0
__IMPORT_misc	dd	0

__IMPORT_init:
	cmp DWORD [ecx], 0xACE55051
	jz	.init
	mov eax, 1
	ret
.init:
	mov eax, DWORD [ecx+0x04]		;VFS
	mov DWORD [__IMPORT_vfs], eax
	mov eax, DWORD [ecx+0x08]		;DevFS
	mov DWORD [__IMPORT_devfs], eax
	mov eax, DWORD [ecx+0x0C]		;Module Register
	mov DWORD [__IMPORT_register], eax
	mov eax, DWORD [ecx+0x10]		;Misc
	mov DWORD [__IMPORT_misc], eax
	
	mov eax, 0
	ret
	
; Aligned 32-Bit Copy
_memcpyd:
_memcpyda:
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
_memcpy:
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
_memset:
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
_memsetw:
_memsetwa:
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
_memsetd:
_memsetda:
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
_strlen:
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
_strcmp:
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
_strncmp:
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
_strcpy:
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
_inb:
	push edx
	xor eax, eax
	mov dx, WORD [esp+0x8]
	in	al, dx	;IN to `al` from port `dx`
	pop edx
	ret

; Write to a port
_outb:
	push edx
	xor eax, eax
	mov dx, WORD [esp+0x8]
	mov al, BYTE [esp+0xC]
	out	dx, al	;OUT from `[esp+0xC]` to port `dx`
	pop edx
	ret
	
; Read from a port
_inw:
	push edx
	mov dx, WORD [esp+0x8]
	in	ax, dx	;IN to `eax` from port `dx`
	pop edx
	ret

; Write to a port
_outw:
	push edx
	mov dx, WORD [esp+0x8]
	mov ax, WORD [esp+0xC]
	out	dx, ax	;OUT from `eax` to port `dx`
	pop edx
	ret
	
; Read from a port
_ind:
	push edx
	mov dx, WORD [esp+0x8]
	in	eax, dx	;IN to `eax` from port `ebx`
	pop edx
	ret

	
; Write to a port
_outd:
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
_DevFS_AddDevice:
	mov eax, [__IMPORT_devfs]
	jmp DWORD [eax]
_DevFS_DelDevice:
	mov eax, [__IMPORT_devfs]
	add eax, 0x4
	jmp DWORD [eax]
_DevFS_AddSymlink:
	mov eax, [__IMPORT_devfs]
	add eax, 0x8
	jmp DWORD [eax]
_DevFS_DelSymlink:
	mov eax, [__IMPORT_devfs]
	add eax, 0xC
	jmp DWORD [eax]

; VFS
_VFS_AddFS:
	mov eax, [__IMPORT_vfs]
	add eax, 0x0
	jmp DWORD [eax]
_VFS_Open:
	mov eax, [__IMPORT_vfs]
	add eax, 0x4
	jmp DWORD [eax]
_VFS_Close:
	mov eax, [__IMPORT_vfs]
	add eax, 0x8
	jmp DWORD [eax]
_VFS_Read:
	mov eax, [__IMPORT_vfs]
	add eax, 0xC
	jmp DWORD [eax]
_VFS_Write:
	mov eax, [__IMPORT_vfs]
	add eax, 0x10
	jmp DWORD [eax]
_VFS_Seek:
	mov eax, [__IMPORT_vfs]
	add eax, 0x14
	jmp DWORD [eax]
_VFS_Tell:
	mov eax, [__IMPORT_vfs]
	add eax, 0x18
	jmp DWORD [eax]
_VFS_ReadDir:
	mov eax, [__IMPORT_vfs]
	add eax, 0x1C
	jmp DWORD [eax]
_VFS_Mount:
	mov eax, [__IMPORT_vfs]
	add eax, 0x20
	jmp DWORD [eax]
_Inode_InitCache:
	mov eax, [__IMPORT_vfs]
	add eax, 0x24
	jmp DWORD [eax]
_Inode_ClearCache:
	mov eax, [__IMPORT_vfs]
	add eax, 0x28
	jmp DWORD [eax]
_Inode_GetCache:
	mov eax, [__IMPORT_vfs]
	add eax, 0x2C
	jmp DWORD [eax]
_Inode_CacheNode:
	mov eax, [__IMPORT_vfs]
	add eax, 0x30
	jmp DWORD [eax]
_Inode_UncacheNode:
	mov eax, [__IMPORT_vfs]
	add eax, 0x34
	jmp DWORD [eax]

;Misc Functions
_now:
	mov eax, [__IMPORT_misc]
	add eax, 0x0
	jmp DWORD [eax]
_timestamp:
	mov eax, [__IMPORT_misc]
	add eax, 0x4
	jmp DWORD [eax]
_malloc:
	mov eax, [__IMPORT_misc]
	add eax, 0x8
	jmp DWORD [eax]
_free:
	mov eax, [__IMPORT_misc]
	add eax, 0xC
	jmp DWORD [eax]
_realloc:
	mov eax, [__IMPORT_misc]
	add eax, 0x10
	jmp DWORD [eax]
_DMA_SetRW:
	mov eax, [__IMPORT_misc]
	add eax, 0x14
	jmp DWORD [eax]
_DMA_ReadData:
	mov eax, [__IMPORT_misc]
	add eax, 0x18
	jmp DWORD [eax]
_Time_CreateTimer:
	mov eax, [__IMPORT_misc]
	add eax, 0x1C
	jmp DWORD [eax]
_Time_RemoveTimer:
	mov eax, [__IMPORT_misc]
	add eax, 0x20
	jmp DWORD [eax]
_IRQ_Set:
	mov eax, [__IMPORT_misc]
	add eax, 0x24
	jmp DWORD [eax]
_IRQ_Clear:
	mov eax, [__IMPORT_misc]
	add eax, 0x28
	jmp DWORD [eax]
_LogF:
	mov eax, [__IMPORT_misc]
	add eax, 0x2C
	jmp DWORD [eax]
_MM_MapHW:
	mov eax, [__IMPORT_misc]
	add eax, 0x30
	jmp DWORD [eax]
_MM_UnmapHW:
	mov eax, [__IMPORT_misc]
	add eax, 0x34
	jmp DWORD [eax]
