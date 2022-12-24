/**
 * @file     spi.h
 *
 * @author   José Daniel Muñoz Frías
 * 
 * @version  1.0.0. Módulo original
 *
 * @date     31/04/2018
 *
 * @brief    Módulo encargado de gestionar las comunicaciones con otros
 *           dispositivos a través del bus SPI.
 */

#ifndef _SPI_H
#define _SPI_H

// -----------------------------------------------------------------------------
// -------------------------------- PARAMETROS ---------------------------------
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ------------------- PROTOTIPOS DE LAS FUNCIONES PÚBLICAS --------------------
// -----------------------------------------------------------------------------

void InicializarSPI2(int frec);
void SPI_SendFrame(uint8_t dato);
    

#endif
