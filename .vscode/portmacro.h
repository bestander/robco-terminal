#pragma once
#include <stdint.h>

// Stub portmacro.h for VS Code IntelliSense
// This satisfies the #include "portmacro.h" dependency without requiring
// the full FreeRTOS portable layer configuration

#ifndef portENTER_CRITICAL
#define portENTER_CRITICAL() 
#define portEXIT_CRITICAL()
#define portDISABLE_INTERRUPTS()
#define portENABLE_INTERRUPTS()
#define portYIELD()
#define portYIELD_FROM_ISR()

// Basic types that might be referenced
typedef int BaseType_t;
typedef unsigned int UBaseType_t;
typedef uint32_t TickType_t;
typedef void* TaskHandle_t;

#endif // portENTER_CRITICAL
