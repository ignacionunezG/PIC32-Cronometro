#include <stdio.h>
#include <xc.h>
#include <string.h>
#include "Pic32Ini.h"
#include "uart1.h"
#include "bocina.h"
#include "Temporizadores.h"
#define pr1 4777 //con frecuencia do5. SE UTILIZARÁ PARA LA CUENTA ATRÁS
#define pr2 3579 //con frecuencia fa6  SE UTILIZARÁ PARA ANUNCIAR LA SALIDA Y PARA INDICAR QUE SE HA ACTUALIZADO LA MEJOR VUELTA


static uint8_t pulso = 0;

void InicializarBocina(void) { //SE INICIALIZA LA BOCINA
    SYSKEY = 0xAA996655;
    SYSKEY = 0x556699AA;
    RPA0R = 5; // OC1 conectado a RPA0
    SYSKEY = 0x1CA11CA1;
    OC1CON = 0; // OC OFF
}

void suenaTono(int tono) { //SIRVE PARA INDICAR QUÉ TONO QUEREMOS QUE SUENE.
    if (tono == 1) {
        OC1R = (pr1 / 2); // SE ESTABLECE QUE EL OC1R SERÁ EL pr1/2
        OC1RS = (pr1 / 2);
        OC1CON = 0x8006; // OC ON, modo PWM sin faltas. SE USA TIMER2.
        T2CON = 0;
        TMR2 = 0;
        PR2 = pr1;

    } else if (tono == 2) {
        OC1R = (pr2 / 2); // SE ESTABLECE QUE EL OC1R SERÁ EL pr2/2
        OC1RS = (pr2 / 2);
        OC1CON = 0x8006; // OC ON, modo PWM sin faltas. SE USA TIMER2.
        T2CON = 0;
        TMR2 = 0;
        PR2 = pr2;
    }
}

void bocina(void) { //SUENA LA BOCINA SI LA SEÑAL PULSO_BOCINA DE TEMPORIADORES ESTÁ A 1.
    pulso = getPulsoBocina();
    if (pulso == 1) { 
        T2CON = 0x8000;
    } else {
        T2CON = 0x0000;
    }
}

void apagarBocina(void){ //APAGAR LA BOCINA
    T2CON=0x0000;
    pulso=0;
}