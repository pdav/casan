/* dtls -- a very basic DTLS implementation
 *
 * Copyright (C) 2011--2013 Olaf Bergmann <bergmann@tzi.org>
 * Copyright (C) 2013 Hauke Mehrtens <hauke@hauke-m.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file dtls.h
 * @brief High level DTLS API and visible structures. 
 */

#ifndef _DTLS_DTLS_H_
#define _DTLS_DTLS_H_

//#include <stdint.h>

#include "t_list.h"
#include "state.h"
#include "peer.h"

#ifdef WITH_CONTIKI
#elif  WITH_ARDUINO
#else
#include <stdlib.h>
#include "uthash.h"
#endif /* WITH_CONTIKI */

#include "alert.h"
#include "crypto.h"
#include "hmac.h"

#include "global.h"
#include "dtls_time.h"

#ifndef DTLSv12
#define DTLS_VERSION 0xfeff	/* DTLS v1.1 */
#else
#define DTLS_VERSION 0xfefd	/* DTLS v1.2 */
#endif

// TODO

/* // test
struct mysocket {
    size_t (*cb_send)(void * data, size_t len);
    size_t (*cb_recv)(void * data, size_t *len);
};

size_t send_smth(struct mysocket msock) {
    return msock.cb_send((void*)"coucou", 6);
}

size_t recv_smth(struct mysocket msock) {
    size_t len;
    void *data;;
    msock.cb_recv(data, &len);
    return len;
}
*/

typedef enum dtls_credentials_type_t {
  DTLS_PSK_HINT, DTLS_PSK_IDENTITY, DTLS_PSK_KEY
} dtls_credentials_type_t;

typedef struct dtls_ecdsa_key_t {
  dtls_ecdh_curve curve;
  const unsigned char *priv_key;	/** < private key as bytes > */
  const unsigned char *pub_key_x;	/** < x part of the public key for the given private key > */
  const unsigned char *pub_key_y;	/** < y part of the public key for the given private key > */
} dtls_ecdsa_key_t;

/** Length of the secret that is used for generating Hello Verify cookies. */
#define DTLS_COOKIE_SECRET_LENGTH 12

struct dtls_context_t;

/**
 * This structure contains callback functions used by tinydtls to
 * communicate with the application. At least the write function must
 * be provided. It is called by the DTLS state machine to send packets
 * over the network. The read function is invoked to deliver decrypted
 * and verfified application data. The third callback is an event
 * handler function that is called when alert messages are encountered
 * or events generated by the library have occured.
 */ 
