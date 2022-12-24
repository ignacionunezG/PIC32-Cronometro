/**
 * @file     TftDriver.c
 *
 * @author   José Daniel Muñoz Frías
 *
 * @version  1.0.0. Versión inicial
 *
 * @date     16/11/2016
 *
 * @brief    Módulo encargado de gestionar el shield TFT1.8SP de 
 *           elecfreaks (ref. EF02005). El shield está basado en un display con
 *           el controlador ST7735S. 
 *           Este driver está basado en el UTFT driver, aunque se ha portado a C
 *           y se han eliminado el soporte para el resto de TFTs que incluía el
 *           driver original.
 */

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include "../TftDriver.h"
#include "spi.h"

// Documentación interna (Sólo para desarrolladores del driver)
/// @cond INTERNAL
// Constantes

// Pines del conector arduino a los que se conecta el display del shield 
// TFT1.8SP de Elecfreaks. Para documentación solamente
//#define PIN_SCK 10
//#define PIN_SDO 11 
//#define PIN_SS  9
//#define PIN_CMD_DATO 7 // 0 comando, 1 dato.
//#define PIN_RST 6 // Reset del display
// Pines no SPI del TFT
#define PIN_CMD_DAT 6 // Puerto C (RC6), que está conectado al pin 4 del arduino
#define PIN_RST  5    // Puerto C (RC5), que está conectado al pin 3 del arduino

// Macros
#define swap(type, i, j) {type t = i; i = j; j = t;}

