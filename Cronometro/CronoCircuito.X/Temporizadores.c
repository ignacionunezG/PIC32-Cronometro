#include "Temporizadores.h"
#include "bocina.h"
#include "display.h"
#include "uart1.h"
#include "Pic32Ini.h"
#include "TftDriver.h"

#include <math.h>
#include <xc.h>
#include <stdint.h>

#define PIN_SEM_ROJO 14//RB14
#define PIN_SEM_VERDE 4//RC4
#define PIN_SENSOR 8// RB8
//INICIALIZO VARIABLES
static uint8_t enable_cuentaAtras = 0;
static uint8_t bocina_victoriosa = 0;
static uint8_t pulso_bocina = 0;
static uint8_t enable_carrera = 0;
static uint8_t fin_cuenta_atras = 0;
static uint8_t inactivo = 0;
static int tiempo_vuelta = 0;
static int mejor_vuelta = 0;
static int sensorir_act, sensorir_ant;
int cs = 0;
int ticks = 0;

void InicializarTimer4(void) {
    TMR4 = 0;
    PR4 = 49999; // Se configura el timer para que termine la cuenta en 1 centesima
    IPC4bits.T4IP = 2; // Se configura la prioridad de la interrupci�n
    IPC4bits.T4IS = 0; // Subprioridad 0
    IFS0bits.T4IF = 0; // Se borra el flag de interrupci�n por si estaba pendiente
    IEC0bits.T4IE = 1; // y por �ltimo se habilita su interrupci�n
    T4CON = 0x8000; // preescalado=1
}

__attribute__((vector(_TIMER_4_VECTOR), interrupt(IPL2SOFT), nomips16)) void
InterrupcionTimer4(void) {
    IFS0bits.T4IF = 0; //PONGO A CERO EL FLAG

    if (enable_cuentaAtras == 1) { //SI ESTOY EN CUENTA ATRAS:
        if (ticks < 100) {
            LATCCLR = 1 << PIN_SEM_ROJO; //DURANTE EL PRIMER SEGUNDO, SE MANTENDRÁ EL SEMAFORO EN ROJO. POR AHORA, LA BOCINA SE MANTIENE EN REPOSO.
        } else if (ticks == 100) { 
            suenaTono(1);//AL PRIMER SEGUNDO, SUENA EL PRIMER TONO DE LA CUENTA ATRÁS. SE PONE EL OCRS PARA QUE SUENE A UNA DETERMINADA FRECUENCIA
            pulso_bocina = 1;//SE HABILITA EL TONO
        } else if (ticks > 100 && ticks < 400) {
            if (ticks % 50 == 0) {
                LATBINV = 1 << PIN_SEM_ROJO; //CADA MEDIO SEGUNDO, SE APAGARÁ Y SE ENCENDERÁ EL SEMÁFORO ROJO
                suenaTono(1); //EN CASO DE QUE SUENE LA BOCINA, SE ESPECIFICA A LA FRECUENCIA A LA QUE QUEREMOS QUE SUENE
                pulso_bocina ^= 1;//LA BOCINA ÚNICAMENTE SONARÁ CADA MEDIO SEGUNDO, JUSTO CUANDO SE ENCIENDA EL SEMÁFORO ROJO. PARA QUE LA BOCINA SUENE, PULSO_BOCINA TIENE QUE VALER 1.
            }

        } else if (ticks == 400) { //SI HAN PASADO 4 SEGUNDOS:
            ticks = 0; //SE REINICIA LA VARIABLE TICKS, QUE SE USARÁ OTRA VEZ POSTERIORMENTE
            suenaTono(2); //SE PREPARA LA BOCINA PARA QUE SUENE A UNA DETERMINADA FRECUENCIA
            LATBCLR = 1 << PIN_SEM_ROJO;//SE APAGA EL SEMÁFORO ROJO
            pulso_bocina = 1;//SUENA LA BOCINA
            fin_cuenta_atras = 1;//SE ACTIVA LA SEÑAL FINCUENTAATRAS PARA PODER ACTIVAR EL CRONÓMETRO
            LATCSET = 1 << PIN_SEM_VERDE;//SE ENCIENDE EL SEMÁFORO VERDE
            enable_cuentaAtras = 0;//SE DESACTIVA LA CUENTA ATRÁS
        }
        ticks++;//SI SE ESTÁ EN LA CUENTA ATRÁS, TICKS SIEMPRE AUMENTA SU VALOR CADA CENTESIMA
    } else if (enable_carrera == 1) {
        if (cs == 70) { //DURANTE LOS PRIMEROS 0,7 SEGUNDOS DE LA PRIMERA VUELTA SEGUIRÁ SONANDO LA BOCINA QUE HA SONADO CUANDO SE HA HABILITADO EL CRONÓMETRO
            pulso_bocina = 0; //POSTERIORMENTE, LA BOCINA SE APAGRÁ
        }
        if (cs < 3000) {
            cs++; //SI NO HAN PASADO 30 SEGUNDOS, SE AUMENTARÁN LAS CENTÉSIMAS
        } else {
            inactivo = 1;//SI HAN PASADO 30 SEGUNDOS SIN QUE PASE UN COCHE POR DELANTE, SE INTERPRETA QUE HA HABIDO ALGÚN PROBLEMA EN EL CIRCUITO Y SE DESACTIVA EL CRONÓMETRO AUTOMÁTICAMENTE
            cs = 0;//
        }
    }

    if (bocina_victoriosa == 1) {//SI SE HA ACTUALIZADO LA MEJOR VUELTA:
        if (ticks < 70) {
            pulso_bocina = 1; //SONARÁ LA BOCINA DURANTE 0,7 SEGUNDOS
            ticks++;

        } else {
            bocina_victoriosa = 0; //A LOS 0,7 SEGUNDOS, DEJARÁ DE SONAR LA BOCINA
            ticks = 0;
            pulso_bocina = 0;
        }
    }
}

