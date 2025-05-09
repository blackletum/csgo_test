﻿//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#if !defined( PROTO_OOB_H )
#define PROTO_OOB_H
#ifdef _WIN32
#pragma once
#endif

// This is used, unless overridden in the registry
#define VALVE_MASTER_ADDRESS "207.173.177.10:27011"

#define PORT_RCON			27015	// Default RCON port, TCP
#define	PORT_MASTER			27011	// Default master port, UDP
#define PORT_CLIENT			27005	// Default client port, UDP/TCP
#define PORT_SERVER			27015	// Default server port, UDP/TCP
#define PORT_HLTV			27020	// Default hltv port
#define PORT_HLTV1			27021	// Default hltv[instance 1] port

#define PORT_X360_RESERVED_FIRST	27026	// X360 reserved port first
#define PORT_X360_RESERVED_LAST		27034	// X360 reserved port last
#ifdef ENABLE_RPT
#define PORT_RPT			27035	// default RPT (remote perf testing) port, TCP
#define PORT_RPT_LISTEN		27036	// RPT connection listener (remote perf testing) port, TCP
#endif // ENABLE_RPT
#define PORT_REPLAY			27040	// Default replay port

// out of band message id bytes

// M = master, S = server, C = client, A = any
// the second character will always be \n if the message isn't a single
// byte long (?? not true anymore?)


// Requesting for full server list from Server Master
#define	A2M_GET_SERVERS			'c'	// no params

// Master response with full server list
#define	M2A_SERVERS				'd'	// + 6 byte IP/Port list.

// Request for full server list from Server Master done in batches
#define A2M_GET_SERVERS_BATCH	'e' // + in532 uniqueID ( -1 for first batch )

// Master response with server list for channel
#define M2A_SERVER_BATCH		'f' // + int32 next uniqueID( -1 for last batch ) + 6 byte IP/Port list.

// Request for MOTD from Server Master  (Message of the Day)
#define	A2M_GET_MOTD			'g'	// no params

// MOTD response Server Master
#define	M2A_MOTD				'h'	// + string 

// Generic Ping Request
#define	A2A_PING				'i'	// respond with an A2A_ACK

// Generic Ack
#define	A2A_ACK					'j'	// general acknowledgement without info

#define C2S_CONNECT				'k'	// client requests to connect

// Print to client console.
#define	A2A_PRINT				'l'	// print a message on client

// info request
#define S2A_INFO_DETAILED		'm'	// New Query protocol, returns dedicated or not, + other performance info.

#define A2S_RESERVE				'n' // reserves this server for specific players for a short period of time.  Fails if not empty.

#define S2A_RESERVE_RESPONSE	'p' // server response to reservation request

// Another user is requesting a challenge value from this machine
// NOTE: this is currently duplicated in SteamClient.dll but for a different purpose,
// so these can safely diverge anytime. SteamClient will be using a different protocol
// to update the master servers anyway.
#define A2S_GETCHALLENGE		'q'	// Request challenge # from another machine

#define A2S_RCON				'r'	// client rcon command

#define A2A_CUSTOM				't'	// a custom command, follow by a string for 3rd party tools


// A user is requesting the list of master servers, auth servers, and titan dir servers from the Client Master server
#define A2M_GETMASTERSERVERS	'v' // + byte (type of request, TYPE_CLIENT_MASTER or TYPE_SERVER_MASTER)

// Master server list response
#define M2A_MASTERSERVERS		'w'	// + byte type + 6 byte IP/Port List

#define A2M_GETACTIVEMODS		'x' // + string Request to master to provide mod statistics ( current usage ).  "1" for first mod.

#define M2A_ACTIVEMODS			'y' // response:  modname\r\nusers\r\nservers

#define M2M_MSG					'z' // Master peering message

// SERVER TO CLIENT/ANY

// Client connection is initiated by requesting a challenge value
//  the server sends this value back
#define S2C_CHALLENGE			'A' // + challenge value

// Server notification to client to commence signon process using challenge value.
#define	S2C_CONNECTION			'B' // no params

// Response to server info requests

// Request for detailed server/rule information.
#define S2A_INFO_GOLDSRC		'm' // Reserved for use by goldsrc servers

#define S2M_GETFILE				'J'	// request module from master
#define M2S_SENDFILE			'K'	// send module to server

#define S2C_REDIRECT			'L'	// + IP x.x.x.x:port, redirect client to other server/proxy 

#define	C2M_CHECKMD5			'M'	// player client asks secure master if Module MD5 is valid
#define M2C_ISVALIDMD5			'N'	// secure servers answer to C2M_CHECKMD5

