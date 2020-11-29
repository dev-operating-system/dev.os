#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <protos/stivale.h>
#include <protos/stivale2.h>
#include <lib/elf.h>
#include <lib/blib.h>
#include <lib/acpi.h>
#include <lib/config.h>
#include <lib/time.h>
#include <lib/print.h>
#include <lib/rand.h>
#include <lib/real.h>
#include <lib/libc.h>
#include <lib/uri.h>
#include <sys/smp.h>
#include <sys/cpu.h>
#include <drivers/vbe.h>
#include <lib/term.h>
#include <sys/pic.h>
#include <sys/lapic.h>
#include <fs/file.h>
#include <mm/pmm.h>
#include <stivale/stivale2.h>

#define KASLR_SLIDE_BITMASK 0x03FFFF000u

struct stivale2_struct stivale2_struct = {0};

inline static size_t get_phys_addr(uint64_t addr) {
    if (addr & ((uint64_t)1 << 63))
        return addr - FIXED_HIGHER_HALF_OFFSET_64;
    return addr;
}

static void *get_tag(struct stivale2_header *s, uint64_t id) {
    struct stivale2_tag *tag = (void*)get_phys_addr(s->tags);
    for (;;) {
        if (tag == NULL)
            return NULL;
        if (tag->identifier == id)
            return tag;
        tag = (void*)get_phys_addr(tag->next);
    }
}

static void append_tag(struct stivale2_struct *s, struct stivale2_tag *tag) {
    tag->next = s->tags;
    s->tags   = (uint64_t)(size_t)tag;
}

