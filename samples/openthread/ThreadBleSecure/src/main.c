/*
 *  Copyright (c) 2022, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>

// Zephyr OpenThread integration Library
#include <zephyr/net/openthread.h>

// OpenThread API
#include <openthread/platform/ble.h>
#include <openthread/ble_secure.h>

// Mbed TLS
#include <mbedtls/oid.h>

#include "x509_cert_key.h"

#define LOG_LEVEL LOG_LEVEL_INF
LOG_MODULE_REGISTER(bbtc_cli_sample);

struct openthread_context *myOpenThreadContext;
otInstance *myOpenThreadInstance = NULL;

// BleSecure callback functions

void HandleBleSecureClientConnect(bool aConnected, bool aBleConnectionOpen, void *aContext)
{
	LOG_INF("TLS Connected: %s, BLE connection open: %s", aConnected ? "YES" : " NO",
		aBleConnectionOpen ? "YES" : " NO");

	if (aConnected) {
		char buf[20];
		size_t len;

		otBleSecureGetPeerSubjectAttributeByOid(myOpenThreadInstance, MBEDTLS_OID_AT_CN,
							sizeof(MBEDTLS_OID_AT_CN) - 1, buf, &len,
							sizeof(buf) - 1, NULL);

		buf[len] = 0;
		LOG_INF("Peer cert. Common Name:%s", buf);

		otBleSecureGetThreadAttributeFromPeerCertificate(myOpenThreadInstance, 3, buf, &len,
								 sizeof(buf));
		if (len > 0) {
			LOG_INF("Peer OID 1.3.6.1.4.1.44970.3: %02X%02X%02X%02X%02X (len = %d)",
				buf[0], buf[1], buf[2], buf[3], buf[4], len);
		}

		otBleSecureGetThreadAttributeFromOwnCertificate(myOpenThreadInstance, 3, buf, &len,
								sizeof(buf));
		if (len > 0) {
			LOG_INF("Own OID 1.3.6.1.4.1.44970.3: %02X%02X%02X%02X%02X (len = %d)",
				buf[0], buf[1], buf[2], buf[3], buf[4], len);
		}
	}
}

void HandleBleSecureReceive(otMessage *aMessage, void *aContext)
{
	uint16_t nLen;
	char buf[100];

	LOG_INF("TLS Data Received len:%d offset:%d", (int)otMessageGetLength(aMessage),
		(int)otMessageGetOffset(aMessage));

	//nLen = otMessageGetLength(aMessage) - otMessageGetOffset(aMessage);
	nLen = otMessageRead(aMessage, otMessageGetOffset(aMessage), buf + 5, sizeof(buf) - 6);
	buf[nLen + 5] = 0;

	LOG_INF("Received:%s", buf + 5);
	memcpy(buf, "RECV:", 5);
	//otBleSecureSend(myOpenThreadInstance, buf, strlen(buf));
	otBleSecureSendApplicationTlv(myOpenThreadInstance, buf, strlen(buf));
	otBleSecureFlush(myOpenThreadInstance);
}

int main(void)
{
	myOpenThreadContext = openthread_get_default_context();
	myOpenThreadInstance = myOpenThreadContext->instance;

	otBleSecureSetCertificate(myOpenThreadInstance, (const uint8_t *)(OT_CLI_BBTC_X509_CERT),
				  sizeof(OT_CLI_BBTC_X509_CERT),
				  (const uint8_t *)(OT_CLI_BBTC_PRIV_KEY),
				  sizeof(OT_CLI_BBTC_PRIV_KEY));

	otBleSecureSetCaCertificateChain(myOpenThreadInstance,
					 (const uint8_t *)(OT_CLI_BBTC_TRUSTED_ROOT_CERTIFICATE),
					 sizeof(OT_CLI_BBTC_TRUSTED_ROOT_CERTIFICATE));

	otBleSecureSetSslAuthMode(myOpenThreadInstance, true);

	//otBleSecureSetPsk(myOpenThreadInstance, "SECRET", 6, "MyDevice", 8);
	otBleSecureStart(myOpenThreadInstance, HandleBleSecureClientConnect, HandleBleSecureReceive,
			 true, NULL);
	otBleSecureTcatStart(myOpenThreadInstance, "SECRET", NULL, NULL);
	
	return 1;
}
