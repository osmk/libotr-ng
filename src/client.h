/*
 *  This file is part of the Off-the-Record Next Generation Messaging
 *  library (libotr-ng).
 *
 *  Copyright (C) 2016-2018, the libotr-ng contributors.
 *
 *  This library is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OTRNG_CLIENT_H
#define OTRNG_CLIENT_H

#ifndef S_SPLINT_S
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wstrict-prototypes"
#include <libotr/context.h>
#pragma clang diagnostic pop
#endif

#include "list.h"
#include "otrng.h"
#include "prekey_manager.h"
#include "shared.h"

// TODO: @client REMOVE
typedef struct otrng_conversation_s {
  void *conversation_id; /* Data in the messaging application context that
                          represents a conversation and should map directly to
                          it. For example, in libpurple-based apps (like
                          Pidgin) this could be a PurpleConversation */

  char *recipient;
  otrng_s *conn;
} otrng_conversation_s;

typedef struct otrng_client_id_s {
  const char *protocol;
  const char *account;
} otrng_client_id_s;

/* A client handle messages from/to a sender to/from multiple recipients. */
typedef struct otrng_client_s {
  list_element_s *conversations;

  otrng_client_id_s client_id;

  struct otrng_global_state_s *global_state;
  otrng_keypair_s *keypair;
  otrng_public_key *forging_key;

  // TODO: @client One or many?
  otrng_client_profile_s *client_profile;
  otrng_client_profile_s *exp_client_profile;
  otrng_prekey_profile_s *prekey_profile;
  otrng_prekey_profile_s *exp_prekey_profile;
  list_element_s *our_prekeys; /* prekey_message_s */

  unsigned int max_stored_msg_keys;
  unsigned int max_published_prekey_msg;
  unsigned int minimum_stored_prekey_msg;

  uint64_t profiles_extra_valid_time;
  uint64_t client_profile_exp_time;
  uint64_t prekey_profile_exp_time;
  uint64_t profiles_buffer_time;

  uint32_t fragments_exp_time;

  otrng_bool (*should_heartbeat)(long last_sent);
  size_t padding;

  /* This flag will be set when there is anything that should be published
     to prekey servers */
  otrng_bool should_publish;
  otrng_bool is_publishing;
  uint32_t prekey_msgs_num_to_publish;

  // OtrlPrivKey *privkeyv3; // ???
  // otrng_instag_s *instag; // TODO: @client Store the instance tag here rather
  // than use v3 User State as a store for instance tags

  otrng_known_fingerprints_s *fingerprints;

  /* Contains the prekey manager if prekey management has been enabled.
     It is NOT safe to assume that this will be non-null - it is a
     plugins/clients responsibility to ensure that the prekey management system
     has been initialized before calling any functions that use this field.
  */
  // TODO: @prekey - this should be freed
  /*@null@*/ otrng_prekey_manager_s *prekey_manager;
} otrng_client_s;

API otrng_client_s *otrng_client_new(const otrng_client_id_s client_id);

API void otrng_client_free(otrng_client_s *client);

API /*@null@*/ otrng_conversation_s *
otrng_client_get_conversation(int force_create, const char *recipient,
                              otrng_client_s *client);

API otrng_bool otrng_conversation_is_encrypted(otrng_conversation_s *conv);

API otrng_bool otrng_conversation_is_finished(otrng_conversation_s *conv);

API /*@null@*/ char *otrng_client_query_message(const char *recipient,
                                                const char *msg,
                                                otrng_client_s *client);

API /*@null@*/ char *otrng_client_identity_message(const char *recipient,
                                                   otrng_client_s *client);

API /*@null@*/ char *otrng_client_init_message(const char *recipient,
                                               const char *msg,
                                               otrng_client_s *client);

API otrng_result otrng_client_send(char **new_msg, const char *msg,
                                   const char *recipient,
                                   otrng_client_s *client);

API otrng_result otrng_client_send_non_interactive_auth(
    char **new_msg, const prekey_ensemble_s *ensemble, const char *recipient,
    otrng_client_s *client);

API otrng_result otrng_client_send_fragment(otrng_message_to_send_s **new_msg,
                                            const char *msg, int mms,
                                            const char *recipient,
                                            otrng_client_s *client);

API otrng_result otrng_client_smp_start(char **to_send, const char *recipient,
                                        const unsigned char *question,
                                        const size_t q_len,
                                        const unsigned char *secret,
                                        size_t secret_len,
                                        otrng_client_s *client);

API otrng_result otrng_client_smp_respond(char **to_send, const char *recipient,
                                          const unsigned char *secret,
                                          size_t secret_len,
                                          otrng_client_s *client);

API otrng_result otrng_client_smp_abort(char **to_send, const char *recipient,
                                        otrng_client_s *client);

API otrng_result otrng_client_receive(char **new_msg, char **to_display,
                                      const char *msg, const char *recipient,
                                      otrng_client_s *client,
                                      otrng_bool *should_ignore);

API otrng_result otrng_client_disconnect(char **new_msg, const char *recipient,
                                         otrng_client_s *client);

