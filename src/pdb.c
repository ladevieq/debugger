#include "cvinfo.h"

enum STREAM_INDEX {
    OLD_DIRECTORY,
    PDB,
    TPI,
    DBI,
    IPI,
};

void open_pdb(const char* pdb_path, struct module* module) {
    HANDLE pdb_file = CreateFileA(pdb_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (pdb_file == INVALID_HANDLE_VALUE) {
        print("Cannot open pdb file at path %s due to error %u\n", pdb_path, GetLastError());
        return;
    }

    // MSF superblock
    struct msf_superblock {
        u8 magic[32U];
        u32 block_size;
        u32 free_block_map_block;
        u32 num_blocks;
        u32 num_directory_bytes;
        u32 unknown;
        u32 block_map_addr;
    };
    struct msf_superblock superblock = { 0U };
    u8 block[4096U] = { 0U };
    u32 b_read = 0U;
    ReadFile(pdb_file, (void*)&superblock, sizeof(superblock), (LPDWORD)&b_read, NULL);

    // Stream directory
    u32 dir_addr = (superblock.block_map_addr - 1U) * sizeof(block);
    SetFilePointer(pdb_file, dir_addr, NULL, FILE_BEGIN);
    ReadFile(pdb_file, (void*)block, superblock.num_directory_bytes, (LPDWORD)&b_read, NULL);

    // Loog through streams
    u32 num_streams = ((u32*)block)[0U];
    u32* stream_sizes = &((u32*)block)[1U];
    u32* stream_blocks = stream_sizes + num_streams;
    u32* stream_blocks_indices[1024U] = { 0U };

    u32 block_index = 0U;
    for(u32 stream_id = 0u; stream_id < num_streams; stream_id++) {
        print("PDB stream size : %u\n", stream_sizes[stream_id]);

        if (stream_sizes[stream_id] == (u32)-1) {
            print("PDB stream blocks : NONE\n");
            continue;
        }

        if (stream_sizes[stream_id] == 0U) {
            continue;
        }

        stream_blocks_indices[stream_id] = &stream_blocks[block_index];
        for(u32 id = 0u; id < (stream_sizes[stream_id] / sizeof(block)) + 1U; id++) {
            print("PDB stream blocks : %u\n", stream_blocks[block_index]);
            block_index++;
        }
    }

    // PDB Info Stream
    struct pdb_info_stream {
        u32 version;
        u32 signature;
        u32 age;
        u32 guid[4u];
    };

    struct pdb_info_stream pdb_info_stream = { 0U };
    SetFilePointer(pdb_file, stream_blocks_indices[PDB][0U] * sizeof(block), NULL, FILE_BEGIN);
    ReadFile(pdb_file, (void*)&pdb_info_stream, sizeof(pdb_info_stream), (LPDWORD)&b_read, NULL);

    // DBI Stream
    struct DBI_stream_header {
        i32 version_signature;
        u32 version_header;
        u32 age;
        u16 global_stream_index;
        u16 build_number;
        u16 public_stream_index;
        u16 pdb_dll_version;
        u16 sym_record_stream;
        u16 pdb_dll_rbld;
        i32 mod_info_size;
        i32 section_contribution_size;
        i32 section_map_size;
        i32 source_info_size;
        i32 type_server_map_size;
        u32 mfc_type_server_index;
        i32 optional_dbg_header_size;
        i32 ec_substream_size;
        u16 flags;
        u16 machine;
        u32 padding;
    };

    struct mod_info {
        u32 Unused1;
        struct SectionContribEntry {
            u16 Section;
            u8 Padding1[2];
            i32 Offset;
            i32 Size;
            u32 Characteristics;
            u16 ModuleIndex;
            u8 Padding2[2];
            u32 DataCrc;
            u32 RelocCrc;
        } SectionContr;
        u16 Flags;
        u16 ModuleSymStream;
        u32 SymByteSize;
        u32 C11ByteSize;
        u32 C13ByteSize;
        u16 SourceFileCount;
        u8 Padding[2];
        u32 Unused2;
        u32 SourceFileNameIndex;
        u32 PdbFilePathNameIndex;
    } *modules_info[64U];
    u8* modules_name[64U] = { 0U };
    u8* obj_files_name[64U] = { 0U };
    u8 DBI_block[4096U] = { 0U };
    SetFilePointer(pdb_file, stream_blocks_indices[DBI][0U] * sizeof(block), NULL, FILE_BEGIN);
    ReadFile(pdb_file, (void*)&DBI_block, sizeof(DBI_block), (LPDWORD)&b_read, NULL);
    struct DBI_stream_header* DBI_stream_header = (struct DBI_stream_header*)DBI_block;

    // Read Module Info Substreams
    i32 byte_index = sizeof(*DBI_stream_header);
    for (u32 module_index = 0U; byte_index < DBI_stream_header->mod_info_size; module_index++) {
        modules_info[module_index] = (struct mod_info*)&DBI_block[byte_index];
        modules_name[module_index] = &DBI_block[byte_index + sizeof(struct mod_info)];
        print("Module symbol stream index %xu\n", modules_info[module_index]->ModuleSymStream);
        print("Module name : %s\n", modules_name[module_index]);
        size_t module_name_length = 0U;
        StringCchLengthA((STRSAFE_LPSTR)modules_name[module_index], STRSAFE_MAX_CCH, &module_name_length);
        obj_files_name[module_index] = &DBI_block[byte_index + sizeof(struct mod_info) + module_name_length + 1U];

        print("Obj file name : %s\n", obj_files_name[module_index]);
        size_t obj_file_name_length = 0U;
        StringCchLengthA((STRSAFE_LPSTR)obj_files_name[module_index], STRSAFE_MAX_CCH, &obj_file_name_length);

        u32 module_info_size = sizeof(*modules_info[0U]) + (i32)module_name_length + 1 + (i32)obj_file_name_length + 1;
        module_info_size = ((module_info_size + 3U) / 4U) * 4U;
        byte_index += module_info_size;

        // Read curren Module Info Stream
        u8 module_info_block[4096U] = { 0U };
        SetFilePointer(pdb_file, stream_blocks_indices[modules_info[module_index]->ModuleSymStream][0U] * sizeof(module_info_block), NULL, FILE_BEGIN);
        ReadFile(pdb_file, (void*)&module_info_block, sizeof(module_info_block), (LPDWORD)&b_read, NULL);

        u8* symbols = &module_info_block[4U];
        u8* cur_sym = symbols;
        while ((u32)(cur_sym - symbols) < modules_info[module_index]->SymByteSize - 4U) {
            struct SYMTYPE* rec = (struct SYMTYPE*)cur_sym;
            if (rec->rectyp == S_END) {
                print("END SYMBOL\n");
            } else if (rec->rectyp == S_FRAMEPROC) {
                struct FRAMEPROCSYM* frameproc = (struct FRAMEPROCSYM*)cur_sym;
                frameproc;
                print("FRAMEPROC SYMBOL\n");
            } else if (rec->rectyp == S_OBJNAME) {
                struct OBJNAMESYM* objname = (struct OBJNAMESYM*)cur_sym;
                print("OBJNAME SYMBOL %s\n", objname->name);
            } else if (rec->rectyp == S_THUNK32) {
                struct THUNKSYM32* thunk = (struct THUNKSYM32*)cur_sym;
                print("THUNK SYMBOL %s\n", thunk->name);
            } else if (rec->rectyp == S_PUB32) {
                struct PUBSYM32* pub = (struct PUBSYM32*)cur_sym;
                print("PUBLIC SYMBOL %s\n", pub->name);
            } else if (rec->rectyp == S_GPROC32) {
                struct PROCSYM32* proc = (struct PROCSYM32*)cur_sym;
                print("PROC32 SYMBOL %s\n", proc->name);
                // TODO: Add string management code
                assert(string_stack_current - 16U * 4096U < string_stack_base);
                size_t length = 0U;
                StringCchLengthA((PCNZCH)proc->name, STRSAFE_MAX_CCH, &length);
                StringCchCopyA((LPSTR)string_stack_current, STRSAFE_MAX_CCH, (LPCSTR)proc->name);
                string_stack_current[length] = '\0';
                u32 start_address = proc->off + module->nt_header.OptionalHeader.BaseOfCode;
#ifdef HASHMAP
                insert_elem(&module->symbols, start_address, proc->len + module->nt_header.OptionalHeader.BaseOfCode, (u64)string_stack_current);
#else
                for (u32 index = 0U; index < module->functions_count; index++) {
                    if (module->functions_start[index] == start_address) {
                        module->functions_name[index] = string_stack_current;
                        break;
                    }
                }
#endif
                string_stack_current += length + 1U;
            } else if (rec->rectyp == S_REGREL32) {
                struct REGREL32* regrel = (struct REGREL32*)cur_sym;
                print("REGREL32 SYMBOL %s\n", regrel->name);
            } else if (rec->rectyp == S_COMPILE2) {
                struct COMPILESYM* compile = (struct COMPILESYM*)cur_sym;
                compile;
                print("COMPILE2 SYMBOL\n");
            } else if (rec->rectyp == S_SECTION) {
                struct SECTIONSYM* section = (struct SECTIONSYM*)cur_sym;
                print("SECTION SYMBOL index %u, name %s, rva %u, size %u\n", section->isec, section->name, section->rva, section->cb);
            } else if (rec->rectyp == S_COFFGROUP) {
                struct COFFGROUPSYM* coff = (struct COFFGROUPSYM*)cur_sym;
                print("COFFGROUP SYMBOL %s\n", coff->name);
            } else if (rec->rectyp == S_CALLSITEINFO) {
                struct CALLSITEINFO* callsiteinfo = (struct CALLSITEINFO*)cur_sym;
                callsiteinfo;
                print("CALLSITEINFO SYMBOL\n");
            } else if (rec->rectyp == S_COMPILE3) {
                struct COMPILESYM3* compile = (struct COMPILESYM3*)cur_sym;
                print("COMPILE3 SYMBOL %s\n", compile->verSz);
            } else if (rec->rectyp == S_BUILDINFO) {
                struct BUILDINFOSYM* buildinfo = (struct BUILDINFOSYM*)cur_sym;
                buildinfo;
                print("BUILDINFO SYMBOL\n");
            } else {
                print("Unknown symbol kind %xu\n", rec->rectyp);
            }
            cur_sym = (u8*)NextSym((SYMTYPE*)cur_sym);
        }
    }

    // Global Symbol Stream
    SetFilePointer(pdb_file, stream_blocks_indices[DBI_stream_header->global_stream_index][0U] * sizeof(block), NULL, FILE_BEGIN);
    ReadFile(pdb_file, (void*)&block, sizeof(DBI_stream_header), (LPDWORD)&b_read, NULL);

    // Public Symbol Stream
    SetFilePointer(pdb_file, stream_blocks_indices[DBI_stream_header->public_stream_index][0U] * sizeof(block), NULL, FILE_BEGIN);
    ReadFile(pdb_file, (void*)&block, sizeof(block), (LPDWORD)&b_read, NULL);

    CloseHandle(pdb_file);
}
