package com.apiguard.apiary.repository

import android.app.Application
import android.content.SharedPreferences
import androidx.preference.PreferenceManager
import com.apiguard.apiary.data.local.ApiaryDao
import com.apiguard.apiary.data.local.ApiaryDatabase
import com.apiguard.apiary.data.model.ApiaryReading
import com.apiguard.apiary.data.remote.ApiaryApiService
import com.apiguard.apiary.data.remote.ApiaryPayloadMapper
import com.apiguard.apiary.model.ApiaryData
import com.apiguard.apiary.network.RetrofitClient
import com.apiguard.apiary.util.AppConstants
import com.apiguard.apiary.util.isValidApiHost
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.withContext
import java.net.URL
import com.google.gson.JsonObject

class ApiaryRepository(private val application: Application) {
    companion object {
        private const val PREF_IP_ADDRESS = AppConstants.PREF_IP_ADDRESS
        private const val PREF_API_PORT = AppConstants.PREF_API_PORT
        private const val DEFAULT_PORT = AppConstants.DEFAULT_PORT  // Zgodność z README.md
        
        // Walidacja zakresu portów
        private const val MIN_PORT = AppConstants.MIN_PORT
        private const val MAX_PORT = AppConstants.MAX_PORT
        
        @Volatile
        private var apiService: ApiaryApiService? = null
    }

    private val preferences: SharedPreferences by lazy {
        try {
            PreferenceManager.getDefaultSharedPreferences(application)
        } catch (e: SecurityException) {
            throw IllegalStateException("Brak uprawnień do SharedPreferences: ${e.message}", e)
        }
    }
    // Lokalna baza jest trwałym archiwum historii pobranych pomiarów.
    // Nie usuwamy wpisów według wieku: historia wieloletnia jest potrzebna
    // do porównań sezonowych i międzyrocznych.
    private val database: ApiaryDatabase by lazy {
        ApiaryDatabase.getDatabase(application)
    }
    private val dao: ApiaryDao by lazy { database.apiaryDao() }
    

    fun saveIpAddress(ipAddress: String, port: Int = DEFAULT_PORT) {
        // Walidacja hosta i portu przed trwałym zapisem ustawień.
        if (!ipAddress.isValidApiHost()) {
            throw IllegalArgumentException("Nieprawidłowy host lub adres IP")
        }
        if (port < MIN_PORT || port > MAX_PORT) {
            throw IllegalArgumentException("Port musi być w zakresie $MIN_PORT-$MAX_PORT")
        }
        
        preferences.edit().apply {
            putString(PREF_IP_ADDRESS, ipAddress)
            putInt(PREF_API_PORT, port)
            apply()
        }
        // Resetujemy serwis przy zmianie ustawień - zostanie zainicjalizowany ponownie przy następnym użyciu
        synchronized(this) {
            apiService = null
        }
    }

    fun getSavedIpAddress(): String? {
        return preferences.getString(PREF_IP_ADDRESS, null)
    }

    fun getSavedPort(): Int {
        return preferences.getInt(PREF_API_PORT, DEFAULT_PORT)
    }

    fun clearSavedConnection() {
        preferences.edit().apply {
            remove(PREF_IP_ADDRESS)
            remove(PREF_API_PORT)
            apply()
        }
        synchronized(this) {
            apiService = null
        }
    }
    
    private fun initializeApiService(ipAddress: String, port: Int) {
        val baseUrl = RetrofitClient.buildBaseUrl(ipAddress, port)
        apiService = RetrofitClient.createApiService(baseUrl)
    }
    
    fun getApiService(): ApiaryApiService? {
        // Double-checked locking pattern z pobraniem IP/port wewnątrz synchronized
        return synchronized(this) {
            apiService ?: run {
                val ip = getSavedIpAddress() ?: return@synchronized null
                val port = getSavedPort()
                initializeApiService(ip, port)
                apiService
            }
        }
    }

