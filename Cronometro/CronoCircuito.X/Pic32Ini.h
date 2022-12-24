/* 
 * File:   Pic32Ini.h
 * Author: daniel
 *
 * Created on January 27, 2018, 6:04 PM
 */

#ifndef PIC32INI_H
#define	PIC32INI_H

#ifdef	__cplusplus
extern "C" {
#endif

#define SYSCLK 40000000L // Reloj del sistema a 40 MHz
#define PBCLK   5000000L // Reloj de periféricos a 5 MHz
  
void InicializarReloj(void);


#ifdef	__cplusplus
}
#endif

#endif	/* PIC32INI_H */

