extern int WIN32_LastError;

/* ERROR_UNKNOWN is a placeholder for error conditions which haven't
 * been tested yet so we're not exactly sure what will be returned.
 * All instances of ERROR_UNKNOWN should be tested under Win95/NT
 * and replaced.
 */
#define     ERROR_UNKNOWN               99999

#define     ERROR_INVALID_HANDLE        6
#define     ERROR_INVALID_PARAMETER     87
#define     ERROR_CALL_NOT_IMPLEMENTED  120
