package com.apiguard.apiary.network

import org.junit.Assert.assertEquals
import org.junit.Assert.assertThrows
import org.junit.Test

class RetrofitClientTest {
    @Test
    fun baseUrlAlwaysUsesHttps() {
        assertEquals("https://192.168.1.20:8443/", RetrofitClient.buildBaseUrl("192.168.1.20", 8443))
    }

    @Test
    fun supportsDnsNamesForCertificateHostnameVerification() {
        assertEquals("https://apiary.example.org:443/", RetrofitClient.buildBaseUrl("apiary.example.org", 443))
    }

    @Test
    fun rejectsUrlInjectionInHostField() {
        assertThrows(IllegalArgumentException::class.java) {
            RetrofitClient.buildBaseUrl("https://evil.example/path", 443)
        }
    }

    @Test
    fun rejectsInvalidPort() {
        assertThrows(IllegalArgumentException::class.java) {
            RetrofitClient.buildBaseUrl("192.168.1.20", 70000)
        }
    }

    @Test
    fun rejectsBlankHost() {
        assertThrows(IllegalArgumentException::class.java) {
            RetrofitClient.buildBaseUrl(" ", 8443)
        }
    }
}
