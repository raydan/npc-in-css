
#include "extension.h"

struct signature_t
{
	void *allocBase;
	void *memInBase;
	size_t memSize;
	void *offset;
	const char *sig;
	size_t siglen;
};

void *ResolveSig(void *memInBase, const char *pattern, size_t siglen);
bool ResolveAddress(signature_t *sigmem);

bool CheckScanFunction(const char *sign_value, void *laddr)
{
	char real_sig[511];
	size_t real_bytes;
	size_t length;
	real_bytes = 0;
	length = strlen(sign_value);
	for (size_t i=0; i<length; i++)
	{
		if (real_bytes >= sizeof(real_sig))
		{
			break;
		}
		real_sig[real_bytes++] = sign_value[i];
		if (sign_value[i] == '\\' && sign_value[i+1] == 'x')
		{
			if (i + 3 >= length)
			{
				continue;
			}
			char s_byte[3];
			int r_byte;
			s_byte[0] = sign_value[i+2];
			s_byte[1] = sign_value[i+3];
			s_byte[2] = '\0';
			sscanf(s_byte, "%x", &r_byte);
			real_sig[real_bytes-1] = r_byte;
			i += 3;
		}
	}

	void *final_addr = ResolveSig(laddr, real_sig, real_bytes);
	return (final_addr == NULL) ? false : true;
}

void *ResolveSig(void *memInBase, const char *pattern, size_t siglen)
{
	signature_t sig;

	memset(&sig, 0, sizeof(signature_t));

	sig.sig = (const char *)pattern;
	sig.siglen = siglen;
	sig.memInBase = memInBase;

	if (!ResolveAddress(&sig))
		return NULL;

	const char *paddr = (const char *)sig.allocBase;
	bool found;

#ifdef _DEBUG
	int found_count = 0;
	bool done = false;
#endif

	register unsigned int j;

	sig.memSize -= sig.siglen;	//prevent a crash maybe?
	
	for (size_t i=0; i<sig.memSize; i+=sizeof(unsigned long *))
	{
		found = true;
		for (j=0; j<sig.siglen; j++)
		{
			if ( (pattern[j] != (char)0x2A) &&
				 (pattern[j] != paddr[j]) )
			{
				found = false;
				break;
			}
		}
		if (found)
		{
#ifdef _DEBUG
			if(done == false) {
				sig.offset = (void *)paddr;
				done = true;
			}
			found_count++;
#else
			sig.offset = (void *)paddr;
			break;
#endif
		}
		//we're always gonna be on a four byte boundary
		paddr += sizeof(unsigned long *);
	}

#ifdef _DEBUG
	if(found_count > 1) {
		return NULL;
	}
#endif

	return sig.offset;
}

bool ResolveAddress(signature_t *sigmem)
{
#ifdef WIN32
	MEMORY_BASIC_INFORMATION mem;

	if (!VirtualQuery(sigmem->memInBase, &mem, sizeof(MEMORY_BASIC_INFORMATION)))
		return false;

	if (mem.AllocationBase == NULL)
		return false;

	HMODULE dll = (HMODULE)mem.AllocationBase;

	//code adapted from hullu's linkent patch
	union 
	{
		unsigned long mem;
		IMAGE_DOS_HEADER *dos;
		IMAGE_NT_HEADERS *pe;
	} dllmem;

	dllmem.mem = (unsigned long)dll;

	if (IsBadReadPtr(dllmem.dos, sizeof(IMAGE_DOS_HEADER)) || (dllmem.dos->e_magic != IMAGE_DOS_SIGNATURE))
		return false;

	dllmem.mem = ((unsigned long)dll + (unsigned long)(dllmem.dos->e_lfanew));
	if (IsBadReadPtr(dllmem.pe, sizeof(IMAGE_NT_HEADERS)) || (dllmem.pe->Signature != IMAGE_NT_SIGNATURE))
		return false;

	//end adapted hullu's code

	IMAGE_NT_HEADERS *pe = dllmem.pe;

	sigmem->allocBase = mem.AllocationBase;
	sigmem->memSize = (DWORD)(pe->OptionalHeader.SizeOfImage);

	return true;
#else
	Dl_info info;

	if (!dladdr(sigmem->memInBase, &info))
		return false;

	if (!info.dli_fbase || !info.dli_fname)
		return false;

	sigmem->allocBase = info.dli_fbase;

	pid_t pid = getpid();
	char file[255];
	char buffer[2048];
	snprintf(file, sizeof(file)-1, "/proc/%d/maps", pid);
	FILE *fp = fopen(file, "rt");
	if (!fp)
		return false;
	void *start=NULL;
	void *end=NULL;
	void *found=NULL;
	while (!feof(fp))
	{
		fgets(buffer, sizeof(buffer)-1, fp);
#if defined AMD64
		sscanf(buffer, "%Lx-%Lx", &start, &end);
#else
		sscanf(buffer, "%lx-%lx", &start, &end);
#endif

		if (start == sigmem->allocBase)
		{
			found = end;
			break;
		}
	}
	fclose(fp);

	if (!found)
		return false;

	sigmem->memSize = (unsigned long)end - (unsigned long)start;

#ifdef DEBUG
	Msg("Alloc base: %p\n", sigmem->allocBase);
#endif

	return true;
#endif
}



