#include "diamondcore.h"

#include <stdint.h>

bool dc_parse_u64_dec_strict(const char *s, uint64_t *out, const char *label,
                             dc_error_t *err) {
  if (!out || !err) return false;
  dc_err_init(err);

  if (!s || !*s) {
    dc_err_set(err, DC_ERR_USAGE, "invalid %s", (label && *label) ? label : "number");
    return false;
  }

  /* No sign allowed. */
  if (*s == '+' || *s == '-') {
    dc_err_set(err, DC_ERR_USAGE, "invalid %s", (label && *label) ? label : "number");
    return false;
  }

  /* Digits only. */
  if (*s < '0' || *s > '9') {
    dc_err_set(err, DC_ERR_USAGE, "invalid %s", (label && *label) ? label : "number");
    return false;
  }

  /* Leading zeros forbidden unless exactly "0". */
  if (s[0] == '0' && s[1] != '\0') {
    dc_err_set(err, DC_ERR_USAGE, "invalid %s", (label && *label) ? label : "number");
    return false;
  }

  uint64_t v = 0;
  for (const char *p = s; *p; p++) {
    if (*p < '0' || *p > '9') {
      dc_err_set(err, DC_ERR_USAGE, "invalid %s", (label && *label) ? label : "number");
      return false;
    }
    uint64_t d = (uint64_t)(*p - '0');
    if (v > (UINT64_MAX - d) / 10) {
      dc_err_set(err, DC_ERR_USAGE, "invalid %s", (label && *label) ? label : "number");
      return false;
    }
    v = v * 10 + d;
  }

  *out = v;
  return true;
}