typedef struct {

  /** 
   * Called from dtls_handle_message() to send DTLS packets over the
   * network. The callback function must use the network interface
   * denoted by session->ifindex to send the data.
   *
   * @param ctx  The current DTLS context.
   * @param session The session object, including the address of the
   *              remote peer where the data shall be sent.
   * @param buf  The data to send.
   * @param len  The actual length of @p buf.
   * @return The callback function must return the number of bytes 
   *         that were sent, or a value less than zero to indicate an 
   *         error.
   */
  int (*write)(struct dtls_context_t *ctx, 
	       session_t *session, uint8 *buf, size_t len);

  /** 
   * Called from dtls_handle_message() deliver application data that was 
   * received on the given session. The data is delivered only after
   * decryption and verification have succeeded. 
   *
   * @param ctx  The current DTLS context.
   * @param session The session object, including the address of the
   *              data's origin. 
   * @param buf  The received data packet.
   * @param len  The actual length of @p buf.
   * @return ignored
   */
  int (*read)(struct dtls_context_t *ctx, 
	       session_t *session, uint8 *buf, size_t len);

  /**
   * The event handler is called when a message from the alert
   * protocol is received or the state of the DTLS session changes.
   *
   * @param ctx     The current dtls context.
   * @param session The session object that was affected.
   * @param level   The alert level or @c 0 when an event ocurred that 
   *                is not an alert. 
   * @param code    Values less than @c 256 indicate alerts, while
   *                @c 256 or greater indicate internal DTLS session changes.
   * @return ignored
   */
  int (*event)(struct dtls_context_t *ctx, session_t *session, 
		dtls_alert_level_t level, unsigned short code);

#ifdef DTLS_PSK
  /**
   * Called during handshake to get information related to the
   * psk key exchange. The type of information requested is
   * indicated by @p type which will be one of DTLS_PSK_HINT,
   * DTLS_PSK_IDENTITY, or DTLS_PSK_KEY. The called function
   * must store the requested item in the buffer @p result of
   * size @p result_length. On success, the function must return
   * the actual number of bytes written to @p result, of a
   * value less than zero on error. The parameter @p desc may
   * contain additional request information (e.g. the psk_identity
   * for which a key is requested when @p type == @c DTLS_PSK_KEY.
   *
   * @param ctx     The current dtls context.
   * @param session The session where the key will be used.
   * @param type    The type of the requested information.
   * @param desc    Additional request information
   * @param desc_len The actual length of desc.
   * @param result  Must be filled with the requested information.
   * @param result_length  Maximum size of @p result.
   * @return The number of bytes written to @p result or a value
   *         less than zero on error.
   */
  int (*get_psk_info)(struct dtls_context_t *ctx,
		      const session_t *session,
		      dtls_credentials_type_t type,
		      const unsigned char *desc, size_t desc_len,
		      unsigned char *result, size_t result_length);

#endif /* DTLS_PSK */

#ifdef DTLS_ECC
  /**
   * Called during handshake to get the server's or client's ecdsa
   * key used to authenticate this server or client in this 
   * session. If found, the key must be stored in @p result and 
   * the return value must be @c 0. If not found, @p result is 
   * undefined and the return value must be less than zero.
   *
   * If ECDSA should not be supported, set this pointer to NULL.
   *
   * Implement this if you want to provide your own certificate to 
   * the other peer. This is mandatory for a server providing ECDSA
   * support and optional for a client. A client doing DTLS client
   * authentication has to implementing this callback.
   *
   * @param ctx     The current dtls context.
   * @param session The session where the key will be used.
   * @param result  Must be set to the key object to used for the given
   *                session.
   * @return @c 0 if result is set, or less than zero on error.
   */
  int (*get_ecdsa_key)(struct dtls_context_t *ctx, 
		       const session_t *session,
		       const dtls_ecdsa_key_t **result);

  /**
   * Called during handshake to check the peer's pubic key in this
   * session. If the public key matches the session and should be
   * considerated valid the return value must be @c 0. If not valid,
   * the return value must be less than zero.
   *
   * If ECDSA should not be supported, set this pointer to NULL.
   *
   * Implement this if you want to verify the other peers public key.
   * This is mandatory for a DTLS client doing based ECDSA
   * authentication. A server implementing this will request the
   * client to do DTLS client authentication.
   *
   * @param ctx          The current dtls context.
   * @param session      The session where the key will be used.
   * @param other_pub_x  x component of the public key.
   * @param other_pub_y  y component of the public key.
   * @return @c 0 if public key matches, or less than zero on error.
   * error codes:
   *   return dtls_alert_fatal_create(DTLS_ALERT_BAD_CERTIFICATE);
   *   return dtls_alert_fatal_create(DTLS_ALERT_UNSUPPORTED_CERTIFICATE);
   *   return dtls_alert_fatal_create(DTLS_ALERT_CERTIFICATE_REVOKED);
   *   return dtls_alert_fatal_create(DTLS_ALERT_CERTIFICATE_EXPIRED);
   *   return dtls_alert_fatal_create(DTLS_ALERT_CERTIFICATE_UNKNOWN);
   *   return dtls_alert_fatal_create(DTLS_ALERT_UNKNOWN_CA);
   */
  int (*verify_ecdsa_key)(struct dtls_context_t *ctx, 
			  const session_t *session,
			  const unsigned char *other_pub_x,
			  const unsigned char *other_pub_y,
			  size_t key_size);
#endif /* DTLS_ECC */
} dtls_handler_t;

