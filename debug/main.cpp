#include <iostream>

#include <cstdlib>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <inttypes.h>

#include <shared_mutex>
#include <mutex>

//-----------------------------------------------------------------------------

#define printerr(fmt,...) \
do {\
        fprintf(stderr, fmt, ## __VA_ARGS__); fflush(stderr); \
   } while(0)

//-----------------------------------------------------------------------------

void read_region(uint32_t start, int size);
void handler(void * map_base, uint32_t start, uint32_t offset, int size);

//-----------------------------------------------------------------------------

void dmacr(uint32_t * _reg);
void dmasr(uint32_t *  _reg);
void curdesc(uint32_t * _reg);
void curdesc_msb(uint32_t * _reg);
void taildesc(uint32_t * _reg);
void taildesc_msb(uint32_t * _reg);
void sg_ctl(uint32_t * _reg);

//-----------------------------------------------------------------------------

void mm2s_dmacr(uint32_t * _reg);
void mm2s_dmasr(uint32_t * _reg);
void mm2s_curdesc(uint32_t * _reg);
void mm2s_curdesc_msb(uint32_t * _reg);
void mm2s_taildesc(uint32_t * _reg);
void mm2s_taildesc_msb(uint32_t * _reg);

//-----------------------------------------------------------------------------

void s2mm_dmacr(uint32_t * _reg);
void s2mm_dmasr(uint32_t * _reg);
void s2mm_curdesc(uint32_t * _reg);
void s2mm_curdesc_msb(uint32_t * _reg);
void s2mm_taildesc(uint32_t * _reg);
void s2mm_taildesc_msb(uint32_t * _reg);

//=============================================================================

void printRegister(uint32_t * _reg);
void dmaBufLen(uint32_t * _reg);

//-----------------------------------------------------------------------------

void mm2s_sa(uint32_t * _reg);
void mm2s_sa_msb(uint32_t * _reg);
void mm2s_length(uint32_t * _reg);

//-----------------------------------------------------------------------------

void s2mm_da(uint32_t * _reg);
void s2mm_da_msb(uint32_t * _reg);
void s2mm_length(uint32_t * _reg);

//-----------------------------------------------------------------------------

int main(int argc, char ** argv)
{
    int region_size = 0;
    uint32_t target;
    char * endp = NULL;

    if (argc == 3)
    {
        target = strtoull(argv[1], &endp, 0);
        if (errno != 0 || (endp && 0 != *endp))
        {
            printerr("Invalid memory address: %s\n", argv[1]);
            exit(2);
        }

        region_size = atoi(argv[2]);
    }
    else
    {
        printf("Usage: ./region_read base_addr reg_count\n");
        exit(2);
    }

    read_region(target, region_size);

    return 0;
}

//=============================================================================

void read_region(uint32_t start, int size)
{
    unsigned int pagesize = (unsigned)getpagesize();
    unsigned int map_size = pagesize;
    void * map_base;
    int fd;
    uint32_t offset = (unsigned int)(start & (pagesize-1));

    if (!size)
        return;

    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1)
    {
        printerr("Error opening /dev/mem (%d) : %s\n", errno, strerror(errno));
        exit(1);
    }

    map_base = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                    fd,
                    start & ~((typeof(start))pagesize-1));

    if (map_base == (void *) -1)
    {
        printerr("Error mapping (%d) : %s\n", errno, strerror(errno));
        exit(1);
    }

    // Handle registers
    handler(map_base, start, offset, size);

    if (munmap(map_base, map_size) != 0)
    {
        printerr("ERROR munmap (%d) %s\n", errno, strerror(errno));
    }

    close(fd);
}

//-----------------------------------------------------------------------------

