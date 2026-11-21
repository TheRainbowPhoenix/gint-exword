// SPDX-FileCopyrightText: 2026 Artem Novak
// SPDX-License-Identifier: BSD-3-Clause

#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/clock.h>
#include <gint/mpu/cpg.h>
#include <gint/mmu.h>

#include <gint/exword.h>
#include <gint/gint.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define ROM_SIZE 0x180000u
#define ROM_MID  0x080000u
#define ROM_LAST (ROM_SIZE - 1u)
// \\\\crd0\\CASIO\\SHPRB\\_CONTENT\\romprobe.bin
#define ROM_PATH "\\\\drv0\\CASIO\\SHPRB\\_CONTENT\\romprobe.bin"
#define ROM_SD_PATH "\\\\crd0\\CASIO\\romprobe.bin"
#define RESULTS_PATH "\\\\crd0\\CASIO\\results.txt"

static uint8_t read_buffer[1024];

typedef struct {
    int opened;
    int size_ok;
    int complete;
    int markers[3];
    uint32_t read_bytes;
    uint32_t first_bad;
    uint8_t *resident;
} rom_result_t;

static inline uint32_t reg32(uint32_t address)
{
    return *(volatile uint32_t const *)address;
}

static int translate_any(uint32_t va, int at_enabled,
    uint32_t *pa, uint32_t *page_size)
{
    if(va >= 0x80000000u && va < 0xc0000000u) {
        *pa = va & 0x1fffffffu;
        *page_size = 0;
        return 1;
    }
    *pa = 0xffffffffu;
    *page_size = 0;
    if(!at_enabled || va >= 0x80000000u) return 0;

    static const uint32_t sizes[] = {1024, 4096, 65536, 1048576};
    for(unsigned int i = 0; i < sizeof(sizes)/sizeof(sizes[0]); i++) {
        uint32_t base = va & ~(sizes[i] - 1u);
        uint32_t pg = 0;
        uint32_t pbase = mmu_translate(base, &pg);
        if(pbase == 0xffffffffu || pg == 0 || (va - base) >= pg)
            continue;
        *pa = pbase + va - base;
        *page_size = pg;
        return 1;
    }
    return 0;
}

static int next_page(void)
{
    dupdate();
    return getkey().key != KEY_EXIT;
}

static int check_rom(rom_result_t *r)
{
    int fd = exword_sys_open(ROM_PATH, 1);
    if(fd < 0) fd = exword_sys_open(ROM_SD_PATH, 1);
    if(fd < 0) return -1;
    r->opened = 1;

    int length = exword_sys_get_filesize(fd);
    r->size_ok = (length == (int)ROM_SIZE);
    if(!r->size_ok) {
        exword_sys_close(fd);
        return -1;
    }

    r->first_bad = 0xffffffffu;
    r->resident = NULL;
    while(r->read_bytes < ROM_SIZE) {
        unsigned int count = ROM_SIZE - r->read_bytes;
        if(count > sizeof(read_buffer)) count = sizeof(read_buffer);
        int n = exword_sys_read(fd, read_buffer, count);
        if(n <= 0) break;

        for(int i = 0; i < n; i++) {
            uint32_t offset = r->read_bytes + (uint32_t)i;
            uint8_t expected = 0;
            if(offset == 0) expected = 0x5a;
            if(offset == ROM_MID) expected = 0xa5;
            if(offset == ROM_LAST) expected = 0x3c;
            if(read_buffer[i] != expected && r->first_bad == 0xffffffffu)
                r->first_bad = offset;
            if(offset == 0) r->markers[0] = (read_buffer[i] == 0x5a);
            if(offset == ROM_MID) r->markers[1] = (read_buffer[i] == 0xa5);
            if(offset == ROM_LAST) r->markers[2] = (read_buffer[i] == 0x3c);
        }
        if(r->resident)
            memcpy(r->resident + r->read_bytes, read_buffer, (size_t)n);
        r->read_bytes += (uint32_t)n;
    }

    r->complete = (r->read_bytes == ROM_SIZE &&
                   r->first_bad == 0xffffffffu);
    exword_sys_close(fd);
    if(!r->complete) {
        free(r->resident);
        r->resident = NULL;
    }
    return 0;
}

