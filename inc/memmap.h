#ifndef _MEMMAP_H_
#define _MEMMAP_H_
#if defined(__cplusplus)
    extern "C" {
#endif

#if defined(STM32L0)
    #define FLASH_R_BASE    0x40022000
    #define FLASH_ACR       0x00
    #define FLASH_PECR      0x04
    #define FLASH_PEKEYR    0x0C
    #define FLASH_PRGKEYR   0x10
    #define FLASH_SR        0x18
    #define FLASH_PEKEY0    0x89ABCDEF
    #define FLASH_PEKEY1    0x02030405
    #define FLASH_PRGKEY0   0x8C9DAEBF
    #define FLASH_PRGKEY1   0x13141516

    #define FLASH_PAGESZ    0x80

    #define RCC_BASE        0x40021000
    #define RCC_CR          0x00
    #define RCC_CFGR        0x0C
    #define RCC_APB1ENR     0x38
    #define RCC_IOPENR      0x2C
    #define RCC_IOPRSTR     0x1C

    #define PWR_BASE        0x40007000
    #define PWR_CR          0x00
    #define PWR_CSR         0x04

    #define GPIOA           0x50000000
    #define GPIOB           0x50000400
    #define GPIOC           0x50000800
    #define GPIOD           0x50000C00
    #define GPIOE           0x50001000
    #define GPIOH           0x50001C00

    #define GPIO_MODER      0x00
    #define GPIO_OTYPER     0x04
    #define GPIO_PUPDR      0x0C
    #define GPIO_IDR        0x10


#elif defined(STM32L1)
    #define FLASH_R_BASE    0x40023C00
    #define FLASH_ACR       0x00
    #define FLASH_PECR      0x04
    #define FLASH_PEKEYR    0x0C
    #define FLASH_PRGKEYR   0x10
    #define FLASH_SR        0x18
    #define FLASH_PEKEY0    0x89ABCDEF
    #define FLASH_PEKEY1    0x02030405
    #define FLASH_PRGKEY0   0x8C9DAEBF
    #define FLASH_PRGKEY1   0x13141516

    #define FLASH_PAGESZ    256

    #define RCC_BASE        0x40023800
    #define RCC_CR          0x00
    #define RCC_CFGR        0x08
    #define RCC_AHBRSTR     0x10
    #define RCC_AHBENR      0x1C
    #define RCC_APB1ENR     0x24

    #define PWR_BASE        0x40007000
    #define PWR_CR          0x00
    #define PWR_CSR         0x04

    #define GPIOA           0x40020000
    #define GPIOB           0x40020400
    #define GPIOC           0x40020800
    #define GPIOD           0x40020C00
    #define GPIOE           0x40021000
    #define GPIOH           0x40021400
    #define GPIOF           0x40021800
    #define GPIOG           0x40021C00

    #define SCB             0xE000ED00
    #define SCB_VTOR        0x08

    #define GPIO_MODER      0x00
    #define GPIO_OTYPER     0x04
    #define GPIO_PUPDR      0x0C
    #define GPIO_IDR        0x10


#endif

#if defined(__cplusplus)
    }
#endif
#endif //_MEMMAP_H_