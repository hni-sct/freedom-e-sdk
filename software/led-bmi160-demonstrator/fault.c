#include "encoding.h"
#include "plic/plic_driver.h"
#include <stdio.h>
#include <stdlib.h>
#include "watchdog.h"

static uint8_t float_stuck = 0;
static uint8_t random_flip = 0;
float *sensor_x_ptr;
float *matrix_fx_ptr;

/* fault activation code:
 * This is activated using the buttons 0 and 1 on the arty board
 * which tigger an interrupt each that we handle here
 */
typedef void (*function_ptr_t) (void);
void no_interrupt_handler (void) {}
function_ptr_t g_ext_interrupt_handlers[PLIC_NUM_INTERRUPTS];
plic_instance_t g_plic;

static void button_0_handler(void)
{
    float_stuck = 1 - float_stuck;
    printf("Float_stuck = %d\n", float_stuck);
    uint32_t csr_val = read_csr(0xbc4);

    /* Custom fault injection registers:
     * We write the to be faulty memory address into CSR 0xbc0
     * and a mask to 0xbc1. Each set bit is then stuck-at 0
     */
    write_csr(0xbc0, sensor_x_ptr);
    write_csr(0xbc1, 0xffffffff);

    /* CSR 0xbc4 holds the status flags for the fault injection registers.
     * Writing 0x1 activates it, writing 0x0 deactivates it.
     */
    if (float_stuck) {
        csr_val |= 1;
    } else {
        csr_val &= ~1;
    }
    write_csr(0xbc4, csr_val);
    GPIO_REG(GPIO_RISE_IP) = (0x1 << BUTTON_0_OFFSET);
}

static void button_1_handler(void)
{
    random_flip = 1 - random_flip;
    printf("random flip = %d\n", random_flip);
    uint32_t csr_val = read_csr(0xbc4);

    /* 0xbc2 is the address register
     * 0xbc3 is a mask
     * On a read of address in 0xbc2 a random number is generated in hw and only
     * the bits indicated on the mask are returned
     */
    write_csr(0xbc2, matrix_fx_ptr);
    write_csr(0xbc3, 0x41c00000);
    if (random_flip) {
        csr_val |= 2;
    } else {
        csr_val &= ~2;
    }
    write_csr(0xbc4, csr_val);
    GPIO_REG(GPIO_RISE_IP) = (0x1 << BUTTON_1_OFFSET);

}

static void button_2_handler(void) {
  wdt_pet();
  GPIO_REG(GPIO_RISE_IP) = (0x1 << BUTTON_2_OFFSET);
}

void init_plic(void)
{
    PLIC_init(&g_plic, PLIC_CTRL_ADDR, PLIC_NUM_INTERRUPTS,
              PLIC_NUM_PRIORITIES);

    clear_csr(mie, MIP_MEIP);
    clear_csr(mie, MIP_MTIP);

    for (int ii = 0; ii < PLIC_NUM_INTERRUPTS; ii ++){
      g_ext_interrupt_handlers[ii] = no_interrupt_handler;
    }

    GPIO_REG(GPIO_OUTPUT_EN)  &= ~((0x1 << BUTTON_0_OFFSET)
                                | (0x1 << BUTTON_1_OFFSET)
                                | (0x1 << BUTTON_2_OFFSET));
    GPIO_REG(GPIO_PULLUP_EN)  &= ~((0x1 << BUTTON_0_OFFSET)
                                | (0x1 << BUTTON_1_OFFSET)
                                | (0x1 << BUTTON_2_OFFSET));
    GPIO_REG(GPIO_INPUT_EN)   |=  ((0x1 << BUTTON_0_OFFSET)
                                | (0x1 << BUTTON_1_OFFSET)
                                | (0x1 << BUTTON_2_OFFSET));

    g_ext_interrupt_handlers[INT_DEVICE_BUTTON_0] = button_0_handler;
    g_ext_interrupt_handlers[INT_DEVICE_BUTTON_1] = button_1_handler;
    g_ext_interrupt_handlers[INT_DEVICE_BUTTON_2] = button_2_handler;

    // Have to enable the interrupt both at the GPIO level,
    // and at the PLIC level.
    PLIC_enable_interrupt (&g_plic, INT_DEVICE_BUTTON_0);
    PLIC_enable_interrupt (&g_plic, INT_DEVICE_BUTTON_1);
    PLIC_enable_interrupt(&g_plic, INT_DEVICE_BUTTON_2);

    // Priority must be set > 0 to trigger the interrupt.
    PLIC_set_priority(&g_plic, INT_DEVICE_BUTTON_0, 1);
    PLIC_set_priority(&g_plic, INT_DEVICE_BUTTON_1, 1);
    PLIC_set_priority(&g_plic, INT_DEVICE_BUTTON_2, 1);

    GPIO_REG(GPIO_RISE_IE) |= (1 << BUTTON_0_OFFSET);
    GPIO_REG(GPIO_RISE_IE) |= (1 << BUTTON_1_OFFSET);
    GPIO_REG(GPIO_RISE_IE) |= (1 << BUTTON_2_OFFSET);

    // Enable the Machine-External bit in MIE
    set_csr(mie, MIP_MEIP);

    // Setup LED for Fault indication
    GPIO_REG(GPIO_INPUT_EN)    &= ~((0x1<< RED_LED_OFFSET) | (0x1<< GREEN_LED_OFFSET) | (0x1 << BLUE_LED_OFFSET));
    GPIO_REG(GPIO_OUTPUT_EN)   |=  ((0x1<< RED_LED_OFFSET)| (0x1<< GREEN_LED_OFFSET) | (0x1 << BLUE_LED_OFFSET));
    GPIO_REG(GPIO_OUTPUT_VAL)  &=  ~((0x1<< RED_LED_OFFSET) | (0x1<< GREEN_LED_OFFSET) | (0x1 << BLUE_LED_OFFSET));

    // Enable interrupts in general.
    set_csr(mstatus, MSTATUS_MIE);
    printf("Reset Demo\n");
}

void handle_m_ext_interrupt(){
  plic_source int_num  = PLIC_claim_interrupt(&g_plic);
  if ((int_num >=1 ) && (int_num < PLIC_NUM_INTERRUPTS)) {
    g_ext_interrupt_handlers[int_num]();
  }
  else {
    exit(1 + (uintptr_t) int_num);
  }
  PLIC_complete_interrupt(&g_plic, int_num);
}