void handler(void * map_base, uint32_t start, uint32_t offset, int size)
{
    void * virt_addr;
    uint32_t tmp_val = 0;
    int curPos = 0;

    for (int i = 0; i < size; i++)
    {
        virt_addr = map_base + offset;
        tmp_val = *((uint32_t *) virt_addr);
        curPos = i * sizeof(uint32_t);

        switch (curPos)
        {
        case 0x0:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            mm2s_dmacr(&tmp_val);
            break;

        case 0x4:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            mm2s_dmasr(&tmp_val);
            break;

        case 0x8:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            mm2s_curdesc(&tmp_val);
            break;

        case 0xC:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            mm2s_curdesc_msb(&tmp_val);
            break;

        case 0x10:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            mm2s_taildesc(&tmp_val);
            break;

        case 0x14:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            mm2s_taildesc_msb(&tmp_val);
            break;

        case 0x18:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            mm2s_sa(&tmp_val);
            break;

        case 0x1C:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            mm2s_sa_msb(&tmp_val);
            break;

        case 0x2C:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            sg_ctl(&tmp_val);
            break;

        case 0x28:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            mm2s_length(&tmp_val);
            break;

        case 0x30:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            s2mm_dmacr(&tmp_val);
            break;

        case 0x34:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            s2mm_dmasr(&tmp_val);
            break;

        case 0x38:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            s2mm_curdesc(&tmp_val);
            break;

        case 0x3C:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            s2mm_curdesc_msb(&tmp_val);
            break;

        case 0x40:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            s2mm_taildesc(&tmp_val);
            break;

        case 0x44:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            s2mm_taildesc_msb(&tmp_val);
            break;

        case 0x48:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            s2mm_da(&tmp_val);
            break;

        case 0x4C:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            s2mm_da_msb(&tmp_val);
            break;

        case 0x58:
            printf("0x%08X - 0x%08X\n", start + curPos, tmp_val);
            s2mm_length(&tmp_val);
            break;

        default:
            //std::cout << "There is not such value" << std::endl;
            break;
        }

        offset += sizeof(uint32_t);
    }
}

//=============================================================================

void dmacr(uint32_t * _reg)
{
    struct reg
    {
        bool rs                 : 1;
        bool reserved1          : 1;
        bool reset              : 1;
        bool keyhole            : 1;
        bool cyclicBdEn         : 1;
        uint16_t reserved2      : 7;
        bool iocIrqEn           : 1;
        bool dlyIrqEn           : 1;
        bool errIrqEn           : 1;
        bool reserved3          : 1;
        uint16_t irqThreshold   : 8;
        uint16_t irqDelay       : 8;
    } __attribute__((__packed__));

    struct reg * tmp = reinterpret_cast<struct reg *>(_reg);

    std::cout << "Rs:               | "
              << (tmp->rs ? "run" : "stop") << std::endl;

    std::cout << "Reset:            | "
              << (tmp->reset ? "Reset" : "normal") << std::endl;

    std::cout << "Keyhole:          | "
              << (tmp->keyhole ? "true" : "false") << std::endl;

    std::cout << "Cyclic Bd:        | "
              << (tmp->cyclicBdEn ? "enable" : "disable") << std::endl;

    std::cout << "IOC_IrqEn:        | "
              << (tmp->iocIrqEn ? "enable" : "disable") << std::endl;

    std::cout << "Dly_IrqEn:        | "
              << (tmp->dlyIrqEn ? "enable" : "disable") << std::endl;

    std::cout << "Err_IrqEn:        | "
              << (tmp->errIrqEn ? "enable" : "disable") << std::endl;

    std::cout << "IRQThreshold:     | "
              << std::hex << tmp->irqThreshold << std::endl;

    std::cout << "IRQDelay:         | "
              << std::hex << tmp->irqDelay << std::endl;

    std::cout << "---------------------------------------------"
              << std::endl << std::endl;
}

//-----------------------------------------------------------------------------

