#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h> // Para sin, cos y tan
#include "Pic32Ini.h"
#include "TftDriver.h"
#include "Temporizadores.h"
#include "display.h"

static int ticks_display = 0;
extern uint8_t SmallFont[];
extern uint8_t BigFont[];

char *actual_orig = "ACTUAL: ";
char *mejor_orig = "MEJOR: - : -";
char *c3 = "MEJOR: ";
char actual[12]; //va a contener la vuelta actual y el tiempo
char mejor[11]; //va a contener la mejor vuelta y el tiempo


char seg_mejor[2];
char cent_mejor[2];
char cent_aux[1];

char seg_actual[2];
char cent_actual[2];

void InicializarPantalla(void) { //SE INICIALIZA DISPLAY
    
    inicializarTFT(LANDSCAPE);
    clrScr();
    setBackColor(VGA_TRANSPARENT);
    clrScr(); // Borra la pantalla
    setFont(BigFont);

}

void mostrarCuentaAtras(void) {
    if (getEnableCuentaAtras() == 1) { //SI ESTAMOS EN LA CUENTA ATRÁS:
        ticks_display = getTicks(); //TICKS_DISPLAY=TICKS UTILIZADOS PARA MANTENER LA CUENTA DE LA CUENTA ATRÁS

        if (ticks_display < 95) { //EN EL PRIMER SEGUNDO, NO SE MUESTRA NADA POR PANTALLA 
            print("", CENTER, 55, 0);
        } else if (ticks_display == 95) {//(SE PONE 95 CENTESIMAS Y NO 100 PORQUE EN GENERAL TARDA 5 CENTESIMAS MÁS EN SONAR EL DISPLAY QUE EN QUE SE ENCIENDA EL LED Y SUENE LA BOCINA)
            clrScr(); //SE BORRA EL DISPLAY, POR SI HUBIESE ALGO EN PANTALLA
            print("3", CENTER, 55, 0); //CUANDO PASA 1 SEGUNDO, SE MUESTRA EL "3"
        } else if (ticks_display < 195) {//SE MUESTRA EL "3" DURANTE UN SEGUNDO
            print("3", CENTER, 55, 0);
        } else if (ticks_display == 195) {//CUANDO PASAN 2 SEGUNDOS, SE MUESTRA EL "2"
            clrScr();
            print("2", CENTER, 55, 0);
        } else if (ticks_display < 295) { //SE MUESTRA EL "2" DURANTE UN SEGUNDO
            print("2", CENTER, 55, 0);
        } else if (ticks_display == 295) { //CUANDO PASAN 3 SEGUNDOS, SE MUESTRA EL "1"
            clrScr();
            print("1", CENTER, 55, 0);
        } else if (ticks_display < 400) { //SE MUESTRA EL "1" DURANTE UN SEGUNDO
            print("1", CENTER, 55, 0);
        } else if (ticks_display == 400) { //CUANDO PASAN 4 SEGUNDOS, SE BORRA LA PANTALLA Y SE PREPARA EL FONT PARA LOS TIEMPOS
            clrScr();
            setFont(SmallFont); //EL FONT VA A SER MÁS PEQUEÑO CUANDO SE MUESTREN TIEMPOS
        }
    }
}

void mostrarTiempos(void) {
    if (getEnableCarrera() == 1) { //SI SE ESTÁ CORRIENDO
        setFont(SmallFont); //SE ESTABLECE EL FONT
        //LÓGICA PARA MOSTRAR LOS TIEMPOS ACTUALES
        
        print(actual_orig, LEFT, 30, 0); //POR DEFECTO, SIEMPRE QUEREMOS QUE APAREZCA LA PALABRA "ACTUAL" (ASEGURARNOS DE QUE EL LA COMUNICACIÓN SPI DEL DISPLAY VA BIEN)
        ticks_display = getCentesimas(); //TICKS_DISPLAY = TICKS UTILIZADOS DURANTE LA CARRERA.

        int segundos = (int) (ticks_display / 100); //SEGUNDOS SON LOS SEGUNDOS DE LA VUELTA ACUAL
        int centesimas = (ticks_display - (segundos * 100));//CENTESIMAS SON LAS CENTESIMAS DE LA VUELTA ACTUAL

        sprintf(seg_actual, "%d", segundos); //segActual SERÁN LOS SEGUNDOS DE LA VUELTA ACTUAL PASADO A STRING

        if (centesimas < 10) { //SI LAS CENTESIMAS ACTUALES SON INFERIORES A 10:
            
            strcpy(cent_actual, "0");
            sprintf(cent_aux, "%d", centesimas);
            strcat(cent_actual, cent_aux);
            //DE ESTA FORMA, SI POR EJEMPLO EN EL INSTANTE ACTUAL ESTAMOS EN 7 SEGUNDOS Y 7 CENTÉSIMAS, 
            //EN EL DISPLAY SE MOSTRARÁ 7:07 SECS, Y NO 7:7 SECS, QUE PUEDE DAR LUGAR A CONFUSIÓN.

        } else {
            sprintf(cent_actual, "%d", centesimas);
        }

        strcat(strcpy(actual, actual_orig), seg_actual);
        strcat(actual, " : ");
        strcat(actual, cent_actual);

        fillRect(60, 0, 120, 45);  //SE PINTARÁ UN CUADRADO NEGRO ENCIMA DEL TIEMPO ACTUAL CONSTANTEMENTE, PARA LUEGO PODER ESCRIBIR EL TIEMPO ACTUAL SOBRE ESE CUADRADO NEGRO, Y ASI SUCESIVAMENTE.
        //DE ESTA FORMA, LOS TIEMPOS ACTUALES NO SE SOBRE PONEN UNO ENCIMA DEL OTRO.
        //ESTA FUNCIÓN SE HA MODIFICADO PARA QUE EL CUADRADO QUE SE PINTA SEA NEGRO Y NO BLANCO
        print(actual, LEFT, 30, 0);//FINALMENTE, SE MOSTRARÁ LO SIGUIENTE:  ACTUAL: SEGACTUAL:CENTACTUAL 

        
        //LÓGICA PARA MOSTRAR LOS MEJORES TIEMPOS
     
        int segundos_mejor = (int) (getMejorVuelta() / 100);//OBTENER SEGUNDOS DEL MEJOR TIEMPO
        int centesimas_mejor = (getMejorVuelta()-(segundos_mejor * 100));//OBTENER CENTÉSIMAS DEL MEJOR TIEMPO
        
        sprintf(seg_mejor, "%d", segundos_mejor);//segMejor será el tiempo de los segundos del mejor tiempo

        if (centesimas_mejor < 10) {
            strcpy(cent_mejor, "0");
            sprintf(cent_aux, "%d", centesimas_mejor);
            strcat(cent_mejor, cent_aux);
            //MISMA LÓGICA QUE ANTES PARA OBTENER LAS DÉCIMA EN UN FORMATO QUE NO PROVOQUE CONFUSIÓN

        } else {
            sprintf(cent_mejor, "%d", centesimas_mejor);

        }

        strcat(strcpy(mejor, c3), seg_mejor);
        strcat(mejor, " : ");
        strcat(mejor, cent_mejor);

        if (getMejorVuelta() == 0) {
            print(mejor_orig, LEFT, 80, 0); //SI NO SE HA DADO NINGUNA VUELTA TODAVÍA, SE MOSTRARÁ MEJOR: -:-
        } else {
            print(mejor, LEFT, 80, 0); //SI YA HAY UNA MEJOR VUELTA, SE MOSTRARÁ LO SIGUIENTE: MEJOR: SEGMEJOR:CENTMEJOR
        }
    }
}
