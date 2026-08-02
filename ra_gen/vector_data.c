/* generated vector source file - do not edit */
#include "bsp_api.h"
/* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
#if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_NUM_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = sci_b_uart_rxi_isr, /* SCI9 RXI (Receive data full) */
            [1] = sci_b_uart_txi_isr, /* SCI9 TXI (Transmit data empty) */
            [2] = sci_b_uart_tei_isr, /* SCI9 TEI (Transmit end) */
            [3] = sci_b_uart_eri_isr, /* SCI9 ERI (Receive error) */
            [4] = ceu_isr, /* CEU CEUI (CEU interrupt) */
            [5] = iic_master_rxi_isr, /* IIC0 RXI (Receive data full) */
            [6] = iic_master_txi_isr, /* IIC0 TXI (Transmit data empty) */
            [7] = iic_master_tei_isr, /* IIC0 TEI (Transmit end) */
            [8] = iic_master_eri_isr, /* IIC0 ERI (Transfer error) */
            [9] = sci_b_uart_rxi_isr, /* SCI6 RXI (Receive data full) */
            [10] = sci_b_uart_txi_isr, /* SCI6 TXI (Transmit data empty) */
            [11] = sci_b_uart_tei_isr, /* SCI6 TEI (Transmit end) */
            [12] = sci_b_uart_eri_isr, /* SCI6 ERI (Receive error) */
            [13] = iic_master_rxi_isr, /* IIC2 RXI (Receive data full) */
            [14] = iic_master_txi_isr, /* IIC2 TXI (Transmit data empty) */
            [15] = iic_master_tei_isr, /* IIC2 TEI (Transmit end) */
            [16] = iic_master_eri_isr, /* IIC2 ERI (Transfer error) */
            [17] = sci_b_uart_rxi_isr, /* SCI1 RXI (Receive data full) */
            [18] = sci_b_uart_txi_isr, /* SCI1 TXI (Transmit data empty) */
            [19] = sci_b_uart_tei_isr, /* SCI1 TEI (Transmit end) */
            [20] = sci_b_uart_eri_isr, /* SCI1 ERI (Receive error) */
            [21] = sci_b_uart_rxi_isr, /* SCI2 RXI (Receive data full) */
            [22] = sci_b_uart_txi_isr, /* SCI2 TXI (Transmit data empty) */
            [23] = sci_b_uart_tei_isr, /* SCI2 TEI (Transmit end) */
            [24] = sci_b_uart_eri_isr, /* SCI2 ERI (Receive error) */
            [25] = sci_b_uart_rxi_isr, /* SCI3 RXI (Receive data full) */
            [26] = sci_b_uart_txi_isr, /* SCI3 TXI (Transmit data empty) */
            [27] = sci_b_uart_tei_isr, /* SCI3 TEI (Transmit end) */
            [28] = sci_b_uart_eri_isr, /* SCI3 ERI (Receive error) */
            [29] = gpt_counter_overflow_isr, /* GPT0 COUNTER OVERFLOW (Overflow) */
            [30] = ssi_rxi_isr, /* SSI1 RXI (Receive data full/Transmit data empty) */
            [31] = ssi_int_isr, /* SSI1 INT (Error interrupt) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_NUM_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_SCI9_RXI,GROUP0), /* SCI9 RXI (Receive data full) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_SCI9_TXI,GROUP1), /* SCI9 TXI (Transmit data empty) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_SCI9_TEI,GROUP2), /* SCI9 TEI (Transmit end) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_SCI9_ERI,GROUP3), /* SCI9 ERI (Receive error) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_CEU_CEUI,GROUP4), /* CEU CEUI (CEU interrupt) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_IIC0_RXI,GROUP5), /* IIC0 RXI (Receive data full) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_IIC0_TXI,GROUP6), /* IIC0 TXI (Transmit data empty) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_IIC0_TEI,GROUP7), /* IIC0 TEI (Transmit end) */
            [8] = BSP_PRV_VECT_ENUM(EVENT_IIC0_ERI,GROUP0), /* IIC0 ERI (Transfer error) */
            [9] = BSP_PRV_VECT_ENUM(EVENT_SCI6_RXI,GROUP1), /* SCI6 RXI (Receive data full) */
            [10] = BSP_PRV_VECT_ENUM(EVENT_SCI6_TXI,GROUP2), /* SCI6 TXI (Transmit data empty) */
            [11] = BSP_PRV_VECT_ENUM(EVENT_SCI6_TEI,GROUP3), /* SCI6 TEI (Transmit end) */
            [12] = BSP_PRV_VECT_ENUM(EVENT_SCI6_ERI,GROUP4), /* SCI6 ERI (Receive error) */
            [13] = BSP_PRV_VECT_ENUM(EVENT_IIC2_RXI,GROUP5), /* IIC2 RXI (Receive data full) */
            [14] = BSP_PRV_VECT_ENUM(EVENT_IIC2_TXI,GROUP6), /* IIC2 TXI (Transmit data empty) */
            [15] = BSP_PRV_VECT_ENUM(EVENT_IIC2_TEI,GROUP7), /* IIC2 TEI (Transmit end) */
            [16] = BSP_PRV_VECT_ENUM(EVENT_IIC2_ERI,GROUP0), /* IIC2 ERI (Transfer error) */
            [17] = BSP_PRV_VECT_ENUM(EVENT_SCI1_RXI,GROUP1), /* SCI1 RXI (Receive data full) */
            [18] = BSP_PRV_VECT_ENUM(EVENT_SCI1_TXI,GROUP2), /* SCI1 TXI (Transmit data empty) */
            [19] = BSP_PRV_VECT_ENUM(EVENT_SCI1_TEI,GROUP3), /* SCI1 TEI (Transmit end) */
            [20] = BSP_PRV_VECT_ENUM(EVENT_SCI1_ERI,GROUP4), /* SCI1 ERI (Receive error) */
            [21] = BSP_PRV_VECT_ENUM(EVENT_SCI2_RXI,GROUP5), /* SCI2 RXI (Receive data full) */
            [22] = BSP_PRV_VECT_ENUM(EVENT_SCI2_TXI,GROUP6), /* SCI2 TXI (Transmit data empty) */
            [23] = BSP_PRV_VECT_ENUM(EVENT_SCI2_TEI,GROUP7), /* SCI2 TEI (Transmit end) */
            [24] = BSP_PRV_VECT_ENUM(EVENT_SCI2_ERI,GROUP0), /* SCI2 ERI (Receive error) */
            [25] = BSP_PRV_VECT_ENUM(EVENT_SCI3_RXI,GROUP1), /* SCI3 RXI (Receive data full) */
            [26] = BSP_PRV_VECT_ENUM(EVENT_SCI3_TXI,GROUP2), /* SCI3 TXI (Transmit data empty) */
            [27] = BSP_PRV_VECT_ENUM(EVENT_SCI3_TEI,GROUP3), /* SCI3 TEI (Transmit end) */
            [28] = BSP_PRV_VECT_ENUM(EVENT_SCI3_ERI,GROUP4), /* SCI3 ERI (Receive error) */
            [29] = BSP_PRV_VECT_ENUM(EVENT_GPT0_COUNTER_OVERFLOW,GROUP5), /* GPT0 COUNTER OVERFLOW (Overflow) */
            [30] = BSP_PRV_VECT_ENUM(EVENT_SSI1_RXI,GROUP6), /* SSI1 RXI (Receive data full/Transmit data empty) */
            [31] = BSP_PRV_VECT_ENUM(EVENT_SSI1_INT,GROUP7), /* SSI1 INT (Error interrupt) */
        };
        #endif
        #endif
