
#include <stdint.h> // Define uint32_t

void InicializarTimer4(void);
int getCentesimas(void);
void resetCentesimas(void);
int deteccionVuelta(int* medida);
void resetPulsoBocina(void);
void comparacion(int tiempo_vuelta);
int getMejorVuelta(void);
void enableCuentaAtras(uint8_t enable);
uint8_t getEnableCuentaAtras(void);
uint8_t getEnableCarrera(void);
uint8_t getFinCuentaAtras(void);
void resetFinCuentaAtras(void);
void resetInactivo(void);
void enableCarrera(uint8_t enable);
uint8_t getInactivo(void);
int getTicks(void);
void resetTicks(void);
void resetMejorVuelta(void);
uint8_t getPulsoBocina(void);