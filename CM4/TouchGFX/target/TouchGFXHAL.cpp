/**
  ******************************************************************************
  * File Name          : TouchGFXHAL.cpp
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
#include <TouchGFXHAL.hpp>

/* USER CODE BEGIN TouchGFXHAL.cpp */

#include "stm32h7xx_hal.h"
#include <touchgfx/hal/OSWrappers.hpp>
#include "main.h"

/* USER CODE BEGIN Includes */
#include <touchgfx/hal/GPIO.hpp>
#include "../Components/otm8009a/otm8009a.h"
#include <STM32H7Instrumentation.hpp>
#include "FreeRTOS.h"
#include "task.h"
/* USER CODE END Includes */

/* USER CODE BEGIN private defines */
/**
  * @brief  LCD Display OTM8009A ID
  */
#define LCD_OTM8009A_ID  ((uint32_t) 0)
/* USER CODE END private defines */

/* USER CODE BEGIN private variables */
volatile bool displayRefreshing = false;
volatile bool refreshRequested = true;
static int updateRegion = 0;
static uint16_t* currFbBase = 0;
/* USER CODE END private variables */

/* USER CODE BEGIN private functions */

/* USER CODE END private functions */

/* USER CODE BEGIN extern C prototypes */
extern "C" {
    extern DSI_HandleTypeDef hdsi;
    extern LTDC_HandleTypeDef hltdc;

    uint8_t pCols[4][4] =
    {
        {0x00, 0x00, 0x00, 0xC7}, /*   0 -> 199 */
        {0x00, 0xC8, 0x01, 0x8F}, /* 200 -> 399 */
        {0x01, 0x90, 0x02, 0x57}, /* 400 -> 599 */
        {0x02, 0x58, 0x03, 0x1F}, /* 600 -> 799 */
    };

    uint8_t pColLeft[] = { 0x00, 0x00, 0x01, 0x8f }; /*   0 -> 399 */
    uint8_t pColRight[] = { 0x01, 0x90, 0x03, 0x1F }; /* 400 -> 799 */

    /* Request tear interrupt at specific scanline. Implemented in BoardConfiguration.cpp */
    void LCD_ReqTear();

    /* Configures display to update indicated region of the screen (200pixel wide chunks) - 16bpp mode */
    void LCD_SetUpdateRegion(int idx);

    /* Configures display to update left half of the screen. Implemented in BoardConfiguration.cpp  - 24bpp mode*/
    void LCD_SetUpdateRegionLeft();

    /* Configures display to update right half of the screen. Implemented in BoardConfiguration.cpp - 24bpp mode*/
    void LCD_SetUpdateRegionRight();
}
/* USER CODE END extern C prototypes */

using namespace touchgfx;

/* USER CODE BEGIN private class objects */
static STM32H7Instrumentation mcuInstr;
/* USER CODE END private class objects */

TouchGFXHAL::TouchGFXHAL(touchgfx::DMA_Interface& dma, touchgfx::LCD& display, touchgfx::TouchController& tc, uint16_t width, uint16_t height)
/* USER CODE BEGIN TouchGFXHAL Constructor */
    : TouchGFXGeneratedHAL(dma,
                           display,
                           tc,
                           width,
                           height)
      /* USER CODE END TouchGFXHAL Constructor */
{
    /* USER CODE BEGIN TouchGFXHAL Constructor Code */

    /* USER CODE END TouchGFXHAL Constructor Code */
}

void TouchGFXHAL::initialize()
{
    /* USER CODE BEGIN initialize step 1 */
    GPIO::init();
    /* USER CODE END initialize step 1 */

    // Calling parent implementation of initialize().
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.
    // Please note, HAL::initialize() must be called to initialize the framework.

    TouchGFXGeneratedHAL::initialize();

    /* USER CODE BEGIN initialize step 2 */
    lockDMAToFrontPorch(false);

    mcuInstr.init();
    setMCUInstrumentation(&mcuInstr);
    enableMCULoadCalculation(true);

    /* USER CODE END initialize step 2 */
}

