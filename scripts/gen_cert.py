import os
import urllib.request
import datetime as dt


CERT_URL = "https://letsencrypt.org/certs/isrgrootx1.pem"
OUTPUT_FILE = "board/src/certs/isrg_root_x1.hpp"
VAR_NAME = "ISRG_ROOT_X1_CA"

def main():
    os.makedirs(os.path.dirname(OUTPUT_FILE), exist_ok=True)
    # Fetch certificate
    with urllib.request.urlopen(CERT_URL) as response:
        pem = response.read().decode("ascii")

    if "BEGIN CERTIFICATE" not in pem:
        raise RuntimeError("Downloaded content does not look like a PEM certificate")

    # Generate header content
    timestamp = dt.datetime.now(dt.timezone.utc).isoformat() + "Z"

    hpp = f"""#pragma once
#include <Arduino.h>
/*
 * ISRG Root X1 certificate
 * Source: {CERT_URL}
 * Generated: {timestamp}
 */

namespace certs {{
    static const char {VAR_NAME}[] PROGMEM = R"EOF(
{pem.strip()}
)EOF";
}}
"""

    # Write output file
    with open(OUTPUT_FILE, "w", newline="\n") as f:
        f.write(hpp)

    print(f"Generated {OUTPUT_FILE}")

if __name__ == "__main__":
    main()