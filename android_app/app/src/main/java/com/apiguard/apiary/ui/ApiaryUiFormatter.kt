package com.apiguard.apiary.ui

import com.apiguard.apiary.data.model.ApiaryReading
import com.apiguard.apiary.model.ApiaryData
import com.apiguard.apiary.util.AppConstants
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * Czysta logika prezentacji statusu i czasu, współdzielona przez adapter i testy JVM.
 */
internal object ApiaryUiFormatter {
    fun determineStatus(data: ApiaryData): String = when {
        !data.online -> "OFFLINE"
        !data.available -> "BRAK DANYCH"
        data.temperature < AppConstants.TEMP_MIN_THRESHOLD ||
            data.temperature > AppConstants.TEMP_MAX_THRESHOLD -> "ALERT"
        data.humidity < AppConstants.HUMIDITY_MIN_THRESHOLD ||
            data.humidity > AppConstants.HUMIDITY_MAX_THRESHOLD -> "UWAGA"
        data.battery < AppConstants.BATTERY_LOW_THRESHOLD -> "UWAGA"
        else -> "OK"
    }

    fun formatArchivedReading(reading: ApiaryReading): String = String.format(
        Locale.getDefault(),
        "%s | %.1f°C | %.1f%% | %.2f kg | %d%%",
        formatTimestamp(reading.timestamp),
        reading.temperature,
        reading.humidity,
        reading.weight,
        reading.batteryLevel
    )

    fun archivedApiaryIds(readings: List<ApiaryReading>): List<String> =
        readings.map { it.apiaryId }.distinct().sorted()

    fun filterArchivedReadings(
        readings: List<ApiaryReading>,
        apiaryId: String,
        sinceTimestamp: Long
    ): List<ApiaryReading> = readings
        .asSequence()
        .filter { it.apiaryId == apiaryId && it.timestamp >= sinceTimestamp }
        .sortedByDescending { it.timestamp }
        .take(100)
        .toList()

    fun formatTimestamp(timestamp: Long): String {
        if (timestamp <= 0L) return "brak danych"
        return try {
            SimpleDateFormat("dd.MM.yyyy HH:mm", Locale.getDefault()).format(Date(timestamp))
        } catch (_: IllegalArgumentException) {
            "błąd daty"
        }
    }
}
