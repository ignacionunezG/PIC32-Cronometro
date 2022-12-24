#include <xc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TAM_COLA 100



int u1 = 0;
static int icab_rx = 0, icol_rx = 0, icab_tx = 0, icol_tx = 0; //dos colas para transmisor y receptor
static char cola_rx[TAM_COLA], cola_tx[TAM_COLA];

char getcUART(void) {
    char c;
    if (icol_rx != icab_rx) { // Hay datos nuevos
        c = cola_rx[icol_rx];
        icol_rx++;
        if (icol_rx == TAM_COLA) {
            icol_rx = 0;
        }
    } else {
        c = '\0'; // no ha llegado nada
    }
    return c;
}

void putsUART(char * ps) {
    while (*ps != '\0') {
        if ((icab_tx + 1 == icol_tx) ||
                (icab_tx + 1 == TAM_COLA && icol_tx == 0)) {
            break; // La cola estA llena. Se aborta el env�o de los caracteres que restan
        } else {
            cola_tx[icab_tx] = *ps; // Copia el carActer en la cola
            ps++;
            icab_tx++;
            if (icab_tx == TAM_COLA) {
                icab_tx = 0;
            }
        }
    }
    // Se habilitan las interrupciones del transmisor para comenzar a enviar
    IEC1bits.U1TXIE = 1;
}

void InicializarUART1(int baudios) {
    ANSELB &= ~((1 << 13) | (1 << 7)); // Se configura RB13 y RB7 como digital
    TRISB |= 1 << 13; // y como entrada
    LATB |= 1 << 7;

    SYSKEY = 0xAA996655; // Se desbloquean los registros
    SYSKEY = 0x556699AA; // de configuraci�n.
    RPB7R = 1; // Pin transmision conectado a RB7.
    U1RXR = 3;
    SYSKEY = 0x1CA11CA1; // Se vuelven a bloquear.


    IFS1bits.U1RXIF = 0; // Borro flag por si acaso
    IEC1bits.U1RXIE = 1; // Habilito interrupciones
    IFS1bits.U1TXIF = 0; // Borro flag del transmisor por si acaso
    IPC8bits.U1IP = 3; // Prioridad 3
    IPC8bits.U1IS = 1; // Subprioridad 1
    U1STAbits.URXISEL = 0; // Interrupci�n cuando llegue 1 char.
    U1STAbits.UTXISEL = 2; // Interrupci�n cuando FIFO vac�a.
    U1STAbits.URXEN = 1; // Se habilita el receptor.
    U1STAbits.UTXEN = 1; // Y el transmisor, fuerza un 1er envio


    if (baudios < 38400) {

        u1 = ((5000000 / ((16) * baudios)) - 1);
        U1BRG = u1;
        U1MODE = 0x8000;
    } else {
        u1 = ((5000000 / ((4) * baudios)) - 1);
        U1BRG = u1;
        U1MODE = 0x8008;
    }

}

__attribute__((vector(32), interrupt(IPL3SOFT), nomips16)) void InterrupcionUART1(void) {

    if (IFS1bits.U1RXIF == 1) { // Ha interrumpido el receptor
        if ((icab_rx + 1 == icol_rx) ||
                (icab_rx + 1 == TAM_COLA && icol_rx == 0)) {
            // La cola est� llena
        } else {
            cola_rx[icab_rx] = U1RXREG; // Lee car�cter de la UART
            icab_rx++;
            if (icab_rx == TAM_COLA) {
                icab_rx = 0;
            }
        }
        IFS1bits.U1RXIF = 0; // Y para terminar se borra el flag
    }
    if (IFS1bits.U1TXIF == 1) { // Ha interrumpido el transmisor
        if (icol_tx != icab_tx) { // Hay datos nuevos
            U1TXREG = cola_tx[icol_tx];
            icol_tx++;
            if (icol_tx == TAM_COLA) {
                icol_tx = 0;
            }
        } else { // Se ha vaciado la cola.
            IEC1bits.U1TXIE = 0; // Para evitar bucle sin fin
        }
        IFS1bits.U1TXIF = 0; // Y para terminar se borra el flag
    }

}

void mandaRegistro(int segundos, int centesimas) {

    char tiempo[14];
    char segundos_str[2];
    char centesimas_str[2];
    char cent_aux[1];

    sprintf(segundos_str, "%d", segundos); //SEGUNDOS_STR SON LOS SEGUNDOS COMO STRING
    
    if (centesimas < 10) {
        strcpy(centesimas_str, "0");
        sprintf(cent_aux, "%d", centesimas);
        strcat(centesimas_str, cent_aux);
        //DE ESTA FORMA, SI POR EJEMPLO EL TIEMPO DE LA VUELTA ACTUAL HAN SIDO 7 SEGUNDOS Y 7 CENTÉSIMAS, 
        //POR LA UART SE MANDARÁN 7:07 SECS, Y NO 7:7 SECS, QUE PUEDE DAR LUGAR A CONFUSIÓN.
        
    } else {
        sprintf(centesimas_str, "%d", centesimas);
    }
    
    
    
    strcat(strcpy(tiempo, "Tiempo-"), segundos_str);
    strcat(tiempo, ".");
    strcat(tiempo, centesimas_str);
    strcat(tiempo, "s");
    strcat(tiempo, "!\n\r");
    //tiempo="Tiempo-" + segundos_sec + ":" + centesimas_sec + "\n\r";

    putsUART(tiempo);
}