static int write_report(char const *text)
{
    int status = exword_sys_delete(RESULTS_PATH);
    (void)status;
    status = exword_sys_create(RESULTS_PATH, 1);
    if(status < 0) return status;
    int fd = exword_sys_open(RESULTS_PATH, 2);
    if(fd < 0) return -1;
    size_t remaining = strlen(text);
    while(remaining) {
        int n = exword_sys_write(fd, text, remaining);
        if(n <= 0) {
            exword_sys_close(fd);
            return -2;
        }
        text += n;
        remaining -= (size_t)n;
    }
    return exword_sys_close(fd) == 0 ? 0 : -3;
}

static char report[2100];
static size_t report_used;

static void append_line(char const *label, char const *value)
{
    int n = snprintf(report + report_used, sizeof(report) - report_used,
        "%s%s\n", label, value);
    if(n>0 && (size_t)n<sizeof(report)-report_used)report_used+=(size_t)n;
}
static void append_hex(char const *name, uint32_t value, int available)
{
    char temp[18];
    if(available) snprintf(temp,sizeof(temp),"%08X",value);
    else snprintf(temp,sizeof(temp),"N/A");
    append_line(name,temp);
}
static void append_sample(char const *title, uint8_t const *ptr, int at)
{
    uint32_t pa=0xffffffffu,pg=0;
    int ok=ptr && translate_any((uint32_t)ptr,at,&pa,&pg);
    char temp[32];
    append_line("\n",title);
    append_hex("VA ",(uint32_t)ptr,ptr!=NULL);
    append_hex("PA ",pa,ok);
    append_hex("P1 ",pa|0x80000000u,ok);
    append_hex("PAGE bytes ",pg,ok && pg!=0);
    append_hex("VA value ",ptr ? (uint32_t)*ptr : 0,ptr!=NULL);
    append_hex("P1 value ",ok ?
        (uint32_t)*(volatile uint8_t const *)(pa|0x80000000u) : 0,ok);
    if(!ok)append_line("Alias match: ","N/A");
    else {
        uint8_t alias=*(volatile uint8_t const *)(pa|0x80000000u);
        append_line("Alias match: ",*ptr==alias?"yes":"no");
    }
    (void)temp;
}
static void build_report(uint32_t pvr,uint32_t prr,uint32_t cvr,
    uint32_t cpidr,uint32_t frqcr,uint32_t pllcr,uint32_t fllfrq,
    uint32_t ram_pa,uint32_t ram_pg,uint32_t ram_size,int ram_ok,
    uint32_t code_va,uint32_t code_pa,uint32_t code_pg,int code_ok,
    uint16_t code_word,rom_result_t const *rom,int at)
{
    uint32_t p1=code_pa|0x80000000u;
    int sample_ok=0,sample_contiguous=0;
    uint32_t p0=0,p_mid=0,p_end=0,g=0;
    report_used=0;
    report[0]=0;
    append_line("SH7305 PROBE RESULTS\n====================\n","");
    append_hex("\nPVR   ",pvr,1);
    append_hex("PRR   ",prr,1);
    append_hex("CVR   ",cvr,1);
    append_hex("CPIDR ",cpidr,1);

    append_line("\nUSER RAM","");
    append_hex("VA ",0x74000000u,1);
    append_hex("PA ",ram_pa,ram_ok);
    append_hex("P1 ",ram_pa|0x80000000u,ram_ok);
    append_hex("PAGE bytes ",ram_pg,ram_ok && ram_pg!=0);
    append_hex("RAM span bytes ",ram_size,ram_ok && ram_size!=0);

    append_line("\nSH7305 CLOCKS (raw)","");
    append_hex("FRQCR  ",frqcr,1);
    append_hex("PLLCR  ",pllcr,1);
    append_hex("PLLFRQ ",fllfrq,1);
    append_line("Frequency Hz: ","N/A");

    append_line("\nADDIN CODE","");
    append_hex("VA ",code_va,1);
    append_hex("PA ",code_pa,code_ok);
    append_hex("P1 ",p1,code_ok);
    append_hex("PAGE bytes ",code_pg,code_ok && code_pg!=0);
    append_hex("VA value ",code_word,1);
    append_hex("P1 value ",code_ok ?
        *(volatile uint16_t const *)p1 : 0,code_ok);
    append_line("Alias match: ",!code_ok?"N/A":
        code_word==*(volatile uint16_t const *)p1?"yes":"no");

    append_sample("ROM 1.5M START",rom->resident,at);
    append_sample("ROM 1.5M MID",rom->resident?
        rom->resident+ROM_MID:NULL,at);
    append_sample("ROM 1.5M END",rom->resident?
        rom->resident+ROM_LAST:NULL,at);
    if(rom->resident &&
       translate_any((uint32_t)rom->resident,at,&p0,&g) &&
       translate_any((uint32_t)(rom->resident+ROM_MID),at,&p_mid,&g) &&
       translate_any((uint32_t)(rom->resident+ROM_LAST),at,&p_end,&g)) {
        sample_ok=1;
        sample_contiguous=p_mid-p0==ROM_MID && p_end-p0==ROM_LAST;
    }
    append_line("\nLINUX ROM TEST","");
    append_hex("ROM bytes ",ROM_SIZE,1);
    append_line("Sample map: ",sample_ok?"YES":"N/A");
    append_line("Samples contiguous: ",sample_ok?
        (sample_contiguous?"YES":"NO"):"N/A");
    append_line("File integrity: ",rom->complete?"PASS":"FAIL");
    append_hex("File first bad: ",rom->first_bad,1);
    append_hex("File read bytes: ",rom->read_bytes,1);
    append_line("Note: ","streamed file; no resident image for alias/contiguity tests.");
}

