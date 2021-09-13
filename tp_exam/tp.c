/* GPLv2 (c) Airbus */
#include <debug.h>

#include <segmem.h>
#include <intr.h>

#include <cr.h>
#include <pagemem.h>

#include <info.h>

#include "tp.h"

extern info_t *info;

// PAGING

void enable_paging(){
   uint32_t cr0 = get_cr0();
   set_cr0(cr0|CR0_PG);
}

void init_paging(){   

   pde32_t * pgd_kernel = (pde32_t *) PGD_KERNEL;
   pde32_t * ptb_k      = (pde32_t *) PTB_K     ;

   pde32_t * pgd_user1  = (pde32_t *) PGD_USER1;
   pde32_t * ptb_k1     = (pde32_t *) PTB_K1;
   pde32_t * ptb_u1     = (pde32_t *) PTB_U1;

   pde32_t * pgd_user2  = (pde32_t *) PGD_USER2;
   pde32_t * ptb_k2     = (pde32_t *) PTB_K2;
   pde32_t * ptb_u2     = (pde32_t *) PTB_U2;
   
   int i;

   // init kernel
   for(i=0;i<1024;i++){
      pg_set_entry(&ptb_k[i], PG_KRN|PG_RW, i);
   }
   memset((void*)pgd_kernel, 0, PAGE_SIZE);
   pg_set_entry(&pgd_kernel[0], PG_KRN|PG_RW, page_nr(ptb_k));

   // init user1
   for(i=0;i<1024;i++){
      pg_set_entry(&ptb_k1[i], PG_USR|PG_RW, i);
      pg_set_entry(&ptb_u1[i], PG_USR|PG_RW, i + 1024);
   }
   memset((void*)pgd_user1, 0, PAGE_SIZE);
   pg_set_entry(&pgd_user1[0], PG_USR|PG_RW, page_nr(ptb_k1));
   pg_set_entry(&pgd_user1[1], PG_USR|PG_RW, page_nr(ptb_u1));
   pg_set_entry(&ptb_u1[pt32_idx(COUNTER_V1)], PG_USR|PG_RW|PG_P, page_nr(COUNTER_P));

   // init user2
   for(i=0;i<1024;i++){
      pg_set_entry(&ptb_k2[i], PG_USR|PG_RW, i);
      pg_set_entry(&ptb_u2[i], PG_USR|PG_RW, i + 2048);
   }
   memset((void*)pgd_user2, 0, PAGE_SIZE);
   pg_set_entry(&pgd_user2[0], PG_USR|PG_RW, page_nr(ptb_k2));
   pg_set_entry(&pgd_user2[2], PG_USR|PG_RW, page_nr(ptb_u2));
   pg_set_entry(&ptb_u2[pt32_idx(COUNTER_V2)], PG_USR|PG_RW|PG_P, page_nr(COUNTER_P));
   
   set_cr3(PGD_KERNEL);
   enable_paging();
}

// TASKS

void __attribute__((section(".user1"))) user1(){
   debug("user1\n");
   uint32_t * i = (uint32_t *) COUNTER_V1;
   do{
      (*i)++;
   }
   while(1);
}

void __attribute__((section(".user2"))) user2(){
   debug("user2\n");
   uint32_t * i = (uint32_t *) COUNTER_V2;
   do{
      sys_counter(i);
   }
   while(1);
}

// INTERUPTION

void sti(){
   asm volatile ("sti"); // enable material interruption
}

void __attribute__((section(".sys_counter"))) sys_counter(uint32_t* counter) {
   asm volatile ("mov %0, %%esi" :: "m"(*counter));
   asm volatile ("int $80");
}

void __regparm__(1) syscall_hdlr(int_ctx_t *ctx){
   debug("Counter %d\n", ctx->gpr.esi);
}

void init_user1(int_ctx_t *ctx){
   ctx->eip.raw            = (uint32_t) &user1;
   ctx->cs.raw             = c3_sel;
   ctx->eflags.raw         = EFLAGS_IF;
   ctx->esp.raw            = STACK_U1;
   ctx->ss.raw             = d3_sel;
   ctx->gpr.ebp.raw        = STACK_U1;
}

void init_user2(int_ctx_t *ctx){
   ctx->eip.raw            = (uint32_t) &user2;
   ctx->cs.raw             = c3_sel;
   ctx->eflags.raw         = EFLAGS_IF;
   ctx->esp.raw            = STACK_U2;
   ctx->ss.raw             = d3_sel;
   ctx->gpr.ebp.raw        = STACK_U2;
}

void __regparm__(1) timer_hdlr(int_ctx_t *ctx){
   // initialisation
   if (ctx->eip.raw < COUNT_START){    
      // init user1 and user2
      init_user1((int_ctx_t *) STACK_K1_ESP);
      init_user2((int_ctx_t *) STACK_K2_ESP);

      // start user1()
      set_esp(STACK_K1_ESP);
      set_cr3(PGD_USER1);
      TSS.s0.esp = STACK_K1_EBP;
      resume_from_intr();
   } 

   // user1() -> user2() | sys_count() (last one)
   else if ( ctx->eip.raw >= USER1_START && ctx->eip.raw <= USER1_END){ 
      int_ctx_t * last_ctx = (int_ctx_t *) STACK_K2_ESP;
      debug("user1@eip:0x%x -> user2@eip:0x%x\n", ctx->eip.raw, last_ctx->eip.raw);
      set_esp(STACK_K2_ESP);
      set_cr3(PGD_USER2);
      TSS.s0.esp = STACK_K2_EBP;
      resume_from_intr();
   } 

   // user2() | sys_count() -> user1()
   else if ( (ctx->eip.raw >= USER2_START && ctx->eip.raw <= USER2_END) || (ctx->eip.raw >= COUNT_START && ctx->eip.raw <= COUNT_END)){ 
      int_ctx_t * last_ctx = (int_ctx_t *) STACK_K1_ESP;
      debug("user2@eip:0x%x -> user1@eip:0x%x\n", ctx->eip.raw, last_ctx->eip.raw);
      set_esp(STACK_K1_ESP);
      set_cr3(PGD_USER1);
      TSS.s0.esp = STACK_K1_EBP;
      resume_from_intr();
   }

   else{
      panic("strange behavior");
   }
}

// MEMORY

void init_memory(){
   gdt_reg_t gdtr;

   GDT[0].raw = 0ULL;

   c0_dsc( &GDT[c0_idx] );
   d0_dsc( &GDT[d0_idx] );
   c3_dsc( &GDT[c3_idx] );
   d3_dsc( &GDT[d3_idx] );

   gdtr.desc  = GDT;
   gdtr.limit = sizeof(GDT) - 1;
   set_gdtr(gdtr);

   set_ds(d3_sel);
   set_es(d3_sel);
   set_fs(d3_sel);
   set_gs(d3_sel);

   TSS.s0.esp = STACK_K;
   TSS.s0.ss  = d0_sel;
   tss_dsc(&GDT[ts_idx], (offset_t)&TSS);
   set_tr(ts_sel);
}

void init_intr(){
   int_desc_t *dsc;
   idt_reg_t  idtr;

   get_idtr(idtr);
   dsc = &idtr.desc[80];
   dsc->dpl = 3;
}

// MAIN

void tp(){
   init_intr();      // enable IDT for syscall
   init_memory();    // initialize memory
   init_paging();    // initialize paging
   
   sti();            // enable material interruption
   
   while(1);         // infinit loop  
}
