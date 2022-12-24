#include <stdio.h>
#include <xc.h>
#include <string.h>
#include "Pic32Ini.h"
#include "uart1.h"
#include "Temporizadores.h"
#include "display.h"
#include "bocina.h"
#include "TftDriver.h"


#define PIN_BOCINA 14 //RB14 
#define PIN_SEM_ROJO 14//RC7
#define PIN_SEM_VERDE 4//RC4
#define PIN_SENSOR 8//RB8
#define PULSADOR_START 5//RB5
#define PULSADOR_GO 7//RA7
extern uint8_t BigFont[];

int main(void) {

    uint8_t pulsador_act_start, pulsador_ant_start;
    uint8_t pulsador_act_go, pulsador_ant_go;
    uint8_t inactivo, start, go, fin_cuenta_atras;
    int medida, segundos, centesimas;

    //Inicializar pines
    ANSELA = ~(1 << PULSADOR_GO);
    ANSELB = ~((1 << PIN_BOCINA) | (1 << PIN_SENSOR) | (1 << PULSADOR_START) | (1 << PIN_SEM_ROJO));
    ANSELC = ~(1 << PIN_SEM_VERDE);

    TRISA = (1 << PULSADOR_GO);
    TRISB = ((1 << PIN_SENSOR) | (1 << PULSADOR_START));
    TRISC = 0;
    LATA = 0;
    LATB = 0;
    LATC = 0xF;
    //Inicializar Periféricos
    InicializarBocina();
    InicializarPantalla();
    InicializarTimer4();
    InicializarUART1(9600);
    //Habilitar interrupciones    
    INTCONbits.MVEC = 1;
    asm("ei");
    //Inicializo los pulsaodores
    pulsador_ant_start = (PORTB >> PULSADOR_START) & 1;
    pulsador_ant_go = (PORTA >> PULSADOR_GO) & 1;

    enum {
        estado_apagado, estado_idle, estado_cuentaAtras, estado_finTemp, estado_corriendo //Inicializo estados
    } estado;
    estado = estado_apagado;

    while (1) {
        pulsador_act_start = (PORTB >> PULSADOR_START) & 1;
        pulsador_act_go = (PORTA >> PULSADOR_GO) & 1;
        //inactivo = getInactivo();

        if ((pulsador_act_start != pulsador_ant_start) && (pulsador_act_start == 0)) {
            // Flanco de bajada en la patilla del pulsador START
            start = 1;

        } else {
            start = 0;
        }
        if ((pulsador_act_go != pulsador_ant_go) && (pulsador_act_go == 0)) {
            // Flanco de bajada en la patilla del pulsador GO
            go = 1;

        } else {
            go = 0;
        }


        switch (estado) {
            case estado_apagado://SI EL CRONÓMETO ESTÁ APAGADO Y SE PULSA EL BOTÓN DE START, SE PASA AL ESTADO DE IDLE
                if (start == 1) {
                    estado = estado_idle;
                }
                break;
            case estado_idle://SI SE ESTÁ EN ESTADO DE IDLE Y SE VULVE A PULSAR EL BOTON DE START, SE PASA AL ESTADO APAGADO
                if (start == 1) {
                    estado = estado_apagado;
                } else if (go == 1) {//MIENTRAS QUE SI SE PULSA GO, SE INICIALIZARÁ LA CUENTA ATRÁS
                    estado = estado_cuentaAtras;
                }
                break;
            case estado_cuentaAtras:
                if (start == 1) {
                    estado = estado_finTemp; //SI MIENTRAS QUE ESTÁ REALIZANDO LA CUENTA ATRÁS SE PULSA EL BOTÓN DE START, SE VA A ESTADO_FINTEMP
                } else if (fin_cuenta_atras == 1) {
                    estado = estado_corriendo; //SI SE ACABA CORRECTAMENTE LA CUENTA ATRÁS, SE PASA AL ESTADO DE CORRIENDO
                }
                break;
            case estado_finTemp:
                estado = estado_idle; //ESTE ESTADO ES AL QUE SE VA CUANDO ESTÁN OCURRIENDO COSAS(CUENTA ATRÁS O SE ESTÁ CORRIENDO) Y SE QUIERE VOLVER AL ESTADO DE IDLE
                break; //ES UN ESTADO INTERMEDIO ENTRE ESTADO_CUENTAATRAS O ESTADO_CORRIENDO Y ESTADO_IDLE


            case estado_corriendo:
                if (inactivo == 1) {
                    estado = estado_finTemp; //SI PASAN 30 SEGUNDOS SIN QUE PASE NINGUN COCHE POR EL SENSOR, SE VA A ESTADO_FINTEMP
                } else if (start == 1) {
                    estado = estado_finTemp; //SI SE PULSA EL BOTÓN START, SE VA A ESTADO_FINTEMP
                }
                break;
        }

        if (estado == estado_apagado) {
            LATCCLR = 1 << PIN_SEM_VERDE; //SEMÁFORO VERDE APAGADO
            resetMejorVuelta(); //REINICIAMOS LA MEJOR VUELTA
            LATBCLR = 1 << PIN_SEM_ROJO; //SEMÁFORO ROJO APAGADO

        } else if (estado == estado_idle) {
            LATBSET = 1 << PIN_SEM_ROJO; //ENCENDEMOS SEMÁFORO ROJO

        } else if (estado == estado_cuentaAtras) {
            enableCuentaAtras(1); //ENABLE LA CUENTA ATRÁS
            bocina(); //SUENA LA BOCINA
            mostrarCuentaAtras(); //PERMITE LA CUENTA ATRÁS
            fin_cuenta_atras = getFinCuentaAtras(); //SE ESCUCHA A LA SEÑAL FINCUENTARAS. CUANDO ESTA SE ACTIVA, SE INICIALIZARÁ EL CRONÓMETRO



        } else if (estado == estado_finTemp) {
            LATBSET = 1 << PIN_SEM_ROJO; //SE PONE EL SEMÁFORO EN ROJO
            resetCentesimas(); //SE APAGA LA CUENTA DEL CRONÓMETRO
            enableCuentaAtras(0); //SE APAGA LA CUENTA ATRAS POR SI SE VIENESE DE LA CUENTA ATRAS
            enableCarrera(0); //SE APAGA EL CRONÓMETO POR SI SE VINIESE DEL ESTADO_CARRERA
            apagarBocina(); //SE APAGA LO BOCINA POR SI JUSTO ESTÁ SONANDO CUANDO SE LLEGA A ESTE ESTADO
            resetPulsoBocina(); //SE PAGA LA SEÑAL QUE HABILITA LA BOCINA

            resetTicks(); //RESETEA TICKS, SEÑAL USADA PARA LA CUENTA ATRAS Y PARA MEDIR EL TIEMPO QUE SUENA LA BOCINA CUANDO PASA LA MEJOR VUELTA
            resetInactivo(); //SE PONE A 0 LA SEÑAL DE INACTIVO. SE RECUERDA QUE LA SEÑLA "INACTIVO" ÚNICAMENTE SIRVE PARA PASAR DE ESTADO_CORRIENDO A ESTADO_FINTEMP
            LATCCLR = 1 << PIN_SEM_VERDE; //SE APAGA EL SEMÁFORO VERDE

            ///PANTALLA
            clrScr(); //SE BORRA LA PANTALLA
            setBackColor(VGA_TRANSPARENT); //SE PREPARA LA PANTALLA PARA CUANDO VAYA A VOLVER A APARECER LA CUENTA ATRÁS
            setFont(BigFont);


        } else if (estado == estado_corriendo) {
            LATBCLR = 1 << PIN_SEM_ROJO; //SE APAGA EL SEMÁFORO ROJO
            bocina(); //SE ACTIVA LA BOCINA
            enableCuentaAtras(0); //SE DESACTIVA LA CUENTA ATRÁS
            resetFinCuentaAtras(); //A ESTE ESTADO SOLO SE LLEGA CUANDO FINCUENTAATRAS=1. POR TANTO, HAY QUE RESETEAR DICHA SEÑAL
            enableCarrera(1); //ENABLE_CARRERA=1;
            inactivo = getInactivo(); //SE ESCUCHA LA SEÑAL INACTIVO. CUANDO HAYAN PASADO 30 SEGUNDOS Y NO HAYA PASADO NINGÚN COCHE, SE ACTIVARÁ INACTIVO Y S EPASARÁ AL ESTADO FIN_TEMP, PARA PASAR A IDLE POSTERIORMENTE
            LATCSET = 1 << PIN_SEM_VERDE; //SE PONE EL SEMÁFORO EN VERDE
            if (deteccionVuelta(&medida)) {//SI SE DETECTA QUE PASA UN COCHE POR DELANTE:
                segundos = (int) (medida / 100);
                centesimas = medida - (segundos * 100);
                mandaRegistro(segundos, centesimas); //MANDA POR LA UART EL TIEMPO QUE SE HA TARDADO EN DAR LA ÚLTIMA VUELTA
                comparacion(medida); //SE COMPARA LA ÚLTIMA VUELTA CON LA VUELTA ACTUAL MEJOR, PARA VER SI LA ÚLTIMA VUELTA QUE SE HA DADO ES LA NUEVA MEJOR
            }
            mostrarTiempos(); //SE MUESTRAN LOS TIEMPOS POR EL DISPLAY

        }

        //ACTUALIZO LOS PULSADORES
        pulsador_ant_start = pulsador_act_start;
        pulsador_ant_go = pulsador_act_go;
    }
}