void dmasr(uint32_t * _reg)
{
    struct reg
    {
        bool halted              : 1;
        bool idle                : 1;
        bool reserved1           : 1;
        bool sgIncld             : 1;
        bool dmaIntErr           : 1;
        bool dmaSlvErr           : 1;
        bool dmaDecErr           : 1;
        bool reserved2           : 1;
        bool sgIntErr            : 1;
        bool sgSlvErr            : 1;
        bool sgDecErr            : 1;
        bool reserved3           : 1;
        bool iocIrq              : 1;
        bool dlyIrq              : 1;
        bool errIrq              : 1;
        bool reserved4           : 1;
        uint16_t irqThresholdSts : 8;
        uint16_t irqDelaySts     : 8;
    } __attribute__((__packed__));

    struct reg * tmp = reinterpret_cast<struct reg *>(_reg);

    std::cout << "Halted:           | "
              << (tmp->halted ? "halted" : "running") << std::endl;

    std::cout << "Idle:             | "
              << (tmp->idle ? "idle" : "not idle") << std::endl;

    std::cout << "SGIncld:          | "
              << (tmp->sgIncld ? "enabled" : "disabled") << std::endl;

    std::cout << "DMAIntErr:        | "
              << (tmp->dmaIntErr ? "errors" : "no errors") << std::endl;

    std::cout << "DMASlvErr:        | "
              << (tmp->dmaSlvErr ? "errors" : "no errors") << std::endl;

    std::cout << "DMADecErr:        | "
              << (tmp->dmaDecErr ? "errors" : "no errors") << std::endl;

    std::cout << "SGIntErr:         | "
              << (tmp->sgIntErr ? "errors" : "no errors") << std::endl;

    std::cout << "SGSlvErr:         | "
              << (tmp->sgSlvErr ? "errors" : "no errors") << std::endl;

    std::cout << "SGDecErr:         | "
              << (tmp->sgDecErr ? "errors" : "no errors") << std::endl;

    std::cout << "IOC_Irq:          | "
              << (tmp->iocIrq ? "interrupt" : "no interrupt") << std::endl;

    std::cout << "Dly_Irq:          | "
              << (tmp->dlyIrq ? "interrupt" : "no interrupt") << std::endl;

    std::cout << "Err_Irq:          | "
              << (tmp->errIrq ? "interrupt" : "no interrupt") << std::endl;

    std::cout << "IRQThresholdSts:  | "
              << std::hex << tmp->irqThresholdSts << std::endl;

    std::cout << "IRQDelaySts:      | "
              << std::hex << tmp->irqDelaySts << std::endl;

    std::cout << "---------------------------------------------"
              << std::endl << std::endl;
}

//-----------------------------------------------------------------------------

void sg_ctl(uint32_t * _reg)
{
    /* is available only when DMA is configured in multichannel mode */

    struct reg
    {
        uint16_t sgCache   : 4;
        uint16_t reserved1 : 4;
        uint16_t sgUser    : 4;
        uint32_t reserved2 : 20;
    } __attribute__((__packed__));

    struct reg * tmp = reinterpret_cast<struct reg *>(_reg);

    std::cout << "Scatter/Gather User and Cache"
              << std::endl << std::endl;

    std::cout << "sgCache:          | "
              << std::hex << tmp->sgCache  << std::endl;

    std::cout << "sgUser:           | "
              << std::hex << tmp->sgUser << std::endl;

    std::cout << "---------------------------------------------"
              << std::endl << std::endl;
}

//-----------------------------------------------------------------------------

void curdesc(uint32_t * _reg)
{
    struct reg
    {
        uint16_t reserved1      : 6;
        uint32_t curDescPointer : 26;
    } __attribute__((__packed__));

    struct reg * tmp = reinterpret_cast<struct reg *>(_reg);

    std::cout << "curDescPointer:   | "
              << std::hex << tmp->curDescPointer  << std::endl;

    std::cout << "---------------------------------------------"
              << std::endl << std::endl;
}

//-----------------------------------------------------------------------------

