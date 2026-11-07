package com.apiguard.apiary.repository

import com.apiguard.apiary.util.isValidApiHost
import com.apiguard.apiary.util.isValidPort
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

/** Testuje te same walidatory hosta i portu, których używa ApiaryRepository. */
class ApiaryRepositoryTest {
    @Test fun acceptsValidIPv4AndDnsHosts() {
        listOf("192.168.1.1", "10.0.0.1", "172.16.0.1", "localhost", "apiary.local", "example.org")
            .forEach { assertTrue("$it should be a valid API host", it.isValidApiHost()) }
    }

    @Test fun rejectsMalformedHostsAndUrls() {
        listOf("", "   ", "256.1.1.1", "192.168.1", "192.168.1.1.1", "https://example.org", "example.org:443", "bad host")
            .forEach { assertFalse("$it should be rejected", it.isValidApiHost()) }
    }

    @Test fun acceptsValidPortNumbers() {
        listOf(1, 80, 443, 5000, 8080, 65535).forEach { assertTrue(it.isValidPort()) }
    }

    @Test fun rejectsInvalidPortNumbers() {
        listOf(-1, 0, 65536, 100000).forEach { assertFalse(it.isValidPort()) }
    }
}
