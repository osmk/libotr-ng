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

#include <glib.h>

#include "test_helpers.h"

#include "auth.h"
#include "deserialize.h"
#include "random.h"

static void test_rsig_calculate_c() {
  const char *message = "hey";
  uint8_t expected_c[ED448_SCALAR_BYTES] = {
      0xe3, 0xec, 0x15, 0x07, 0xb3, 0x3b, 0x63, 0x6c, 0xd1, 0x5a, 0x77, 0x0a,
      0x7e, 0xd7, 0xfd, 0xd8, 0xf2, 0x03, 0xea, 0x76, 0x66, 0xe1, 0x85, 0x3a,
      0x44, 0xae, 0x49, 0x32, 0x1a, 0xb0, 0x82, 0x44, 0x6e, 0xc1, 0x0a, 0x15,
      0x70, 0xab, 0x86, 0xf6, 0xae, 0xfa, 0x23, 0xb0, 0xba, 0x2a, 0x8e, 0x2d,
      0x01, 0xe8, 0x70, 0xd4, 0x8a, 0x4c, 0x8f, 0x0b,
  };

  otrng_keypair_s a1, a2, a3, t1, t2, t3;
  uint8_t sym1[ED448_PRIVATE_BYTES] = {1}, sym2[ED448_PRIVATE_BYTES] = {2},
          sym3[ED448_PRIVATE_BYTES] = {3}, sym4[ED448_PRIVATE_BYTES] = {4},
          sym5[ED448_PRIVATE_BYTES] = {5}, sym6[ED448_PRIVATE_BYTES] = {6};

  otrng_keypair_generate(&a1, sym1);
  otrng_keypair_generate(&a2, sym2);
  otrng_keypair_generate(&a3, sym3);
  otrng_keypair_generate(&t1, sym4);
  otrng_keypair_generate(&t2, sym5);
  otrng_keypair_generate(&t3, sym6);

  goldilocks_448_scalar_p c;
  otrng_rsig_calculate_c_with_usage_and_domain(
      OTRNG_PROTOCOL_USAGE_AUTH, OTRNG_PROTOCOL_DOMAIN_SEPARATION, c, a1.pub,
      a2.pub, a3.pub, t1.pub, t2.pub, t3.pub, (const uint8_t *)message,
      strlen(message));

  uint8_t serialized_c[ED448_SCALAR_BYTES] = {0};
  goldilocks_448_scalar_encode(serialized_c, c);
  otrng_assert_cmpmem(expected_c, serialized_c, ED448_SCALAR_BYTES);
}

static void test_rsig_auth() {
  const char *message = "hi";

  otrng_keypair_s p1, p2, p3;
  uint8_t sym1[ED448_PRIVATE_BYTES] = {0}, sym2[ED448_PRIVATE_BYTES] = {0},
          sym3[ED448_PRIVATE_BYTES] = {0};

  random_bytes(sym1, ED448_PRIVATE_BYTES);
  random_bytes(sym2, ED448_PRIVATE_BYTES);
  random_bytes(sym3, ED448_PRIVATE_BYTES);

  otrng_keypair_generate(&p1, sym1);
  otrng_keypair_generate(&p2, sym2);
  otrng_keypair_generate(&p3, sym3);

  ring_sig_s dst;
  otrng_assert_is_error(
      otrng_rsig_authenticate(&dst, p1.priv, p1.pub, p2.pub, p3.pub, p2.pub,
                              (unsigned char *)message, strlen(message)));

  otrng_assert_is_error(
      otrng_rsig_authenticate(&dst, p1.priv, p1.pub, p1.pub, p3.pub, p1.pub,
                              (unsigned char *)message, strlen(message)));

  otrng_assert_is_success(
      otrng_rsig_authenticate(&dst, p1.priv, p1.pub, p1.pub, p2.pub, p3.pub,
                              (unsigned char *)message, strlen(message)));

  otrng_assert(otrng_rsig_verify(&dst, p1.pub, p2.pub, p3.pub,
                                 (unsigned char *)message, strlen(message)));

  otrng_assert_is_success(
      otrng_rsig_authenticate(&dst, p1.priv, p1.pub, p3.pub, p1.pub, p2.pub,
                              (unsigned char *)message, strlen(message)));

  otrng_assert(otrng_rsig_verify(&dst, p3.pub, p1.pub, p2.pub,
                                 (unsigned char *)message, strlen(message)));
}

