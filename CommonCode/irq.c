/*
AcessOS 0.1
IRQ (Interrupt Request) Manager

The IDT is very similar to the GDT
*/
#include <acess.h>

//IMPORT
// IRQ Prototypes (in start.asm)
extern void irq0();	extern void irq1();	extern void irq2(); extern void irq3();
extern void irq4();	extern void irq5();	extern void irq6(); extern void irq7();
extern void irq8(); extern void irq9(); extern void irq10(); extern void irq11();
extern void irq12(); extern void irq13(); extern void irq14(); extern void irq15();

//GLOBALS
//IRQ List - Array of void functions
static void *irq_routines[16] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

//CODE
/**
 \fn void irq_install_handler(int irq, void (*handler)(struct regs *r))
 \brief Assign callback to a IRQ
 \param irq	integer - IRQ number
 \param handler	function - Function to call on IRQ
*/
void irq_install_handler(int irq, void (*handler)(struct regs *r))
{
	printf("[IRQ ] IRQ#%i => 0x%x\n", irq, (Uint)handler);
    irq_routines[irq] = handler;
}

/**
 \fn void irq_uninstall_handler(int irq)
 \brief Clears the callback on an IRQ
 \param irq	integer - IRQ to clear
*/
void irq_uninstall_handler(int irq)
{
    irq_routines[irq] = 0;
}

/**
 \fn void irq_remap(void)
 \brief Tells the PIC to remap the IRQs
 
 By default IRQs 0-7 are bound to IDT entries 8-15. These
 are used as error interrupts in protected mode. This
 tells the PIC (8259) to re-assign the IRQs
*/
void irq_remap(void)
{
    outportb(0x20, 0x11);	//Init Command
    outportb(0x21, IRQ_OFFSET);	//Offset (Start of IDT Range)
    outportb(0x21, 0x04);	//IRQ connected to Slave (00000100b) = IRQ2
    outportb(0x21, 0x01);	//Set Mode
    outportb(0x21, 0x0);	//Set Mask
	
    outportb(0xA0, 0x11);
    outportb(0xA1, IRQ_OFFSET+8);
    outportb(0xA1, 0x02);	// IRQ Line connected to master
    outportb(0xA1, 0x01);	//Set Mode
    outportb(0xA1, 0x0);	//Set Mask
}

/**
 \fn void irq_install()
 \brief Set up IRQ handling
 
 Called by main - Remaps the IRQs and then sets the IDT
 entries to the IRQ hanlers in assembler.asm
*/
void irq_install()
{
    irq_remap();

    IDT_SetGate(IRQ_OFFSET, (unsigned)irq0, 0x08, 0x8E);
    IDT_SetGate(IRQ_OFFSET+1, (unsigned)irq1, 0x08, 0x8E);
    IDT_SetGate(IRQ_OFFSET+2, (unsigned)irq2, 0x08, 0x8E);
    IDT_SetGate(IRQ_OFFSET+3, (unsigned)irq3, 0x08, 0x8E);
    IDT_SetGate(IRQ_OFFSET+4, (unsigned)irq4, 0x08, 0x8E);
    IDT_SetGate(IRQ_OFFSET+5, (unsigned)irq5, 0x08, 0x8E);
    IDT_SetGate(IRQ_OFFSET+6, (unsigned)irq6, 0x08, 0x8E);
    IDT_SetGate(IRQ_OFFSET+7, (unsigned)irq7, 0x08, 0x8E);

    IDT_SetGate(IRQ_OFFSET+8, (unsigned)irq8, 0x08, 0x8E);
    IDT_SetGate(IRQ_OFFSET+9, (unsigned)irq9, 0x08, 0x8E);
    IDT_SetGate(IRQ_OFFSET+10, (unsigned)irq10, 0x08, 0x8E);
    IDT_SetGate(IRQ_OFFSET+11, (unsigned)irq11, 0x08, 0x8E);
    IDT_SetGate(IRQ_OFFSET+12, (unsigned)irq12, 0x08, 0x8E);
    IDT_SetGate(IRQ_OFFSET+13, (unsigned)irq13, 0x08, 0x8E);
    IDT_SetGate(IRQ_OFFSET+14, (unsigned)irq14, 0x08, 0x8E);
    IDT_SetGate(IRQ_OFFSET+15, (unsigned)irq15, 0x08, 0x8E);
}

/**
 \fn void irq_handler(struct regs *r)
 \brief 
 \param r	registers - Register state at the time of interrupt
 
 Called by the irqX functions
 Calls the callbacks assigned to the fired interrupt and
 then tells the interrupt controller that the interrupt
 has been handled.
*/
void irq_handler(struct regs *r)
{
    void (*handler)(struct regs *r);
	
	//LogF("IRQ%i\n", r->int_no);
	
    //Determine the callback
    handler = irq_routines[r->int_no];
    if (handler) {	//And If it has been set,
        handler(r);		//Call it
    }

	// Determine if it was called by the slave controller
    if (r->int_no >= 8) {
        outportb(0xA0, 0x20);	//If so send EOI to Slave
    }

    // Send EOI to Master Controller
    outportb(0x20, 0x20);
}