/** Holds global information of the DTLS engine. */
typedef struct dtls_context_t {
  unsigned char cookie_secret[DTLS_COOKIE_SECRET_LENGTH];
  clock_time_t cookie_secret_age; /**< the time the secret has been generated */

#ifdef WITH_ARDUINO
  dtls_peer_t *peers;		/**< peer hash map */
  void (*say)(char * txt_to_say);
#elif defined WITH_CONTIKI
  LIST_STRUCT(peers);

  struct etimer retransmit_timer; /**< fires when the next packet must be sent */
#else
  dtls_peer_t *peers;		/**< peer hash map */
#endif /* WITH_CONTIKI */

  LIST_STRUCT(sendqueue);	/**< the packets to send */

  void *app;			/**< application-specific data */

  dtls_handler_t *h;		/**< callback handlers */

  unsigned char readbuf[DTLS_MAX_BUF];
} dtls_context_t;

/** 
 * This function initializes the tinyDTLS memory management and must
 * be called first.
 */
void dtls_init();

/** 
 * Creates a new context object. The storage allocated for the new
 * object must be released with dtls_free_context(). */
dtls_context_t *dtls_new_context(void *app_data);

/** Releases any storage that has been allocated for \p ctx. */
void dtls_free_context(dtls_context_t *ctx);

#define dtls_set_app_data(CTX,DATA) ((CTX)->app = (DATA))
#define dtls_get_app_data(CTX) ((CTX)->app)

/** Sets the callback handler object for @p ctx to @p h. */
static inline void dtls_set_handler(dtls_context_t *ctx, dtls_handler_t *h) {
  ctx->h = h;
}

/**
 * Establishes a DTLS channel with the specified remote peer @p dst.
 * This function returns @c 0 if that channel already exists, a value
 * greater than zero when a new ClientHello message was sent, and
 * a value less than zero on error.
 *
 * @param ctx    The DTLS context to use.
 * @param dst    The remote party to connect to.
 * @return A value less than zero on error, greater or equal otherwise.
 */
int dtls_connect(dtls_context_t *ctx, const session_t *dst);

/**
 * Establishes a DTLS channel with the specified remote peer.
 * This function returns @c 0 if that channel already exists, a value
 * greater than zero when a new ClientHello message was sent, and
 * a value less than zero on error.
 *
 * @param ctx    The DTLS context to use.
 * @param peer   The peer object that describes the session.
 * @return A value less than zero on error, greater or equal otherwise.
 */
int dtls_connect_peer(dtls_context_t *ctx, dtls_peer_t *peer);

/**
 * Closes the DTLS connection associated with @p remote. This function
 * returns zero on success, and a value less than zero on error.
 */
int dtls_close(dtls_context_t *ctx, const session_t *remote);

int dtls_renegotiate(dtls_context_t *ctx, const session_t *dst);

/** 
 * Writes the application data given in @p buf to the peer specified
 * by @p session. 
 * 
 * @param ctx      The DTLS context to use.
 * @param session  The remote transport address and local interface.
 * @param buf      The data to write.
 * @param len      The actual length of @p data.
 * 
 * @return The number of bytes written or @c -1 on error.
 */
int dtls_write(struct dtls_context_t *ctx, session_t *session, 
	       uint8 *buf, size_t len);

/**
 * Checks sendqueue of given DTLS context object for any outstanding
 * packets to be transmitted. 
 *
 * @param context The DTLS context object to use.
 * @param next    If not NULL, @p next is filled with the timestamp
 *  of the next scheduled retransmission, or @c 0 when no packets are
 *  waiting.
 */
