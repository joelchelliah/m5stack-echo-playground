/*
 * Printer
 *
 * Wrapper for printing formatted messages to the provided HardwareSerial,
 * with added support for disabling the printer via toggleable flag.
 *
 */

#include "printer.h"

Printer::Printer(HardwareSerial& serial) :
  serial(serial),
  isDisabled(false) {}

void Printer::msg(const char* val) {
  if(isDisabled) return;
  serial.println(val);
}

void Printer::msg(int val) {
  if(isDisabled) return;
  serial.println(val);
}

void Printer::msg(uint8_t val) {
  if(isDisabled) return;
  serial.println(val);
}

void Printer::msg(unsigned long val) {
  if(isDisabled) return;
  serial.println(val);
}

void Printer::msg(bool val) {
  if(isDisabled) return;
  serial.println(val);
}

void Printer::keyVal(const char* key, int val) {
  if(isDisabled) return;
  serial.print(key);
  serial.print(": ");
  serial.println(val);
}

void Printer::keyVal(const char* key, uint8_t val) {
  if(isDisabled) return;
  serial.print(key);
  serial.print(": ");
  serial.println(val);
}

void Printer::keyVal(const char* key, unsigned long val) {
  if(isDisabled) return;
  serial.print(key);
  serial.print(": ");
  serial.println(val);
}

void Printer::keyVal(const char* key, bool val) {
  if(isDisabled) return;
  serial.print(key);
  serial.print(": ");
  serial.println(val);
}
