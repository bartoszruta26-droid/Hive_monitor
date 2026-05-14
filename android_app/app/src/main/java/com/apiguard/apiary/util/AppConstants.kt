package com.apiguard.apiary.util

/**
 * Obiekt utility zawierający stałe i funkcje pomocnicze dla całej aplikacji.
 * Centralizacja stałych ułatwia utrzymanie i testowanie kodu.
 */
object AppConstants {
    
    // Prefiksy dla SharedPreferences
    const val PREF_IP_ADDRESS = "pref_ip_address"
    const val PREF_API_PORT = "pref_api_port"
    
    // Domyślny port API
    const val DEFAULT_PORT = 5000
    
    // Zakres portów
    const val MIN_PORT = 1
    const val MAX_PORT = 65535
    
    // Progi statusów - używane w ApiaryAdapter
    const val TEMP_MIN_THRESHOLD = 10
    const val TEMP_MAX_THRESHOLD = 40
    const val HUMIDITY_MIN_THRESHOLD = 30
    const val HUMIDITY_MAX_THRESHOLD = 80
    const val BATTERY_LOW_THRESHOLD = 20
    
    // Zakresy walidacji danych telemetrycznych
    const val TEMP_MIN_VALID = -50.0
    const val TEMP_MAX_VALID = 100.0
    const val HUMIDITY_MIN_VALID = 0.0
    const val HUMIDITY_MAX_VALID = 100.0
    const val WEIGHT_MIN_VALID = 0.0
    const val WEIGHT_MAX_VALID = 200.0
    const val BATTERY_MIN_VALID = 0
    const val BATTERY_MAX_VALID = 100
    
    // Timeout sieciowy w sekundach
    const val NETWORK_TIMEOUT_SECONDS = 10L
    
    // Nazwa bazy danych Room
    const val DATABASE_NAME = "apiary_database"
}

/**
 * Rozszerzenia dla String ułatwiające walidację
 */
fun String.isValidIpAddress(): Boolean {
    val ipPattern = Regex(
        "^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}" +
        "(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$"
    )
    return ipPattern.matches(this)
}

/**
 * Sprawdza czy numer portu jest w prawidłowym zakresie
 */
fun Int.isValidPort(): Boolean {
    return this in AppConstants.MIN_PORT..AppConstants.MAX_PORT
}

/**
 * Sprawdza czy temperatura jest w realistycznym zakresie
 */
fun Double.isValidTemperature(): Boolean {
    return this in AppConstants.TEMP_MIN_VALID..AppConstants.TEMP_MAX_VALID
}

/**
 * Sprawdza czy wilgotność jest w prawidłowym zakresie
 */
fun Double.isValidHumidity(): Boolean {
    return this in AppConstants.HUMIDITY_MIN_VALID..AppConstants.HUMIDITY_MAX_VALID
}

/**
 * Sprawdza czy waga jest w realistycznym zakresie
 */
fun Double.isValidWeight(): Boolean {
    return this in AppConstants.WEIGHT_MIN_VALID..AppConstants.WEIGHT_MAX_VALID
}

/**
 * Sprawdza czy poziom baterii jest w prawidłowym zakresie
 */
fun Int.isValidBatteryLevel(): Boolean {
    return this in AppConstants.BATTERY_MIN_VALID..AppConstants.BATTERY_MAX_VALID
}
