#include "consoleio.h"

void putc(char character)
{
  static char* buf = (char*)&_CIOBUF_[CIO_HEADER_SIZE];

  // Append any non-null char to buffer.
  if (character)
  {
    *buf++ = character;
  }

  // Write to host when buffer is full or char is null.
  if ((buf >= (char*)&_CIOBUF_[sizeof(_CIOBUF_) - 1]) || (character == 0))
  {
    const unsigned len_incl_null = buf - (char*)&_CIOBUF_[CIO_HEADER_SIZE - 1];

    // Null terminate string
    *buf = 0;

    // Set data and string length LSB.
    _CIOBUF_[0] = len_incl_null & 0xFF;
    _CIOBUF_[5] =  _CIOBUF_[0];

    // Set data and string length MSB.
    _CIOBUF_[1] = len_incl_null >> 8;
    _CIOBUF_[6] = _CIOBUF_[1];

    // Write command
    _CIOBUF_[2] = 0xF3;

    // stdout stream.
    _CIOBUF_[3] = 1;
    _CIOBUF_[4] = 0;

    // CIO breakpoint.
    __asm(" .global C$$IO$$");
    __asm("C$$IO$$:nop");

    // Reset string buffer pointer.
    buf = (char *)&_CIOBUF_[CIO_HEADER_SIZE];
  }
}

void puts(char *str)
{
  while (*str)
  {
    putc(*str++);
  }
}

