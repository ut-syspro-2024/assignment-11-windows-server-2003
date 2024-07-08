#include <efi.h>
#include <file.h>
#include <util.h>
#include "elf.h"

static int
iself(struct ehdr *e)
{
	return (e->e_ident[0] == 0x7f && e->e_ident[1] == 'E' &&
		e->e_ident[2] == 'L' && e->e_ident[3] == 'F') &&
		e->e_type == ET_EXEC;
}

static void
elfread(struct EFI_FILE_PROTOCOL *file, ulong offset, void *dst, UINTN size)
{
	EFI_STATUS status;

	file->SetPosition(file, offset);
	status = file->Read(file, &size, dst);
	assert(status, L"read error");
}

static int
elfstrtable(struct EFI_FILE_PROTOCOL *file, struct ehdr *e, struct shdr *strtable)
{
	ulong offset;
	uint size = sizeof (*strtable);

	if (e->e_shstrndx == SHN_UNDEF)
	{
		return -1;
	}

	offset = e->e_shoff + e->e_shentsize * e->e_shstrndx;

	elfread(file, offset, strtable, size);

	return 0;
}

static void
sectionname(struct EFI_FILE_PROTOCOL *file, struct shdr *strtable, uint sh_name, char *name)
{
	ulong offset = strtable->sh_offset + sh_name;
	UINTN size = 32;

	elfread(file, offset, name, size);
}

static void
getbss(struct EFI_FILE_PROTOCOL *file, struct ehdr *e, struct header *hdr)
{
	struct shdr shdr;
	struct shdr strtable;
	int bssfound = 0;
	UINTN ssz = e->e_shentsize;
	ulong soff;
	char name[32];

	assert (sizeof(shdr) != e->e_shentsize, L"oi");

	elfstrtable(file, e, &strtable);	

	soff = e->e_shoff;
	for (int i = 0; i < e->e_shnum; i++, soff += e->e_shentsize)
	{
		elfread(file, soff, &shdr, ssz);

		sectionname(file, &strtable, shdr.sh_name, name);

		if (compare(".bss", name, 4))
		{
			bssfound = 1;
			puts(L"found bss ");
			break;
		}
	}

	if (bssfound)
	{
		hdr->bss_start = (void *)shdr.sh_addr;
		hdr->bss_size = shdr.sh_size;
	}
	else
	{
		hdr->bss_start = NULL;
		hdr->bss_size = 0;
	}
}

int
loadelf(struct EFI_FILE_PROTOCOL *file, void *dst, struct header *hdr)
{
	struct ehdr e;
	UINTN esz = sizeof (e);
	EFI_STATUS status;
	struct phdr phdr;
	UINTN psz = sizeof (phdr);
	ulong poff;
	ulong filesz;

	status = file->Read(file, &esz, &e);
	assert(status, L"read error");

	if (!iself(&e))
	{
		return -1;
	}

	getbss(file, &e, hdr);

	poff = e.e_phoff;
	for (int i = 0; i < e.e_phnum; i++, poff += sizeof (phdr))
	{
		elfread(file, poff, &phdr, psz);

		if (phdr.p_type != PT_LOAD)
		{
			continue;
		}

		filesz = phdr.p_filesz;

		elfread(file, phdr.p_offset, dst, filesz);
	}

	return 0;
}