void stivale2_load(char *cmdline) {
    char buf[128];

    struct file_handle *kernel = conv_mem_alloc(sizeof(struct file_handle));

    if (!config_get_value(buf, 0, 128, "KERNEL_PATH"))
        panic("KERNEL_PATH not specified");

    if (!uri_open(kernel, buf))
        panic("Could not open kernel resource");

    struct stivale2_header stivale2_hdr;

    int bits = elf_bits(kernel);

    int ret;

    uint64_t slide = 0;

    bool level5pg = false;
    switch (bits) {
        case 64: {
            // Check if 64 bit CPU
            uint32_t eax, ebx, ecx, edx;
            if (!cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx) || !(edx & (1 << 29))) {
                panic("stivale: This CPU does not support 64-bit mode.");
            }
            // Check if 5-level paging is available
            if (cpuid(0x00000007, 0, &eax, &ebx, &ecx, &edx) && (ecx & (1 << 16))) {
                print("stivale: CPU has 5-level paging support\n");
                level5pg = true;
            }

            ret = elf64_load_section(kernel, &stivale2_hdr, ".stivale2hdr", sizeof(struct stivale2_header), slide);

            if (!ret && (stivale2_hdr.flags & 1)) {
                // KASLR is enabled, set the slide
                slide = rand64() & KASLR_SLIDE_BITMASK;

                // Re-read the .stivale2hdr with slid relocations
                ret = elf64_load_section(kernel, &stivale2_hdr, ".stivale2hdr", sizeof(struct stivale2_header), slide);
            }

            break;
        }
        case 32:
            ret = elf32_load_section(kernel, &stivale2_hdr, ".stivale2hdr", sizeof(struct stivale2_header));
            break;
        default:
            panic("stivale2: Not 32 nor 64 bit x86 ELF file.");
    }

    print("stivale2: %u-bit ELF file detected\n", bits);

    switch (ret) {
        case 1:
            panic("stivale2: File is not a valid ELF.");
        case 2:
            panic("stivale2: Section .stivale2hdr not found.");
        case 3:
            panic("stivale2: Section .stivale2hdr exceeds the size of the struct.");
        case 4:
            panic("stivale2: Section .stivale2hdr is smaller than size of the struct.");
    }

    print("stivale2: Requested stack at %X\n", stivale2_hdr.stack);

    uint64_t entry_point   = 0;
    uint64_t top_used_addr = 0;

    switch (bits) {
        case 64:
            elf64_load(kernel, &entry_point, &top_used_addr, slide, 0x1001);
            break;
        case 32:
            elf32_load(kernel, (uint32_t *)&entry_point, (uint32_t *)&top_used_addr, 0x1001);
            break;
    }

    if (stivale2_hdr.entry_point != 0)
        entry_point = stivale2_hdr.entry_point;

    print("stivale2: Kernel slide: %X\n", slide);

    print("stivale2: Top used address in ELF: %X\n", top_used_addr);

    strcpy(stivale2_struct.bootloader_brand, "Limine");
    strcpy(stivale2_struct.bootloader_version, LIMINE_VERSION);

    //////////////////////////////////////////////
    // Create firmware struct tag
    //////////////////////////////////////////////
    {
    struct stivale2_struct_tag_firmware *tag = conv_mem_alloc(sizeof(struct stivale2_struct_tag_firmware));
    tag->tag.identifier = STIVALE2_STRUCT_TAG_FIRMWARE_ID;

    tag->flags = 1 << 0;   // bit 0 = BIOS boot

    append_tag(&stivale2_struct, (struct stivale2_tag *)tag);
    }

    //////////////////////////////////////////////
    // Create modules struct tag
    //////////////////////////////////////////////
    {
    size_t module_count;
    for (module_count = 0; ; module_count++) {
        char module_file[64];
        if (!config_get_value(module_file, module_count, 64, "MODULE_PATH"))
            break;
    }

    struct stivale2_struct_tag_modules *tag =
        conv_mem_alloc(sizeof(struct stivale2_struct_tag_modules)
                     + sizeof(struct stivale2_module) * module_count);

    tag->tag.identifier = STIVALE2_STRUCT_TAG_MODULES_ID;
    tag->module_count   = module_count;

    for (int i = 0; ; i++) {
        char module_file[64];
        if (!config_get_value(module_file, i, 64, "MODULE_PATH"))
            break;

        struct stivale2_module *m = &tag->modules[i];

        if (!config_get_value(m->string, i, 128, "MODULE_STRING")) {
            m->string[0] = '\0';
        }

        struct file_handle f;
        if (!uri_open(&f, module_file))
            panic("Requested module with path \"%s\" not found!\n", module_file);

        void *module_addr = (void *)(((uint32_t)top_used_addr & 0xfff) ?
            ((uint32_t)top_used_addr & ~((uint32_t)0xfff)) + 0x1000 :
            (uint32_t)top_used_addr);

        print("stivale2: Loading module `%s`...\n", module_file);

        memmap_alloc_range((size_t)module_addr, f.size, 0x1001);
        fread(&f, module_addr, 0, f.size);

        m->begin = (uint64_t)(size_t)module_addr;
        m->end   = m->begin + f.size;

        top_used_addr = (uint64_t)(size_t)m->end;

        print("stivale2: Requested module %u:\n", i);
        print("          Path:   %s\n", module_file);
        print("          String: %s\n", m->string);
        print("          Begin:  %X\n", m->begin);
        print("          End:    %X\n", m->end);
    }

    append_tag(&stivale2_struct, (struct stivale2_tag *)tag);
    }

    //////////////////////////////////////////////
    // Create RSDP struct tag
    //////////////////////////////////////////////
    {
    struct stivale2_struct_tag_rsdp *tag = conv_mem_alloc(sizeof(struct stivale2_struct_tag_rsdp));
    tag->tag.identifier = STIVALE2_STRUCT_TAG_RSDP_ID;

    tag->rsdp = (uint64_t)(size_t)acpi_get_rsdp();

    append_tag(&stivale2_struct, (struct stivale2_tag *)tag);
    }

    //////////////////////////////////////////////
    // Create cmdline struct tag
    //////////////////////////////////////////////
    {
    struct stivale2_struct_tag_cmdline *tag = conv_mem_alloc(sizeof(struct stivale2_struct_tag_cmdline));
    tag->tag.identifier = STIVALE2_STRUCT_TAG_CMDLINE_ID;

    tag->cmdline = (uint64_t)(size_t)cmdline;

    append_tag(&stivale2_struct, (struct stivale2_tag *)tag);
    }

    //////////////////////////////////////////////
    // Create epoch struct tag
    //////////////////////////////////////////////
    {
    struct stivale2_struct_tag_epoch *tag = conv_mem_alloc(sizeof(struct stivale2_struct_tag_epoch));
    tag->tag.identifier = STIVALE2_STRUCT_TAG_EPOCH_ID;

    tag->epoch = time();
    print("stivale2: Current epoch: %U\n", tag->epoch);

    append_tag(&stivale2_struct, (struct stivale2_tag *)tag);
    }

    //////////////////////////////////////////////
    // Create framebuffer struct tag
    //////////////////////////////////////////////
    {
    struct stivale2_header_tag_framebuffer *hdrtag = get_tag(&stivale2_hdr, STIVALE2_HEADER_TAG_FRAMEBUFFER_ID);

    term_deinit();

    if (hdrtag != NULL) {
        int req_width  = hdrtag->framebuffer_width;
        int req_height = hdrtag->framebuffer_height;
        int req_bpp    = hdrtag->framebuffer_bpp;

        if (config_get_value(buf, 0, 128, "RESOLUTION"))
            parse_resolution(&req_width, &req_height, &req_bpp, buf);

        struct vbe_framebuffer_info fbinfo;
        if (init_vbe(&fbinfo, req_width, req_height, req_bpp)) {
            struct stivale2_struct_tag_framebuffer *tag = conv_mem_alloc(sizeof(struct stivale2_struct_tag_framebuffer));
            tag->tag.identifier = STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID;

            tag->memory_model       = STIVALE2_FBUF_MMODEL_RGB;
            tag->framebuffer_addr   = fbinfo.framebuffer_addr;
            tag->framebuffer_width  = fbinfo.framebuffer_width;
            tag->framebuffer_height = fbinfo.framebuffer_height;
            tag->framebuffer_bpp    = fbinfo.framebuffer_bpp;
            tag->framebuffer_pitch  = fbinfo.framebuffer_pitch;
            tag->red_mask_size      = fbinfo.red_mask_size;
            tag->red_mask_shift     = fbinfo.red_mask_shift;
            tag->green_mask_size    = fbinfo.green_mask_size;
            tag->green_mask_shift   = fbinfo.green_mask_shift;
            tag->blue_mask_size     = fbinfo.blue_mask_size;
            tag->blue_mask_shift    = fbinfo.blue_mask_shift;

            append_tag(&stivale2_struct, (struct stivale2_tag *)tag);
        }
    }
    }

    // Check if 5-level paging tag is requesting support
    bool level5pg_requested = get_tag(&stivale2_hdr, STIVALE2_HEADER_TAG_5LV_PAGING_ID) ? true : false;

    pagemap_t pagemap = {0};
    if (bits == 64)
        pagemap = stivale_build_pagemap(level5pg && level5pg_requested);

    //////////////////////////////////////////////
    // Create SMP struct tag
    //////////////////////////////////////////////
    {
    struct stivale2_header_tag_smp *smp_hdr_tag = get_tag(&stivale2_hdr, STIVALE2_HEADER_TAG_SMP_ID);
    if (smp_hdr_tag != NULL) {
        struct stivale2_struct_tag_smp *tag;
        struct smp_information *smp_info;
        size_t cpu_count;
        uint32_t bsp_lapic_id;
        smp_info = init_smp(sizeof(struct stivale2_struct_tag_smp), (void **)&tag,
                            &cpu_count, &bsp_lapic_id,
                            bits == 64, level5pg && level5pg_requested,
                            pagemap, smp_hdr_tag->flags & 1);

        if (smp_info != NULL) {
            tag->tag.identifier = STIVALE2_STRUCT_TAG_SMP_ID;
            tag->bsp_lapic_id   = bsp_lapic_id;
            tag->cpu_count      = cpu_count;
            tag->flags         |= (smp_hdr_tag->flags & 1) && x2apic_check();

            append_tag(&stivale2_struct, (struct stivale2_tag *)tag);
        }
    }
    }

    print("Generated tags:\n");
    struct stivale2_tag *taglist = (void*)(size_t)stivale2_struct.tags;
    for (size_t i = 0; ; i++) {
        print("Tag #%u  ID: %X\n", i, taglist->identifier);
        if (taglist->next)
            taglist = (void*)(size_t)taglist->next;
        else
            break;
    }

    //////////////////////////////////////////////
    // Create memmap struct tag
    //////////////////////////////////////////////
    {
    size_t memmap_entries;
    struct e820_entry_t *memmap = get_memmap(&memmap_entries);

    struct stivale2_struct_tag_memmap *tag =
        conv_mem_alloc(sizeof(struct stivale2_struct_tag_memmap) +
                       sizeof(struct e820_entry_t) * memmap_entries);

    tag->tag.identifier = STIVALE2_STRUCT_TAG_MEMMAP_ID;
    tag->entries = (uint64_t)memmap_entries;

    memcpy((void*)tag + sizeof(struct stivale2_struct_tag_memmap),
           memmap, sizeof(struct e820_entry_t) * memmap_entries);

    append_tag(&stivale2_struct, (struct stivale2_tag *)tag);
    }

    stivale_spinup(bits, level5pg && level5pg_requested, pagemap,
                   entry_point, &stivale2_struct, stivale2_hdr.stack);
}
