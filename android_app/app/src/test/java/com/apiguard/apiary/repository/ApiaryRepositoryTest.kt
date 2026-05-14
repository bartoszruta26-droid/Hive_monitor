package com.apiguard.apiary.repository

import org.junit.Assert.*
import org.junit.Test

/**
 * Testy jednostkowe dla ApiaryRepository
 */
class ApiaryRepositoryTest {

    @Test
    fun `verifyIpAddress should return true for valid IPv4 addresses`() {
        val validIpAddresses = listOf(
            "192.168.1.1",
            "10.0.0.1",
            "172.16.0.1",
            "255.255.255.255",
            "0.0.0.0",
            "127.0.0.1"
        )

        validIpAddresses.forEach { ip ->
            assertTrue("IP $ip should be valid", isValidIpAddress(ip))
        }
    }

    @Test
    fun `verifyIpAddress should return false for invalid IPv4 addresses`() {
        val invalidIpAddresses = listOf(
            "256.1.1.1",
            "192.168.1",
            "192.168.1.1.1",
            "abc.def.ghi.jkl",
            "192.168.1.-1",
            "",
            "   ",
            "192.168.1.256"
        )

        invalidIpAddresses.forEach { ip ->
            assertFalse("IP $ip should be invalid", isValidIpAddress(ip))
        }
    }

    @Test
    fun `validatePort should accept valid port numbers`() {
        val validPorts = listOf(1, 80, 443, 5000, 8080, 65535)

        validPorts.forEach { port ->
            assertTrue("Port $port should be valid", isValidPort(port))
        }
    }

    @Test
    fun `validatePort should reject invalid port numbers`() {
        val invalidPorts = listOf(-1, 0, 65536, 100000)

        invalidPorts.forEach { port ->
            assertFalse("Port $port should be invalid", isValidPort(port))
        }
    }

    /**
     * Helper function copied from ApiaryRepository for testing purposes
     */
    private fun isValidIpAddress(ip: String): Boolean {
        val ipPattern = Regex(
            "^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}" +
            "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$"
        )
        return ipPattern.matches(ip)
    }

    /**
     * Helper function to validate port range
     */
    private fun isValidPort(port: Int): Boolean {
        return port in 1..65535
    }
}
