/*
 * TeamSpeak 3 plugin
 *
 * Copyright (c) TeamSpeak Systems GmbH
 */

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_rare_definitions.h"
#include "teamspeak/clientlib_publicdefinitions.h"
#include "ts3_functions.h"
#include "plugin.h"

static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 25
//#define ENV_DEV

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

static char* pluginID = NULL;

#ifdef _WIN32
/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if(WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
		*result = NULL;
		return -1;
	}
	return 0;
}
#endif


#ifdef ENV_DEV
#define ASCHELPSERVERUUID "Sb210P1Jdgea8s7IpNgNqTaPjLE="
#else
#define ASCHELPSERVERUUID "L0tx2WMFvQhXfs+QXdvV9+oLOWA="
#endif


/*********************************** Required functions ************************************/
/*
 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
 */

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
#ifdef _WIN32
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if(!result) {
		const wchar_t* name = L"AscHelpRequiredHeadphones";
		if(wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
			result = "AscHelpRequiredHeadphones";  /* Conversion failed, fallback here */
		}
	}
	return result;
#else
	return "AscHelpRequiredHeadphones";
#endif
}

/* Plugin version */
const char* ts3plugin_version() {
    return "1.0.0";
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "Emilien Cosson <emilien@em-ilien.fr>";
}

/* Plugin description */
const char* ts3plugin_description() {
#ifdef _WIN32
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if (!result) {
		const wchar_t* name = L"Ce plugin permet d'empêcher les utilisateurs n'ayant pas de hauts parleurs branchés ou configurés de rejoindre les salons d'aide sur le serveur Support Ascentia.";
		if (wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
			result = "Ce plugin permet d'empêcher les utilisateurs n'ayant pas de hauts parleurs branchés ou configurés de rejoindre les salons d'aide sur le serveur Support Ascentia.";  /* Conversion failed, fallback here */
		}
	}
	return result;
#else
	return "Ce plugin permet d'empêcher les utilisateurs n'ayant pas de hauts parleurs branchés ou configurés de rejoindre les salons d'aide sur le serveur Support Ascentia.";
#endif
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

/*
 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
 * If the function returns 1 on failure, the plugin will be unloaded again.
 */
int ts3plugin_init() {
    char appPath[PATH_BUFSIZE];
    char resourcesPath[PATH_BUFSIZE];
    char configPath[PATH_BUFSIZE];
	char pluginPath[PATH_BUFSIZE];

    /* Your plugin init code here */
    printf("PLUGIN: init\n");

    /* Example on how to query application, resources and configuration paths from client */
    /* Note: Console client returns empty string for app and resources path */
    ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
    ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
    ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE, pluginID);

	printf("PLUGIN: App path: %s\nResources path: %s\nConfig path: %s\nPlugin path: %s\n", appPath, resourcesPath, configPath, pluginPath);

    return 0;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
	/* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
	 * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
	 * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
    /* Your plugin cleanup code here */
    printf("PLUGIN: shutdown\n");

	/*
	 * Note:
	 * If your plugin implements a settings dialog, it must be closed and deleted here, else the
	 * TeamSpeak client will most likely crash (DLL removed but dialog from DLL code still open).
	 */

	/* Free pluginID if we registered it */
	if(pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

/****************************** Optional functions ********************************/
/*
 * Following functions are optional, if not needed you don't need to implement them.
 */



/*
 * Implement the following three functions when the plugin should display a line in the server/channel/client info.
 * If any of ts3plugin_infoTitle, ts3plugin_infoData or ts3plugin_freeMemory is missing, the info text will not be displayed.
 */

 /* Static title shown in the left column in the info frame */
const char* ts3plugin_infoTitle() {
	return "";
}

/*
 * Dynamic content shown in the right column in the info frame. Memory for the data string needs to be allocated in this
 * function. The client will call ts3plugin_freeMemory once done with the string to release the allocated memory again.
 * Check the parameter "type" if you want to implement this feature only for specific item types. Set the parameter
 * "data" to NULL to have the client ignore the info data.
 */
void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data) {
}

/* Required to release the memory for parameter "data" allocated in ts3plugin_infoData and ts3plugin_initMenus */
void ts3plugin_freeMemory(void* data) {
	free(data);
}

/*
 * Plugin requests to be always automatically loaded by the TeamSpeak 3 client unless
 * the user manually disabled it in the plugin dialog.
 * This function is optional. If missing, no autoload is assumed.
 */
int ts3plugin_requestAutoload() {
	return 0;  /* 1 = request autoloaded, 0 = do not request autoload */
}



/************************** TeamSpeak callbacks ***************************/
/*
 * Following functions are optional, feel free to remove unused callbacks.
 * See the clientlib documentation for details on each function.
 */

/* Clientlib */

void ts3plugin_onClientMoveEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* moveMessage) {
	unsigned int error;

	char* serverUUID;
	if ((error = ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_UNIQUE_IDENTIFIER, &serverUUID)) != ERROR_ok) {
		printf("Erreur lors de la récupération de l'UUID du serveur : %d\n", error);
		return;
	}

	if (strcmp(serverUUID, ASCHELPSERVERUUID) != 0) {
		ts3plugin_freeMemory(serverUUID);
		return;
	}

	ts3plugin_freeMemory(serverUUID);

	int maxClients;
	if ((error = ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, newChannelID, CHANNEL_MAXCLIENTS, &maxClients)) != ERROR_ok) {
		printf("Erreur lors de la récupération de maxclients : %d\n", error);
		return;
	}

	if (maxClients < 0 || maxClients > 5)
		return;

	int clientOutputHardware;
	if ((error = ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, clientID, CLIENT_OUTPUT_HARDWARE, &clientOutputHardware)) != ERROR_ok) {
		printf("Erreur lors de la récupération de l'état du haut-parleur du client : %d\n", error);
		return;
	}

	if (clientOutputHardware != NULL) {
		return;
	}

	if ((error = ts3Functions.requestClientMove(serverConnectionHandlerID, clientID, oldChannelID, "", NULL)) != ERROR_ok) {
		printf("Error moving the client back to the previous channel: %d\n", error);
	}

	if ((error = ts3Functions.requestClientPoke(serverConnectionHandlerID, clientID, "Branchez ou configurez vos hauts-parleurs. [url=https://bit.ly/AscHelpHeadphones][AIDE][/url]", NULL)) != ERROR_ok) {
		printf("Error poking the client: %d\n", error);
	}
}