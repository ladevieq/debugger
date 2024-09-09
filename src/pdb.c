void open_pdb(const char* pdb_path) {
    HANDLE pdb_file = CreateFileA(pdb_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (pdb_file == INVALID_HANDLE_VALUE) {
        print("Cannot open pdb file at path %s due to error %u\n", pdb_path, GetLastError());
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
    u32 num_streams = ((u32*)block)[0U];
    u32* stream_sizes = &((u32*)block)[1U];
    u32* stream_blocks = stream_sizes + num_streams;
    u32* pdb_info_stream_blocks = NULL;
    u32* pdb_DBI_stream_blocks = NULL;

    u32 block_index = 0U;
    for(u32 stream_id = 0u; stream_id < num_streams; stream_id++) {
        print("PDB stream size : %u\n", stream_sizes[stream_id]);
        if (stream_id == 1U) {
            pdb_info_stream_blocks = &stream_blocks[block_index];
        }
        if (stream_id == 3U) {
            pdb_DBI_stream_blocks = &stream_blocks[block_index];
        }
        if (stream_sizes[stream_id] == (u32)-1) {
            continue;
        }
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
    SetFilePointer(pdb_file, pdb_info_stream_blocks[0U] * sizeof(block), NULL, FILE_BEGIN);
    ReadFile(pdb_file, (void*)&pdb_info_stream, sizeof(pdb_info_stream), (LPDWORD)&b_read, NULL);
    pdb_info_stream;

    // DBI Stream
    struct pdb_DBI_stream {
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
    struct pdb_DBI_stream pdb_DBI_stream = { 0U };
    SetFilePointer(pdb_file, pdb_DBI_stream_blocks[0U] * sizeof(block), NULL, FILE_BEGIN);
    ReadFile(pdb_file, (void*)&pdb_DBI_stream, sizeof(pdb_DBI_stream), (LPDWORD)&b_read, NULL);
    print("");
    pdb_DBI_stream;

    // Global Symbol Stream
    SetFilePointer(pdb_file, pdb_DBI_stream.global_stream_index * sizeof(block), NULL, FILE_BEGIN);
    ReadFile(pdb_file, (void*)&pdb_DBI_stream, sizeof(pdb_DBI_stream), (LPDWORD)&b_read, NULL);
}
