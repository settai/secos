/* GPLv2 (c) Airbus */
#include <intr.h>

#include <debug.h>

#include <segmem.h>
#include <intr.h>

#include <cr.h>
#include <pagemem.h>

#include <info.h>


#ifndef __TP_H__
#define __TP_H__

#define c0_idx  1
#define d0_idx  2
#define c3_idx  3
#define d3_idx  4
#define ts_idx  5

#define c0_sel  gdt_krn_seg_sel(c0_idx)
#define d0_sel  gdt_krn_seg_sel(d0_idx)
#define c3_sel  gdt_usr_seg_sel(c3_idx)
#define d3_sel  gdt_usr_seg_sel(d3_idx)
#define ts_sel  gdt_krn_seg_sel(ts_idx)

seg_desc_t GDT[6];
tss_t      TSS;

// PGD[0] Kernel

#define KERNEL_START    0x200000

#define PGD_KERNEL      0x200000
#define PTB_K           0x201000

#define PGD_USER1       0x210000
#define PTB_K1          0x211000
#define PTB_U1          0x212000

#define PGD_USER2       0x220000
#define PTB_K2          0x221000
#define PTB_U2          0x222000


#define COUNT_START     0x3c0000
#define COUNT_END       0x3cffff

#define STACK_K1_ESP    0x3dffc3 
#define STACK_K1_EBP    0x3dffff
#define STACK_K2_ESP    0x3effc3 
#define STACK_K2_EBP    0x3effff
#define STACK_K         0x3fffff
#define KERNEL_END      0x3fffff

// PGD[1] User1

#define USER1_START     0x400000
#define COUNTER_V1      0x4c0000
#define STACK_U1        0x7fffff
#define USER1_END       0x7fffff

// PGD[2] User2

#define USER2_START     0x800000
#define COUNTER_V2      0x8c0000
#define STACK_U2        0xbfffff
#define USER2_END       0xbfffff

#define COUNTER_P       0xcccccc

#define gdt_flat_dsc(_dSc_,_pVl_,_tYp_)                                 \
   ({                                                                   \
      (_dSc_)->raw     = 0;                                             \
      (_dSc_)->limit_1 = 0xffff;                                        \
      (_dSc_)->limit_2 = 0xf;                                           \
      (_dSc_)->type    = _tYp_;                                         \
      (_dSc_)->dpl     = _pVl_;                                         \
      (_dSc_)->d       = 1;                                             \
      (_dSc_)->g       = 1;                                             \
      (_dSc_)->s       = 1;                                             \
      (_dSc_)->p       = 1;                                             \
   })

#define tss_dsc(_dSc_,_tSs_)                                            \
   ({                                                                   \
      raw32_t addr    = {.raw = _tSs_};                                 \
      (_dSc_)->raw    = sizeof(tss_t);                                  \
      (_dSc_)->base_1 = addr.wlow;                                      \
      (_dSc_)->base_2 = addr._whigh.blow;                               \
      (_dSc_)->base_3 = addr._whigh.bhigh;                              \
      (_dSc_)->type   = SEG_DESC_SYS_TSS_AVL_32;                        \
      (_dSc_)->p      = 1;                                              \
   })

#define c0_dsc(_d) gdt_flat_dsc(_d,0,SEG_DESC_CODE_XR)
#define d0_dsc(_d) gdt_flat_dsc(_d,0,SEG_DESC_DATA_RW)
#define c3_dsc(_d) gdt_flat_dsc(_d,3,SEG_DESC_CODE_XR)
#define d3_dsc(_d) gdt_flat_dsc(_d,3,SEG_DESC_DATA_RW)

#define resume_from_intr() asm volatile("popa ; add $8, %esp ; iret")

void __regparm__(1) timer_hdlr(int_ctx_t *ctx);
void __regparm__(1) syscall_hdlr(int_ctx_t *ctx);
void __attribute__((section(".sys_counter"))) sys_counter(uint32_t* counter);

#endif
