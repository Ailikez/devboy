//******************************************************************************
//  @file Application.cpp
//  @author Nicolai Shlapunov
//
//  @details Application: User Application Class, implementation
//
//  @copyright Copyright (c) 2016, Devtronic & Nicolai Shlapunov
//             All rights reserved.
//
//  @section SUPPORT
//
//   Devtronic invests time and resources providing this open source code,
//   please support Devtronic and open-source hardware/software by
//   donations and/or purchasing products from Devtronic.
//
//******************************************************************************

// *****************************************************************************
// ***   Includes   ************************************************************
// *****************************************************************************
#include "Application.h"

#include "Tetris.h"
#include "Pong.h"
#include "Gario.h"
#include "Calc.h"
#include "GraphDemo.h"
#include "InputTest.h"

#include "fatfs.h"
#include "usbd_cdc.h"

#include "StHalIic.h"
#include "BoschBME280.h"
//#include "Eeprom24.h"

// *****************************************************************************
// ***   Get Instance   ********************************************************
// *****************************************************************************
Application& Application::GetInstance(void)
{
   static Application application;
   return application;
}

// *****************************************************************************
// ***   Test get function   ***************************************************
// *****************************************************************************
char* Application::GetMenuStr(void* ptr, char* buf, uint32_t n, uint32_t add_param)
{
//  Application* app = (Application*)ptr;
  snprintf(buf, n, "%lu", add_param);
  return buf;
}

