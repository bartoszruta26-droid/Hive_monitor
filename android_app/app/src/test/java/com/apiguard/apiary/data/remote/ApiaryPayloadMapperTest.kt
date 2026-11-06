package com.apiguard.apiary.data.remote

import com.google.gson.JsonParser
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class ApiaryPayloadMapperTest {
    private fun payload(hiveFields: String, timestamp: Long = 2_000L) =
        JsonParser().parse(
            """{"timestamp":$timestamp,"hive_count":1,"hives":{"UL-1":{$hiveFields}}}"""
        ).asJsonObject

    @Test
    fun mapsCurrentCompleteHiveAsOnline() {
        val result = ApiaryPayloadMapper.map(
            payload(""""temp":34.5,"hum":61.2,"weight":42.75,"bat":92,"online":true,"last_seen":1990"""),
            nowEpochSeconds = 2_000L
        ).single()

        assertEquals("UL-1", result.id)
        assertEquals(34.5, result.temperature, 0.001)
        assertEquals(61.2, result.humidity, 0.001)
        assertEquals(42.75, result.weight, 0.001)
        assertEquals(92, result.battery)
        assertEquals(1_990_000L, result.timestamp)
        assertTrue(result.available)
        assertTrue(result.online)
    }

    @Test
    fun marksStaleHiveOffline() {
        val result = ApiaryPayloadMapper.map(
            payload(""""temp":34.5,"hum":61.2,"weight":42.75,"bat":92,"online":true,"last_seen":1929"""),
            nowEpochSeconds = 2_000L
        ).single()

        assertTrue(result.available)
        assertFalse(result.online)
    }

    @Test
    fun marksFutureTimestampOffline() {
        val result = ApiaryPayloadMapper.map(
            payload(""""temp":34.5,"hum":61.2,"weight":42.75,"bat":92,"online":true,"last_seen":2001"""),
            nowEpochSeconds = 2_000L
        ).single()

        assertFalse(result.online)
    }

    @Test
    fun missingMetricIsUnavailableRatherThanZeroMeasurement() {
        val result = ApiaryPayloadMapper.map(
            payload(""""temp":34.5,"hum":61.2,"weight":42.75,"online":true,"last_seen":1990"""),
            nowEpochSeconds = 2_000L
        ).single()

        assertFalse(result.available)
        assertEquals(0, result.battery)
    }

    @Test
    fun responseTimestampDoesNotReplaceMissingPerHiveTimestamp() {
        val result = ApiaryPayloadMapper.map(
            payload(""""temp":34.5,"hum":61.2,"weight":42.75,"bat":92,"online":true"""),
            nowEpochSeconds = 2_000L
        ).single()

        assertFalse(result.available)
        assertFalse(result.online)
    }

    @Test(expected = IllegalArgumentException::class)
    fun rejectsPayloadWithoutHiveMap() {
        ApiaryPayloadMapper.map(JsonParser().parse("""{"timestamp":2000}""").asJsonObject)
    }
}