void TouchGFXHAL::taskEntry()
{
    /* USER CODE BEGIN taskEntry step 1 */

    /* USER CODE END taskEntry step 1 */

    enableLCDControllerInterrupt();
    enableInterrupts();

    /* USER CODE BEGIN taskEntry step 2 */

    /* USER CODE END taskEntry step 2 */

    OSWrappers::waitForVSync();
    backPorchExited();

    /* USER CODE BEGIN taskEntry step 3 */
    /* Enable the LCD, Send Display on DCS command to display */
    HAL_DSI_ShortWrite(&hdsi, LCD_OTM8009A_ID, DSI_DCS_SHORT_PKT_WRITE_P1, OTM8009A_CMD_DISPON, 0x00);
    /* USER CODE END taskEntry step 3 */

    for (;;)
    {
        OSWrappers::waitForVSync();
        backPorchExited();
    }
}

/**
 * Gets the frame buffer address used by the TFT controller.
 *
 * @return The address of the frame buffer currently being displayed on the TFT.
 */
uint16_t* TouchGFXHAL::getTFTFrameBuffer() const
{
    // Calling parent implementation of getTFTFrameBuffer().
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.

    /* USER CODE BEGIN getTFTFrameBuffer */
    return currFbBase;
    /* USER CODE END getTFTFrameBuffer */
}

/**
 * Sets the frame buffer address used by the TFT controller.
 *
 * @param [in] address New frame buffer address.
 */
void TouchGFXHAL::setTFTFrameBuffer(uint16_t* address)
{
    // Calling parent implementation of setTFTFrameBuffer(uint16_t* address).
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.

    /* USER CODE BEGIN setTFTFrameBuffer */
    TouchGFXGeneratedHAL::setTFTFrameBuffer(address);
    /* USER CODE END setTFTFrameBuffer */
}

/**
 * This function is called whenever the framework has performed a partial draw.
 *
 * @param rect The area of the screen that has been drawn, expressed in absolute coordinates.
 *
 * @see flushFrameBuffer().
 */
void TouchGFXHAL::flushFrameBuffer(const touchgfx::Rect& rect)
{
    // Calling parent implementation of flushFrameBuffer(const touchgfx::Rect& rect).
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.
    // Please note, HAL::flushFrameBuffer(const touchgfx::Rect& rect) must
    // be called to notify the touchgfx framework that flush has been performed.

    /* USER CODE BEGIN flushFrameBuffer step 1 */
    TouchGFXGeneratedHAL::flushFrameBuffer(rect);
    /* USER CODE END flushFrameBuffer step 1 */
}

/**
 * Configures the interrupts relevant for TouchGFX. This primarily entails setting
 * the interrupt priorities for the DMA and LCD interrupts.
 */
void TouchGFXHAL::configureInterrupts()
{
    // Calling parent implementation of configureInterrupts().
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.

    /* USER CODE BEGIN configureInterrupts */
    TouchGFXGeneratedHAL::configureInterrupts();
    /* USER CODE END configureInterrupts */
}

/**
 * Used for enabling interrupts set in configureInterrupts()
 */
void TouchGFXHAL::enableInterrupts()
{
    // Calling parent implementation of enableInterrupts().
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.

    /* USER CODE BEGIN enableInterrupts */
    TouchGFXGeneratedHAL::enableInterrupts();
    /* USER CODE END enableInterrupts */
}

/**
 * Used for disabling interrupts set in configureInterrupts()
 */
void TouchGFXHAL::disableInterrupts()
{
    // Calling parent implementation of disableInterrupts().
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.

    /* USER CODE BEGIN disableInterrupts */
    TouchGFXGeneratedHAL::disableInterrupts();
    /* USER CODE END disableInterrupts */
}

/**
 * Configure the LCD controller to fire interrupts at VSYNC. Called automatically
 * once TouchGFX initialization has completed.
 */
