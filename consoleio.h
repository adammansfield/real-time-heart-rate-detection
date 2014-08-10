#ifndef CONSOLEIO_H_
#define CONSOLEIO_H_

#include <msp430.h>

/**
  @brief The size of the CIO header.
  */
#define CIO_HEADER_SIZE 11

/**
  @brief The size of the message including the null terminator.
  */
#define CIO_MESSAGE_SIZE 33

/**
  @brief The CIO write buffer.
  @note This must be global and named _CIOBUF_.
  */
unsigned char _CIOBUF_[CIO_HEADER_SIZE + CIO_MESSAGE_SIZE];

/**
  @brief Send char to debugger using CIO.
  */
void putc(char character);

/**
  @brief Send string to debugger using CIO.
  */
void puts(char *str);

/**
  @brief Flush the CIO write buffer.
  */
inline void cio_flush(void)
{
  cio_putc(0);
}

#endif // CONSOLEIO_H_

