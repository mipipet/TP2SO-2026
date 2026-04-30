#ifndef KEYSTATE_H
#define KEYSTATE_H

#include <stdint.h>

// Función para verificar si una tecla está presionada
uint8_t is_key_pressed(uint8_t scancode);

// Arreglo para mantener el estado de las teclas
extern uint8_t key_states[256];

#endif
