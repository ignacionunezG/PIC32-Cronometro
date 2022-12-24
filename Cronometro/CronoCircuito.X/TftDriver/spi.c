// ----------------------------------------------------------------------------------
// --------------------- MÓDULO SPI (Serial Peripheral Interface) ----------------------
// ----------------------------------------------------------------------------------
//
/**
 * @file     spi.c
 *
 * @author   José Daniel Muñoz Frías   (daniel)
 * @author   Carlos Rodriguez-Morcillo (carlosrg)
 * 
 * @version  1.0.0. Módulo original (probado) (carlosrg)
 * @version  1.0.1. Modificada la función de inicialización para mapear el
 *                  periférico a cualquier pin del micro. (daniel)
 *
 * @date     16/11/2016
 *
 * @brief    Módulo encargado de gestionar las comunicaciones con otros
 *           dispositivos a través del bus SPI.
 */

#include <xc.h>
#include <stdint.h>
#include "spi.h"
#include "../Pic32Ini.h"

#define PIN_SCK 15 // Puerto B (RB15), que está conectado al pin 10 del arduino
#define PIN_SDO  8 // Puerto A (RA8), que está conectado al pin 11 del arduino
/* Por desgracia, los pines a los que están conectados el reloj y el dato del SPI
 * del tft no son los estándar del arduino (estos están conectados a la tarjeta
 * SD disponible en la placa del display). Por tanto hay que cablear externamente
 * las salidas del reloj y de dato del SPI del micro a las entradas del TFT, que
 * están conectadas a los pines 6 y 7 del arduino, las cuales se corresponden
 * con los pines 8 (clk) y 9 (sdo) del puerto C. Para que no interfieran defino
 * también estos pines y los configuraré como entradas.
 */
#define PIN_SDO_TFT 9 // Puerto C (RC9), que está conectado al pin 7 del arduino
#define PIN_SCK_TFT 8 // Puerto C (RC8), que está conectado al pin 6 del arduino
#define PIN_SS   7    // Puerto C (RC7), que está conectado al pin 5 del arduino
#define PIN_CMD_DAT 6 // Puerto C (RC6), que está conectado al pin 4 del arduino
#define PIN_RST  5    // Puerto C (RC5), que está conectado al pin 3 del arduino

// --------------------------------------------------------------------------
// ------------------ PROTOTIPOS DE LAS FUNCIONES PRIVADAS ------------------
// --------------------------------------------------------------------------

int InicializaRelojSPI2(int frecuencia);

/**
 * Incializa el módulo SPI en modo maestro. La función mapea el pin 
 * SDO (MOSI) al pin RA8 (pin 4 del conector P7). Se usa el modo estándar
 * del buffer y se configura para funcionar sin interrupciones.
 * El pin SS se maneja a mano, pues el pin SS no está conectado en la
 * tarjeta PicTrainer32 al pin SS del arduino.
 *
 * @param frec frecuencia del reloj SCK del SPI.
 */
void InicializarSPI2(int frec) {
    char dato;

    // Configura los pines como salidas. Se incluyen los del SPI por si acaso.
    TRISA &= ~((1 << PIN_SS) | (1 << PIN_SDO));
    TRISB &= ~((1 << PIN_SCK));
    TRISC &= ~((1 << PIN_CMD_DAT) | (1 << PIN_RST));
    TRISC |= (1 << PIN_SCK_TFT) | (1 << PIN_SDO_TFT); // Los pines del TFT como entradas

    // También se configuran como digitales
    ANSELA &= ~((1 << PIN_SS) | (1 << PIN_SDO));
    ANSELB &= ~((1 << PIN_SCK));
    ANSELC &= ~((1 << PIN_CMD_DAT) | (1 << PIN_RST) | (1 << PIN_SCK_TFT) | (1 << PIN_SDO_TFT));

    LATBSET = 1 << PIN_SCK;
    LATASET = 1 << PIN_SDO;

    LATASET = 1 << PIN_SS; // Arrancamos con el SS desactivado

    // Se configura el pin SDO2 (el SCK2 está asignado a fuego)
    SYSKEY = 0xAA996655; // Se desbloquean los registros
    SYSKEY = 0x556699AA; // de configuración.
    RPA8R = 4; // SDO2 conectado a RPA8
    //SDI2R   = 7;          // SDI2 conectado a RC3 (no se usa)
    SYSKEY = 0x1CA11CA1; // Se vuelven a bloquear.

    // Primero se apaga el módulo y se deja en el estado de reset
    SPI2STAT = 0;
    SPI2CON = 0;
    SPI2CON2 = 0;
    dato = SPI2BUF; // Vacía el buffer de lectura por si acaso

    // Se configura la frecuencia del reloj SCK
    InicializaRelojSPI2(frec);

    // Se borra la bandera de interrupción
    //IEC1bits.SPI2TXIE = 0;
    IFS1bits.SPI2TXIF = 0;
    IFS1bits.SPI2RXIF = 0;

    //  SPI2CONbits.MSTEN = 1; // Habilito modo maestro
    //  SPI2CONbits.CKE = 1;   // CPHA = 0 (dato cambia en flanco de activo a inactivo)
    //  //SPI2CONbits.CKP = 0; // CPOL = 0 (reloj inactivo a nivel bajo)
    //  //SPI2CONbits.DISSDI = 1; // SDI no se usa y por tanto se inhabilita
    //  SPI2CONbits.ON = 1; // Arranco el módulo

    SPI2CON |= (1 << 15) | (1 << 8) | (1 << 5); //|(1<<4); // Si pongo DISSSI a 1 no funciona la espera por polling en el SPIRBE ¿?

    // Nota, el "framed mode" es un timo. Genera un pulso al principio de la transmisión, no un pulso
    // durante TODA la transmisión, que es lo que necesitan la mayoría de los dispositivos. Se lo podían
    // haber ahorrado.
    //  SPI2CONbits.FRMEN = 0;     // Framed SPI suppport deshabilitado


    SPI2STATbits.SPIROV = 0; // Se limpia flag de overflow por si acaso.


}

