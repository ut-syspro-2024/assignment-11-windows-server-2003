typedef unsigned char 		uchar;
typedef unsigned short 		ushort;
typedef unsigned int 		uint;
typedef unsigned long long 	ulong;

/* e_type */
#define ET_NONE 0
#define ET_REL  1
#define ET_EXEC 2
#define ET_DYN  3
#define ET_CORE 4

/* p_type */
#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC  2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB  5
#define PT_PHDR 6

/* p_flags */
#define PF_X  0x1
#define PF_W  0x2
#define PF_R  0x4
#define PF_MASKOS 0x00ff0000
#define PF_MASKPROC 0xff000000

#define SHN_UNDEF	0

struct ehdr {
	uchar	e_ident[16];
	ushort	e_type;
	ushort	e_machine;
	uint	e_version;
	ulong	e_entry;
	ulong	e_phoff;
	ulong	e_shoff;
	uint	e_flags;
	ushort	e_ehsize;
	ushort	e_phentsize;
	ushort	e_phnum;
	ushort	e_shentsize;
	ushort	e_shnum;
	ushort	e_shstrndx;
} __attribute__((packed));

struct phdr {
	uint	p_type;
	uint	p_flags;
	ulong	p_offset;
	ulong	p_vaddr;
	ulong	p_paddr;
	ulong	p_filesz;
	ulong	p_memsz;
	ulong	p_align;
} __attribute__((packed));

struct shdr {
	uint	sh_name;
	uint	sh_type;
	ulong	sh_flags;
	ulong	sh_addr;
	ulong	sh_offset;
	ulong	sh_size;
	uint	sh_link;
	uint	sh_info;
	ulong	sh_addralign;
	ulong	sh_entsize;
} __attribute__((packed));

//////////////////////////////////////////////

struct header {
	void *bss_start;
	UINTN bss_size;
};

int loadelf(struct EFI_FILE_PROTOCOL *elf, void *dst, struct header *hdr);
