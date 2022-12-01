/* Reference : https://fr.wikipedia.org/wiki/Chiffrement_RSA#Cr%C3%A9ation_des_cl%C3%A9s */
#ifndef RSA_H
#define RSA_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

int e, d, n;

static int gcd(int a, int b);
static int PrimarityTest(int a, int i);
static int FindT(int a, int m, int n);
static void FastExponention(int bit, int n, int* y, int* a);
static int inverse(int a, int b);
static void KeyGeneration();
static void Encryption(int value, FILE* out);
static void Decryption(int value, FILE* out);

#endif