    suspend fun verifyConnection(ipAddress: String, port: Int = DEFAULT_PORT): ConnectionResult {
        // Walidacja zakresu portów przed próbą połączenia
        if (port < MIN_PORT || port > MAX_PORT) {
            return ConnectionResult.Error("Nieprawidłowy numer portu: musi być w zakresie $MIN_PORT-$MAX_PORT")
        }
        
        // Walidacja hosta DNS lub adresu IPv4.
        if (!ipAddress.isValidApiHost()) {
            return ConnectionResult.Error("Nieprawidłowy host lub adres IP")
        }
        
        return withContext(Dispatchers.IO) {
            try {
                val baseUrl = RetrofitClient.buildBaseUrl(ipAddress, port)
                
                // Dodatkowa walidacja URL
                try {
                    URL(baseUrl)
                } catch (e: Exception) {
                    return@withContext ConnectionResult.Error("Nieprawidłowy format URL: ${e.message}")
                }
                
                val testApiService = RetrofitClient.createApiService(baseUrl)
                val response = testApiService.checkHealth()

                if (response.isSuccessful) {
                    ConnectionResult.Success("Połączono z API na $ipAddress:$port")
                } else {
                    ConnectionResult.Error("Błąd połączenia: kod ${response.code()}")
                }
            } catch (e: IllegalArgumentException) {
                ConnectionResult.Error("Nieprawidłowy format adresu URL: ${e.message}")
            } catch (e: java.net.UnknownHostException) {
                ConnectionResult.Error("Nie odnaleziono hosta. Sprawdź adres IP.")
            } catch (e: java.net.ConnectException) {
                ConnectionResult.Error("Nie udało się połączyć. Sprawdź czy urządzenie jest w sieci.")
            } catch (e: java.net.SocketTimeoutException) {
                ConnectionResult.Error("Upłynął czas oczekiwania na połączenie.")
            } catch (e: Exception) {
                ConnectionResult.Error("Nie udało się połączyć: ${e.message ?: "Nieznany błąd"}")
            }
        }
    }
    
    suspend fun fetchApiaries(): Result<List<ApiaryData>> {
        return withContext(Dispatchers.IO) {
            val service = getApiService() ?: return@withContext Result.failure(Exception("Brak zapisanego adresu IP"))
            
            try {
                val response = kotlinx.coroutines.withTimeout(AppConstants.NETWORK_TIMEOUT_SECONDS * 1000) {
                    service.getApiaries()
                }
                
                if (response.isSuccessful) {
                    response.body()?.let { payload ->
                        val apiaries = ApiaryPayloadMapper.map(payload)
                        cacheReadings(apiaries.filter { it.available && it.online })
                        return@withContext Result.success(apiaries)
                    } ?: Result.failure(Exception("Pusta odpowiedź z serwera"))
                } else {
                    Result.failure(Exception("Błąd pobierania danych: kod ${response.code()}"))
                }
            } catch (e: kotlinx.coroutines.TimeoutCancellationException) {
                Result.failure(Exception("Upłynął czas oczekiwania na odpowiedź serwera"))
            } catch (e: Exception) {
                Result.failure(e)
            }
        }
    }
    
    private suspend fun cacheReadings(apiaries: List<ApiaryData>) {
        withContext(Dispatchers.IO) {
            val readings = apiaries.filter { it.available && it.online }.map { apiary ->
                ApiaryReading(
                    apiaryId = apiary.id,
                    timestamp = apiary.timestamp,
                    temperature = apiary.temperature.toFloat(),
                    humidity = apiary.humidity.toFloat(),
                    weight = apiary.weight.toFloat(),
                    batteryLevel = apiary.battery,
                    isCached = true
                )
            }
            dao.insertReadings(readings)
        }
    }
    
    fun getCachedReadings(apiaryId: String): Flow<List<ApiaryReading>> {
        return dao.getReadingsByApiary(apiaryId)
    }
    
    fun getAllCachedReadings(): Flow<List<ApiaryReading>> {
        return dao.getAllReadings()
    }
}

sealed class ConnectionResult {
    data class Success(val message: String) : ConnectionResult()
    data class Error(val message: String) : ConnectionResult()
}