void TouchGFXHAL::enableLCDControllerInterrupt()
{
    // Calling parent implementation of enableLCDControllerInterrupt().
    //
    // To overwrite the generated implementation, omit call to parent function
    // and implemented needed functionality here.


    /* USER CODE BEGIN enableLCDControllerInterrupt */
    LCD_ReqTear();
    __HAL_DSI_CLEAR_FLAG(&hdsi, DSI_IT_ER);
    __HAL_DSI_CLEAR_FLAG(&hdsi, DSI_IT_TE);
    __HAL_DSI_ENABLE_IT(&hdsi, DSI_IT_TE);
    __HAL_DSI_ENABLE_IT(&hdsi, DSI_IT_ER);
    LTDC->IER = 3; /* Enable line and FIFO underrun interrupts */
    /* USER CODE END enableLCDControllerInterrupt */
}

/* USER CODE BEGIN virtual overloaded methods */
void TouchGFXHAL::setFrameBufferStartAddresses(void* frameBuffer, void* doubleBuffer, void* animationStorage)
{
    currFbBase = (uint16_t*)frameBuffer;
    HAL::setFrameBufferStartAddresses(frameBuffer, doubleBuffer, animationStorage);
}

bool TouchGFXHAL::beginFrame()
{
    refreshRequested = false;
    return HAL::beginFrame();
}

void TouchGFXHAL::endFrame()
{
    HAL::endFrame();
    if (frameBufferUpdatedThisFrame)
    {
        refreshRequested = true;
    }
}
/* USER CODE END virtual overloaded methods */

