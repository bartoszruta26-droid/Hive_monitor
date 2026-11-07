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

    @Test fun archivedReadingsCanBeFilteredByApiaryAndTimeAndAreNewestFirst() {
        val readings = listOf(
            ApiaryReading(apiaryId = "hive-a", timestamp = 300L, temperature = 20f, humidity = 50f, weight = 40f, batteryLevel = 90),
            ApiaryReading(apiaryId = "hive-b", timestamp = 400L, temperature = 21f, humidity = 51f, weight = 41f, batteryLevel = 91),
            ApiaryReading(apiaryId = "hive-a", timestamp = 100L, temperature = 19f, humidity = 49f, weight = 39f, batteryLevel = 89)
        )
        assertEquals(listOf("hive-a", "hive-b"), ApiaryUiFormatter.archivedApiaryIds(readings))
        val filtered = ApiaryUiFormatter.filterArchivedReadings(readings, "hive-a", 150L)
        assertEquals(1, filtered.size)
        assertEquals(300L, filtered.first().timestamp)
    }

    @Test fun historyComparisonGroupsRecordsByUtcYearAndWeightsBySourceCount() {
        val response = com.apiguard.apiary.data.model.ApiaryHistoryResponse(
            hiveId = "hive-1",
            aggregation = "DAY",
            start = 1_672_531_200L,
            end = 1_735_689_600L,
            dataQuality = "ARCHIVE",
            records = listOf(
                com.apiguard.apiary.data.model.ApiaryHistoryRecord(
                    timestampStart = 1_672_531_200L, timestampEnd = 1_672_617_600L,
                    temperatureAverage = 10.0, temperatureMinimum = 8.0, temperatureMaximum = 12.0,
                    humidityAverage = 40.0, humidityMinimum = 35.0, humidityMaximum = 45.0,
                    weightAverage = 30.0, weightMinimum = 29.0, weightMaximum = 31.0,
                    batteryAverage = 90.0, co2Average = 400.0, vocAverage = 20.0, recordCount = 1L
                ),
                com.apiguard.apiary.data.model.ApiaryHistoryRecord(
                    timestampStart = 1_704_067_200L, timestampEnd = 1_704_153_600L,
                    temperatureAverage = 10.0, temperatureMinimum = 8.0, temperatureMaximum = 12.0,
                    humidityAverage = 40.0, humidityMinimum = 35.0, humidityMaximum = 45.0,
                    weightAverage = 30.0, weightMinimum = 29.0, weightMaximum = 31.0,
                    batteryAverage = 90.0, co2Average = 400.0, vocAverage = 20.0, recordCount = 1L
                ),
                com.apiguard.apiary.data.model.ApiaryHistoryRecord(
                    timestampStart = 1_704_067_200L, timestampEnd = 1_704_153_600L,
                    temperatureAverage = 20.0, temperatureMinimum = 18.0, temperatureMaximum = 22.0,
                    humidityAverage = 60.0, humidityMinimum = 55.0, humidityMaximum = 65.0,
                    weightAverage = 40.0, weightMinimum = 39.0, weightMaximum = 41.0,
                    batteryAverage = 90.0, co2Average = 400.0, vocAverage = 20.0, recordCount = 3L
                )
            )
        )
        val comparison = ApiaryUiFormatter.formatHistoryYearComparison(response)
        assertTrue(comparison.indexOf("2023") < comparison.indexOf("2024"))
        assertTrue(comparison.contains("17.5°C"))
        assertTrue(comparison.contains("55.0%"))
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
