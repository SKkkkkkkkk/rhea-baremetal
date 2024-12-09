/*
 * Copyright (c) 2024, M2Semi. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CLCI_ERRNO_H
#define CLCI_ERRNO_H


/*
 * Return Codes
 */

/*! Success */
#define CLCI_SUCCESS         0

/*! Invalid parameter(s) */
#define CLCI_E_PARAM         -1

/*! Invalid alignment */
#define CLCI_E_ALIGN         -2

/*! Invalid size */
#define CLCI_E_SIZE          -3

/*! Invalid handler or callback */
#define CLCI_E_HANDLER       -4

/*! Invalid access or permission denied */
#define CLCI_E_ACCESS        -5

/*! Value out of range */
#define CLCI_E_RANGE         -6

/*! Operation timed out */
#define CLCI_E_TIMEOUT       -7

/*! Memory allocation failed */
#define CLCI_E_NOMEM         -8

/*! Invalid power state */
#define CLCI_E_PWRSTATE      -9

/*! Not supported or disabled */
#define CLCI_E_SUPPORT       -10

/*! Device error */
#define CLCI_E_DEVICE        -11

/*! Handler or resource busy */
#define CLCI_E_BUSY          -12

/*! Unexpected or invalid data */
#define CLCI_E_DATA          -13

/*! Invalid state for the device or component */
#define CLCI_E_STATE         -14

/*! Accessing an uninitialized resource */
#define CLCI_E_INIT          -15

/*! Configuration overwritten */
#define CLCI_E_OVERWRITTEN   -16

/*! Unrecoverable error */
#define CLCI_E_PANIC         -17

/*! Data verification error */
#define CLCI_E_VERIFY_FAIL    -18

/*! Unkown error */
#define CLCI_E_UNKNOWN        -19

#endif /* CLCI_ERRNO_H */