INTERNAL void otrng_client_expire_session(otrng_conversation_s *conv);

INTERNAL void otrng_client_expire_sessions(otrng_client_s *client);

/**
 * @brief Expires old fragments based on the threshold set in the client struct
 *
 *  @params
 *  [client] The otrng client instance.
 *
 * @return 0 if success, 2 if any error happened.
 *
 * @details Details around this function if any
 **/
INTERNAL otrng_result otrng_client_expire_fragments(otrng_client_s *client);

API otrng_result otrng_client_get_our_fingerprint(otrng_fingerprint fp,
                                                  const otrng_client_s *client);

INTERNAL void otrng_client_store_my_prekey_message(prekey_message_s *msg,
                                                   otrng_client_s *client);

API /*@null@*/ prekey_message_s **
otrng_client_build_prekey_messages(uint8_t num_messages,
                                   otrng_client_s *client);

INTERNAL OtrlPrivKey *
otrng_client_get_private_key_v3(const otrng_client_s *client);

INTERNAL otrng_keypair_s *otrng_client_get_keypair_v4(otrng_client_s *client);

INTERNAL otrng_result otrng_client_add_private_key_v4(
    otrng_client_s *client, const uint8_t sym[ED448_PRIVATE_BYTES]);

INTERNAL otrng_public_key *otrng_client_get_forging_key(otrng_client_s *client);

INTERNAL void otrng_client_ensure_forging_key(otrng_client_s *client);

INTERNAL otrng_result otrng_client_add_forging_key(
    otrng_client_s *client, const otrng_public_key forging_key);

API otrng_client_profile_s *
otrng_client_get_client_profile(otrng_client_s *client);

API /*@null@*/ otrng_client_profile_s *
otrng_client_build_default_client_profile(otrng_client_s *client);

API otrng_result otrng_client_add_client_profile(
    otrng_client_s *client, const otrng_client_profile_s *profile);

API const otrng_client_profile_s *
otrng_client_get_exp_client_profile(otrng_client_s *client);

API otrng_result otrng_client_add_exp_client_profile(
    otrng_client_s *client, const otrng_client_profile_s *exp_profile);

API otrng_prekey_profile_s *
otrng_client_get_prekey_profile(otrng_client_s *client);

API /*@null@*/ otrng_prekey_profile_s *
otrng_client_build_default_prekey_profile(otrng_client_s *client);

API otrng_result otrng_client_add_prekey_profile(
    otrng_client_s *client, const otrng_prekey_profile_s *profile);

API /*@null@*/ const otrng_prekey_profile_s *
otrng_client_get_exp_prekey_profile(otrng_client_s *client);

API otrng_result otrng_client_add_exp_prekey_profile(
    otrng_client_s *client, const otrng_prekey_profile_s *exp_profile);

INTERNAL unsigned int otrng_client_get_instance_tag(otrng_client_s *client);

INTERNAL otrng_result otrng_client_add_instance_tag(otrng_client_s *client,
                                                    unsigned int instag);

INTERNAL /*@null@*/ const prekey_message_s *
otrng_client_get_prekey_by_id(uint32_t id, const otrng_client_s *client);

INTERNAL void
otrng_client_delete_my_prekey_message_by_id(uint32_t id,
                                            otrng_client_s *client);

API void otrng_client_set_padding(size_t granularity, otrng_client_s *client);

API void otrng_client_set_max_stored_msg_keys(unsigned int max_stored_msg_keys,
                                              otrng_client_s *client);

API void otrng_client_state_set_max_published_prekey_msg(
    unsigned int max_published_prekey_msg, otrng_client_s *client);

API void otrng_client_state_set_minimum_stored_prekey_msg(
    unsigned int minimum_stored_prekey_msg, otrng_client_s *client);

API void
otrng_client_set_profiles_extra_valid_time(uint64_t profiles_extra_valid_time,
                                           otrng_client_s *client);

API void
otrng_client_set_client_profile_exp_time(uint64_t client_profile_exp_time,
                                         otrng_client_s *client);

API uint64_t otrng_client_get_prekey_profile_exp_time(otrng_client_s *client);

API void
otrng_client_set_prekey_profile_exp_time(uint64_t prekey_profile_exp_time,
                                         otrng_client_s *client);

API void otrng_client_start_publishing(otrng_client_s *client);
API otrng_bool otrng_client_should_publish(otrng_client_s *client);
API void otrng_client_failed_published(otrng_client_s *client);
API void otrng_client_published(otrng_client_s *client);

/* tstatic int v3_privkey_generate(otrng_client_s *client, FILE *privf); */

/* tstatic int v3_instag_generate(otrng_client_s *client, FILE *privf); */

#ifdef DEBUG_API

API void otrng_client_debug_print(FILE *, int, otrng_client_s *);

API void otrng_conversation_debug_print(FILE *, int, otrng_conversation_s *);

/* API void otrng_client_debug_print(FILE *, int, otrng_client_s *); */

#endif

#ifdef OTRNG_CLIENT_PRIVATE

tstatic uint64_t
otrng_client_get_client_profile_exp_time(otrng_client_s *client);

#endif

#endif
