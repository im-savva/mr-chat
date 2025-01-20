#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <QString>
#include <openssl/rsa.h>
#include <openssl/aes.h>

// Function to generate RSA key pair
RSA* generateRSAKeyPair();

// Function to generate a random AES key
AES_KEY* generateAESKey();

// Function to generate AES key from password using PBKDF2
AES_KEY* generateAESKeyFromPassword(const QString &password);

unsigned char* rsaEncryptAESKey(RSA *rsa, const unsigned char *aesKey, int aesKeyLength, int *encryptedKeyLength);
unsigned char* rsaDecryptAESKey(RSA *rsa, const unsigned char *encryptedKey, int encryptedKeyLength, int *decryptedKeyLength);

// Function to encrypt plaintext using AES
QByteArray encryptTextAES(const unsigned char *plaintext, int plaintextLength, const unsigned char *aesKey, unsigned char *iv);

// Function to decrypt ciphertext using AES
QByteArray decryptTextAES(const unsigned char *ciphertext, int ciphertextLength, const unsigned char *aesKey, unsigned char *iv);

// Function to convert RSA public key to string
QString rsaPublicKeyToString(RSA *rsaKey);

// Function to convert RSA public key string to RSA structure
RSA* rsaPublicKeyFromString(const QString &publicKeyString);

// Function to convert AES key to string
QString aesKeyToString(const AES_KEY *aesKey);

// Function to convert AES key string to AES_KEY structure
AES_KEY* aesKeyFromString(const QString &aesKeyString);

// Function to generate a random IV
QByteArray generateRandomIV();

// Function to encrypt QString using AES
QByteArray encryptQStringAES(const QString &plaintext, const AES_KEY *aesKey, unsigned char *iv);

// Function to decrypt QByteArray using AES
QString decryptQStringAES(const QByteArray &ciphertext, const AES_KEY *aesKey, unsigned char *iv);

#endif // ENCRYPTION_H
