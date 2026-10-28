#include "cert_pin.h"
#include "text_buffer.h"
#include "http_endpoint.h"

#include <string.h>

#include "config_manager.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509_crt.h"

static const char *TAG = "cert_pin";

esp_err_t cert_pin_fetch_and_store(const char *url, char *err_out, size_t err_out_len)
{
    if (err_out && err_out_len > 0)
        err_out[0] = '\0';

    if (!url || url[0] == '\0') {
        text_buffer_message(err_out, err_out_len, "URL is empty");
        return ESP_FAIL;
    }

    http_endpoint_t endpoint;
    if (!http_endpoint_parse(url, &endpoint) || !endpoint.tls) {
        text_buffer_message(err_out, err_out_len, "Invalid HTTPS URL");
        return ESP_FAIL;
    }
    const char *host = endpoint.host;
    int port = (int)endpoint.port;

    ESP_LOGI(TAG, "Fetching TLS certificate from %s:%d", host, port);

    esp_tls_cfg_t tls_cfg = {0};
    esp_tls_t *tls = esp_tls_init();
    if (!tls) {
        text_buffer_message(err_out, err_out_len, "TLS init failed");
        return ESP_FAIL;
    }

    int ret = esp_tls_conn_new_sync(host, strlen(host), port, &tls_cfg, tls);
    if (ret != 1) {
        int esp_tls_err = 0;
        int mbedtls_err = 0;
        esp_tls_error_handle_t error_handle = NULL;
        esp_tls_get_error_handle(tls, &error_handle);
        if (error_handle) {
            esp_tls_get_and_clear_last_error(error_handle, &esp_tls_err, &mbedtls_err);
        }
        ESP_LOGE(TAG, "TLS connection failed to %s:%d (esp_tls=0x%x, mbedtls=-0x%04x)", host, port,
                 esp_tls_err, -mbedtls_err);
        esp_tls_conn_destroy(tls);
        if (err_out) {
            snprintf(err_out, err_out_len,
                     "TLS handshake failed with %s:%d (mbedtls error -0x%04x)", host, port,
                     -mbedtls_err);
        }
        return ESP_FAIL;
    }

    mbedtls_ssl_context *ssl = (mbedtls_ssl_context *) esp_tls_get_ssl_context(tls);
    const mbedtls_x509_crt *peer_cert = mbedtls_ssl_get_peer_cert(ssl);
    if (!peer_cert) {
        esp_tls_conn_destroy(tls);
        text_buffer_message(err_out, err_out_len, "No certificate received from server");
        return ESP_FAIL;
    }

    // Pin the issuer (next cert in the chain), not the leaf, so standard mbedtls
    // chain verification succeeds during subsequent HTTPS fetches: leaf is
    // signed by issuer, and the issuer matches our trust anchor.
    const mbedtls_x509_crt *issuer = peer_cert->next;
    if (!issuer) {
        esp_tls_conn_destroy(tls);
        text_buffer_message(err_out, err_out_len,
                            "Server sent only a leaf certificate; pinning requires an intermediate");
        return ESP_FAIL;
    }

    size_t cert_der_len = issuer->raw.len;
    unsigned char *cert_der = malloc(cert_der_len);
    if (!cert_der) {
        esp_tls_conn_destroy(tls);
        text_buffer_message(err_out, err_out_len, "Out of memory");
        return ESP_FAIL;
    }
    memcpy(cert_der, issuer->raw.p, cert_der_len);

    esp_tls_conn_destroy(tls);

#if CONFIG_BOARD_DRIVER_READ_PICO
    // Publish the URL, trust anchor and ETag invalidation together. The caller
    // owns the outer configuration transaction/revision update.
    esp_err_t saved = config_manager_set_image_source_checked(url, cert_der, cert_der_len);
#else
    config_manager_set_ca_cert_der(cert_der, cert_der_len);
    config_manager_touch_config();
#endif

    free(cert_der);
#if CONFIG_BOARD_DRIVER_READ_PICO
    if (saved != ESP_OK) {
        if (err_out && err_out_len) snprintf(err_out, err_out_len, "Image source save not confirmed");
        return saved;
    }
#endif

    ESP_LOGI(TAG, "Pinned issuer cert for %s:%d (%zu bytes)", host, port, cert_der_len);
    return ESP_OK;
}

void cert_pin_clear(void)
{
#if CONFIG_BOARD_DRIVER_READ_PICO
    download_config_t *patch = malloc(sizeof(*patch));
    if (!patch) { ESP_LOGE(TAG, "Certificate clear allocation failed"); return; }
    memset(patch, 0, sizeof(*patch));
    esp_err_t err = config_manager_patch_download_checked(DOWNLOAD_CONFIG_CERTIFICATE, patch);
    free(patch);
    if (err == ESP_OK) err = config_manager_touch_config_checked();
    if (err != ESP_OK) ESP_LOGE(TAG, "Certificate clear not confirmed: %s", esp_err_to_name(err));
#else
    config_manager_set_ca_cert_der(NULL, 0);
    config_manager_touch_config();
#endif
}
