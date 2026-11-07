package com.apiguard.apiary.ui

import com.apiguard.apiary.data.model.ApiaryReading
import com.apiguard.apiary.model.ApiaryData
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class ApiaryAdapterLogicTest {
    private fun apiary(
        temperature: Double = 25.0,
        humidity: Double = 55.0,
        battery: Int = 75,
        available: Boolean = true,
        online: Boolean = true,
        timestamp: Long = 1_700_000_000_000L
    ) = ApiaryData(
        id = "test-id",
        name = "Test Ul",
        temperature = temperature,
        humidity = humidity,
        weight = 50.0,
        battery = battery,
        timestamp = timestamp,
        available = available,
        online = online
    )

    @Test fun offlineStatusHasPriority() {
        assertEquals("OFFLINE", ApiaryUiFormatter.determineStatus(apiary(online = false, available = false)))
    }

    @Test fun unavailableStatusIsDistinctFromHealthyStatus() {
        assertEquals("BRAK DANYCH", ApiaryUiFormatter.determineStatus(apiary(available = false)))
    }

    @Test fun temperatureOutsideLimitsIsAlert() {
        assertEquals("ALERT", ApiaryUiFormatter.determineStatus(apiary(temperature = 5.0)))
        assertEquals("ALERT", ApiaryUiFormatter.determineStatus(apiary(temperature = 45.0)))
    }

    @Test fun humidityOutsideLimitsIsWarning() {
        assertEquals("UWAGA", ApiaryUiFormatter.determineStatus(apiary(humidity = 25.0)))
        assertEquals("UWAGA", ApiaryUiFormatter.determineStatus(apiary(humidity = 85.0)))
    }

    @Test fun lowBatteryIsWarning() {
        assertEquals("UWAGA", ApiaryUiFormatter.determineStatus(apiary(battery = 15)))
    }

    @Test fun healthyMeasurementsAreOk() {
        assertEquals("OK", ApiaryUiFormatter.determineStatus(apiary()))
    }

    @Test fun temperatureAlertHasPriorityOverHumidityWarning() {
        assertEquals("ALERT", ApiaryUiFormatter.determineStatus(apiary(temperature = 45.0, humidity = 85.0)))
    }

    @Test fun archivedReadingShowsTimestampAndAllMeasurements() {
        val row = ApiaryUiFormatter.formatArchivedReading(
            ApiaryReading(
                apiaryId = "hive-1",
                timestamp = 1_700_000_000_000L,
                temperature = 24.5f,
                humidity = 61.0f,
                weight = 42.25f,
                batteryLevel = 88
            )
        )
        assertTrue(row.contains("24,5°C") || row.contains("24.5°C"))
        assertTrue(row.contains("61,0%") || row.contains("61.0%"))
        assertTrue(row.contains("42,25 kg") || row.contains("42.25 kg"))
        assertTrue(row.contains("88%"))
    }

    @Test fun missingTimestampIsExplicit() {
        assertEquals("brak danych", ApiaryUiFormatter.formatTimestamp(0L))
        assertEquals("brak danych", ApiaryUiFormatter.formatTimestamp(-100L))
    }

    @Test fun validTimestampUsesExpectedDisplayPattern() {
        val formatted = ApiaryUiFormatter.formatTimestamp(1_700_000_000_000L)
        assertTrue(formatted.matches(Regex("\\d{2}\\.\\d{2}\\.\\d{4} \\d{2}:\\d{2}")))
    }
}
