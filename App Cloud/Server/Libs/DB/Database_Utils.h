#ifndef DATABASE_UTILS_H
#define DATABASE_UTILS_H

#include<stdlib.h> // Needed for fprintf
#include<stdbool.h> // Needed for boolean values
#include<libpq-fe.h> // Header file for libpq (PostgreSQL C client library)
#include<string.h> // Needed for strlen

// Function which authenticates the user credentials
bool authenticate_user(const char *username, const char *pswd);

// Function which register the user credentials inside the database
bool register_user(const char *username, const char *pswd);

#endif
