package com.apiguard.apiary.model

import com.apiguard.apiary.util.AppConstants
import com.google.gson.annotations.SerializedName

/**
 * Model danych reprezentujący pojedynczy ul z danymi telemetrycznymi.
 * Wszystkie pola mają domyślne wartości aby uniknąć crashów przy niekompletnych danych z API.
 */
data class ApiaryData(
    @SerializedName("id") val id: String,
    @SerializedName("name") val name: String,
    @SerializedName("temperature") val temperature: Double = 0.0,
    @SerializedName("humidity") val humidity: Double = 0.0,
    @SerializedName("weight") val weight: Double = 0.0,
    @SerializedName("battery") val battery: Int = 0,
    @SerializedName("timestamp") val timestamp: Long = 0L
) {
    /**
     * Waliduje czy dane są kompletne i poprawne
     */
    fun isValid(): Boolean {
        return id.isNotBlank() && 
               name.isNotBlank() && 
               timestamp > 0 &&
               temperature.isValidTemperature() &&
               humidity.isValidHumidity() &&
               weight.isValidWeight() &&
               battery.isValidBatteryLevel()
    }
}

/**
 * Rozszerzenia dla walidacji pól ApiaryData
 */
private fun Double.isValidTemperature(): Boolean {
    return this in AppConstants.TEMP_MIN_VALID..AppConstants.TEMP_MAX_VALID
}

private fun Double.isValidHumidity(): Boolean {
    return this in AppConstants.HUMIDITY_MIN_VALID..AppConstants.HUMIDITY_MAX_VALID
}

private fun Double.isValidWeight(): Boolean {
    return this in AppConstants.WEIGHT_MIN_VALID..AppConstants.WEIGHT_MAX_VALID
}

private fun Int.isValidBatteryLevel(): Boolean {
    return this in AppConstants.BATTERY_MIN_VALID..AppConstants.BATTERY_MAX_VALID
}