static void title(char const *name)
{
    dclear(C_WHITE);
    dtext(1, 1, C_BLACK, name);
}

int main(void)
{
    uint32_t pvr = reg32(0xff000030);
    uint32_t cvr = reg32(0xff000040);
    uint32_t prr = reg32(0xff000044);
    uint32_t cpidr = reg32(0xff000048);

    uint32_t frqcr = SH7305_CPG.FRQCR.lword;
    uint32_t pllcr = SH7305_CPG.PLLCR.lword;
    uint32_t fllfrq = SH7305_CPG.FLLFRQ.lword;
    clock_frequency_t const *f = clock_freq();

    uint32_t mmucr = reg32(0xff000010);
    int at_enabled = (mmucr & 1u) != 0;
    uint32_t ram_pa = 0xffffffffu, ram_pg = 0;
    int ram_ok = translate_any(0x74000000u, at_enabled, &ram_pa, &ram_pg);
    uint32_t ram_size = mmu_uram_size();
    uint32_t ram_p1 = (uint32_t)mmu_uram();

    uint32_t code_va = (uint32_t)&main;
    uint32_t code_pa = 0xffffffffu, code_pg = 0;
    int code_ok = translate_any(code_va, at_enabled, &code_pa, &code_pg);
    uint32_t code_p1 = code_ok ? (code_pa | 0x80000000u) : 0xffffffffu;
    uint16_t va_word = *(volatile uint16_t const *)code_va;
    uint16_t code_halfword = *(volatile uint16_t const *)code_va;
    uint32_t p1_word = code_ok
        ? *(volatile uint16_t const *)code_p1 : 0;

    rom_result_t rom = {.first_bad = 0xffffffffu};
    gint_world_switch(GINT_CALL(check_rom, (void *)&rom));

    build_report(pvr,prr,cvr,cpidr,frqcr,pllcr,fllfrq,
        ram_pa,ram_pg,ram_size,ram_ok,
        code_va,code_pa,code_pg,code_ok,code_halfword,&rom,at_enabled);
    int save_status = gint_world_switch(GINT_CALL(write_report, report));
    free(rom.resident);

    title("SH7305 CPU");
    dprint(1, 13, C_BLACK, "PVR %08X", pvr);
    dprint(1, 23, C_BLACK, "PRR %08X", prr);
    dprint(1, 33, C_BLACK, "CVR %08X", cvr);
    dprint(1, 43, C_BLACK, "CPID %08X", cpidr);
    if(!next_page()) return 0;

    title("USER RAM / MMU");
    dprint(1, 13, C_BLACK, "MMUCR %08X", mmucr);
    dprint(1, 23, C_BLACK, "AT %s PA %08X",
        at_enabled ? "ON" : "OFF", ram_pa);
    dprint(1, 33, C_BLACK, "P1 %08X", ram_p1);
    dprint(1, 43, C_BLACK, "SIZE %u", ram_size);
    dprint(1, 53, C_BLACK, "PG %u %s", ram_pg,
        ram_ok ? "OK" : "NONE");
    if(!next_page()) return 0;

    title("SH7305 CLOCKS");
    dprint(1, 13, C_BLACK, "FRQ %08X", frqcr);
    dprint(1, 23, C_BLACK, "PLL %08X", pllcr);
    dprint(1, 33, C_BLACK, "FLL %08X", fllfrq);
    dprint(1, 43, C_BLACK, "I %u", f->Iphi_f);
    dprint(1, 53, C_BLACK, "B%u P%u", f->Bphi_f, f->Pphi_f);
    if(!next_page()) return 0;

    title("CODE MAP");
    dprint(1, 13, C_BLACK, "VA %08X", code_va);
    dprint(1, 25, C_BLACK, "PA %08X", code_pa);
    dprint(1, 37, C_BLACK, "P1 %08X", code_p1);
    dprint(1, 49, C_BLACK, "PG %u", code_pg);
    if(!next_page()) return 0;

    title("P1 CODE TEST");
    dprint(1, 15, C_BLACK, "VA %08X", va_word);
    dprint(1, 29, C_BLACK, "P1 %08X", p1_word);
    dprint(1, 45, C_BLACK, "MATCH %s",
        code_ok && va_word == p1_word ? "YES" : "NO");
    if(!next_page()) return 0;

    title("ROM 1.5M START");
    dtext(1, 13, C_BLACK, "FILE SHPRB/romprobe.bin");
    dtext(1, 25, C_BLACK, "OFFSET 00000000");
    dtext(1, 37, C_BLACK, "EXPECTED 5A");
    dprint(1, 49, C_BLACK, "MATCH %s", rom.markers[0] ? "YES" : "NO");
    if(!next_page()) return 0;

    title("ROM 1.5M MID");
    dtext(1, 13, C_BLACK, "FILE SHPRB/romprobe.bin");
    dtext(1, 25, C_BLACK, "OFFSET 00080000");
    dtext(1, 37, C_BLACK, "EXPECTED A5");
    dprint(1, 49, C_BLACK, "MATCH %s", rom.markers[1] ? "YES" : "NO");
    if(!next_page()) return 0;

    title("ROM 1.5M END");
    dtext(1, 13, C_BLACK, "FILE SHPRB/romprobe.bin");
    dtext(1, 25, C_BLACK, "OFFSET 0017FFFF");
    dtext(1, 37, C_BLACK, "EXPECTED 3C");
    dprint(1, 49, C_BLACK, "MATCH %s", rom.markers[2] ? "YES" : "NO");
    if(!next_page()) return 0;

    title("EXTERNAL ROM TEST");
    dprint(1, 13, C_BLACK, "SIZE %u", ROM_SIZE);
    dprint(1, 23, C_BLACK, "OPEN %s", rom.opened ? "YES" : "NO");
    dprint(1, 33, C_BLACK, "LEN %s", rom.size_ok ? "OK" : "BAD");
    dprint(1, 43, C_BLACK, "READ %u", rom.read_bytes);
    dprint(1, 53, C_BLACK, "DATA %s", rom.complete ? "PASS" : "FAIL");
    if(!next_page()) return 0;

    title("RESULTS FILE");
    dtext(1, 13, C_BLACK, "SD /results.txt");
    dtext(1, 25, C_BLACK, "Storage root (crd0)");
    dprint(1, 43, C_BLACK, "SAVE %s (%d)",
        save_status == 0 ? "OK" : "FAILED", save_status);
    if(!next_page()) return 0;

    return 1;
}