// *****************************************************************************
// ***   Application Loop   ****************************************************
// *****************************************************************************
Result Application::Loop()
{
  Result result;

  StHalIic iic(BME280_HI2C);

  // Sound control on the touchscreen
  SoundControlBox snd_box(0, 0);
  snd_box.Move(display_drv.GetScreenW() - snd_box.GetWidth(), display_drv.GetScreenH() - snd_box.GetHeight());
  snd_box.Show(32768);

  // ***   Menu Items   ********************************************************
  UiMenu::MenuItem main_menu_items[] =
  {{"Tetris",          nullptr, &Application::GetMenuStr, this, 1},
   {"Pong",            nullptr, &Application::GetMenuStr, this, 2},
   {"Gario",           nullptr, &Application::GetMenuStr, this, 3},
   {"Calc",            nullptr, &Application::GetMenuStr, this, 4},
   {"Graphic demo",    nullptr, &Application::GetMenuStr, this, 5},
   {"Input test",      nullptr, &Application::GetMenuStr, this, 6},
   {"SD write test",   nullptr, &Application::GetMenuStr, this, 7},
   {"USB test",        nullptr, &Application::GetMenuStr, this, 8},
   {"Servo test",      nullptr, &Application::GetMenuStr, this, 9},
   {"Touch calibrate", nullptr, &Application::GetMenuStr, this, 10},
   {"I2C Ping",        nullptr, &Application::GetMenuStr, this, 11}};

  // Create menu object
  UiMenu menu("Main Menu", main_menu_items, NumberOf(main_menu_items));

  // Main cycle
  while(1)
  {
    // Call menu main function
    if(menu.Run())
    {
      switch(menu.GetCurrentPosition())
      {
        // Tetris Application
        case 0:
          Tetris::GetInstance().Loop();
          break;

        // Pong Application
        case 1:
          Pong::GetInstance().Loop();
          break;

        case 2:
          // Gario Application
          Gario::GetInstance().Loop();
          break;

        // Calc Application
        case 3:
          Calc::GetInstance().Loop();
          break;

        // GraphDemo Application
        case 4:
          GraphDemo::GetInstance().Loop();
          break;

        // InputTest Application
        case 5:
        	InputTest::GetInstance().Loop();
          break;

        // SD write test
        case 6:
        {
          // Mount SD
          FRESULT fres = f_mount(&SDFatFS, (TCHAR const*)SDPath, 0);
          // Open file
          if(fres == FR_OK)
          {
            fres = f_open(&SDFile, "STM32.TXT", FA_CREATE_ALWAYS | FA_WRITE);
          }
          // Write data to file
          if(fres == FR_OK)
          {
            // File write counts
            UINT wbytes;
            // File write buffer
            char wtext[128];
            snprintf(wtext, NumberOf(wtext), "SD write test. Timestamp: %lu\r\n", HAL_GetTick());
            for(uint32_t i = 0U; i < 10U; i++)
            {
              fres = f_write(&SDFile, wtext, strlen(wtext), &wbytes);
              if(fres != FR_OK) break;
            }
          }
          // Close file
          if(fres == FR_OK)
          {
            fres = f_close(&SDFile);
          }
          // Show result
          if(fres == FR_OK)
          {
            UiMsgBox msg_box("File written successfully", "Success");
            msg_box.Run(3000U);
          }
          else
          {
            UiMsgBox msg_box("File write error", "Error");
            msg_box.Run(3000U);
          }
          break;
        }

        // USB CDC test
        case 7:
        {
          uint8_t str[64];
          // Create string
          snprintf((char*)str, sizeof(str), "USB test. Timestamp: %lu\r\n", HAL_GetTick());
          // Send to USB
          if(USBD_CDC_SetTxBuffer(&hUsbDeviceFS, str, strlen((char*)str)) == USBD_OK)
          {
            USBD_CDC_TransmitPacket(&hUsbDeviceFS);
            // Wait until transmission finished
            while((hUsbDeviceFS.pClassData != nullptr) && (((USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData)->TxState == 1))
            {
              RtosTick::DelayTicks(1U);
            }
          }
          break;
        }

        // Servo test
        case 8:
        {
          // Configure GPIO pin : PB12
          GPIO_InitTypeDef GPIO_InitStruct;
          GPIO_InitStruct.Pin = GPIO_PIN_12;
          GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
          GPIO_InitStruct.Pull = GPIO_NOPULL;
          GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
          HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

          for(uint32_t n = 0U; n < 100U; n++)
          {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
            RtosTick::DelayTicks(1U);
            for(volatile uint32_t i = 0U; i < 1000U; i++);
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
            RtosTick::DelayTicks(18U);
          }
        }
        break;

        // Touchscreen Calibration
        case 9:
          display_drv.TouchCalibrate();
          break;

        case 10:
          IicPing(iic);
          break;
         
        default:
          break;
      }
    }
  }

  // Always run
  return Result::RESULT_OK;
}

// *****************************************************************************
// ***   IicPing   *************************************************************
// *****************************************************************************
Result Application::IicPing(IIic& iic)
{
  // Set error by default for initialize sensor first time
  Result result = Result::ERR_I2C_UNKNOWN;

  // Strings
  String str_arr[2+8+1];
  // Buffer for strings
  static char str_buf[8+1][64] = {0};

  // Header
  str_arr[8].SetParams("  | x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 xA xB xC xD xE xF", 0U, 0, COLOR_WHITE, String::FONT_6x8);
  str_arr[9].SetParams("---------------------------------------------------", 0U, 8, COLOR_WHITE, String::FONT_6x8);
  // Sensor data
  str_arr[10U].SetParams(str_buf[8U], 0U, 8 * (2+10), COLOR_WHITE, String::FONT_8x12);
  // Show strings
  for(uint32_t i = 0U; i < NumberOf(str_arr); i++)
  {
    // Set params for
    if(i < 8U)
    {
      // Set result string
      str_arr[i].SetParams(str_buf[i], 0U, 8 * (2+i), COLOR_WHITE, String::FONT_6x8);
    }
    str_arr[i].Show(10000);
  }

  // TODO: test code below works, but by some reason break
  // BME280 communication. Use Logic Analyzer to figureout
  // what happens.
//  Eeprom24 eeprom(iic);
//  eeprom.Init();
//  uint8_t buf_out[] = "Test!!!";
//  result = eeprom.Write(0U, buf_out, sizeof(buf_out));
//  uint8_t buf_in[16] = {0};
//  result |= eeprom.Read(0U, buf_in, sizeof(buf_in));

  // Sensor object
  BoschBME280 bmp280(iic);

  // Loop until user press "Left"
  while(input_drv.GetButtonState(InputDrv::EXT_LEFT, InputDrv::BTN_LEFT) == false)
  {
    // Ping all I2C adresses
    for(uint32_t i = 0U; i < 8U; i++)
    {
      // Entry
      sprintf(str_buf[i], "%Xx|", (unsigned int)i);
      // Set pointer to empty space
      char* str_ptr = str_buf[i] + 3U;
      // 16 addresses ping
      for(uint32_t j = 0U; j < 16U; j++)
      {
        // Ping device
        Result res = iic.IsDeviceReady((i << 4) | j, 3);
        // Check result
        if(res == Result::RESULT_OK)
        {
          sprintf(str_ptr, " %02X", (unsigned int)((i << 4) | j)); // Received an ACK at that address
        }
        else
        {
          sprintf(str_ptr, " --"); // No ACK received at that address
        }
        str_ptr += 3U;
      }
    }

    // Reinitialize sensor
    if(result.IsBad())
    {
      result = bmp280.Initialize();
      result |= bmp280.SetSampling(BoschBME280::MODE_FORCED);
    }
    // Take measurement
    result |= bmp280.TakeMeasurement();
    // Get values
    int32_t temp = bmp280.GetTemperature_x100();
    int32_t press = bmp280.GetPressure_x256() / 256;
    int32_t humid = (bmp280.GetHumidity_x1024() * 100) / 1024;
    // Generate string
    sprintf(str_buf[8U], "T=%ld.%02ldC P=%ldPa H=%ld.%02ld%% %s", temp/100, abs(temp%100), press, humid/100, abs(humid%100), result.IsGood() ? "" : "ERROR"); // Received an ACK at that address

    // Update display
    display_drv.UpdateDisplay();
    // Wait
    RtosTick::DelayMs(100U);
  }

  // Always Ok
  return Result::RESULT_OK;
}

// *****************************************************************************
// *****************************************************************************
// ***   SoundControlBox   *****************************************************
// *****************************************************************************
// *****************************************************************************

const uint8_t mute_off_img[] = {
0xD7, 0xD7, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 
0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xD7, 0xD7, 0xD7, 0xFB, 0xFB, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFB, 0xD7, 0xFB, 0xFB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x31, 0x62, 0x68, 0x68, 0x62, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xFB, 0xFB, 0xFB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x68, 0x93, 0x93, 0x69, 0x93, 
0x93, 0x93, 0x68, 0x37, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 
0xFB, 0x00, 0x00, 0x00, 0x00, 0x31, 0x93, 0x68, 0x99, 0xC4, 0xCA, 0xC4, 0x99, 0x68, 0x69, 0x68, 
0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFB, 0x00, 0x00, 0x00, 
0x31, 0x93, 0x93, 0xCB, 0xFB, 0xCA, 0xCA, 0xCA, 0xFB, 0xFB, 0x99, 0x68, 0x68, 0x62, 0x37, 0x68, 
0x62, 0x68, 0x31, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFB, 0x00, 0x00, 0x00, 0x93, 0x99, 0xFB, 0xC4, 
0x93, 0x68, 0x93, 0x93, 0x99, 0xCA, 0xFB, 0xC4, 0x68, 0x68, 0x62, 0x62, 0x93, 0x68, 0x99, 0x62, 
0x00, 0x00, 0x00, 0xFB, 0xFB, 0x00, 0x00, 0x38, 0x93, 0xFB, 0x99, 0x68, 0x93, 0x99, 0x99, 0x99, 
0x99, 0x99, 0xC4, 0xFB, 0xC4, 0x68, 0x68, 0x31, 0x99, 0x93, 0x68, 0x99, 0x31, 0x00, 0x00, 0xFB, 
0xFB, 0x00, 0x00, 0x68, 0xCA, 0xC4, 0x68, 0x93, 0x99, 0x99, 0x99, 0x9A, 0xC4, 0xC4, 0x99, 0xCA, 
0xFB, 0x93, 0x68, 0x62, 0x62, 0x99, 0x68, 0x99, 0x93, 0x00, 0x00, 0xFB, 0xFB, 0x00, 0x31, 0x93, 
0xF5, 0x68, 0x93, 0x99, 0x99, 0x99, 0xC4, 0xC4, 0xC4, 0xCA, 0x99, 0x68, 0xCA, 0xCA, 0x62, 0x68, 
0x31, 0x99, 0x93, 0x93, 0x99, 0x31, 0x00, 0xFB, 0xFB, 0x00, 0x62, 0x99, 0xC4, 0x69, 0x93, 0x99, 
0x99, 0xC4, 0xC4, 0xCA, 0xCA, 0x99, 0x93, 0x93, 0x99, 0xFB, 0x93, 0x68, 0x37, 0x99, 0xC4, 0x68, 
0xC4, 0x37, 0x00, 0xFB, 0xFB, 0x00, 0x62, 0xCA, 0x93, 0x93, 0x99, 0x99, 0xC4, 0xC4, 0xCA, 0xCA, 
0xCA, 0x99, 0xC4, 0xC4, 0x68, 0xFB, 0x9A, 0x62, 0x62, 0x68, 0xC4, 0x62, 0x93, 0x37, 0x00, 0xFB, 
0xFB, 0x00, 0x68, 0xFB, 0x68, 0x93, 0x99, 0x99, 0x99, 0xC4, 0xCA, 0xCA, 0x99, 0x93, 0xC4, 0x99, 
0x62, 0xCA, 0xCA, 0x99, 0x93, 0x38, 0x68, 0x38, 0x68, 0x31, 0x00, 0xFB, 0xFB, 0x00, 0x68, 0xFB, 
0x68, 0x93, 0x93, 0x99, 0x99, 0x99, 0xC4, 0xCA, 0x99, 0x37, 0x68, 0x99, 0xCA, 0x94, 0x64, 0x64, 
0x6A, 0xC5, 0xC4, 0x38, 0x62, 0x31, 0x00, 0xFB, 0xFB, 0x00, 0x62, 0xF5, 0x68, 0x93, 0x93, 0x93, 
0x99, 0xC4, 0xCA, 0xCA, 0xCA, 0x31, 0x99, 0x9B, 0x34, 0x34, 0x3A, 0x3A, 0x3A, 0x34, 0x64, 0xCA, 
0x62, 0x00, 0x00, 0xFB, 0xFB, 0x00, 0x62, 0xCA, 0x93, 0x93, 0x93, 0x99, 0xC4, 0xCA, 0xCA, 0xCA, 
0xFB, 0xCA, 0x9B, 0x34, 0x3A, 0x3A, 0x3A, 0x3A, 0x3A, 0x64, 0x3A, 0x3A, 0x9A, 0x00, 0x00, 0xFB, 
0xFB, 0x00, 0x37, 0x99, 0x99, 0x93, 0x99, 0x99, 0x99, 0x99, 0x99, 0xCA, 0xCB, 0xF5, 0x34, 0x64, 
0xF5, 0xF5, 0x3B, 0x3B, 0x9B, 0xFB, 0x95, 0x34, 0x95, 0x31, 0x00, 0xFB, 0xFB, 0x00, 0x31, 0x93, 
0xCA, 0x69, 0x93, 0x99, 0x99, 0x99, 0xC4, 0xCA, 0xCA, 0x9B, 0x3A, 0x3B, 0xF5, 0xFB, 0xF5, 0x9B, 
0xFB, 0xFB, 0x6A, 0x3A, 0x3A, 0x93, 0x00, 0xFB, 0xFB, 0x00, 0x00, 0x62, 0xCA, 0x93, 0x93, 0x93, 
0x93, 0x99, 0xC4, 0xCA, 0xCA, 0x65, 0x3B, 0x3B, 0x3B, 0xCB, 0xFB, 0xFB, 0xFB, 0x6A, 0x35, 0x3B, 
0x35, 0xC4, 0x00, 0xFB, 0xFB, 0x00, 0x00, 0x37, 0x93, 0xCA, 0x68, 0x93, 0x93, 0x99, 0xC4, 0xC4, 
0xCA, 0x65, 0x3B, 0x11, 0x0B, 0x9B, 0xFB, 0xFB, 0xFB, 0x0B, 0x0B, 0x3B, 0x35, 0xC4, 0x00, 0xFB, 
0xFB, 0x00, 0x00, 0x00, 0x62, 0xC4, 0x99, 0x92, 0x93, 0x99, 0x99, 0x99, 0xCA, 0x6B, 0x3B, 0x0B, 
0x71, 0xFB, 0xFB, 0xFB, 0xFB, 0xCB, 0x11, 0x0B, 0x35, 0x9A, 0x00, 0xFB, 0xFB, 0x00, 0x00, 0x00, 
0x31, 0x62, 0xC4, 0x99, 0x68, 0x93, 0x99, 0x99, 0x99, 0xCB, 0x0B, 0x3B, 0xFB, 0xFB, 0x6A, 0x11, 
0xCB, 0xFB, 0x9B, 0x0B, 0x65, 0x62, 0x00, 0xFB, 0xFB, 0x00, 0x00, 0x00, 0x00, 0x37, 0x62, 0x99, 
0xCA, 0x99, 0x93, 0x93, 0x99, 0xFB, 0x3B, 0x0B, 0x6B, 0x6A, 0x11, 0x11, 0x11, 0x9B, 0x0A, 0x0B, 
0xCB, 0x00, 0x00, 0xFB, 0xFB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x62, 0x62, 0x99, 0xCA, 0xCA, 
0xCA, 0x99, 0xCB, 0x3B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x9B, 0x31, 0x00, 0x00, 0xFB, 
0xFB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x62, 0x62, 0x37, 0x37, 0x37, 0x38, 0x62, 0xCA, 
0x9B, 0x3B, 0x0B, 0x0B, 0x0B, 0x6B, 0xCB, 0x31, 0x00, 0x00, 0x00, 0xFB, 0xFB, 0xFB, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x31, 0x31, 0x31, 0x31, 0x06, 0x00, 0x31, 0x99, 0xCA, 0xCA, 
0x9A, 0x62, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFB, 0xD7, 0xFB, 0xFB, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0xFB, 0xFB, 0xD7, 0xD7, 0xD7, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 
0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xD7, 0xD7};

const uint8_t mute_on_img[] = {
0xD7, 0xD7, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 
0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xD7, 0xD7, 0xD7, 0xFB, 0xFB, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFB, 0xD7, 0xFB, 0xFB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x31, 0x62, 0x68, 0x68, 0x62, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0xFB, 0xFB, 0xFB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x68, 0x93, 0x93, 0x69, 0x93, 
0x93, 0x93, 0x68, 0x37, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 
0xFB, 0x00, 0x00, 0x00, 0x00, 0x31, 0x93, 0x68, 0x99, 0xC4, 0xCA, 0xC4, 0x99, 0x68, 0x69, 0x68, 
0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFB, 0x00, 0x00, 0x00, 
0x31, 0x93, 0x93, 0xCB, 0xFB, 0xCA, 0xCA, 0xCA, 0xFB, 0xFB, 0x99, 0x68, 0x68, 0x62, 0x37, 0x68, 
0x62, 0x68, 0x31, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFB, 0x00, 0x00, 0x00, 0x93, 0x99, 0xFB, 0xC4, 
0x93, 0x68, 0x93, 0x93, 0x99, 0xCA, 0xFB, 0xC4, 0x68, 0x68, 0x62, 0x62, 0x93, 0x68, 0x99, 0x62, 
0x00, 0x00, 0x00, 0xFB, 0xFB, 0x00, 0x00, 0x38, 0x93, 0xFB, 0x99, 0x68, 0x93, 0x99, 0x99, 0x99, 
0x99, 0x99, 0xC4, 0xFB, 0xC4, 0x68, 0x68, 0x31, 0x99, 0x93, 0x68, 0x99, 0x31, 0x00, 0x00, 0xFB, 
0xFB, 0x00, 0x00, 0x68, 0xCA, 0xC4, 0x68, 0x93, 0x99, 0x99, 0x99, 0x9A, 0xC4, 0xC4, 0x99, 0xCA, 
0xFB, 0x93, 0x68, 0x62, 0x62, 0x99, 0x68, 0x99, 0x93, 0x00, 0x00, 0xFB, 0xFB, 0x00, 0x31, 0x93, 
0xF5, 0x68, 0x93, 0x99, 0x99, 0x99, 0xC4, 0xC4, 0xC4, 0xCA, 0x99, 0x68, 0xCA, 0xCA, 0x62, 0x68, 
0x31, 0x99, 0x93, 0x93, 0x99, 0x31, 0x00, 0xFB, 0xFB, 0x00, 0x62, 0x99, 0xC4, 0x69, 0x93, 0x99, 
0x99, 0xC4, 0xC4, 0xCA, 0xCA, 0x99, 0x93, 0x93, 0x99, 0xFB, 0x93, 0x68, 0x37, 0x99, 0xC4, 0x68, 
0xC4, 0x37, 0x00, 0xFB, 0xFB, 0x00, 0x62, 0xCA, 0x93, 0x93, 0x99, 0x99, 0xC4, 0xC4, 0xCA, 0xCA, 
0xCA, 0x99, 0xC4, 0xC4, 0x68, 0xFB, 0x9A, 0x62, 0x62, 0x68, 0xC4, 0x62, 0x93, 0x37, 0x00, 0xFB, 
0xFB, 0x00, 0x68, 0xFB, 0x68, 0x93, 0x99, 0x99, 0x99, 0xC4, 0xCA, 0xCA, 0x99, 0x93, 0xC4, 0x99, 
0x62, 0xCA, 0xCA, 0x62, 0x62, 0x31, 0x68, 0x38, 0x68, 0x31, 0x00, 0xFB, 0xFB, 0x00, 0x68, 0xFB, 
0x68, 0x93, 0x93, 0x99, 0x99, 0x99, 0xC4, 0xCA, 0x99, 0x37, 0x68, 0x62, 0x31, 0x99, 0xFB, 0x62, 
0x62, 0x31, 0x62, 0x37, 0x62, 0x31, 0x00, 0xFB, 0xFB, 0x00, 0x62, 0xF5, 0x68, 0x93, 0x93, 0x93, 
0x99, 0xC4, 0xCA, 0xCA, 0xCA, 0x31, 0x31, 0x31, 0x31, 0x93, 0xFB, 0x62, 0x62, 0x31, 0x62, 0x31, 
0x62, 0x00, 0x00, 0xFB, 0xFB, 0x00, 0x62, 0xCA, 0x93, 0x93, 0x93, 0x99, 0xC4, 0xCA, 0xCA, 0xCA, 
0xFB, 0x99, 0x31, 0x31, 0x31, 0x93, 0xFB, 0x62, 0x62, 0x31, 0x38, 0x31, 0x31, 0x00, 0x00, 0xFB, 
0xFB, 0x00, 0x37, 0x99, 0x99, 0x93, 0x99, 0x99, 0x99, 0x99, 0x99, 0xCA, 0xCB, 0xF5, 0xCA, 0x69, 
0x68, 0x99, 0xF5, 0x62, 0x62, 0x31, 0x31, 0x31, 0x00, 0x00, 0x00, 0xFB, 0xFB, 0x00, 0x31, 0x93, 
0xCA, 0x69, 0x93, 0x99, 0x99, 0x99, 0xC4, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0xC4, 0x62, 
0x37, 0x31, 0x06, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFB, 0x00, 0x00, 0x62, 0xCA, 0x93, 0x93, 0x93, 
0x93, 0x99, 0xC4, 0xCA, 0xCA, 0xCA, 0xCA, 0xCA, 0x99, 0xFB, 0x93, 0x62, 0x31, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0xFB, 0xFB, 0x00, 0x00, 0x37, 0x93, 0xCA, 0x68, 0x93, 0x93, 0x99, 0xC4, 0xC4, 
0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xF5, 0x62, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 
0xFB, 0x00, 0x00, 0x00, 0x62, 0xC4, 0x99, 0x92, 0x93, 0x99, 0x99, 0x99, 0x99, 0xC4, 0x99, 0x99, 
0xF5, 0x93, 0x62, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFB, 0x00, 0x00, 0x00, 
0x31, 0x62, 0xC4, 0x99, 0x68, 0x93, 0x99, 0x99, 0x99, 0x99, 0x99, 0xCA, 0x99, 0x37, 0x37, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFB, 0x00, 0x00, 0x00, 0x00, 0x37, 0x62, 0x99, 
0xCA, 0x99, 0x93, 0x93, 0x99, 0xC4, 0xF5, 0x99, 0x37, 0x37, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0xFB, 0xFB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x62, 0x62, 0x99, 0xCA, 0xCA, 
0xCA, 0x99, 0x62, 0x37, 0x37, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 
0xFB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x62, 0x62, 0x37, 0x37, 0x37, 0x38, 0x37, 0x31, 
0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFB, 0xFB, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x31, 0x31, 0x31, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFB, 0xD7, 0xFB, 0xFB, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0xFB, 0xFB, 0xD7, 0xD7, 0xD7, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 
0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xFB, 0xD7, 0xD7};

const ImageDesc mute_img[] = {
{28, 28, 8, {.img8 = mute_off_img}, PALETTE_676, PALETTE_676[0xD7]},
{28, 28, 8, {.img8 = mute_on_img},  PALETTE_676, PALETTE_676[0xD7]}};

// *****************************************************************************
// ***   Constructor   *********************************************************
// *****************************************************************************
SoundControlBox::SoundControlBox(int32_t x, int32_t y, bool mute_flag) :
                                                        Image(x, y, mute_img[0])
{
  // Store mute flag
  mute = mute_flag;
  // If mute is false - update image
  if(mute == false)
  {
    SetImage(mute_img[0]);
  }
  sound_drv.Mute(mute);
  // This object is active
  active = true;
}

// *****************************************************************************
// ***   Action   **************************************************************
// *****************************************************************************
void SoundControlBox::Action(VisObject::ActionType action, int32_t tx, int32_t ty)
{
  // Switch for process action
  switch(action)
  {
    // Touch action
    case VisObject::ACT_TOUCH:
      // Change checked state
      mute = !mute;
      // Update image
      if(mute == true)
      {
        SetImage(mute_img[0], true); // Don't take semaphore - already taken
      }
      else
      {
        SetImage(mute_img[1], true); // Don't take semaphore - already taken
      }
      // Mute control
      sound_drv.Mute(mute);
      break;
  
    // Untouch action 
    case VisObject::ACT_UNTOUCH:
      break;

    default:
      break;
  }
}