void curdesc_msb(uint32_t * _reg)
{
    uint32_t * curDescPointer = _reg;

    std::cout << "curDescPointer:   | "
              << std::hex << *curDescPointer  << std::endl;

    std::cout << "---------------------------------------------"
              << std::endl << std::endl;
}

//-----------------------------------------------------------------------------

void taildesc(uint32_t * _reg)
{
    struct reg
    {
        uint16_t reserved1       : 6;
        uint32_t tailDescPointer : 26;
    } __attribute__((__packed__));

    struct reg * tmp = reinterpret_cast<struct reg *>(_reg);

    std::cout << "tailDescPointer:  | "
              << std::hex << tmp->tailDescPointer  << std::endl;

    std::cout << "---------------------------------------------"
              << std::endl << std::endl;
}

//-----------------------------------------------------------------------------

void taildesc_msb(uint32_t * _reg)
{
    uint32_t * tailDescPointer = _reg;

    std::cout << "tailDescPointer:  | "
              << std::hex << *tailDescPointer  << std::endl;

    std::cout << "---------------------------------------------"
              << std::endl << std::endl;
}

//=============================================================================

void mm2s_dmacr(uint32_t * _reg)
{
    /*
     * MM2S DMA Control register
     */

    std::cout << "MM2S DMA Control register"
              << std::endl << std::endl;
    dmacr(_reg);
}

//-----------------------------------------------------------------------------

void mm2s_dmasr(uint32_t * _reg)
{
    /*
     * MM2S DMA Status register
     */

    std::cout << "MM2S DMA Status register"
              << std::endl << std::endl;
    dmasr(_reg);
}

//-----------------------------------------------------------------------------

void mm2s_curdesc(uint32_t * _reg)
{
    /*
     * MM2S Current Descriptor Pointer. Lower 32 bits of the address.
     */

    std::cout << "MM2S Current Descriptor Pointer (lower 32)"
              << std::endl << std::endl;

    curdesc(_reg);
}

//-----------------------------------------------------------------------------

void mm2s_curdesc_msb(uint32_t * _reg)
{
    /*
     * MM2S Current Descriptor Pointer. Upper 32 bits of address.
     */

    std::cout << "MM2S Current Descriptor Pointer (upper 32)"
              << std::endl << std::endl;

    curdesc_msb(_reg);
}

//-----------------------------------------------------------------------------

void mm2s_taildesc(uint32_t * _reg)
{
    /*
     * MM2S Tail Descriptor Pointer. Lower 32 bits.
     */

    std::cout << "MM2S Tail Descriptor Pointer (lower 32)"
              << std::endl << std::endl;

    taildesc(_reg);
}

//-----------------------------------------------------------------------------

void mm2s_taildesc_msb(uint32_t * _reg)
{
    /*
     * MM2S Tail Descriptor Pointer. Upper 32 bits of address
     */

    std::cout << "MM2S Tail Descriptor Pointer (upper 32)"
              << std::endl << std::endl;

    taildesc_msb(_reg);
}

//-----------------------------------------------------------------------------

void s2mm_dmacr(uint32_t * _reg)
{
    /*
     * S2MM DMA Control register
     */

    std::cout << "S2MM DMA Control register"
              << std::endl << std::endl;

    dmacr(_reg);
}

//-----------------------------------------------------------------------------

void s2mm_dmasr(uint32_t * _reg)
{
    /*
     * S2MM DMA Status register
     */

    std::cout << "S2MM DMA Status register"
              << std::endl << std::endl;

    dmasr(_reg);
}

//-----------------------------------------------------------------------------

void s2mm_curdesc(uint32_t * _reg)
{
    /*
     * S2MM Current Descriptor Pointer. Lower 32 address bits
     */

    std::cout << "S2MM Current Descriptor Pointer (lower 32)"
              << std::endl << std::endl;

    curdesc(_reg);
}

//-----------------------------------------------------------------------------