// MASTER TO SERVER
#define M2A_ACTIVEMODS3			'P' // response:  keyvalues struct of mods
#define A2M_GETACTIVEMODS3		'Q' // get a list of mods and the stats about them

#define S2A_LOGSTRING			'R'	// send a log string
#define S2A_LOGKEY				'S'	// send a log event as key value
#define S2A_LOGSTRING2			'S'	// send a log string including a secret value << this clashes with S2A_LOGKEY that nothing seems to use in CS:GO and is followed by secret value so should be compatible for server ops with their existing tools

#define A2S_SERVERQUERY_GETCHALLENGE		'W'	// Request challenge # from another machine

#define A2S_KEY_STRING		"Source Engine Query" // required postfix to a A2S_INFO query

#define A2M_GET_SERVERS_BATCH2	'1' // New style server query

#define A2M_GETACTIVEMODS2		'2' // New style mod info query

#define C2S_AUTHREQUEST1        '3' // 
#define S2C_AUTHCHALLENGE1      '4' //
#define C2S_AUTHCHALLENGE2      '5' //
#define S2C_AUTHCOMPLETE        '6'
#define C2S_AUTHCONNECT         '7'  // Unused, signals that the client has
									 // authenticated the server

#define C2S_VALIDATE_SESSION    '8'
// #define UNUSED_A2S_LANSEARCH			'C'	 // LAN game details searches
// #define UNUSED_S2A_LANSEARCHREPLY		'F'	 // LAN game details reply

#define S2C_CONNREJECT			'9'  // Special protocol for rejected connections.

#define MAX_OOB_KEYVALUES		600		// max size in bytes for keyvalues included in an OOB msg
#define MAKE_4BYTES( a, b, c, d ) ( ( ((unsigned char)(d)) << 24 ) | ( ((unsigned char)(c)) << 16 ) | ( ((unsigned char)(b)) << 8 ) | ( ((unsigned char)(a)) << 0 ) )

#define A2A_KV_CMD				'?'	// generic KeyValues command [1 byte: version] [version dependent data...]
#define A2A_KV_VERSION			1	// version of generic KeyValues command
									// [4 bytes: header] [4 bytes: replyid] [4 bytes: challenge] [4 bytes: extra] [4 bytes: numbytes] [numbytes: serialized KV]

// These can be owned by Steam after we get rid of this legacy code.
#define S2A_INFO_SRC			'I'	// + Address, hostname, map, gamedir, gamedescription, active players, maxplayers, protocol
#define	S2M_HEARTBEAT			'a'	// + challeange + sequence + active + #channels + channels
#define S2M_HEARTBEAT2			'0' // New style heartbeat
#define	S2M_SHUTDOWN			'b' // no params
#define M2A_CHALLENGE			's'	// + challenge value 
#define M2S_REQUESTRESTART		'O' // HLMaster rejected a server's connection because the server needs to be updated
#define A2S_RULES				'V'	// request rules list from server
#define S2A_RULES				'E' // + number of rules + string key and string value pairs
#define A2S_INFO				'T' // server info request - this must match the Goldsrc engine
#define S2A_PLAYER				'D' // + Playernum, name, frags, /*deaths*/, time on server
#define A2S_PLAYER				'U'	// request player list

#define A2S_PING2				'Y' // new-style minimalist ping request
#define S2A_PING2REPLY			'Z' // new-style minimalist ping reply


// temp hack until we kill the legacy interface
// The new S2A_INFO_SRC packet has a byte at the end that has these bits in it, telling 
// which data follows.
#define S2A_EXTRA_DATA_HAS_GAME_PORT				0x80		// Next 2 bytes include the game port.
#define S2A_EXTRA_DATA_HAS_SPECTATOR_DATA			0x40		// Next 2 bytes include the spectator port, then the spectator server name.
#define S2A_EXTRA_DATA_HAS_GAMETAG_DATA				0x20		// Next bytes are the game tag string
#define S2A_EXTRA_DATA_HAS_STEAMID					0x10		// Next 8 bytes are the steamID
#define S2A_EXTRA_DATA_GAMEID						0x01		// Next 8 bytes are the gameID of the server

#define A2S_RESERVE_CHECK			'!' // check if server reservation cookie is same as the one we are holding
#define S2A_RESERVE_CHECK_RESPONSE	'%' // server response to reservation request

#define A2S_PING					'$'
#define S2A_PING_RESPONSE			'^'

#endif

