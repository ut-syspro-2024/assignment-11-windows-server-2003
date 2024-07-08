
.global io_read32
.global io_read16
.global io_read8
.global io_write32
.global io_write16
.global io_write8

.text
io_read32:
	mov %di, %dx
	in %dx, %eax
	ret

io_read16:
	mov %di, %dx
	in %dx, %ax
	ret

io_read8:
	mov %di, %dx
	in %dx, %al
	ret

io_write32:
	mov %di, %dx
	mov %esi, %eax
	out %eax, %dx
	ret

io_write16:
	mov %di, %dx
	mov %si, %ax
	out %ax, %dx
	ret

io_write8:
	mov %di, %dx
	mov %sil, %al
	out %al, %dx
	ret
