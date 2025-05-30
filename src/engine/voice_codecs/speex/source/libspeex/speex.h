﻿/* Copyright (C) 2002 Jean-Marc Valin*/
/**
  @file speex.h
  @brief Describes the different modes of the codec
*/
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef SPEEX_H
#define SPEEX_H

#include "speex_bits.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Values allowed for *ctl() requests */

/** Set enhancement on/off (decoder only) */
#define SPEEX_SET_ENH 0
/** Get enhancement state (decoder only) */
#define SPEEX_GET_ENH 1

/*Would be SPEEX_SET_FRAME_SIZE, but it's (currently) invalid*/
/** Obtain frame size used by encoder/decoder */
#define SPEEX_GET_FRAME_SIZE 3

/** Set quality value */
#define SPEEX_SET_QUALITY 4
/** Get current quality setting */
#define SPEEX_GET_QUALITY 5

/** Set sub-mode to use */
#define SPEEX_SET_MODE 6
/** Get current sub-mode in use */
#define SPEEX_GET_MODE 7

/** Set low-band sub-mode to use (wideband only)*/
#define SPEEX_SET_LOW_MODE 8
/** Get current low-band mode in use (wideband only)*/
#define SPEEX_GET_LOW_MODE 9

/** Set high-band sub-mode to use (wideband only)*/
#define SPEEX_SET_HIGH_MODE 10
/** Get current high-band mode in use (wideband only)*/
#define SPEEX_GET_HIGH_MODE 11

/** Set VBR on (1) or off (0) */
#define SPEEX_SET_VBR 12
/** Get VBR status (1 for on, 0 for off) */
#define SPEEX_GET_VBR 13

/** Set quality value for VBR encoding (0-10) */
#define SPEEX_SET_VBR_QUALITY 14
/** Get current quality value for VBR encoding (0-10) */
#define SPEEX_GET_VBR_QUALITY 15

/** Set complexity of the encoder (0-10) */
#define SPEEX_SET_COMPLEXITY 16
/** Get current complexity of the encoder (0-10) */
#define SPEEX_GET_COMPLEXITY 17

/** Set bit-rate used by the encoder (or lower) */
#define SPEEX_SET_BITRATE 18
/** Get current bit-rate used by the encoder or decoder */
#define SPEEX_GET_BITRATE 19

/**Define a handler function for in-band Speex request*/
#define SPEEX_SET_HANDLER 20

/**Define a handler function for in-band user-defined request*/
#define SPEEX_SET_USER_HANDLER 22

/** Set sampling rate used in bit-rate computation */
#define SPEEX_SET_SAMPLING_RATE 24
/** Get sampling rate used in bit-rate computation */
#define SPEEX_GET_SAMPLING_RATE 25

/** Reset the encoder/decoder memories to zero*/
#define SPEEX_RESET_STATE 26

/** Get VBR info (mostly used internally) */
#define SPEEX_GET_RELATIVE_QUALITY 29

/** Set VAD status (1 for on, 0 for off) */
#define SPEEX_SET_VAD 30

/** Get VAD status (1 for on, 0 for off) */
#define SPEEX_GET_VAD 31

/** Set Average Bit-Rate (ABR) to n bits per seconds */
#define SPEEX_SET_ABR 32
/** Get Average Bit-Rate (ABR) setting (in bps) */
#define SPEEX_GET_ABR 33

/** Set DTX status (1 for on, 0 for off) */
#define SPEEX_SET_DTX 34
/** Get DTX status (1 for on, 0 for off) */
#define SPEEX_GET_DTX 35


/* Used internally, not to be used in applications */
/** Used internally*/
#define SPEEX_GET_PI_GAIN 100
/** Used internally*/
#define SPEEX_GET_EXC     101
/** Used internally*/
#define SPEEX_GET_INNOV   102
/** Used internally*/
#define SPEEX_GET_DTX_STATUS   103


/* Preserving compatibility:*/
/** Equivalent to SPEEX_SET_ENH */
#define SPEEX_SET_PF 0
/** Equivalent to SPEEX_GET_ENH */
#define SPEEX_GET_PF 1


/* Values allowed for mode queries */
/** Query the frame size of a mode */
#define SPEEX_MODE_FRAME_SIZE 0

/** Query the size of an encoded frame for a particular sub-mode */
#define SPEEX_SUBMODE_BITS_PER_FRAME 1


/** Number of defined modes in Speex */
#define SPEEX_NB_MODES 3

struct SpeexMode;


/* Prototypes for mode function pointers */

/** Encoder state initialization function */
typedef void *(*encoder_init_func)(struct SpeexMode *mode);

/** Encoder state destruction function */
typedef void (*encoder_destroy_func)(void *st);

/** Main encoding function */
typedef int (*encode_func)(void *state, float *in, SpeexBits *bits);

/** Function for controlling the encoder options */
typedef int (*encoder_ctl_func)(void *state, int request, void *ptr);

