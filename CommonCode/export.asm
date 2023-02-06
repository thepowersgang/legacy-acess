; AcessOS 0.1
; Exported Functions
;
; Provides a contstant point for all kernel modules to look
; for kernel functions (VFS etc)

[BITS 32]

; =====
; = Double Pointers - List in relevant file
; =====
[global _k_export_magic]
[extern _vfs_functions]
[extern _devfs_functions]
[extern _Module_Register]	;Function
[extern _lib_functions]
_k_export_magic:	dd	0xACE55051
_k_vfs_export:		dd	_vfs_functions
_k_devfs_export:	dd	_devfs_functions
;_k_module_export:	dd	_Module_Register
_k_module_export:	dd	0
_k_misc_export:		dd	miscFunctions

;Miscelaneous Functions
extern _now
extern _time_wait
extern _timestamp
extern _malloc
extern _free
extern _realloc
extern _dma_setChannel
extern _dma_readData
extern _time_createTimer
extern _time_removeTimer
extern _irq_install_handler
extern _irq_uninstall_handler
extern	_LogF
extern	_MM_MapHW
extern	_MM_UnmapHW
miscFunctions:
dd	_now
dd	_time_wait
dd	_timestamp
dd	_malloc
dd	_free
dd	_realloc
dd	_dma_setChannel
dd	_dma_readData
dd	_time_createTimer
dd	_time_removeTimer
dd	_irq_install_handler
dd	_irq_uninstall_handler
dd	_LogF
dd	_MM_MapHW
dd	_MM_UnmapHW

[extern _LogF]
[extern _puts]
[extern _vfs_seek]
[global k_printf]
[global k_puts]
[global k_fseek]
k_printf:	jmp _LogF
k_fseek:	jmp	_vfs_seek
k_puts:		jmp _puts