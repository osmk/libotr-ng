#include <string.h>

#include "serialize.h"

static int
serialize_int(uint8_t *target, const uint64_t data, const int offset) {
  int i;
  int shift = offset * 8;

  for(i = 0; i < offset; i++) {
    shift -= 8;
    target[i] = data >> shift;
  }

  return offset;
}

int
serialize_uint64(uint8_t *dst, const uint64_t data) {
  return serialize_int(dst, data, sizeof(uint64_t));
}

int
serialize_uint32(uint8_t *dst, const uint32_t data) {
  return serialize_int(dst, data, sizeof(uint32_t));
}

int
serialize_uint16(uint8_t *dst, const uint16_t data) {
  return serialize_int(dst, data, sizeof(uint16_t));
}

int
serialize_uint8(uint8_t *dst, const uint8_t data) {
  return serialize_int(dst, data, sizeof(uint8_t));
}

int
serialize_bytes_array(uint8_t *target, const uint8_t data[], int len) {
  memcpy(target, data, len);
  return len;
}

int
serialize_mpi(uint8_t *dst, const uint8_t *data, uint32_t len) {
  uint8_t *cursor = dst;

  if (data == NULL) {
    len = 0;
  }

  cursor += serialize_uint32(cursor, len);
  cursor += serialize_bytes_array(cursor, data, len);

  return cursor - dst;
}

int
serialize_ec_public_key(uint8_t *dst, const ec_public_key_t pub) {
  ec_public_key_serialize(dst, sizeof(ec_public_key_t), pub);
  return sizeof(ec_public_key_t);
}

int
serialize_dh_public_key(uint8_t *dst, const dh_public_key_t pub) {
  uint8_t buf[DH3072_MOD_LEN_BYTES] = {0}; //TODO: should this be cleared?
  size_t written = dh_mpi_serialize(buf, DH3072_MOD_LEN_BYTES, pub);
  return serialize_mpi(dst, buf, written);
}