/** 
 * Envía una trama de 1 byte por el puerto SPI. Controla el PIN_SS para
 * seleccionar el dispositivo mientras dura la transmisión.
 * 
 * @param dato Dato enviado por SPI
 */
void SPI_SendFrame(uint8_t dato) {
    LATCCLR = 1 << PIN_SS; // Activa SS

    while (SPI2STATbits.SPITBF)
        ; // Se espera a que haya sitio en el buffer de transmisión.

    SPI2BUF = dato; // Se carga el dato a transmitir.

    while (SPI2STATbits.SPIRBE)
        //while(IFS1bits.SPI2RXIF == 0)
        ; // Se espera el fin de la transmisión

    dato = SPI2BUF; // Leo el dato, aunque no lo quiero para nada. Si no tengo error de overflow y no se para en el while anterior
    LATCSET = 1 << PIN_SS; // Desactiva SS para finalizar la trama.
}

// -----------------------------------------------------------------------------
// ---------------------------- FUNCIONES PRIVADAS -----------------------------
// -----------------------------------------------------------------------------

/** 
 * Inicializa el reloj del SPI2 para que genere la frecuencia dada por su
 * parámetro
 *
 * @param frecuencia Frencuencia del reloj en Hz
 * @return 0 si todo OK, -1 si no se puede generar la frecuencia solicitada
 */

int InicializaRelojSPI2(int frecuencia) {
    int baud_reg;

    if (frecuencia > SYSCLK / 2) {
        return -1;
    } else if (frecuencia > PBCLK / 2) {
        // Se usa el reloj del sistema
        SPI2CONbits.MCLKSEL = 1;
        baud_reg = SYSCLK / 2 / frecuencia - 1;
        if (baud_reg >= 8192) { // Frecuencia demasiado baja
            return -1;
        }
    } else { // Se usa el reloj de periféricos
        SPI2CONbits.MCLKSEL = 0;
        baud_reg = PBCLK / 2 / frecuencia - 1;
        if (baud_reg >= 8192) { // Frecuencia demasiado baja
            return -1;
        }
    }
    SPI2BRG = baud_reg;
    return 0;
}

// Funciones no usadas de momento.

/* Nombre: _SPI1Interrupt (establecido por defecto por el microcontrolador)
 * Descripción: Rutina de atención a interrupción que gestiona el protocolo
 *              SPI del módulo maestro. Se activa cuando se recibe un dato
 *              por el SDI.
 * Argumentos: Ninguno
 * Valor devuelto: Ninguno
 */

//void __attribute__((interrupt, no_auto_psv)) _SPI1Interrupt(void)
//{    
//	int dato;
//	
//    IFS0bits.SPI1IF = 0;	// Borra la bandera de la interrupción
//
//    dato = SPI1BUF; // leo el dato, aunque no me sirve para nada, pero así no se generan errores de overflow.
//	
//	PORTB |=  (0x01 << _pin_ss);     // Desactiva SS para finalizar la trama.
//}

/* Nombre: _SPI1ErrInterrupt (establecido por defecto por el microcontrolador)
 * Descripción: Rutina de atención a interrupción que gestiona el protocolo
 *              SPI del módulo maestro.
 * Argumentos: Ninguno
 * Valor devuelto: Ninguno
 */

/*
void __attribute__((interrupt, no_auto_psv)) _SPI1ErrInterrupt(void)
{
    IFS0bits.SPI1EIF = 0;	// Borrar la bandera de la interrupción
    
    SPI1STATbits.SPIROV = 0;
    // HAY QUE LIMPIAR EL BIT SPIROV, QUE ES QUIEN HA GENERADO ESTA INTERRUPCION.
    
    while(1);
}
 */
