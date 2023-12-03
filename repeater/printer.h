#ifndef PRINTER_H
#define PRINTER_H

#include <Arduino.h>

class Printer {
public:
  Printer(HardwareSerial& hSerial);
  void msg(const char* val);
  void msg(int val);
  void msg(uint8_t val);
  void msg(unsigned long val);
  void msg(bool val);

  void keyVal(const char* key, int val);
  void keyVal(const char* key, uint8_t val);
  void keyVal(const char* key, unsigned long val);
  void keyVal(const char* key, bool val);

private:
  HardwareSerial& serial;

  bool isDisabled;
};

#endif