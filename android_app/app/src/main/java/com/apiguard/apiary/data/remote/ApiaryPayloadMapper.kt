package com.apiguard.apiary.data.remote

import com.apiguard.apiary.model.ApiaryData
import com.google.gson.JsonElement
import com.google.gson.JsonObject

/**
 * Maps the Raspberry Pi collector response to the Android UI model.
 * The collector timestamp fields are UNIX seconds, not milliseconds.
 */
object ApiaryPayloadMapper {
    fun map(payload: JsonObject, nowEpochSeconds: Long = System.currentTimeMillis() / 1000L): List<ApiaryData> {
        val hives = payload.getAsJsonObject("hives")
            ?: throw IllegalArgumentException("Brak pola hives w odpowiedzi kolektora")
        val responseTimestamp = payload.longOrNull("timestamp") ?: 0L

        return hives.entrySet().map { entry ->
            val values = entry.value.asJsonObject
            val temperature = values.doubleOrNull("temp")
            val humidity = values.doubleOrNull("hum")
            val weight = values.doubleOrNull("weight")
            val battery = values.intOrNull("bat")
            val reportedLastSeen = values.longOrNull("last_seen")
            val lastSeen = reportedLastSeen ?: responseTimestamp
            val hasCompleteMetrics = temperature != null && humidity != null &&
                weight != null && battery != null && reportedLastSeen != null && lastSeen > 0L
            val reportedOnline = values.booleanOrNull("online") == true
            val ageSeconds = nowEpochSeconds - lastSeen
            val isOnline = reportedOnline && reportedLastSeen != null && lastSeen > 0L && ageSeconds in 0L..60L

            ApiaryData(
                id = entry.key,
                name = entry.key,
                temperature = temperature ?: 0.0,
                humidity = humidity ?: 0.0,
                weight = weight ?: 0.0,
                battery = battery ?: 0,
                timestamp = lastSeen * 1000L,
                available = hasCompleteMetrics,
                online = isOnline
            )
        }
    }

    private fun JsonObject.valueOrNull(name: String): JsonElement? =
        get(name)?.takeUnless { it.isJsonNull }

    private fun JsonObject.doubleOrNull(name: String): Double? =
        runCatching { valueOrNull(name)?.asDouble }.getOrNull()

    private fun JsonObject.intOrNull(name: String): Int? =
        runCatching { valueOrNull(name)?.asInt }.getOrNull()

    private fun JsonObject.longOrNull(name: String): Long? =
        runCatching { valueOrNull(name)?.asLong }.getOrNull()

    private fun JsonObject.booleanOrNull(name: String): Boolean? =
        runCatching { valueOrNull(name)?.asBoolean }.getOrNull()
}
