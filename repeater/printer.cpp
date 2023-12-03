#include "printer.h"

Printer::Printer(HardwareSerial& serial) : serial(serial) {}

void Printer::msg(const char* val) {
  serial.println(val);
}

void Printer::msg(int val) {
  serial.println(val);
}

void Printer::msg(uint8_t val) {
  serial.println(val);
}

void Printer::msg(unsigned long val) {
  serial.println(val);
}

void Printer::msg(bool val) {
  serial.println(val);
}

void Printer::keyVal(const char* key, int val) {
  serial.print(key);
  serial.print(": ");
  serial.println(val);
}

void Printer::keyVal(const char* key, uint8_t val) {
  serial.print(key);
  serial.print(": ");
  serial.println(val);
}

void Printer::keyVal(const char* key, unsigned long val) {
  serial.print(key);
  serial.print(": ");
  serial.println(val);
}

void Printer::keyVal(const char* key, bool val) {
  serial.print(key);
  serial.print(": ");
  serial.println(val);
}
