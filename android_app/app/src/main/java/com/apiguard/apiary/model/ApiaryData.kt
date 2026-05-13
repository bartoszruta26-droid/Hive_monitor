package com.apiguard.apiary.model

import com.google.gson.annotations.SerializedName

/**
 * Model danych reprezentujący pojedynczy ul z danymi telemetrycznymi.
 * Wszystkie pola są opcjonalne (nullable) aby uniknąć crashów przy niekompletnych danych z API.
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
               temperature >= -50 && temperature <= 100 && // Realistyczny zakres temperatur
               humidity >= 0 && humidity <= 100 &&          // Zakres wilgotności
               weight >= 0 && weight <= 200 &&              // Realistyczny zakres wagi ula
               battery >= 0 && battery <= 100               // Zakres baterii
    }
}
