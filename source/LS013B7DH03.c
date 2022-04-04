/*
 * LS013B7DH03.C
 *
 *  Created on: 28 Mar 2022
 *      Author: Gabriele
 */


#include "shm_graphics.h"
#include "shm_fonts.h"
#include "LS013B7DH03.h"
#include "main.h"

extern SPI_HandleTypeDef hspi1;       //SPI interface for display
extern TIM_HandleTypeDef htim5;       //Timer for 50Hz external clock generation

// Screen object
volatile LCD_128x128_t shm_128x128;

// Screenbuffer
volatile uint8_t LCD_128x128_Buffer[LCD_HEIGHT * LCD_HEIGHT / 8];

/**
  * @brief  LCD Init function.
  * @note   Select External Clock Source, Enable the display, clear internal memory.
  * @param  None.
  * @retval None.
  */
void lcd_init(void){
	/*Reset configuration Display Pins*/
	HAL_GPIO_WritePin(EXTMODE_GPIO_Port, EXTMODE_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(EXTCOMIN_GPIO_Port, EXTCOMIN_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(DISP_DISPLAY_GPIO_Port, DISP_DISPLAY_Pin, GPIO_PIN_RESET);

	lcd_clear();
	HAL_GPIO_WritePin(EXTMODE_GPIO_Port, EXTMODE_Pin, GPIO_PIN_SET);				//Extern Clock Source Selection
	//HAL_Delay(1000);
	HAL_GPIO_WritePin(DISP_DISPLAY_GPIO_Port, DISP_DISPLAY_Pin, GPIO_PIN_SET); 		//Enable Display
	HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_4);										//Start PWM 50Hz
	/*Configure Display Struct*/
	shm_128x128.CurrentX = 0;
	shm_128x128.CurrentY = 0;
	shm_128x128.Inverted = CU_FALSE;
	shm_128x128.Initialized = CU_TRUE;

	if(shm_128x128.Inverted == CU_FALSE){
		for(uint16_t idx_init = 0; idx_init < LCD_HEIGHT*LCD_WIDTH/8; idx_init ++ ){
			LCD_128x128_Buffer[idx_init] = 0xFF;									//bit set to 1 stands for white pixel, 0 is for Black pixel
		}
	}

}

/**
  * @brief  LCD Clear function.
  * @note   Send the Clear command to the display and initialise the LCD_128x128_Buffer.
  * @param  None.
  * @retval None.
  */
void lcd_clear(void){
	//uint8_t clear_data[2] = {SHARPMEM_BIT_CLEAR | vcom, 0x00};
	uint8_t clear_data[2] = {SHARPMEM_BIT_CLEAR , 0x00};

	HAL_GPIO_WritePin(CS_DISPLAY_GPIO_Port, CS_DISPLAY_Pin, GPIO_PIN_SET);
	HAL_SPI_Transmit(&hspi1, &clear_data[0], 1, HAL_MAX_DELAY);
	HAL_SPI_Transmit(&hspi1, &clear_data[1], 1, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(CS_DISPLAY_GPIO_Port, CS_DISPLAY_Pin, GPIO_PIN_RESET);

	if(shm_128x128.Inverted == CU_FALSE){
		for(uint16_t idx_init = 0; idx_init < LCD_HEIGHT*LCD_WIDTH/8; idx_init ++ ){
			LCD_128x128_Buffer[idx_init] = 0xFF;
		}
	}else{
		for(uint16_t idx_init = 0; idx_init < LCD_HEIGHT*LCD_WIDTH/8; idx_init ++ ){
			LCD_128x128_Buffer[idx_init] = 0x00;
		}
	}
	shm_128x128.CurrentX = 0;
	shm_128x128.CurrentY = 0;

}

/**
  * @brief  Draw a single pixel on LCD.
  * @note   Send the Clear command to the display and initialise the LCD_128x128_Buffer.
  * @param  uint8_t x => X position .
  * @param  uint8_t y => Y position .
  * @param  LCD_COLOR color => Color of the pixel, Black or White .
  * @retval None.
  */
void lcd_DrawPixel(uint8_t x, uint8_t y, LCD_COLOR color){
	uint16_t buff_addr = x/8 + (LCD_WIDTH/8)*y ;
	uint8_t bit_to_set = 1 << ((x % 8));

    if(x >= LCD_WIDTH || y >= LCD_HEIGHT) {	// Don't write outside the buffer
        return;
    }
    // Check if pixel should be inverted
    if(shm_128x128.Inverted) {
        color = (LCD_COLOR)!color;
    }
    // Draw in the right color
    if(color == White) {
    	LCD_128x128_Buffer[buff_addr] |= bit_to_set;
    } else {
    	LCD_128x128_Buffer[buff_addr] &= ~bit_to_set;
    }
}

/**
  * @brief  Write single character on LCD.
  * @note   Automatic new line and checking of remaining space.
  * @param  char ch => Character to write .
  * @param  FontDef Font => Font to use .
  * @param  LCD_COLOR color => Color, Black or White .
  * @retval Return written char for validation.
  */
char lcd_WriteChar(char ch, FontDef Font, LCD_COLOR color){
    uint32_t i, b, j;

    // Check if character is valid
    if (ch < 32 || ch > 126)
        return 0;
    // Check remaining space on current line
    if(LCD_WIDTH < (shm_128x128.CurrentX + Font.FontWidth)){
    	if(LCD_HEIGHT > (shm_128x128.CurrentY + 2*Font.FontHeight -2)){		// -2 => Margin of the character
    		shm_128x128.CurrentX = 0;
    		shm_128x128.CurrentY = shm_128x128.CurrentY + Font.FontHeight - 1;		//-1 => Margin of the character fort
    	}else{
    		return 0;
    	}
    }

    // Use the font to write
    for(i = 0; i < Font.FontHeight; i++) {
        b = Font.data[(ch - 32) * Font.FontHeight + i];
        for(j = 0; j < Font.FontWidth; j++) {
            if((b << j) & 0x8000)  {
            	lcd_DrawPixel(shm_128x128.CurrentX + j, (shm_128x128.CurrentY + i), (LCD_COLOR) color);
            } else {
            	lcd_DrawPixel(shm_128x128.CurrentX + j, (shm_128x128.CurrentY + i), (LCD_COLOR)!color);
            }
        }
    }
    // The current space is now taken
    shm_128x128.CurrentX += Font.FontWidth;

    // Return written char for validation
    return ch;
}


/**
  * @brief  LCD refresh - send the complete LCD_128x128_Buffer to internal memory display.
  * @note   Blocking function.
  * @param  None.
  * @retval None.
  */
void lcd_refresh(void){
	uint8_t cmd_data = SHARPMEM_BIT_WRITECMD;
	uint8_t adr_data = 0x01;
	uint8_t dmy_data = 0x00;

	HAL_GPIO_WritePin(CS_DISPLAY_GPIO_Port, CS_DISPLAY_Pin, GPIO_PIN_SET);
	HAL_SPI_Transmit(&hspi1, &cmd_data, 1, HAL_MAX_DELAY);
	HAL_SPI_Transmit(&hspi1, &adr_data, 1, HAL_MAX_DELAY);
	for(uint16_t s_data = 0; s_data < 16; s_data ++){
		HAL_SPI_Transmit(&hspi1, &LCD_128x128_Buffer[s_data], 1, HAL_MAX_DELAY);
	}
	for(uint16_t s_add = 2; s_add <= 128; s_add ++){
		adr_data = s_add;
		HAL_SPI_Transmit(&hspi1, &dmy_data, 1, HAL_MAX_DELAY);
		HAL_SPI_Transmit(&hspi1, &adr_data, 1, HAL_MAX_DELAY);
		for(uint16_t s_data = (16*(s_add-1)); s_data < ((16*(s_add-1))+16); s_data ++){
			HAL_SPI_Transmit(&hspi1, &LCD_128x128_Buffer[s_data], 1, HAL_MAX_DELAY);
		}
	}
	HAL_SPI_Transmit(&hspi1, &dmy_data, 1, HAL_MAX_DELAY);
	HAL_SPI_Transmit(&hspi1, &dmy_data, 1, HAL_MAX_DELAY);

	HAL_GPIO_WritePin(CS_DISPLAY_GPIO_Port, CS_DISPLAY_Pin, GPIO_PIN_RESET);
}

