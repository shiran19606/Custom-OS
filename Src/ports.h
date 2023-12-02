#ifndef PORTS_H
#define PORTS_H

#include <stdint.h>

// Function declarations
uint8_t port_byte_in(uint16_t port);
void port_byte_out(uint16_t port, uint8_t data);

#endif /* PORTS_H */