/** Decoder state initialization function */
typedef void *(*decoder_init_func)(struct SpeexMode *mode);

/** Decoder state destruction function */
typedef void (*decoder_destroy_func)(void *st);

/** Main decoding function */
typedef int  (*decode_func)(void *state, SpeexBits *bits, float *out);

/** Function for controlling the decoder options */
typedef int (*decoder_ctl_func)(void *state, int request, void *ptr);


/** Query function for a mode */
typedef int (*mode_query_func)(void *mode, int request, void *ptr);

/** Struct defining a Speex mode */ 
typedef struct SpeexMode {
   /** Pointer to the low-level mode data */
   void *mode;

   /** Pointer to the mode query function */
   mode_query_func query;
   
   /** The name of the mode (you should not rely on this to identify the mode)*/
   char *modeName;

   /**ID of the mode*/
   int modeID;

   /**Version number of the bitstream (incremented every time we break
    bitstream compatibility*/
   int bitstream_version;

   /** Pointer to encoder initialization function */
   encoder_init_func enc_init;

   /** Pointer to encoder destruction function */
   encoder_destroy_func enc_destroy;

   /** Pointer to frame encoding function */
   encode_func enc;

   /** Pointer to decoder initialization function */
   decoder_init_func dec_init;

   /** Pointer to decoder destruction function */
   decoder_destroy_func dec_destroy;

   /** Pointer to frame decoding function */
   decode_func dec;

   /** ioctl-like requests for encoder */
   encoder_ctl_func enc_ctl;

   /** ioctl-like requests for decoder */
   decoder_ctl_func dec_ctl;

} SpeexMode;

/**
 * Returns a handle to a newly created Speex encoder state structure. For now, 
 * the "mode" argument can be &nb_mode or &wb_mode . In the future, more modes 
 * may be added. Note that for now if you have more than one channels to 
 * encode, you need one state per channel.
 *
 * @param mode The mode to use (either speex_nb_mode or speex_wb.mode) 
 * @return A newly created encoder
 */
void *speex_encoder_init(SpeexMode *mode);

/** Frees all resources associated to an existing Speex encoder state. 
 * @param state Encoder state to be destroyed */
void speex_encoder_destroy(void *state);

/** Uses an existing encoder state to encode one frame of speech pointed to by
    "in". The encoded bit-stream is saved in "bits".
 @param state Encoder state
 @param in Frame that will be encoded with a +-2^16 range
 @param bits Bit-stream where the data will be written
 */
int speex_encode(void *state, float *in, SpeexBits *bits);

/** Used like the ioctl function to control the encoder parameters
 *
 * @param state Encoder state
 * @param request ioctl-type request (one of the SPEEX_* macros)
 * @param ptr Data exchanged to-from function
 * @return 0 if frame needs not be transmitted (DTX only), 1 otherwise
 */
int speex_encoder_ctl(void *state, int request, void *ptr);


/** Returns a handle to a newly created decoder state structure. For now, 
 * the mode argument can be &nb_mode or &wb_mode . In the future, more modes
 * may be added.  Note that for now if you have more than one channels to
 * decode, you need one state per channel.
 *
 * @param mode Speex mode (one of speex_nb_mode or speex_wb_mode)
 * @return A newly created decoder state
 */ 
void *speex_decoder_init(SpeexMode *mode);

/** Frees all resources associated to an existing decoder state.
 *
 * @param state State to be destroyed
 */
void speex_decoder_destroy(void *state);

/** Uses an existing decoder state to decode one frame of speech from
 * bit-stream bits. The output speech is saved written to out.
 *
 * @param state Decoder state
 * @param bits Bit-stream from which to decode the frame (NULL if the packet was lost)
 * @param out Where to write the decoded frame
 * @return return status (0 for no error, -1 for end of stream, -2 other)
 */
int speex_decode(void *state, SpeexBits *bits, float *out);

/** Used like the ioctl function to control the encoder parameters
 *
 * @param state Decoder state
 * @param request ioctl-type request (one of the SPEEX_* macros)
 * @param ptr Data exchanged to-from function
 * @return 0 for no error, 1 if a terminator is reached, 2 for another error
 */
int speex_decoder_ctl(void *state, int request, void *ptr);


/** Query function for mode information
 *
 * @param mode Speex mode
 * @param request ioctl-type request (one of the SPEEX_* macros)
 * @param ptr Data exchanged to-from function
 */
int speex_mode_query(SpeexMode *mode, int request, void *ptr);


/** Default narrowband mode */
extern SpeexMode speex_nb_mode;

/** Default wideband mode */
extern SpeexMode speex_wb_mode;

/** Default "ultra-wideband" mode */
extern SpeexMode speex_uwb_mode;

/** List of all modes available */
extern SpeexMode *speex_mode_list[SPEEX_NB_MODES];

#ifdef __cplusplus
}
#endif


#endif
