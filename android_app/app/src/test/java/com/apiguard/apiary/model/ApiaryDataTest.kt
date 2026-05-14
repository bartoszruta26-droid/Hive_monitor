package com.apiguard.apiary.model

import org.junit.Assert.*
import org.junit.Test

/**
 * Testy jednostkowe dla modelu ApiaryData
 */
class ApiaryDataTest {

    @Test
    fun `isValid should return true for valid data`() {
        val validApiary = ApiaryData(
            id = "apiary-1",
            name = "Ul 1",
            temperature = 25.0,
            humidity = 60.0,
            weight = 45.5,
            battery = 85,
            timestamp = System.currentTimeMillis()
        )

        assertTrue("Valid apiary data should pass validation", validApiary.isValid())
    }

    @Test
    fun `isValid should return false for blank id`() {
        val invalidApiary = ApiaryData(
            id = "",
            name = "Ul 1",
            temperature = 25.0,
            humidity = 60.0,
            weight = 45.5,
            battery = 85,
            timestamp = System.currentTimeMillis()
        )

        assertFalse("Blank ID should fail validation", invalidApiary.isValid())
    }

    @Test
    fun `isValid should return false for blank name`() {
        val invalidApiary = ApiaryData(
            id = "apiary-1",
            name = "",
            temperature = 25.0,
            humidity = 60.0,
            weight = 45.5,
            battery = 85,
            timestamp = System.currentTimeMillis()
        )

        assertFalse("Blank name should fail validation", invalidApiary.isValid())
    }

    @Test
    fun `isValid should return false for zero timestamp`() {
        val invalidApiary = ApiaryData(
            id = "apiary-1",
            name = "Ul 1",
            temperature = 25.0,
            humidity = 60.0,
            weight = 45.5,
            battery = 85,
            timestamp = 0L
        )

        assertFalse("Zero timestamp should fail validation", invalidApiary.isValid())
    }

    @Test
    fun `isValid should return false for temperature out of range`() {
        val tooCold = ApiaryData(
            id = "apiary-1",
            name = "Ul 1",
            temperature = -60.0,
            humidity = 60.0,
            weight = 45.5,
            battery = 85,
            timestamp = System.currentTimeMillis()
        )

        val tooHot = ApiaryData(
            id = "apiary-1",
            name = "Ul 1",
            temperature = 110.0,
            humidity = 60.0,
            weight = 45.5,
            battery = 85,
            timestamp = System.currentTimeMillis()
        )

        assertFalse("Temperature below -50 should fail validation", tooCold.isValid())
        assertFalse("Temperature above 100 should fail validation", tooHot.isValid())
    }

    @Test
    fun `isValid should return false for humidity out of range`() {
        val tooLow = ApiaryData(
            id = "apiary-1",
            name = "Ul 1",
            temperature = 25.0,
            humidity = -10.0,
            weight = 45.5,
            battery = 85,
            timestamp = System.currentTimeMillis()
        )

        val tooHigh = ApiaryData(
            id = "apiary-1",
            name = "Ul 1",
            temperature = 25.0,
            humidity = 110.0,
            weight = 45.5,
            battery = 85,
            timestamp = System.currentTimeMillis()
        )

        assertFalse("Humidity below 0 should fail validation", tooLow.isValid())
        assertFalse("Humidity above 100 should fail validation", tooHigh.isValid())
    }

    @Test
    fun `isValid should return false for weight out of range`() {
        val negative = ApiaryData(
            id = "apiary-1",
            name = "Ul 1",
            temperature = 25.0,
            humidity = 60.0,
            weight = -5.0,
            battery = 85,
            timestamp = System.currentTimeMillis()
        )

        val tooHeavy = ApiaryData(
            id = "apiary-1",
            name = "Ul 1",
            temperature = 25.0,
            humidity = 60.0,
            weight = 250.0,
            battery = 85,
            timestamp = System.currentTimeMillis()
        )

        assertFalse("Negative weight should fail validation", negative.isValid())
        assertFalse("Weight above 200 should fail validation", tooHeavy.isValid())
    }

    @Test
    fun `isValid should return false for battery out of range`() {
        val negative = ApiaryData(
            id = "apiary-1",
            name = "Ul 1",
            temperature = 25.0,
            humidity = 60.0,
            weight = 45.5,
            battery = -10,
            timestamp = System.currentTimeMillis()
        )

        val over100 = ApiaryData(
            id = "apiary-1",
            name = "Ul 1",
            temperature = 25.0,
            humidity = 60.0,
            weight = 45.5,
            battery = 150,
            timestamp = System.currentTimeMillis()
        )

        assertFalse("Negative battery should fail validation", negative.isValid())
        assertFalse("Battery above 100 should fail validation", over100.isValid())
    }

    @Test
    fun `isValid should accept boundary values`() {
        val minValues = ApiaryData(
            id = "apiary-1",
            name = "Ul 1",
            temperature = -50.0,
            humidity = 0.0,
            weight = 0.0,
            battery = 0,
            timestamp = 1L
        )

        val maxValues = ApiaryData(
            id = "apiary-1",
            name = "Ul 1",
            temperature = 100.0,
            humidity = 100.0,
            weight = 200.0,
            battery = 100,
            timestamp = System.currentTimeMillis()
        )

        assertTrue("Minimum boundary values should pass validation", minValues.isValid())
        assertTrue("Maximum boundary values should pass validation", maxValues.isValid())
    }

    @Test
    fun `default values should be used when not provided`() {
        val apiaryWithDefaults = ApiaryData(
            id = "apiary-1",
            name = "Ul 1"
        )

        assertEquals("Default temperature should be 0.0", 0.0, apiaryWithDefaults.temperature, 0.001)
        assertEquals("Default humidity should be 0.0", 0.0, apiaryWithDefaults.humidity, 0.001)
        assertEquals("Default weight should be 0.0", 0.0, apiaryWithDefaults.weight, 0.001)
        assertEquals("Default battery should be 0", 0, apiaryWithDefaults.battery)
        assertEquals("Default timestamp should be 0L", 0L, apiaryWithDefaults.timestamp)
    }
}
