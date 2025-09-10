#!/usr/bin/env bash

function gen_pkey { # Params: [KEY_FILE]
    openssl genpkey -algorithm RSA  -pkeyopt rsa_keygen_bits:2048 |  openssl pkcs8 -topk8 -outform PEM  -nocrypt -out $1
}

function sign_with_ca { # Params: [KEY_FILE] [CN] [CRT_FILE]
    openssl req -out request.csr -key $1 -subj "/CN=$2" -new -sha256
    openssl x509 -req -in request.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out $3 -days 365 -sha256
}

function export_config { # Params: [FILE/CONFIG_NAME]
    content=`cat $1 | sed '/---/d' | tr -d '\n'`
    echo "CONFIG_WIFI_RMT_OVER_EPPP_$1=\"${content}\""
}

# Check for help flag or too many arguments
if [ "$1" = "--help" ] || [ $# -gt 2 ]; then
    echo "Usage: $0 [SERVER_CN] [CLIENT_CN]"
    echo "  SERVER_CN: Server certificate common name (default: espressif.local)"
    echo "  CLIENT_CN: Client certificate common name (default: client_cn)"
    echo ""
    echo "Examples:"
    echo "  $0                                    # Uses defaults: espressif.local, client_cn"
    echo "  $0 myserver.local                    # Uses: myserver.local, client_cn"
    echo "  $0 myserver.local myclient.local     # Uses: myserver.local, myclient.local"
    exit 0
fi

# Set defaults and handle arguments
SERVER_CN="${1-espressif.local}"
CLIENT_CN="${2-client_cn}"

echo "Server's CN: $SERVER_CN"
echo "Client's CN: $CLIENT_CN"

## First create our own CA
gen_pkey ca.key
openssl req -new -x509 -subj "/C=CZ/CN=Espressif" -days 365 -key ca.key -out ca.crt
# will use the same CA for both server and client side
cp ca.crt SERVER_CA
cp ca.crt CLIENT_CA

# Server side
gen_pkey SERVER_KEY
sign_with_ca SERVER_KEY $SERVER_CN SERVER_CRT

# Client side
gen_pkey CLIENT_KEY
sign_with_ca CLIENT_KEY $CLIENT_CN CLIENT_CRT

## Generate config options
echo -e "\n# Client side: need own cert and key and ca-cert for server validation"
for f in SERVER_CA CLIENT_CRT CLIENT_KEY; do
    export_config $f
done

echo -e "\n# Server side: need own cert and key and ca-cert for client validation"
for f in CLIENT_CA SERVER_CRT SERVER_KEY; do
    export_config $f
done
