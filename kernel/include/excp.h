/* GPLv2 (c) Airbus */
#ifndef __EXCP_H__
#define __EXCP_H__

#include <types.h>

/*
** Total number of exceptions
*/
#define NR_EXCP    32

/*
** Exceptions without error code
*/
#define DE_EXCP     0
#define DB_EXCP     1
#define NMI_EXCP    2
#define BP_EXCP     3
#define OF_EXCP     4
#define BR_EXCP     5
#define UD_EXCP     6
#define NM_EXCP     7
#define MO_EXCP     9
#define MF_EXCP    16
#define MC_EXCP    18
#define XF_EXCP    19

#define IRQ0_EXCP     32
#define SYSCALL_EXCP  80

/*
** Exceptions with error code
*/
#define DF_EXCP     8
#define TS_EXCP    10
#define NP_EXCP    11
#define SS_EXCP    12
#define GP_EXCP    13
#define PF_EXCP    14
#define AC_EXCP    17

/*
** Exceptions reserved by intel
*/
#define RSV_EXCP   15

/*
** Contributory exceptions
*/
#define contributory_exception(_e_)                             \
   ((_e_) == GP_EXCP || (_e_) == NP_EXCP || (_e_) == TS_EXCP || \
    (_e_) == DE_EXCP || (_e_) == SS_EXCP)

/*
** Double fault condition
*/
#define double_fault(_e1_,_e2_)                                         \
   ((contributory_exception(_e1_) && contributory_exception(_e2_)) ||   \
    (_e1_ == PF_EXCP && (_e2_ == PF_EXCP || contributory_exception(_e2_))))

/*
** Triple fault condition
*/
#define triple_fault(_e_)           ((_e_) == DF_EXCP)

/*
** Selector error code
*/
typedef union selector_error_code
{
   struct
   {
      uint32_t   ext:1;     /* external event */
      uint32_t   idt:1;     /* index from (1) IDT (0) GDT/LDT (cf. ti) */
      uint32_t   ti:1;      /* index from (0) GDT or (1) LDT */
      uint32_t   idx:13;    /* selector index */
      uint32_t   r:16;      /* reserved */

   } __attribute__((packed));

   raw32_t;

} __attribute__((packed)) sl_err_t;

/*
** Page-Fault error code
*/
typedef union page_fault_error_code
{
   struct
   {
      uint32_t      p:1;      /* non-present (0), violation (1) */
      uint32_t      wr:1;     /* read (0), write (1)  */
      uint32_t      us:1;     /* kernel (0), user (1) */
      uint32_t      rsv:1;    /* reserved bits violation */
      uint32_t      id:1;     /* 1 insn fetch */
      uint32_t      r:27;     /* reserved */

   } __attribute__((packed));

   raw32_t;

} __attribute__((packed)) pf_err_t;

/*
** Versatile exception error code
*/
typedef union exception32_error_code
{
   raw32_t;

   sl_err_t sl;
   pf_err_t pf;

} __attribute__((packed)) excp_err_code_t;

/*
** Functions
*/
struct interrupt_context;

void excp_hdlr(struct interrupt_context*) __regparm__(1);

#endif