int getCentesimas(void) { //FUNCION QUE SE LLAMA DESDE DISPLAY.C PARA PODER MOSTRAR LOS TIEMPOS DE LA VUELTA ACTUAL
    int cs_aux; 
    asm("di");
    cs_aux = cs;
    asm("ei");
    return cs_aux;
}

void resetCentesimas(void) { //RESETAMOS LOS TIEMPOS DE LA VUELTA ACTUAL CUANDO PASA UN COCHE POR DELANTE DEL SENSOR
    asm("di");
    cs = 0;
    asm("ei");
}

int deteccionVuelta(int* medida) { 
    sensorir_act = (PORTB >> PIN_SENSOR) & 1; 
    int deteccion = 0; //POR DEFECTO, DETECCIÓN VALE 0
    if ((sensorir_act != sensorir_ant) && (sensorir_act == 0)) { //SI PASA UN COCHE POR DELANTE DEL SENSOR (QUE ES ACTIVO A NIVEL BAJO)
        *medida = getCentesimas(); //MEDIDA=TIEMPO QUE SE HA TARDADO EN DAR UNA VUELTA
        resetCentesimas();//LA VUELTA ACTUAL SE REINICIA
        deteccion = 1; //SE INDICA QUE SE HA DETECTADO UN COCHE PASANDO POR DELANTE DEL SENSOR
    }
    sensorir_ant = sensorir_act; //SE REESTABLECE EL VALOR DE SENSORIR_ANT
    return deteccion;
}

void resetPulsoBocina(void) { //SE DESHABILITA LA BOCINA
    asm("di");
    pulso_bocina = 0;
    asm("ei");
}

void comparacion(int tiempo_vuelta) { //FUNCIÓN EN LA QUE SE VERÁ SI SE TIENE QUE ACTUALIZAR LA MEJOR VUELTA
    asm("di");
    if (tiempo_vuelta < mejor_vuelta || mejor_vuelta == 0) { //SI EL TIEMPO DE LA VUELTA ACTUAL ES INFERIOR A LA MEJOR VUELTA; O SI LA VUELTA ACTUAL ES LA PRIMERA VUELTA QUE SE DA:
        mejor_vuelta = tiempo_vuelta; //SE ACTUALIZA LA MEJOR VUELTA
        suenaTono(2); //SE PREPARA EL OCRS PARA QUE SUENE A UNA DETERMINADA FRECUENCIA
        bocina_victoriosa = 1; //SUENA LA BOCINA
        clrScr();// SE BORRA LA PANATALLA
    }
    asm("ei");
}

