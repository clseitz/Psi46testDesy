
#ifndef TEST_H
#define TEST_H

#include "config.h"

void InitDAC(bool reset = false);
void InitChip();

void WriteSettings();
void test_current();
int test_tout();
int test_i2c();
int test_TempSensor();
int getTemperature();
bool test_Pixel();

// int test_roc(bool &repeat);
int test_roc_dig(bool &repeat);
int test_roc_bumpbonder();

#endif