/* USER CODE BEGIN extern C functions */
extern "C" {

    /**************************** LINK OTM8009A (Display driver) ******************/
    /**
      * @brief  DCS or Generic short/long write command
      * @param  ChannelNbr Virtual channel ID
      * @param  Reg Register to be written
      * @param  pData pointer to a buffer of data to be write
      * @param  Size To precise command to be used (short or long)
      * @retval BSP status
      */
    int32_t DSI_IO_Write(uint16_t ChannelNbr, uint16_t Reg, uint8_t* pData, uint16_t Size)
    {
        int32_t ret = BSP_ERROR_NONE;

        if (Size <= 1U)
        {
            if (HAL_DSI_ShortWrite(&hdsi, ChannelNbr, DSI_DCS_SHORT_PKT_WRITE_P1, Reg, (uint32_t)pData[Size]) != HAL_OK)
            {
                ret = BSP_ERROR_BUS_FAILURE;
            }
        }
        else
        {
            if (HAL_DSI_LongWrite(&hdsi, ChannelNbr, DSI_DCS_LONG_PKT_WRITE, Size, (uint32_t)Reg, pData) != HAL_OK)
            {
                ret = BSP_ERROR_BUS_FAILURE;
            }
        }

        return ret;
    }

    /**
      * @brief  DCS or Generic read command
      * @param  ChannelNbr Virtual channel ID
      * @param  Reg Register to be read
      * @param  pData pointer to a buffer to store the payload of a read back operation.
      * @param  Size  Data size to be read (in byte).
      * @retval BSP status
      */
    int32_t DSI_IO_Read(uint16_t ChannelNbr, uint16_t Reg, uint8_t* pData, uint16_t Size)
    {
        int32_t ret = BSP_ERROR_NONE;

        if (HAL_DSI_Read(&hdsi, ChannelNbr, pData, Size, DSI_DCS_SHORT_PKT_READ, Reg, pData) != HAL_OK)
        {
            ret = BSP_ERROR_BUS_FAILURE;
        }

        return ret;
    }

    /**
     * Request TE at scanline.
     */
    void LCD_ReqTear(void)
    {
        uint8_t ScanLineParams[2];
        uint16_t scanline = 533;

        ScanLineParams[0] = scanline >> 8;
        ScanLineParams[1] = scanline & 0x00FF;

        HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 2, 0x44, ScanLineParams);
        HAL_DSI_ShortWrite(&hdsi, 0, DSI_DCS_SHORT_PKT_WRITE_P1, OTM8009A_CMD_TEEON, 0x00);
    }

    /**
     * Request DSI enable if it wasn't already done.
     */
    void LCD_ReqEnable(void)
    {
        static bool firstRefreshRequested = false;

        // Enable DSI interface once the very first framebuffer is ready for display
        if (!firstRefreshRequested)
        {
            firstRefreshRequested = true;

            /* Send Display on DCS Command to display */
            HAL_DSI_ShortWrite(&(hdsi),
                               0,
                               DSI_DCS_SHORT_PKT_WRITE_P1,
                               OTM8009A_CMD_DISPON,
                               0x00);
        }
    }

    void LCD_SetUpdateRegion(int idx)
    {
        HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 4, OTM8009A_CMD_CASET, pCols[idx]);
    }

    void LCD_SetUpdateRegionLeft()
    {
        HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 4, OTM8009A_CMD_CASET, pColLeft);
    }

    void LCD_SetUpdateRegionRight()
    {
        HAL_DSI_LongWrite(&hdsi, 0, DSI_DCS_LONG_PKT_WRITE, 4, OTM8009A_CMD_CASET, pColRight);
    }

    void LCD_SetBrightness(int value)
    {
        HAL_DSI_ShortWrite(&hdsi,
                           0, DSI_DCS_SHORT_PKT_WRITE_P1,
                           OTM8009A_CMD_WRDISBV, (uint16_t)(value * 255) / 100);
    }

    void HAL_DSI_TearingEffectCallback(DSI_HandleTypeDef* hdsi)
    {
        GPIO::set(GPIO::VSYNC_FREQ);

        HAL::getInstance()->vSync();
        OSWrappers::signalVSync();

        // In single buffering, only require that the system waits for display update to be finished if we
        // actually intend to update the display in this frame.
        HAL::getInstance()->lockDMAToFrontPorch(refreshRequested);

        if (refreshRequested && !displayRefreshing)
        {

            // Update region 0 = first area of display (First quarter for 16bpp, first half for 24bpp)
            updateRegion = 0;
            LCD_SetUpdateRegionLeft();

            // Transfer a quarter screen of pixel data.
            HAL_DSI_Refresh(hdsi);
            displayRefreshing = true;
        }
        else
        {
            GPIO::clear(GPIO::VSYNC_FREQ);
        }
    }

    void HAL_DSI_EndOfRefreshCallback(DSI_HandleTypeDef* hdsi)
    {
        if (displayRefreshing)
        {
            if (updateRegion == 0)
            {
                HAL_Delay(1);

                // If we transferred the left half, also transfer right half.
                __HAL_DSI_WRAPPER_DISABLE(hdsi);
                LTDC_LAYER(&hltdc, 0)->CFBAR = ((uint32_t)currFbBase) + (HAL::FRAME_BUFFER_WIDTH / 2) * 3;
                __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&hltdc);
                __HAL_DSI_WRAPPER_ENABLE(hdsi);

                LCD_SetUpdateRegionRight(); //Set display column to 400-799
                updateRegion = 1;
                HAL_DSI_Refresh(hdsi);
            }
            else
            {
                // Otherwise we are done refreshing.
                __HAL_DSI_WRAPPER_DISABLE(hdsi);
                LTDC_LAYER(&hltdc, 0)->CFBAR = (uint32_t)currFbBase;
                __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&hltdc);
                __HAL_DSI_WRAPPER_ENABLE(hdsi);

                GPIO::clear(GPIO::VSYNC_FREQ);

                // Turn on display if not already active
                LCD_ReqEnable();

                displayRefreshing = false;
                if (HAL::getInstance())
                {
                    // Signal to the framework that display update has finished.
                    HAL::getInstance()->frontPorchEntered();
                }
            }
        }
    }

    portBASE_TYPE IdleTaskHook(void* p)
    {
        if ((int)p) //idle task sched out
        {
            touchgfx::HAL::getInstance()->setMCUActive(true);
        }
        else //idle task sched in
        {
            touchgfx::HAL::getInstance()->setMCUActive(false);
        }
        return pdTRUE;
    }

}
/* USER CODE END extern C functions */

/* USER CODE END TouchGFXHAL.cpp */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
