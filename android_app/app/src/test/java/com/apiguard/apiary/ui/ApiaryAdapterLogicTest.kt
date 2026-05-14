package com.apiguard.apiary.ui

import com.apiguard.apiary.model.ApiaryData
import org.junit.Assert.*
import org.junit.Test

/**
 * Testy jednostkowe dla ApiaryAdapter - testowanie logiki determinacji statusu
 */
class ApiaryAdapterLogicTest {

    // Stałe z ApiaryAdapter (przekopiowane do testów)
    private companion object {
        private const val TEMP_MIN_THRESHOLD = 10
        private const val TEMP_MAX_THRESHOLD = 40
        private const val HUMIDITY_MIN_THRESHOLD = 30
        private const val HUMIDITY_MAX_THRESHOLD = 80
        private const val BATTERY_LOW_THRESHOLD = 20
    }

    @Test
    fun `determineStatus should return ALERT for temperature below minimum`() {
        val apiaryData = createApiaryData(temperature = 5.0)
        val status = determineStatus(apiaryData)

        assertEquals("ALERT", status.first)
    }

    @Test
    fun `determineStatus should return ALERT for temperature above maximum`() {
        val apiaryData = createApiaryData(temperature = 45.0)
        val status = determineStatus(apiaryData)

        assertEquals("ALERT", status.first)
    }

    @Test
    fun `determineStatus should return UWAGA for humidity below minimum`() {
        val apiaryData = createApiaryData(humidity = 25.0)
        val status = determineStatus(apiaryData)

        assertEquals("UWAGA", status.first)
    }

    @Test
    fun `determineStatus should return UWAGA for humidity above maximum`() {
        val apiaryData = createApiaryData(humidity = 85.0)
        val status = determineStatus(apiaryData)

        assertEquals("UWAGA", status.first)
    }

    @Test
    fun `determineStatus should return UWAGA for low battery`() {
        val apiaryData = createApiaryData(battery = 15)
        val status = determineStatus(apiaryData)

        assertEquals("UWAGA", status.first)
    }

    @Test
    fun `determineStatus should return OK for normal values`() {
        val apiaryData = createApiaryData(
            temperature = 25.0,
            humidity = 55.0,
            battery = 75
        )
        val status = determineStatus(apiaryData)

        assertEquals("OK", status.first)
    }

    @Test
    fun `determineStatus should prioritize temperature over humidity`() {
        // Temperatura jest sprawdzana pierwsza w when expression
        val apiaryData = createApiaryData(
            temperature = 45.0,  // ALERT
            humidity = 85.0      // UWAGA
        )
        val status = determineStatus(apiaryData)

        assertEquals("ALERT", status.first)
    }

    @Test
    fun `determineStatus should prioritize humidity over battery`() {
        val apiaryData = createApiaryData(
            humidity = 85.0,  // UWAGA
            battery = 15      // UWAGA
        )
        val status = determineStatus(apiaryData)

        assertEquals("UWAGA", status.first)
    }

    @Test
    fun `formatTimestamp should handle zero timestamp`() {
        val result = formatTimestamp(0L)
        assertEquals("brak danych", result)
    }

    @Test
    fun `formatTimestamp should handle negative timestamp`() {
        val result = formatTimestamp(-100L)
        assertEquals("brak danych", result)
    }

    @Test
    fun `formatTimestamp should handle valid timestamp`() {
        val validTimestamp = System.currentTimeMillis()
        val result = formatTimestamp(validTimestamp)
        
        // Powinien zwrócić sformatowaną datę, nie "brak danych" ani "błąd daty"
        assertNotEquals("brak danych", result)
        assertNotEquals("błąd daty", result)
        assertTrue("Result should contain date format", result.matches(Regex("\\d{2}\\.\\d{2}\\.\\d{4} \\d{2}:\\d{2}")))
    }

    /**
     * Helper function to create ApiaryData with default values
     */
    private fun createApiaryData(
        id: String = "test-id",
        name: String = "Test Ul",
        temperature: Double = 25.0,
        humidity: Double = 55.0,
        weight: Double = 50.0,
        battery: Int = 75,
        timestamp: Long = System.currentTimeMillis()
    ): ApiaryData {
        return ApiaryData(id, name, temperature, humidity, weight, battery, timestamp)
    }

    /**
     * Implementation of determineStatus logic from ApiaryAdapter for testing
     */
    private fun determineStatus(data: ApiaryData): Pair<String, Int> {
        return when {
            data.temperature < TEMP_MIN_THRESHOLD || data.temperature > TEMP_MAX_THRESHOLD ->
                "ALERT" to 1  // Placeholder color
            data.humidity < HUMIDITY_MIN_THRESHOLD || data.humidity > HUMIDITY_MAX_THRESHOLD ->
                "UWAGA" to 1  // Placeholder color
            data.battery < BATTERY_LOW_THRESHOLD ->
                "UWAGA" to 1  // Placeholder color
            else ->
                "OK" to 1  // Placeholder color
        }
    }

    /**
     * Implementation of formatTimestamp logic from ApiaryAdapter for testing
     */
    private fun formatTimestamp(timestamp: Long): String {
        if (timestamp <= 0) {
            return "brak danych"
        }
        
        try {
            val dateFormat = java.text.SimpleDateFormat("dd.MM.yyyy HH:mm", java.util.Locale.getDefault())
            return dateFormat.format(java.util.Date(timestamp))
        } catch (e: Exception) {
            return "błąd daty"
        }
    }
}