static void test_rsig_compatible_with_prekey_server() {
  otrng_keypair_s p1, p2, p3;

  // Copied from
  uint8_t p1_priv[ED448_SCALAR_BYTES] = {
      0xa1, 0x56, 0xc,  0x5f, 0x84, 0xf5, 0x4a, 0x80, 0x7e, 0x6c, 0xa9, 0xc5,
      0xda, 0xfe, 0x37, 0xcc, 0xe1, 0xf7, 0x84, 0x50, 0x7a, 0x10, 0x68, 0x69,
      0x26, 0x4a, 0xca, 0x59, 0x7a, 0x3e, 0xec, 0xb9, 0x35, 0x42, 0x13, 0x3c,
      0xce, 0xd0, 0x21, 0x88, 0xa1, 0xec, 0x59, 0x14, 0x60, 0x4b, 0xdf, 0x84,
      0x51, 0xdf, 0x5e, 0x6,  0x1d, 0x57, 0xa4, 0x23};

  uint8_t p1_pub[ED448_POINT_BYTES] = {
      0x33, 0x70, 0x3c, 0xf8, 0xe,  0xfb, 0x20, 0x60, 0xb9, 0xf7, 0x10, 0x8b,
      0x6c, 0xa2, 0x9,  0x86, 0x6f, 0x44, 0x7b, 0x12, 0x8,  0x22, 0xd,  0x9d,
      0x63, 0xa6, 0x6c, 0xc5, 0x5f, 0x2c, 0x1c, 0x83, 0x44, 0x7a, 0x96, 0x97,
      0x7e, 0x3a, 0x39, 0xcb, 0x22, 0xe6, 0x19, 0xbd, 0xca, 0x71, 0x7d, 0x15,
      0x2,  0x4f, 0x6,  0xfd, 0xf7, 0x8f, 0xb1, 0x6,  0x80};

  otrng_ec_scalar_decode(p1.priv, p1_priv);
  otrng_ec_point_decode(p1.pub, p1_pub);

  uint8_t p2_priv[ED448_SCALAR_BYTES] = {
      0x39, 0xbf, 0x76, 0xb3, 0xc5, 0x3e, 0x36, 0xc9, 0x30, 0x51, 0xcc, 0x4c,
      0x59, 0xa3, 0x8a, 0x4d, 0x3c, 0x9,  0xfb, 0x82, 0x8d, 0x9e, 0x7a, 0xb2,
      0x2,  0xe,  0xf2, 0xa6, 0xd7, 0x4b, 0xb3, 0xa,  0xf2, 0x32, 0xc5, 0xf3,
      0xf1, 0xc8, 0x22, 0x73, 0xbe, 0x69, 0x53, 0x92, 0xfe, 0x44, 0xe3, 0xea,
      0x59, 0x63, 0x14, 0x7,  0xb9, 0xf3, 0x91, 0x36};

  uint8_t p2_pub[ED448_POINT_BYTES] = {
      0xec, 0xf4, 0xc8, 0x36, 0xfb, 0x91, 0xe5, 0x64, 0x69, 0x6b, 0x40, 0xe5,
      0x3,  0x17, 0x9c, 0x51, 0x79, 0xcc, 0x43, 0xbf, 0x4a, 0x21, 0x41, 0xf0,
      0x30, 0x21, 0xb3, 0xd1, 0x98, 0xa9, 0xd8, 0x90, 0xef, 0xcb, 0x56, 0xc1,
      0x99, 0x69, 0x61, 0x25, 0xac, 0x67, 0x79, 0x34, 0x86, 0xa7, 0x3f, 0x3a,
      0x33, 0x70, 0xfb, 0xcd, 0xa,  0x52, 0x87, 0x64, 0x0};

  otrng_ec_scalar_decode(p2.priv, p2_priv);
  otrng_ec_point_decode(p2.pub, p2_pub);

  uint8_t p3_priv[ED448_SCALAR_BYTES] = {
      0xdd, 0x86, 0x6d, 0x0,  0x25, 0xde, 0xf2, 0xe5, 0x3c, 0xb0, 0x2c, 0x62,
      0xf7, 0x8e, 0x6f, 0x75, 0x2f, 0x90, 0xa6, 0x26, 0x1d, 0x3f, 0x7b, 0x53,
      0x5e, 0x79, 0x65, 0x7a, 0xba, 0x8b, 0x43, 0xe8, 0xea, 0xff, 0xf6, 0x70,
      0xf4, 0xf6, 0x85, 0x8a, 0x22, 0x58, 0xd7, 0x6,  0x26, 0xb4, 0x3f, 0x69,
      0x81, 0x8e, 0xc5, 0x72, 0x7e, 0xef, 0xfb, 0x37};

  uint8_t p3_pub[ED448_POINT_BYTES] = {
      0x61, 0xfa, 0x1f, 0x15, 0x35, 0x82, 0xf5, 0xf6, 0x42, 0xf2, 0x72, 0x2,
      0xe9, 0xc2, 0x57, 0x6,  0x1a, 0x7c, 0xb8, 0xc4, 0x79, 0x91, 0x74, 0xb3,
      0xa9, 0xbd, 0x87, 0xa4, 0xf3, 0xb1, 0x87, 0xf,  0x8c, 0xee, 0x9c, 0x9,
      0xdc, 0x8e, 0x8b, 0x74, 0x31, 0xe,  0x80, 0x55, 0x73, 0x9d, 0x63, 0x43,
      0x30, 0xdb, 0xb9, 0x72, 0x6d, 0x48, 0x4e, 0x27, 0x80};

  otrng_ec_scalar_decode(p3.priv, p3_priv);
  otrng_ec_point_decode(p3.pub, p3_pub);

  // Test if the keys are OK
  const char *message = "hi";
  ring_sig_s dst;
  otrng_assert(otrng_rsig_authenticate(&dst, p1.priv, p1.pub, p1.pub, p2.pub,
                                       p3.pub, (unsigned char *)message, 2));
  otrng_assert(otrng_rsig_verify(&dst, p1.pub, p2.pub, p3.pub,
                                 (const uint8_t *)message, 2));

  uint8_t rsig[6 * ED448_SCALAR_BYTES] = {
      // c1
      0x9c, 0x18, 0x80, 0x76, 0xec, 0xe0, 0x2c, 0x9d, 0x1f, 0x16, 0x34, 0x95,
      0xf1, 0xf5, 0x31, 0x1f, 0x1d, 0xce, 0x4e, 0xfd, 0x33, 0x4a, 0xe8, 0x7e,
      0xd7, 0xd1, 0x1e, 0x71, 0x54, 0x4c, 0x5e, 0xed, 0xf1, 0x98, 0x6d, 0x43,
      0x6a, 0x7e, 0x5b, 0xd3, 0x93, 0x36, 0xd9, 0xf6, 0x33, 0x27, 0x31, 0xd9,
      0x5d, 0x60, 0x32, 0xe1, 0xfe, 0xd6, 0xce, 0x2c,

      // r1
      0x7f, 0xb6, 0x4a, 0x1, 0x7d, 0xa0, 0x78, 0x5c, 0x8a, 0xbb, 0xba, 0xc5,
      0x42, 0x85, 0xf9, 0x6a, 0xdc, 0xd9, 0x8, 0x2f, 0x50, 0x12, 0xa6, 0x93,
      0x6c, 0x7b, 0x36, 0x57, 0x17, 0x9f, 0x33, 0x23, 0x4d, 0x49, 0xb8, 0xd0,
      0x3d, 0xf5, 0xb5, 0xe5, 0x86, 0x4, 0x9a, 0x2d, 0xb7, 0x9f, 0xfa, 0xf,
      0x1b, 0x36, 0xf7, 0x49, 0xd0, 0x3, 0x2b, 0x2b,

      // c2
      0x77, 0xab, 0x9a, 0x55, 0xd1, 0x74, 0x2d, 0x76, 0x9b, 0x51, 0x13, 0x9d,
      0xa6, 0x75, 0x85, 0x98, 0x80, 0xac, 0x12, 0xca, 0x57, 0xae, 0x62, 0xdd,
      0xb7, 0x00, 0xac, 0x7a, 0xd4, 0xdd, 0x0b, 0x25, 0xa9, 0x17, 0x5c, 0xab,
      0x50, 0x3e, 0xfd, 0x39, 0x23, 0x67, 0xd0, 0xc2, 0x41, 0x67, 0xea, 0x38,
      0x96, 0xf4, 0x91, 0x92, 0xc6, 0xf6, 0x6c, 0x26,

      // r2
      0x14, 0x1f, 0x30, 0x68, 0x2e, 0xae, 0x7a, 0x17, 0x25, 0xd8, 0xc5, 0x15,
      0x6c, 0xc9, 0xaa, 0x79, 0x7c, 0xef, 0x43, 0x7f, 0xf4, 0xdc, 0xc1, 0x02,
      0x8b, 0x31, 0x3c, 0x0f, 0xad, 0x3d, 0x2f, 0x2c, 0x91, 0x9d, 0xee, 0x07,
      0x5c, 0x45, 0x08, 0xa5, 0x06, 0x51, 0x0c, 0x2d, 0x6b, 0xe6, 0xd2, 0x43,
      0x05, 0x4a, 0xc1, 0x17, 0xad, 0x14, 0xdd, 0x33,

      // c3
      0x64, 0x8e, 0x4c, 0xc1, 0x91, 0xbd, 0x72, 0xc0, 0x70, 0xd5, 0x82, 0xba,
      0x00, 0x86, 0xa6, 0x39, 0x8d, 0xc7, 0x95, 0x9a, 0xb3, 0x02, 0xf3, 0xfb,
      0xb0, 0x21, 0x8e, 0xc2, 0xd9, 0xd8, 0xeb, 0x5a, 0x11, 0xd2, 0x8f, 0xb7,
      0x33, 0x49, 0xe5, 0x05, 0x54, 0xbb, 0xf9, 0x5b, 0xda, 0x19, 0x68, 0x97,
      0xb3, 0x77, 0x6c, 0xe5, 0xa8, 0x75, 0x15, 0x2c,

      // r3
      0x53, 0x41, 0xf3, 0x67, 0xf3, 0xfa, 0xdd, 0x4b, 0x76, 0x95, 0x6b, 0x8e,
      0x8d, 0xaf, 0x34, 0x67, 0x19, 0x33, 0xe4, 0x6d, 0x49, 0x93, 0x8d, 0xfb,
      0xeb, 0xbc, 0xb8, 0xea, 0x7d, 0x98, 0x56, 0x8a, 0xb1, 0x0d, 0x90, 0xf3,
      0x88, 0x6a, 0x40, 0x94, 0x29, 0x9e, 0xbb, 0xaa, 0x8e, 0x2d, 0xca, 0x5a,
      0xe7, 0x46, 0xf1, 0x73, 0x62, 0xcf, 0x8b, 0x38};

  ring_sig_s proof;
  otrng_assert_is_success(
      otrng_deserialize_ring_sig(&proof, rsig, sizeof(rsig), NULL));

  otrng_assert(otrng_rsig_verify_with_usage_and_domain(
      0x11, "OTR-Prekey-Server", &proof, p1.pub, p2.pub, p3.pub,
      (const uint8_t *)message, 2));
}

void units_auth_add_tests(void) {
  g_test_add_func("/ring-signature/rsig_auth", test_rsig_auth);
  g_test_add_func("/ring-signature/calculate_c", test_rsig_calculate_c);
  g_test_add_func("/ring-signature/compatible_with_prekey_server",
                  test_rsig_compatible_with_prekey_server);
}