void dtls_check_retransmit(dtls_context_t *context, clock_time_t *next);

#define DTLS_COOKIE_LENGTH 16

#define DTLS_CT_CHANGE_CIPHER_SPEC 20
#define DTLS_CT_ALERT              21
#define DTLS_CT_HANDSHAKE          22
#define DTLS_CT_APPLICATION_DATA   23

/** Generic header structure of the DTLS record layer. */
typedef struct __attribute__((__packed__)) {
  uint8 content_type;		/**< content type of the included message */
  uint16 version;		/**< Protocol version */
  uint16 epoch;		        /**< counter for cipher state changes */
  uint48 sequence_number;       /**< sequence number */
  uint16 length;		/**< length of the following fragment */
  /* fragment */
} dtls_record_header_t;

/* Handshake types */

#define DTLS_HT_HELLO_REQUEST        0
#define DTLS_HT_CLIENT_HELLO         1
#define DTLS_HT_SERVER_HELLO         2
#define DTLS_HT_HELLO_VERIFY_REQUEST 3
#define DTLS_HT_CERTIFICATE         11
#define DTLS_HT_SERVER_KEY_EXCHANGE 12
#define DTLS_HT_CERTIFICATE_REQUEST 13
#define DTLS_HT_SERVER_HELLO_DONE   14
#define DTLS_HT_CERTIFICATE_VERIFY  15
#define DTLS_HT_CLIENT_KEY_EXCHANGE 16
#define DTLS_HT_FINISHED            20

/** Header structure for the DTLS handshake protocol. */
typedef struct __attribute__((__packed__)) {
  uint8 msg_type; /**< Type of handshake message  (one of DTLS_HT_) */
  uint24 length;  /**< length of this message */
  uint16 message_seq; 	/**< Message sequence number */
  uint24 fragment_offset;	/**< Fragment offset. */
  uint24 fragment_length;	/**< Fragment length. */
  /* body */
} dtls_handshake_header_t;

/** Structure of the Client Hello message. */
typedef struct __attribute__((__packed__)) {
  uint16 version;	  /**< Client version */
  uint32 gmt_random;	  /**< GMT time of the random byte creation */
  unsigned char random[28];	/**< Client random bytes */
  /* session id (up to 32 bytes) */
  /* cookie (up to 32 bytes) */
  /* cipher suite (2 to 2^16 -1 bytes) */
  /* compression method */
} dtls_client_hello_t;

/** Structure of the Hello Verify Request. */
typedef struct __attribute__((__packed__)) {
  uint16 version;		/**< Server version */
  uint8 cookie_length;	/**< Length of the included cookie */
  uint8 cookie[];		/**< up to 32 bytes making up the cookie */
} dtls_hello_verify_t;  

#if 0
/** 
 * Checks a received DTLS record for consistency and eventually decrypt,
 * verify, decompress and reassemble the contained fragment for 
 * delivery to high-lever clients. 
 * 
 * \param state The DTLS record state for the current session. 
 * \param 
 */
int dtls_record_read(dtls_state_t *state, uint8 *msg, int msglen);
#endif

/** 
 * Handles incoming data as DTLS message from given peer.
 *
 * @param ctx     The dtls context to use.
 * @param session The current session
 * @param msg     The received data
 * @param msglen  The actual length of @p msg.
 * @return A value less than zero on error, zero on success.
 */
int dtls_handle_message(dtls_context_t *ctx, session_t *session,
			uint8 *msg, int msglen);

/**
 * Check if @p session is associated with a peer object in @p context.
 * This function returns a pointer to the peer if found, NULL otherwise.
 *
 * @param context  The DTLS context to search.
 * @param session  The remote address and local interface
 * @return A pointer to the peer associated with @p session or NULL if
 *  none exists.
 */
dtls_peer_t *dtls_get_peer(const dtls_context_t *context,
			   const session_t *session);


#endif /* _DTLS_DTLS_H_ */