int getMejorVuelta(void) {//FUNCIÓN LLAMADA DESDE DISPLAY.C PARA MOSTRAR EL TIEMPO DE LA MEJOR VUELTA
    return mejor_vuelta;
}

void enableCuentaAtras(uint8_t enable) { //SE HABILITA O DESHABILITA LA CUENTA ATRÁS
    asm("di");
    enable_cuentaAtras = enable;
    asm("ei");
}

uint8_t getEnableCuentaAtras(void) { //FUNCIÓN LLAMADA DESDE DISPLAY.C PARA MOSTRAR POR PANTALLA LA CUENTA ATRÁS ÚNICAMENTE CUANDO CORRESPONDA
    uint8_t enable_aux = 0;
    asm("di");
    enable_aux = enable_cuentaAtras;
    asm("ei");
    return enable_aux;
}

uint8_t getEnableCarrera(void) { //FUNCIÓN LLAMADA DESDE DISPLAY.C PARA MOSTRAR POR PANTALLA TIEMPOS ÚNICAMENTE CUANDO CORRESPONDA
    uint8_t enable_carrera_aux = 0;
    asm("di");
    enable_carrera_aux = enable_carrera;
    asm("ei");
    return enable_carrera_aux;
}

uint8_t getFinCuentaAtras(void) { //FUNCIÓN LLAMA DESDE MAIN.C PARA ACTIVAR EL CRONÓMETRO CUANDO SE ACABE LA CUENTA ATRÁS
    uint8_t fin_cuenta_atras_aux;
    asm("di");
    fin_cuenta_atras_aux = fin_cuenta_atras;
    asm("ei");
    return fin_cuenta_atras_aux;
}

void resetFinCuentaAtras(void) { //FUNCIÓN PARA RESETEAR LA VARIABLE FINCUENTAATRAS
    asm("di");
    fin_cuenta_atras = 0;
    asm("ei");
}

void resetInactivo(void) { //FUNCIÓN PARA RESETEAR LA VARIABLE INACTIVO
    asm("di");
    inactivo = 0;
    asm("ei");
}

void enableCarrera(uint8_t enable) {
    asm("di");
    enable_carrera = enable; //SE HABILITA O DESHABILITA LA CARRERA
    asm("ei");
}

uint8_t getInactivo(void) { //FUNCIÓN QUE SE LLAMA DESDE MAIN.C PARA APAGAR EL CRONÓMETRO CUANDO HAYAN PAADO 30 SEGUNDOS SIN NINGÚN COCHE POR EL SENSOR
    uint8_t inactivo_aux;
    asm("di");
    inactivo_aux = inactivo;
    asm("ei");
    return inactivo_aux;
}

int getTicks(void) { //FUNCIÓN LLAMADA DESDE DISPLAY.C PARA QUE SE MUESTRE LA CUENTA ATRÁS ("3,2,1...") A LA VEZ QUE SUENA LA BOCINA Y QUE SE ENCIENDE EL LED ROJO
    int ticks_aux;
    asm("di");
    ticks_aux = ticks;
    asm("ei");
    return ticks_aux;
}

void resetTicks(void) { //RESETA LOS TICKS
    asm("di");
    ticks = 0;
    asm("ei");
}

void resetMejorVuelta(void) { //RESETEA LA MEJOR_VUELTA. ÚNICAMENENTE OCURRIRÁ CUANDO SE ESTÉ EN ESTADO_APAGADO
    mejor_vuelta = 0;
}

uint8_t getPulsoBocina(void) { //FUNCIÓN LLAMADA DESDE BOCINA.C PARA QUE SUENE LA BOCINA ÚNCIAMENTE CUANDO CORRESPONDE.
    uint8_t pulso_bocina_aux;
    asm("di");
    pulso_bocina_aux = pulso_bocina;
    asm("ei");
    return pulso_bocina_aux;
}