/// @cond INTERNAL
// Funciones privadas
void Retardo(unsigned int ms);
void setXY(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void clrXY(void);
void drawHLine(int x, int y, int l);
void drawVLine(int x, int y, int l);
void setPixel(uint16_t color);
void printChar(uint8_t c, int x, int y);
void rotateChar(uint8_t c, int x, int y, int pos, int deg);
void LCD_Write_COM(uint8_t cmd);
void LCD_Write_DATA(uint8_t data);

// Variables globales privadas al módulo
static int _orientacion; // PORTRAIT o LANDSCAPE

static uint8_t fch, fcl, bch, bcl; // Foreground color high/low, background color high/low

struct _current_font {
    uint8_t *font;
    uint8_t x_size;
    uint8_t y_size;
    uint8_t offset;
    uint8_t numchars;
} cfont;

static bool _transparent;
/// @endcond

/**
 * Inicializa el display TFT. Seleccciona negro para el fondo y blanco para
 * escribir.
 * 
 * @param orientacion Orientación del display: LANDSCAPE o PORTRAIT
 */
void inicializarTFT(int orientacion) {
    // En primer lugar se inicializa el SPI2, que es el que está conectado a la
    // tarjeta del display. Se usa una frecuencia de 1 MHz.
    InicializarSPI2(1000000);

    // Configuramos los pines cmd_dato y rst como salidas y como pines digitales
    TRISC &= ~((1 << PIN_CMD_DAT) | (1 << PIN_RST));
    ANSELC &= ~((1 << PIN_CMD_DAT) | (1 << PIN_RST));

    // guardo la orientación en una variable global, pues es usada por otras
    // funciones del módulo
    _orientacion = orientacion;

    // Damos un reset al display
    PORTCSET = 1 << PIN_RST;
    Retardo(5);
    PORTCCLR = 1 << PIN_RST;
    Retardo(15);
    PORTCSET = 1 << PIN_RST;
    Retardo(15);

    //Ahora inicializamos el display

    LCD_Write_COM(0x11); //Sleep exit 
    Retardo(12);

    //ST7735R Frame Rate
    LCD_Write_COM(0xB1);
    LCD_Write_DATA(0x05);
    LCD_Write_DATA(0x3C);
    LCD_Write_DATA(0x3C);
    LCD_Write_COM(0xB2);
    LCD_Write_DATA(0x05);
    LCD_Write_DATA(0x3C);
    LCD_Write_DATA(0x3C);
    LCD_Write_COM(0xB3);
    LCD_Write_DATA(0x05);
    LCD_Write_DATA(0x3C);
    LCD_Write_DATA(0x3C);
    LCD_Write_DATA(0x05);
    LCD_Write_DATA(0x3C);
    LCD_Write_DATA(0x3C);

    LCD_Write_COM(0xB4); //Column inversion 
    LCD_Write_DATA(0x03);

    //ST7735R Power Sequence
    LCD_Write_COM(0xC0);
    LCD_Write_DATA(0x28);
    LCD_Write_DATA(0x08);
    LCD_Write_DATA(0x04);
    LCD_Write_COM(0xC1);
    LCD_Write_DATA(0xC0);
    LCD_Write_COM(0xC2);
    LCD_Write_DATA(0x0D);
    LCD_Write_DATA(0x00);
    LCD_Write_COM(0xC3);
    LCD_Write_DATA(0x8D);
    LCD_Write_DATA(0x2A);
    LCD_Write_COM(0xC4);
    LCD_Write_DATA(0x8D);
    LCD_Write_DATA(0xEE);

    LCD_Write_COM(0xC5); //VCOM 
    LCD_Write_DATA(0x1A);

    LCD_Write_COM(0x36); //MX, MY, RGB mode 
    LCD_Write_DATA(0xC0);

    //ST7735R Gamma Sequence
    LCD_Write_COM(0xE0);
    LCD_Write_DATA(0x03);
    LCD_Write_DATA(0x22);
    LCD_Write_DATA(0x07);
    LCD_Write_DATA(0x0A);
    LCD_Write_DATA(0x2E);
    LCD_Write_DATA(0x30);
    LCD_Write_DATA(0x25);
    LCD_Write_DATA(0x2A);
    LCD_Write_DATA(0x28);
    LCD_Write_DATA(0x26);
    LCD_Write_DATA(0x2E);
    LCD_Write_DATA(0x3A);
    LCD_Write_DATA(0x00);
    LCD_Write_DATA(0x01);
    LCD_Write_DATA(0x03);
    LCD_Write_DATA(0x13);
    LCD_Write_COM(0xE1);
    LCD_Write_DATA(0x04);
    LCD_Write_DATA(0x16);
    LCD_Write_DATA(0x06);
    LCD_Write_DATA(0x0D);
    LCD_Write_DATA(0x2D);
    LCD_Write_DATA(0x26);
    LCD_Write_DATA(0x23);
    LCD_Write_DATA(0x27);
    LCD_Write_DATA(0x27);
    LCD_Write_DATA(0x25);
    LCD_Write_DATA(0x2D);
    LCD_Write_DATA(0x3B);
    LCD_Write_DATA(0x00);
    LCD_Write_DATA(0x01);
    LCD_Write_DATA(0x04);
    LCD_Write_DATA(0x13);

    //LCD_Write_COM(0x2A);
    //LCD_Write_DATA(0x00);
    //LCD_Write_DATA(0x00);
    //LCD_Write_DATA(0x00);
    //LCD_Write_DATA(0x7F);
    //LCD_Write_COM(0x2B);
    //LCD_Write_DATA(0x00);
    //LCD_Write_DATA(0x00);
    //LCD_Write_DATA(0x00);
    //LCD_Write_DATA(0x9F);

    LCD_Write_COM(0x3A); //65k mode 
    LCD_Write_DATA(0x05);
    LCD_Write_COM(0x29); //Display on	

    setColorRGB(255, 255, 255);
    setBackColorRGB(0, 0, 0);
    cfont.font = 0; // OJO Esto es un puntero. Supongo que lo inicializa para dar algún tipo de error si se intenta usar sin inicializar.
    _transparent = false;
}

/**
 * Borra la pantalla poniendola toda a color negro.
 */
void clrScr(void) {
    long i;

    clrXY();
    for (i = 0; i < ((DISP_X_SIZE + 1)*(DISP_Y_SIZE + 1)); i++) {
        LCD_Write_DATA(0);
        LCD_Write_DATA(0);
    }
}

/**
 * Selecciona el color para escribir. Todas las funciones que dibujan en el
 * display (drawXX, fillXX y print) usan este color.
 * 
 * @param r Porcentaje de color rojo  (0-255). Se usan los 5 bits más significativos.
 * @param g Porcentaje de color verde (0-255). Se usan los 6 bits más significativos.
 * @param b Porcentaje de color azul  (0-255). Se usan los 5 bits más significativos.
 */
void setColorRGB(uint8_t r, uint8_t g, uint8_t b) {
    fch = ((r & 248) | g >> 5);
    fcl = ((g & 28) << 3 | b >> 3);
}

/**
 * Selecciona el color para escribir. Todas las funciones que dibujan en el
 * display (draw, fill y print) usan este color. El color se define mediante un
 * uint16_t con el formato RGB565. Se pueden usar constantes predefinidas con
 * los colores usados por las tarjetas VGA: VGA_WHITE, VGA_RED, etc. (ver TftDriver.h).
 * 
 * @param color Color en formato RGB565 (5 bits rojo, 6 bits verde, 5 bits azul).
 */
void setColor(uint16_t color) {
    fch = (uint8_t) (color >> 8);
    fcl = (uint8_t) (color & 0xFF);
}

/**
 * Selecciona el color de fondo del display.
 * 
 * @param r Porcentaje de color rojo  (0-255). Se usan los 5 bits más significativos.
 * @param g Porcentaje de color verde (0-255). Se usan los 6 bits más significativos.
 * @param b Porcentaje de color azul  (0-255). Se usan los 5 bits más significativos.
 */
void setBackColorRGB(uint8_t r, uint8_t g, uint8_t b) {
    bch = ((r & 248) | g >> 5);
    bcl = ((g & 28) << 3 | b >> 3);
    _transparent = false;
}

/**
 * Selecciona el color de fondo usado para imprmir los caracteres. El argumento
 * es el color en el formato RGB565. Se pueden usar constantes predefinidas con
 * los colores usados por las tarjetas VGA: VGA_WHITE, VGA_RED, etc. (ver TftDriver.h).
 * Además se puede seleccionar el color VGA_TRANSPARENT para que el fondo de los
 * caracteres sea transparente.
 * 
 * @param color Color en formato RGB565 (5 bits rojo, 6 bits verde, 5 bits azul).
 *              Usar 0xFFFFFFFF (VGA_TRANSPARENT) para que el fondo de los
 *              caracteres sea transparente.
 */
void setBackColor(uint32_t color) {
    if (color == VGA_TRANSPARENT)
        _transparent = true;
    else {
        bch = (uint8_t) (color >> 8);
        bcl = (uint8_t) (color & 0xFF);
        _transparent = false;
    }
}

/**
 * Dibuja un rectangulo definido por sus dos esquinas.
 * 
 * @param x1 Coordenada X de la primera esquina.
 * @param y1 Coordenada Y de la primera esquina.
 * @param x2 Coordenada X de la segunda esquina.
 * @param y2 Coordenada Y de la segunda esquina.
 */
void drawRect(int x1, int y1, int x2, int y2) {
    if (x1 > x2) {
        swap(int, x1, x2);
    }
    if (y1 > y2) {
        swap(int, y1, y2);
    }

    drawHLine(x1, y1, x2 - x1);
    drawHLine(x1, y2, x2 - x1);
    drawVLine(x1, y1, y2 - y1);
    drawVLine(x2, y1, y2 - y1);
}

/**
 * Dibuja un rectangulo con los bordes redondeados definido por sus dos esquinas.
 * 
 * @param x1 Coordenada X de la primera esquina.
 * @param y1 Coordenada Y de la primera esquina.
 * @param x2 Coordenada X de la segunda esquina.
 * @param y2 Coordenada Y de la segunda esquina.
 */

void drawRoundRect(int x1, int y1, int x2, int y2) {
    if (x1 > x2) {
        swap(int, x1, x2);
    }
    if (y1 > y2) {
        swap(int, y1, y2);
    }
    if ((x2 - x1) > 4 && (y2 - y1) > 4) {
        drawPixel(x1 + 1, y1 + 1);
        drawPixel(x2 - 1, y1 + 1);
        drawPixel(x1 + 1, y2 - 1);
        drawPixel(x2 - 1, y2 - 1);
        drawHLine(x1 + 2, y1, x2 - x1 - 4);
        drawHLine(x1 + 2, y2, x2 - x1 - 4);
        drawVLine(x1, y1 + 2, y2 - y1 - 4);
        drawVLine(x2, y1 + 2, y2 - y1 - 4);
    }
}

/**
 * Dibuja un rectangulo relleno definido por sus dos esquinas.
 * 
 * @param x1 Coordenada X de la primera esquina.
 * @param y1 Coordenada Y de la primera esquina.
 * @param x2 Coordenada X de la segunda esquina.
 * @param y2 Coordenada Y de la segunda esquina.
 */
void fillRect(int x1, int y1, int x2, int y2) {
    int i;

    if (x1 > x2) {
        swap(int, x1, x2);
    }
    if (y1 > y2) {
        swap(int, y1, y2);
    }

    if (_orientacion == PORTRAIT) {
        for (i = 0; i < ((y2 - y1) / 2) + 1; i++) {
            drawHLine(x1, y1 + i, x2 - x1);
            drawHLine(x1, y2 - i, x2 - x1);
        }
    } else {
        for (i = 0; i < ((x2 - x1) / 2) + 1; i++) {
            drawVLine(x1 + i, y1, y2 - y1);
            drawVLine(x2 - i, y1, y2 - y1);
        }
    }
}

/**
 * Dibuja un rectangulo relleno con los bordes redondeados definido por sus dos esquinas.
 * 
 * @param x1 Coordenada X de la primera esquina.
 * @param y1 Coordenada Y de la primera esquina.
 * @param x2 Coordenada X de la segunda esquina.
 * @param y2 Coordenada Y de la segunda esquina.
 */
void fillRoundRect(int x1, int y1, int x2, int y2) {
    int i;

    if (x1 > x2) {
        swap(int, x1, x2);
    }
    if (y1 > y2) {
        swap(int, y1, y2);
    }

    if ((x2 - x1) > 4 && (y2 - y1) > 4) {
        for (i = 0; i < ((y2 - y1) / 2) + 1; i++) {
            switch (i) {
                case 0:
                    drawHLine(x1 + 2, y1 + i, x2 - x1 - 4);
                    drawHLine(x1 + 2, y2 - i, x2 - x1 - 4);
                    break;
                case 1:
                    drawHLine(x1 + 1, y1 + i, x2 - x1 - 2);
                    drawHLine(x1 + 1, y2 - i, x2 - x1 - 2);
                    break;
                default:
                    drawHLine(x1, y1 + i, x2 - x1);
                    drawHLine(x1, y2 - i, x2 - x1);
            }
        }
    }
}

/**
 * Dibuja un círculo definido por su centro y su radio.
 * 
 * @param x Coordenada X del centro.
 * @param y Coordenada Y del dentro.
 * @param radius Radio del círculo.
 */
void drawCircle(int x, int y, int radius) {
    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    int x1 = 0;
    int y1 = radius;

    setXY(x, y + radius, x, y + radius);
    LCD_Write_DATA(fch);
    LCD_Write_DATA(fcl);
    setXY(x, y - radius, x, y - radius);
    LCD_Write_DATA(fch);
    LCD_Write_DATA(fcl);
    setXY(x + radius, y, x + radius, y);
    LCD_Write_DATA(fch);
    LCD_Write_DATA(fcl);
    setXY(x - radius, y, x - radius, y);
    LCD_Write_DATA(fch);
    LCD_Write_DATA(fcl);

    while (x1 < y1) {
        if (f >= 0) {
            y1--;
            ddF_y += 2;
            f += ddF_y;
        }
        x1++;
        ddF_x += 2;
        f += ddF_x;
        setXY(x + x1, y + y1, x + x1, y + y1);
        LCD_Write_DATA(fch);
        LCD_Write_DATA(fcl);
        setXY(x - x1, y + y1, x - x1, y + y1);
        LCD_Write_DATA(fch);
        LCD_Write_DATA(fcl);
        setXY(x + x1, y - y1, x + x1, y - y1);
        LCD_Write_DATA(fch);
        LCD_Write_DATA(fcl);
        setXY(x - x1, y - y1, x - x1, y - y1);
        LCD_Write_DATA(fch);
        LCD_Write_DATA(fcl);
        setXY(x + y1, y + x1, x + y1, y + x1);
        LCD_Write_DATA(fch);
        LCD_Write_DATA(fcl);
        setXY(x - y1, y + x1, x - y1, y + x1);
        LCD_Write_DATA(fch);
        LCD_Write_DATA(fcl);
        setXY(x + y1, y - x1, x + y1, y - x1);
        LCD_Write_DATA(fch);
        LCD_Write_DATA(fcl);
        setXY(x - y1, y - x1, x - y1, y - x1);
        LCD_Write_DATA(fch);
        LCD_Write_DATA(fcl);
    }
    clrXY();
}

/**
 * Dibuja un círculo relleno definido por su centro y su radio.
 * 
 * @param x Coordenada X del centro.
 * @param y Coordenada Y del dentro.
 * @param radius Radio del círculo.
 */
void fillCircle(int x, int y, int radius) {
    int x1, y1;

    for (y1 = -radius; y1 <= 0; y1++) {
        for (x1 = -radius; x1 <= 0; x1++) {
            if (x1 * x1 + y1 * y1 <= radius * radius) {
                drawHLine(x + x1, y + y1, 2 * (-x1));
                drawHLine(x + x1, y - y1, 2 * (-x1));
                break;
            }
        }
    }
}

/**
 * Rellena la pantalla con un color definido mediante sus componentes RGB.
 * 
 * @param r Porcentaje de color rojo  (0-255).
 * @param g Porcentaje de color verde (0-255).
 * @param b Porcentaje de color azul  (0-255).
 */
void fillScrRGB(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t color = ((r & 248) << 8 | (g & 252) << 3 | (b & 248) >> 3);
    fillScr(color);
}

/**
 * Rellena la pantalla con un color definido mediante un valor de 16 bits con
 * el formato RGB565 
 * 
 * @param color Color en formato RGB565 (5 bits rojo, 6 bits verde, 5 bits azul).
 */
void fillScr(uint16_t color) {
    long i;
    char ch, cl;

    ch = (uint8_t) (color >> 8);
    cl = (uint8_t) (color & 0xFF);

    clrXY();
    for (i = 0; i < ((DISP_X_SIZE + 1)*(DISP_Y_SIZE + 1)); i++) {
        LCD_Write_DATA(ch);
        LCD_Write_DATA(cl);
    }
}

/**
 * Dibuja una línea entre los puntos (x1,y1) y (x2,y2)
 * 
 * @param x1 Coordenada X del punto 1.
 * @param y1 Coordenada Y del punto 1.
 * @param x2 Coordenada X del punto 2.
 * @param y2 Coordenada Y del punto 2.
 */
void drawLine(int x1, int y1, int x2, int y2) {
    if (y1 == y2)
        drawHLine(x1, y1, x2 - x1);
    else if (x1 == x2)
        drawVLine(x1, y1, y2 - y1);
    else {
        unsigned int dx = (x2 > x1 ? x2 - x1 : x1 - x2);
        short xstep = x2 > x1 ? 1 : -1;
        unsigned int dy = (y2 > y1 ? y2 - y1 : y1 - y2);
        short ystep = y2 > y1 ? 1 : -1;
        int col = x1, row = y1;

        if (dx < dy) {
            int t = -(dy >> 1);
            while (true) {
                setXY(col, row, col, row);
                LCD_Write_DATA(fch);
                LCD_Write_DATA(fcl);
                if (row == y2)
                    return;
                row += ystep;
                t += dx;
                if (t >= 0) {
                    col += xstep;
                    t -= dy;
                }
            }
        } else {
            int t = -(dx >> 1);
            while (true) {
                setXY(col, row, col, row);
                LCD_Write_DATA(fch);
                LCD_Write_DATA(fcl);
                if (col == x2)
                    return;
                col += xstep;
                t += dy;
                if (t >= 0) {
                    row += ystep;
                    t -= dx;
                }
            }
        }
    }
    clrXY();
}

/**
 * Dibuja un pixel en las coordenadas (x,y).
 * 
 * @param x Coordenada X del pixel.
 * @param y Coordenada Y del pixel.
 */
void drawPixel(int x, int y) {
    setXY(x, y, x, y);
    setPixel((fch << 8) | fcl);
    clrXY();
}

/**
 * Selecciona la fuente a usar por las funciones de texto. Por ejemplo, para
 * seleccionar la fuente SmallFont definida en DefaultFonts.c basta con hacer:
 * 
 * @code
 * extern uint8_t SmallFont[];
 * ...
 * SetFont(SmallFont[];
 * @endcode
 * 
 * @param font Nombre del vector que contiene la definición de la fuente. 
 * 
 */
void setFont(uint8_t* font) {
    cfont.font = font;
    cfont.x_size = font[0];
    cfont.y_size = font[1];
    cfont.offset = font[2];
    cfont.numchars = font[3];
}

/**
 * Obtiene la dirección del vector de la fuente usada.
 * 
 * @return Dirección de la fuente usada.
 */
uint8_t* getFont(void) {
    return cfont.font;
}

/**
 * Retorna el ancho (en pixels) de la fuente en uso.
 * 
 * @return Ancho en pixels de la fuente en uso.
 */
uint8_t getFontXsize(void) {
    return cfont.x_size;
}

/**
 * Retorna el alto (en pixels) de la fuente en uso.
 * 
 * @return Alto en pixels de la fuente en uso.
 */
uint8_t getFontYsize(void) {
    return cfont.y_size;
}

/**
 * Imprime una cadena de caracteres a partir de las coordenadas (x,y) con un
 * ángulo determinado. Las coordenadas definen la esquina superior izquierda del
 * primer carácter de la cadena.
 * 
 * @param st Cadena de caracteres.
 * @param x  Coordenada X de la esquena superior izquierda del primer carácter.
 * @param y  Coordenada Y de la esquena superior izquierda del primer carácter.
 * @param deg Ángulo con el que se imprime la cadena (0 horizontal, 90 vertical, etc.)
 */
void print(char *st, int x, int y, int deg) {
    int stl, i;

    stl = strlen(st);

    if (_orientacion == PORTRAIT) {
        if (x == RIGHT)
            x = (DISP_X_SIZE + 1)-(stl * cfont.x_size);
        if (x == CENTER)
            x = ((DISP_X_SIZE + 1)-(stl * cfont.x_size)) / 2;
    } else {
        if (x == RIGHT)
            x = (DISP_Y_SIZE + 1)-(stl * cfont.x_size);
        if (x == CENTER)
            x = ((DISP_Y_SIZE + 1)-(stl * cfont.x_size)) / 2;
    }

    for (i = 0; i < stl; i++)
        if (deg == 0)
            printChar(*st++, x + (i * (cfont.x_size)), y);
        else
            rotateChar(*st++, x, y, i, deg);
}

/**
 * Dibuja una imagen (bitmap) en la pantalla. El bitmap ha de estar definido 
 * como un vector con los colores de todos los píxeles en formato RGB565.
 * La forma más fácil de generarlo es mediante una aplicación web disponible
 * en la página del autor del driver original:
 *   http://www.rinkydinkelectronics.com/t_imageconverter565.php
 * 
 * @param x Coordenada X de la esquina superior derecha del bitmap.
 * @param y Coordenada Y de la esquina superior derecha del bitmap.
 * @param sx Tamaño horizontal del bitmap en pixels.
 * @param sy Tamaño vertical del bitmap en pixels.
 * @param data Dirección del vector que contiene el bitmap.
 * @param scale Factor de escala para dibujar el bitmap.
 */
void drawBitmap(int x, int y, int sx, int sy, uint16_t data[], int scale) {
    unsigned int col;
    int tx, ty, tc, tsx, tsy;

    if (scale == 1) {
        if (_orientacion == PORTRAIT) {
            setXY(x, y, x + sx - 1, y + sy - 1);
            for (tc = 0; tc < (sx * sy); tc++) {
                col = data[tc];
                LCD_Write_DATA(col >> 8);
                LCD_Write_DATA(col & 0xff);
            }
        } else {
            for (ty = 0; ty < sy; ty++) {
                setXY(x, y + ty, x + sx - 1, y + ty);
                for (tx = sx - 1; tx >= 0; tx--) {
                    col = data[(ty * sx) + tx];
                    LCD_Write_DATA(col >> 8);
                    LCD_Write_DATA(col & 0xff);
                }
            }
        }
    } else {
        if (_orientacion == PORTRAIT) {
            for (ty = 0; ty < sy; ty++) {
                setXY(x, y + (ty * scale), x + ((sx * scale) - 1), y + (ty * scale) + scale);
                for (tsy = 0; tsy < scale; tsy++)
                    for (tx = 0; tx < sx; tx++) {
                        col = data[(ty * sx) + tx];
                        for (tsx = 0; tsx < scale; tsx++)
                            LCD_Write_DATA(col >> 8);
                        LCD_Write_DATA(col & 0xff);
                    }
            }

        } else {

            for (ty = 0; ty < sy; ty++) {
                for (tsy = 0; tsy < scale; tsy++) {
                    setXY(x, y + (ty * scale) + tsy, x + ((sx * scale) - 1), y + (ty * scale) + tsy);
                    for (tx = sx - 1; tx >= 0; tx--) {
                        col = data[(ty * sx) + tx];
                        for (tsx = 0; tsx < scale; tsx++)
                            LCD_Write_DATA(col >> 8);
                        LCD_Write_DATA(col & 0xff);
                    }
                }
            }

        }
    }
    clrXY();
}

/***************************************************************************/
/************************** Funciones privadas *****************************/
/***************************************************************************/
/// @cond INTERNAL

/**
 * Genera un retardo de n milisegundos usando el timer 1 en modo polling. La 
 * función guarda el estado del timer 1 para perturbar lo menos posible.
 *
 * @param n_ms Número de milisegundos a esperar
 *
 */

void Retardo(unsigned int n_ms) {
    int pr1_bak, t1con_bak;

    // Guardo el estado del timer 1.
    pr1_bak = PR1;
    t1con_bak = T1CON;

    // Inicializo el timer
    TMR1 = 0;
    PR1 = 5000; // Timer a 1 milisegundo. Como PBCLK son 5 MHz, hay que
    //contar 5000
    IFS0bits.T1IF = 0; // Borra el flag
    T1CON = 0x8000; // Timer on, Prescaler = 0 while(IFS0bits.T1IF == 0)
    while (n_ms != 0) {
        while (IFS0bits.T1IF == 0)
            ; // Espera fin del timer
        IFS0bits.T1IF = 0; // Borra el flag
        n_ms--;
    }
    // Dejamos el timer 1 como estaba
    T1CON = t1con_bak;
    PR1 = pr1_bak;
}

/**
 * Selecciona el marco de la memoria gráfica del display en la que se van a
 * escribir datos. Para ello envía el comando 0x2A (Column Address Set) que
 * selecciona entre qué dos columnas se va a escribir. Dicho comando recibe
 * como argumantos las coordenadas x del marco. A continuación se envía el
 * comando 0x2B (Row Address Set) para seleccionar entre qué dos filas se va a
 * escribir. El comando recibe como argumentos las dos coordenadas y del marco.
 * Por último se envía el comando 0x2C (Memory Write) para enviar a la memoria
 * los datos a escribir en el marco. Dichos datos los enviará otra función.
 * 
 * @param x1 Coordenada x del primer punto del marco.
 * @param y1 Coordenada y del primer punto del marco.
 * @param x2 Coordenada x del segundo punto del marco.
 * @param y2 Coordenada y del segundo punto del marco.
 */
void setXY(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    if (_orientacion == LANDSCAPE) {
        swap(uint16_t, x1, y1);
        swap(uint16_t, x2, y2)
        y1 = DISP_Y_SIZE - y1;
        y2 = DISP_Y_SIZE - y2;
        swap(uint16_t, y1, y2)
    }
    LCD_Write_COM(0x2a);
    LCD_Write_DATA(x1 >> 8);
    LCD_Write_DATA(x1);
    LCD_Write_DATA(x2 >> 8);
    LCD_Write_DATA(x2);
    LCD_Write_COM(0x2b);
    LCD_Write_DATA(y1 >> 8);
    LCD_Write_DATA(y1);
    LCD_Write_DATA(y2 >> 8);
    LCD_Write_DATA(y2);
    LCD_Write_COM(0x2c);
}

/**
 * Selecciona toda la pantalla como marco para escribir en la memoria gráfica.
 */
void clrXY(void) {
    if (_orientacion == PORTRAIT)
        setXY(0, 0, DISP_X_SIZE, DISP_Y_SIZE);
    else
        setXY(0, 0, DISP_Y_SIZE, DISP_X_SIZE);
}

void clrActual(void) {
    if (_orientacion == PORTRAIT)
        setXY(60, 0, 150, 70);
    else
        setXY(0, 0, 70, 150);
}

/**
 * Dibuja una línea horizontal desde el punto (x,y) con la longitud l.
 * 
 * @param x Coordenada X del orígen de la línea.
 * @param y Coordenada Y del orígen de la línea.
 * @param l Longitud de la línea.
 */
void drawHLine(int x, int y, int l) {
    int i;
    uint16_t color = ((0 & 248) << 8 | (0 & 252) << 3 | (0 & 248) >> 3);
    
    //long i;
    char ch, cl;

    ch = (uint8_t) (color >> 8);
    cl = (uint8_t) (color & 0xFF);
    
    
    if (l < 0) {
        l = -l;
        x -= l;
    }

    setXY(x, y, x + l, y);
    for (i = 0; i < l + 1; i++) {
        LCD_Write_DATA(ch);
        LCD_Write_DATA(cl);
    }
    clrXY();
}

/**
 * Dibuja una línea vertical desde el punto (x,y) con la longitud l.
 * 
 * @param x Coordenada X del orígen de la línea.
 * @param y Coordenada Y del orígen de la línea.
 * @param l Longitud de la línea.
 */

void drawVLine(int x, int y, int l) {
    int i;
    uint16_t color = ((0 & 248) << 8 | (0 & 252) << 3 | (0 & 248) >> 3);
    char ch, cl;

    ch = (uint8_t) (color >> 8);
    cl = (uint8_t) (color & 0xFF);
    
   
    if (l < 0) {
        l = -l;
        y -= l;
    }

    setXY(x, y, x, y + l);
    for (i = 0; i < l + 1; i++) {
        LCD_Write_DATA(ch);
        LCD_Write_DATA(cl);
    }
    clrXY();
}

/**
 * Escribe el color de un pixel en la memoria gráfica. La dirección ha de haber
 * sido previamente establecida mediante una llamada a setXY().
 * 
 * @param color Color del pixel.
 */
void setPixel(uint16_t color) {
    // color viene en el formato rrrrrggggggbbbbb
    LCD_Write_DATA(color >> 8); // Se envían primero los 8 bits más significativos
    LCD_Write_DATA(color & 0xFF); // Y luego los menos significativos
}

/**
 * Imprime un caracter en las coordenadas (x,y). Las coordenadas definen la 
 * esquina superior izquierda del caracter a imprimir.
 * 
 * @param c Caracter a imprimir
 * @param x Coordenada X
 * @param y Coordenada Y
 */

void printChar(uint8_t c, int x, int y) {
    uint8_t i, ch;
    uint16_t j;
    uint16_t temp;
    int zz;

    if (!_transparent) {
        if (_orientacion == PORTRAIT) {
            setXY(x, y, x + cfont.x_size - 1, y + cfont.y_size - 1);

            temp = ((c - cfont.offset)*((cfont.x_size / 8) * cfont.y_size)) + 4;
            for (j = 0; j < ((cfont.x_size / 8) * cfont.y_size); j++) {
                ch = cfont.font[temp];
                for (i = 0; i < 8; i++) {
                    if ((ch & (1 << (7 - i))) != 0) {
                        setPixel((fch << 8) | fcl);
                    } else {
                        setPixel((bch << 8) | bcl);
                    }
                }
                temp++;
            }
        } else {
            temp = ((c - cfont.offset)*((cfont.x_size / 8) * cfont.y_size)) + 4;

            for (j = 0; j < ((cfont.x_size / 8) * cfont.y_size); j += (cfont.x_size / 8)) {
                setXY(x, y + (j / (cfont.x_size / 8)), x + cfont.x_size - 1, y + (j / (cfont.x_size / 8)));
                for (zz = (cfont.x_size / 8) - 1; zz >= 0; zz--) {
                    ch = cfont.font[temp + zz];
                    for (i = 0; i < 8; i++) {
                        if ((ch & (1 << i)) != 0) {
                            setPixel((fch << 8) | fcl);
                        } else {
                            setPixel((bch << 8) | bcl);
                        }
                    }
                }
                temp += (cfont.x_size / 8);
            }
        }
    } else {
        temp = ((c - cfont.offset)*((cfont.x_size / 8) * cfont.y_size)) + 4;
        for (j = 0; j < cfont.y_size; j++) {
            for (zz = 0; zz < (cfont.x_size / 8); zz++) {
                ch = cfont.font[temp + zz];
                for (i = 0; i < 8; i++) {
                    if ((ch & (1 << (7 - i))) != 0) {
                        setXY(x + i + (zz * 8), y + j, x + i + (zz * 8) + 1, y + j + 1);
                        setPixel((fch << 8) | fcl);
                    }
                }
            }
            temp += (cfont.x_size / 8);
        }
    }

    clrXY();
}

/**
 * Dibuja un caracter con la rotación dada por deg.
 * 
 * @param c
 * @param x
 * @param y
 * @param pos
 * @param deg
 */
void rotateChar(uint8_t c, int x, int y, int pos, int deg) {
    uint8_t i, j, ch;
    uint16_t temp;
    int newx, newy;
    double radian;
    int zz;

    radian = deg * 0.0175;

    temp = ((c - cfont.offset)*((cfont.x_size / 8) * cfont.y_size)) + 4;
    for (j = 0; j < cfont.y_size; j++) {
        for (zz = 0; zz < (cfont.x_size / 8); zz++) {
            ch = cfont.font[temp + zz];
            for (i = 0; i < 8; i++) {
                newx = x + (((i + (zz * 8)+(pos * cfont.x_size)) * cos(radian))-((j) * sin(radian)));
                newy = y + (((j) * cos(radian))+((i + (zz * 8)+(pos * cfont.x_size)) * sin(radian)));

                setXY(newx, newy, newx + 1, newy + 1);

                if ((ch & (1 << (7 - i))) != 0) {
                    setPixel((fch << 8) | fcl);
                } else {
                    if (!_transparent)
                        setPixel((bch << 8) | bcl);
                }
            }
        }
        temp += (cfont.x_size / 8);
    }
    clrXY();
}

/**
 * Envía un comando al display
 * 
 * @param cmd Comando a enviar al display 
 */

void LCD_Write_COM(uint8_t cmd) {
    LATCCLR = 1 << PIN_CMD_DAT; // voy a enviar un comando
    SPI_SendFrame(cmd); // Lo envío
}

/**
 * Envía un dato al display
 * 
 * @param cmd Comando a enviar al display 
 */

void LCD_Write_DATA(uint8_t data) {
    LATCSET = (1 << PIN_CMD_DAT); // voy a enviar un dato
    SPI_SendFrame(data); // Lo envío
}
/// @endcond
