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

#include <string.h>

#define OTRNG_SERIALIZE_PRIVATE

#include "alloc.h"
#include "serialize.h"

INTERNAL size_t serialize_uint(uint8_t *target, const uint64_t data,
                               const size_t offset) {
  size_t i;
  size_t shift = offset;

  for (i = 0; i < offset; i++) {
    shift--;
    target[i] = (data >> shift * 8) & 0xFF;
  }

  return offset;
}

INTERNAL size_t otrng_serialize_uint64(uint8_t *destination,
                                       const uint64_t data) {
  return serialize_uint(destination, data, sizeof(uint64_t));
}

INTERNAL size_t otrng_serialize_uint32(uint8_t *destination,
                                       const uint32_t data) {
  return serialize_uint(destination, data, sizeof(uint32_t));
}

INTERNAL size_t otrng_serialize_uint8(uint8_t *destination,
                                      const uint8_t data) {
  return serialize_uint(destination, data, sizeof(uint8_t));
}

INTERNAL size_t otrng_serialize_uint16(uint8_t *destination,
                                       const uint16_t data) {
  return serialize_uint(destination, data, sizeof(uint16_t));
}

INTERNAL size_t otrng_serialize_bytes_array(uint8_t *target,
                                            const uint8_t *data, size_t len) {
  if (!data) {
    return 0;
  }

  // this is just a memcpy thar returns the ammount copied for convenience
  memcpy(target, data, len);
  return len;
}

INTERNAL size_t otrng_serialize_data(uint8_t *destination, const uint8_t *data,
                                     size_t len) {
  uint8_t *cursor = destination;

  cursor += otrng_serialize_uint32(cursor, len);
  cursor += otrng_serialize_bytes_array(cursor, data, len);

  return cursor - destination;
}

INTERNAL size_t otrng_serialize_mpi(uint8_t *destination,
                                    const otrng_mpi_s *mpi) {
  return otrng_serialize_data(destination, mpi->data, mpi->len);
}

INTERNAL int otrng_serialize_ec_point(uint8_t *destination,
                                      const ec_point_t point) {
  if (!otrng_ec_point_encode(destination, ED448_POINT_BYTES, point)) {
    return 0;
  };

  return ED448_POINT_BYTES;
}

INTERNAL size_t otrng_serialize_ec_scalar(uint8_t *destination,
                                          const ec_scalar_t scalar) {
  otrng_ec_scalar_encode(destination, scalar);
  return ED448_SCALAR_BYTES;
}

// Serializes a DH MPI as an OTR MPI data type
INTERNAL otrng_result otrng_serialize_dh_mpi_otr(uint8_t *destination,
                                                 size_t destinationlen,
                                                 size_t *written,
                                                 const dh_mpi_t mpi) {
  uint8_t buf[DH3072_MOD_LEN_BYTES];
  size_t w = 0;
  otrng_mpi_s otr_mpi;

  memset(buf, 0, DH3072_MOD_LEN_BYTES);

  if (destinationlen < DH_MPI_MAX_BYTES) {
    return OTRNG_ERROR;
  }

  /* From gcrypt MPI */

  if (!otrng_dh_mpi_serialize(buf, DH3072_MOD_LEN_BYTES, &w, mpi)) {
    return OTRNG_ERROR;
  }

  // To OTR MPI
  otrng_mpi_set(&otr_mpi, buf, w);
  w = otrng_serialize_mpi(destination, &otr_mpi);
  free(otr_mpi.data);

  if (written) {
    *written = w;
  }

  return OTRNG_SUCCESS;
}

// TODO: REMOVE THIS
INTERNAL otrng_result otrng_serialize_dh_public_key(uint8_t *destination,
                                                    size_t destinationlen,
                                                    size_t *written,
                                                    const dh_public_key_t pub) {
  return otrng_serialize_dh_mpi_otr(destination, destinationlen, written, pub);
}

INTERNAL size_t otrng_serialize_public_key(uint8_t *destination,
                                           const otrng_public_key_t pub) {
  uint8_t *cursor = destination;
  cursor += otrng_serialize_uint16(cursor, ED448_PUBKEY_TYPE);
  cursor += otrng_serialize_ec_point(cursor, pub);

  return cursor - destination;
}

INTERNAL size_t otrng_serialize_forging_key(uint8_t *destination,
                                            const otrng_public_key_t pub) {
  uint8_t *cursor = destination;
  cursor += otrng_serialize_uint16(cursor, ED448_FORGINGKEY_TYPE);
  cursor += otrng_serialize_ec_point(cursor, pub);

  return cursor - destination;
}

INTERNAL size_t otrng_serialize_shared_prekey(
    uint8_t *destination, const otrng_shared_prekey_pub_t shared_prekey) {
  uint8_t *cursor = destination;
  cursor += otrng_serialize_uint16(cursor, ED448_SHARED_PREKEY_TYPE);
  cursor += otrng_serialize_ec_point(cursor, shared_prekey);

  return cursor - destination;
}

INTERNAL size_t otrng_serialize_ring_sig(uint8_t *destination,
                                         const ring_sig_s *proof) {
  uint8_t *cursor = destination;

  cursor += otrng_serialize_ec_scalar(cursor, proof->c1);
  cursor += otrng_serialize_ec_scalar(cursor, proof->r1);
  cursor += otrng_serialize_ec_scalar(cursor, proof->c2);
  cursor += otrng_serialize_ec_scalar(cursor, proof->r2);
  cursor += otrng_serialize_ec_scalar(cursor, proof->c3);
  cursor += otrng_serialize_ec_scalar(cursor, proof->r3);

  return cursor - destination;
}

INTERNAL uint8_t *otrng_serialize_old_mac_keys(list_element_s *old_mac_keys) {
  size_t num_mac_keys = otrng_list_len(old_mac_keys);
  size_t serlen = num_mac_keys * MAC_KEY_BYTES;
  uint8_t *ser_mac_keys;
  unsigned int i;

  if (serlen == 0) {
    return NULL;
  }

  ser_mac_keys = otrng_xmalloc(serlen);

  for (i = 0; i < num_mac_keys; i++) {
    list_element_s *last = otrng_list_get_last(old_mac_keys);
    memcpy(ser_mac_keys + i * MAC_KEY_BYTES, last->data, MAC_KEY_BYTES);
    old_mac_keys = otrng_list_remove_element(last, old_mac_keys);
    otrng_list_free_full(last);
  }

  otrng_list_free_nodes(old_mac_keys);

  return ser_mac_keys;
}

INTERNAL size_t otrng_serialize_phi(uint8_t *destination,
                                    const char *shared_session_state,
                                    const char *init_message,
                                    uint16_t sender_instance_tag,
                                    uint16_t receiver_instance_tag) {
  uint8_t *cursor = destination;

  if (sender_instance_tag < receiver_instance_tag) {
    cursor += otrng_serialize_uint16(cursor, sender_instance_tag);
    cursor += otrng_serialize_uint16(cursor, receiver_instance_tag);
  } else {
    cursor += otrng_serialize_uint16(cursor, receiver_instance_tag);
    cursor += otrng_serialize_uint16(cursor, sender_instance_tag);
  }

  cursor += otrng_serialize_data(cursor, (const uint8_t *)shared_session_state,
                                 strlen(shared_session_state));
  cursor += otrng_serialize_data(cursor, (const uint8_t *)init_message,
                                 init_message ? strlen(init_message) : 0);

  return cursor - destination;
}
