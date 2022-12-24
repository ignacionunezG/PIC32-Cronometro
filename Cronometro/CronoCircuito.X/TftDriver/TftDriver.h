/**
 * @file     TftDriver.h
 *
 * @author   José Daniel Muñoz Frías   (daniel)
 *
 * @version  1.0.0. Módulo original
 *
 * @date     17/11/2016
 *
 * @brief    Módulo encargado de gestionar el TFT1.8 del shield TFT1.8SP de 
 *           elecfreaks (ref. EF02005). El shield está basado en un display con
 *           el controlador ST7735S.
 * 
 *           Este driver está basado en el UTFT driver, aunque se ha portado a C
 *           y se han eliminado el soporte para el resto de TFTs que incluía el
 *           driver original.
 */
#ifndef TFT_DRIVER_H
#define TFT_DRIVER_H

#include <xc.h>
#include <stdint.h>

#define LANDSCAPE 1 // Orientación del display
#define PORTRAIT  0

#define LEFT 0      // Justificación de las cadenas de caracteres
#define RIGHT 9999
#define CENTER 9998

#define DISP_Y_SIZE 159 // La resolución vertical del display es 160 (de 0 a 159)
#define DISP_X_SIZE 127 // La resolución horizontal del display es 128 (de 0 a 127)

//*********************************
// COLORS
//*********************************
// VGA color palette
#define VGA_BLACK		0x0000
#define VGA_WHITE		0xFFFF
#define VGA_RED			0xF800
#define VGA_GREEN		0x0400
#define VGA_BLUE		0x001F
#define VGA_SILVER		0xC618
#define VGA_GRAY		0x8410
#define VGA_MAROON		0x8000
#define VGA_YELLOW		0xFFE0
#define VGA_OLIVE		0x8400
#define VGA_LIME		0x07E0
#define VGA_AQUA		0x07FF
#define VGA_TEAL		0x0410
#define VGA_NAVY		0x0010
#define VGA_FUCHSIA		0xF81F
#define VGA_PURPLE		0x8010
#define VGA_TRANSPARENT	0xFFFFFFFF

// Prototipos de funciones

void inicializarTFT(int orientacion);
void clrScr(void);
void setColorRGB(uint8_t r, uint8_t g, uint8_t b);
void setColor(uint16_t color);
void setBackColorRGB(uint8_t r, uint8_t g, uint8_t b);
void setBackColor(uint32_t color);
void drawRect(int x1, int y1, int x2, int y2);
void drawRoundRect(int x1, int y1, int x2, int y2);
void drawLine(int x1, int y1, int x2, int y2);
void drawPixel(int x, int y);
void setFont(uint8_t* font);
uint8_t* getFont(void);
uint8_t getFontXsize(void);
uint8_t getFontYsize(void);
void print(char *st, int x, int y, int deg);
void fillRect(int x1, int y1, int x2, int y2);
void fillRoundRect(int x1, int y1, int x2, int y2);
void drawCircle(int x, int y, int radius);
void fillCircle(int x, int y, int radius);
void fillScrRGB(uint8_t r, uint8_t g, uint8_t b);
void fillScr(uint16_t color);
void drawBitmap(int x, int y, int sx, int sy, uint16_t data[], int scale);
void clrActual(void);

#endif
