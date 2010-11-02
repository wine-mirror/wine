/*
 *    Gameux library private header
 *
 * Copyright (C) 2010 Mariusz Pluciński
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

extern HRESULT GameExplorer_create(IUnknown* pUnkOuter, IUnknown **ppObj);
extern HRESULT GameStatistics_create(IUnknown* pUnkOuter, IUnknown **ppObj);

/*******************************************************************************
 * Helper functions and structures
 *
 * These are helper function and structures, which  are used widely in gameux
 * implementation. Details about usage and place of implementation is
 * in description of each function/strucutre.
 */

/*******************************************************************************
 * struct GAMEUX_GAME_DATA
 *
 * Structure which contains data about single game. It is used to transfer
 * data inside of gameux module in various places.
 */
struct GAMEUX_GAME_DATA
{
    LPWSTR sGDFBinaryPath;          /* path to binary containing GDF */
    LPWSTR sGameInstallDirectory;   /* directory passed to AddGame/InstallGame methods */
    GAME_INSTALL_SCOPE installScope;/* game's installation scope */
    GUID guidInstanceId;            /* game installation instance identifier */
    GUID guidApplicationId;         /* game's application identifier */
    BSTR bstrName;                  /* game's title */
    BSTR bstrDescription;           /* game's description */
};
/*******************************************************************************
 * GAMEUX_initGameData
 *
 * Initializes GAME_DATA structure fields with proper values. Should be
 * called always before first usage of this structure. Implemented in gameexplorer.c
 *
 * Parameters:
 *  GameData                        [I/O]   pointer to structure to initialize
 */
void GAMEUX_initGameData(struct GAMEUX_GAME_DATA *GameData);
/*******************************************************************************
 * GAMEUX_uninitGameData
 *
 * Properly frees all data stored or pointed by fields of GAME_DATA structure.
 * Should be called before freeing this structure. Implemented in gameexplorer.c
 *
 * Parameters:
 *  GameData                        [I/O]   pointer to structure to uninitialize
 */
void GAMEUX_uninitGameData(struct GAMEUX_GAME_DATA *GameData);
/*******************************************************************************
 *  GAMEUX_RegisterGame
 *
 * Helper function. Registers game associated with given GDF binary in
 * Game Explorer. Implemented in gameexplorer.c
 *
 * Parameters:
 *  sGDFBinaryPath                  [I]     path to binary containing GDF file in
 *                                          resources
 *  sGameInstallDirectory           [I]     path to directory, where game installed
 *                                          it's files.
 *  installScope                    [I]     scope of game installation
 *  pInstanceID                     [I/O]   pointer to game instance identifier.
 *                                          If pointing to GUID_NULL, then new
 *                                          identifier will be generated automatically
 *                                          and returned via this parameter
 */
HRESULT WINAPI GAMEUX_RegisterGame(LPCWSTR sGDFBinaryPath,
        LPCWSTR sGameInstallDirectory,
        GAME_INSTALL_SCOPE installScope,
        GUID *pInstanceID);
/*******************************************************************************
 * GAMEUX_FindGameInstanceId
 *
 * Helper function. Searches for instance identifier of given game in given
 * installation scope. Implemented in gameexplorer.c
 *
 * Parameters:
 *  sGDFBinaryPath                          [I]     path to binary containing GDF
 *  installScope                            [I]     game install scope to search in
 *  pInstanceId                             [O]     instance identifier of given game
 *
 * Returns:
 *  S_OK                    id was returned properly
 *  S_FALSE                 id was not found in the registry
 *  E_OUTOFMEMORY           problem while memory allocation
 */
HRESULT GAMEUX_FindGameInstanceId(
        LPCWSTR sGDFBinaryPath,
        GAME_INSTALL_SCOPE installScope,
        GUID* pInstanceId);
/*******************************************************************************
 * GAMEUX_buildGameRegistryPath
 *
 * Helper function, builds registry path to key, where game's data are stored.
 * Implemented in gameexplorer.c
 *
 * Parameters:
 *  installScope                [I]     the scope which was used in AddGame/InstallGame call
 *  gameInstanceId              [I]     game instance GUID. If NULL, then only
 *                                      path to scope will be returned
 *  lpRegistryPath              [O]     pointer which will receive address to string
 *                                      containing expected registry path. Path
 *                                      is relative to HKLM registry key. It
 *                                      must be freed by calling HeapFree(GetProcessHeap(), 0, ...)
 *
 * Name of game's registry key always follows patterns below:
 *  When game is installed for current user only (installScope is GIS_CURRENT_USER):
 *      HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\
 *          GameUX\[user's security ID]\[game instance ID]
 *
 *  When game is installed for all users (installScope is GIS_ALL_USERS):
 *      HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\
 *          GameUX\Games\[game instance ID]
 *
 *
 */
HRESULT GAMEUX_buildGameRegistryPath(GAME_INSTALL_SCOPE installScope,
        LPCGUID gameInstanceId,
        LPWSTR* lpRegistryPath);