void s2mm_curdesc_msb(uint32_t * _reg)
{
    /*
     * S2MM Current Descriptor Pointer. Upper 32 address bits.
     */

    std::cout << "S2MM Current Descriptor Pointer (upper 32)"
              << std::endl << std::endl;

    curdesc_msb(_reg);
}

//-----------------------------------------------------------------------------

void s2mm_taildesc(uint32_t * _reg)
{
    /*
     * S2MM Tail Descriptor Pointer. Lower 32 address bits.
     */

    std::cout << "S2MM Tail Descriptor Pointer (lower 32)"
              << std::endl << std::endl;

    taildesc(_reg);
}

//-----------------------------------------------------------------------------

void s2mm_taildesc_msb(uint32_t * _reg)
{
    /*
     * S2MM Tail Descriptor Pointer. Upper 32 address bits.
     */

    std::cout << "S2MM Tail Descriptor Pointer (upper 32)"
              << std::endl << std::endl;

    taildesc_msb(_reg);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void printRegister(uint32_t * _reg)
{
    std::cout << std::hex << *_reg << std::endl;
    std::cout << "---------------------------------------------"
              << std::endl << std::endl;
}

//-----------------------------------------------------------------------------

void dmaBufLen(uint32_t * _reg)
{
    struct reg
    {
        uint32_t length   : 23;
        uint32_t reserved : 9;
    } __attribute__((__packed__));

    struct reg * tmp = reinterpret_cast<struct reg *>(_reg);

    std::cout << "Length:           | "
              << std::hex << tmp->length << std::endl;

    std::cout << "---------------------------------------------"
              << std::endl << std::endl;
}

//-----------------------------------------------------------------------------

void mm2s_sa(uint32_t * _reg)
{
    /*
     * MM2S Source Address. Lower 32 bits of address
     */

    std::cout << "MM2S Source Address (lower 32)"
              << std::endl << std::endl;

    std::cout << "Source addr:      | "
              << std::hex << *_reg << std::endl;

    std::cout << "---------------------------------------------"
              << std::endl << std::endl;
}

//-----------------------------------------------------------------------------

void mm2s_sa_msb(uint32_t * _reg)
{
    /*
     * MM2S Source Address. Upper 32 bits of address.
     */

    std::cout << "MM2S Source Address (upper 32)"
              << std::endl << std::endl;

    std::cout << "Source addr:      | " << std::hex
              << *_reg << std::endl;

    std::cout << "---------------------------------------------"
              << std::endl << std::endl;
}

//-----------------------------------------------------------------------------

void mm2s_length(uint32_t * _reg)
{
    /*
     * MM2S Transfer Length (Bytes)
     */

    std::cout << "MM2S Transfer Length (Bytes)"
              << std::endl << std::endl;

    dmaBufLen(_reg);
}

//-----------------------------------------------------------------------------

void s2mm_da(uint32_t * _reg)
{
    /*
     * S2MM Destination Address. Lower 32 bit address.
     */

    std::cout << "S2MM Destination Address (lower)"
              << std::endl << std::endl;

    std::cout << "Dest addr:        | "
              << std::hex << *_reg << std::endl;

    std::cout << "---------------------------------------------"
              << std::endl << std::endl;
}

//-----------------------------------------------------------------------------

void s2mm_da_msb(uint32_t * _reg)
{
    /*
     * S2MM Destination Address. Upper 32 bit address.
     */

    std::cout << "S2MM Destination Address (upper)"
              << std::endl << std::endl;

    std::cout << "Dest addr:        | "
              << std::hex << *_reg << std::endl;

    std::cout << "---------------------------------------------"
              << std::endl << std::endl;
}

//-----------------------------------------------------------------------------

void s2mm_length(uint32_t * _reg)
{
    /*
     * S2MM Buffer Length (Bytes)
     */

    std::cout << "S2MM Buffer Length (Bytes)"
              << std::endl << std::endl;

    dmaBufLen(_reg);
}

