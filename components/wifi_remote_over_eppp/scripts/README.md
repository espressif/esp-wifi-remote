# Test Certificate Generation

This directory contains scripts for generating test certificates and keys for the WiFi Remote over EPPP component.

## Overview

The WiFi Remote over EPPP component uses TLS with mutual authentication for secure communication between the host and slave devices. This requires certificates and keys for both parties.

## Certificate Generation Script

The `generate_test_certs.sh` script generates a complete set of test certificates and keys suitable for development and testing purposes.

### Usage

```bash
./generate_test_certs.sh [SERVER_CN] [CLIENT_CN]
```

**Parameters:**
- `SERVER_CN`: Server certificate common name (default: espressif.local)
- `CLIENT_CN`: Client certificate common name (default: client_cn)

**Examples:**
```bash
# Use default values (espressif.local, client_cn)
./generate_test_certs.sh

# Specify custom server CN
./generate_test_certs.sh myserver.local

# Specify both server and client CN
./generate_test_certs.sh myserver.local myclient.local

# Show help
./generate_test_certs.sh --help
```

### Generated Files

The script generates the following files:

**Certificate Authority:**
- `ca.crt` - Root CA certificate
- `ca.key` - Root CA private key

**Server Certificates:**
- `SERVER_CA` - CA certificate for server validation (copy of ca.crt)
- `SERVER_CRT` - Server certificate
- `SERVER_KEY` - Server private key

**Client Certificates:**
- `CLIENT_CA` - CA certificate for client validation (copy of ca.crt)
- `CLIENT_CRT` - Client certificate
- `CLIENT_KEY` - Client private key

**Configuration Output:**
The script also outputs ESP-IDF configuration options in the correct format for direct use in `sdkconfig` or menuconfig.

## Configuration

### Host Device (RPC Client)

For the host device running the WiFi Remote client, configure these options:

```bash
CONFIG_WIFI_RMT_OVER_EPPP_SERVER_CA="<SERVER_CA content>"
CONFIG_WIFI_RMT_OVER_EPPP_CLIENT_CRT="<CLIENT_CRT content>"
CONFIG_WIFI_RMT_OVER_EPPP_CLIENT_KEY="<CLIENT_KEY content>"
```

### Slave Device (RPC Server)

For the slave device running the WiFi Remote server, configure these options:

```bash
CONFIG_WIFI_RMT_OVER_EPPP_CLIENT_CA="<CLIENT_CA content>"
CONFIG_WIFI_RMT_OVER_EPPP_SERVER_CRT="<SERVER_CRT content>"
CONFIG_WIFI_RMT_OVER_EPPP_SERVER_KEY="<SERVER_KEY content>"
```

## Security Notes

⚠️ **Important Security Considerations:**

1. **Test Certificates Only**: These certificates are generated for development and testing purposes only. They use a self-signed CA and should not be used in production environments.

2. **Default Common Name**: The default server common name is `espressif.local`. This should be changed to match your actual server hostname in production.

3. **Certificate Validity**: Generated certificates are valid for 365 days from creation.

4. **Key Strength**: Certificates use RSA 2048-bit keys, which is suitable for most applications but may need to be updated for high-security requirements.

## Integration with Examples

These certificates are used by the WiFi Remote examples:

- **MQTT Example**: Uses client certificates to connect to the remote WiFi interface
- **Server Example**: Uses server certificates to authenticate incoming connections

For detailed setup instructions, refer to the README files in the respective example directories:
- [MQTT Example](../../esp_wifi_remote/examples/mqtt/README.md)
- [Server Example](../../esp_wifi_remote/examples/server/README.md)

## Troubleshooting

**Certificate Mismatch Errors:**
- Ensure the common name in the server certificate matches the hostname used by the client
- Verify that both devices are using certificates from the same CA

**Connection Failures:**
- Check that all required certificate configurations are properly set
- Ensure the certificate content includes the full PEM format (including headers and footers)
- Verify that the UART/SPI connection pins are correctly configured

**Certificate Expiration:**
- Regenerate certificates if they have expired (365 days from generation)
- Update all configuration files with the new certificate content
