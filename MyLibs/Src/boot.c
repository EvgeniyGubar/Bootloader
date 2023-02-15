/*
 * boot.c
 *
 *  Created on: 9 февр. 2023 г.
 *      Author: Evgeniy
 */

#include "boot.h"
#include "stm32f4xx_hal_flash.h"
#include "fatfs.h"

#define APPLICATION_ADDRESS		(uint32_t)0x08008000	//адрес начала программы

FATFS	fileSystem;
FIL		program;
FILINFO	info;

/************************************************************************
 *
 ***********************************************************************/
void Go_to_App(void)
{
    uint32_t app_jump_address;

    typedef void(*pFunction)(void);		//объявляем пользовательский тип
    pFunction Jump_To_Application;		//и создаём переменную этого типа

    HAL_RCC_DeInit();
    HAL_DeInit();

    app_jump_address = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);	//извлекаем адрес перехода из вектора Reset
    Jump_To_Application = (pFunction)app_jump_address;            	//приводим его к пользовательскому типу
    __disable_irq();												//запрещаем прерывания
    __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);				//устанавливаем SP приложения
    Jump_To_Application();											//запускаем приложение
}

/************************************************************************
 *
 ***********************************************************************/
void flash_erase(void)
{
	HAL_FLASH_Unlock();
//	HAL_FLASHEx_Erase(pEraseInit, SectorError)
//	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);
	FLASH_Erase_Sector(FLASH_SECTOR_2, FLASH_VOLTAGE_RANGE_3);
	FLASH_Erase_Sector(FLASH_SECTOR_3, FLASH_VOLTAGE_RANGE_3);
	FLASH_Erase_Sector(FLASH_SECTOR_4, FLASH_VOLTAGE_RANGE_3);
	FLASH_Erase_Sector(FLASH_SECTOR_5, FLASH_VOLTAGE_RANGE_3);
	FLASH_Erase_Sector(FLASH_SECTOR_6, FLASH_VOLTAGE_RANGE_3);
	FLASH_Erase_Sector(FLASH_SECTOR_7, FLASH_VOLTAGE_RANGE_3);
}

/************************************************************************
 *
 ***********************************************************************/
void flash_program(void)
{
	uint8_t readBuffer[512];
	uint32_t programBytesToRead;
	uint32_t programBytesCounter;
	uint32_t currentAddress;
	UINT readBytes;

	programBytesToRead = info.fsize;	// запоминаем размер файла

	programBytesCounter = 0;
	currentAddress = APPLICATION_ADDRESS;

	while ((programBytesToRead - programBytesCounter) >= 512)
	{
		f_read(&program, readBuffer, 512, &readBytes);
		programBytesCounter += 512;

		for (uint32_t i = 0; i < 512; i += 4)
		{
			HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, currentAddress, *(uint32_t*) &readBuffer[i]);
			currentAddress += 4;
		}
	}

	if (programBytesToRead != programBytesCounter)
	{
		f_read(&program, readBuffer, (programBytesToRead - programBytesCounter), &readBytes);

		for (uint32_t i = 0; i < (programBytesToRead - programBytesCounter); i += 4)
		{
			HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, currentAddress, *(uint32_t*) &readBuffer[i]);
			currentAddress += 4;
		}
		programBytesCounter = programBytesToRead;
	}
	HAL_FLASH_Lock();
}

/************************************************************************
 *
 ***********************************************************************/
void Bootloader_Main(void)
{
	FRESULT res;
	uint8_t path[12] = "program.bin";
	path[11] = '\0';

	printf("Bootloader_main process\r\n");

	res = f_mount(&fileSystem, SDPath, 1);	// монтирование карты памяти
	printf("f_mount is: %d", res);
	if (res == FR_OK)
	{
		res = f_open(&program, (char*) path, FA_READ);	// чтение файла
		printf("f_open is: %d", res);
		if (res == FR_OK)
		{
			f_stat(path, &info);	// считываем длину файла
			flash_erase();
			flash_program();

			res = f_close(&program);	// закрываем файл
			printf("f_close is: %d", res);
			if (res == FR_OK)
			{
				res = f_unlink((char*)path);	// удаляем файл
				printf("f_unlink is: %d", res);
			}

//			NVIC_DisableIRQ(DMA2_Channel4_5_IRQn);
//			NVIC_DisableIRQ(SDIO_IRQn);
//			ResetKey();
//			SetKey();
//			NVIC_SystemReset();
		}
	}

	Go_to_App();	// если карта памяти не найдена


}
