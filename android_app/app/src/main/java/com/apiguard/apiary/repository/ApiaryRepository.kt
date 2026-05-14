package com.apiguard.apiary.repository

import android.app.Application
import androidx.preference.PreferenceManager
import com.apiguard.apiary.data.local.ApiaryDao
import com.apiguard.apiary.data.local.ApiaryDatabase
import com.apiguard.apiary.data.model.ApiaryReading
import com.apiguard.apiary.data.remote.ApiaryApiService
import com.apiguard.apiary.model.ApiaryData
import com.apiguard.apiary.network.RetrofitClient
import com.apiguard.apiary.util.AppConstants
import com.apiguard.apiary.util.isValidIpAddress
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.withContext
import java.net.URL

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

    private val preferences = PreferenceManager.getDefaultSharedPreferences(application)
    private val database: ApiaryDatabase by lazy { ApiaryDatabase.getDatabase(application) }
    private val dao: ApiaryDao by lazy { database.apiaryDao() }

    fun saveIpAddress(ipAddress: String, port: Int = DEFAULT_PORT) {
        // Walidacja zakresu portów przed zapisem
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
        val baseUrl = "http://$ipAddress:$port/"
        apiService = RetrofitClient.createApiService(baseUrl)
    }
    
    fun getApiService(): ApiaryApiService? {
        val ip = getSavedIpAddress() ?: return null
        val port = getSavedPort()
        
        // Double-checked locking pattern dla thread safety
        return apiService ?: synchronized(this) {
            if (apiService == null) {
                initializeApiService(ip, port)
            }
            apiService
        }
    }

    suspend fun verifyConnection(ipAddress: String, port: Int = DEFAULT_PORT): ConnectionResult {
        // Walidacja zakresu portów przed próbą połączenia
        if (port < MIN_PORT || port > MAX_PORT) {
            return ConnectionResult.Error("Nieprawidłowy numer portu: musi być w zakresie $MIN_PORT-$MAX_PORT")
        }
        
        // Walidacja formatu adresu IP
        if (!isValidIpAddress(ipAddress)) {
            return ConnectionResult.Error("Nieprawidłowy format adresu IP")
        }
        
        return withContext(Dispatchers.IO) {
            try {
                val baseUrl = "http://$ipAddress:$port/"
                
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
    
    /**
     * Waliduje format adresu IPv4
     */
    private fun isValidIpAddress(ip: String): Boolean {
        return ip.isValidIpAddress()
    }

    suspend fun fetchApiaries(): Result<List<ApiaryData>> {
        return withContext(Dispatchers.IO) {
            val service = getApiService() ?: return@withContext Result.failure(Exception("Brak zapisanego adresu IP"))
            
            try {
                val response = service.getApiaries()
                if (response.isSuccessful && response.body() != null) {
                    val apiaries = response.body()!!
                    cacheReadings(apiaries)
                    Result.success(apiaries)
                } else {
                    Result.failure(Exception("Błąd pobierania danych: kod ${response.code()}"))
                }
            } catch (e: Exception) {
                Result.failure(e)
            }
        }
    }
    
    suspend fun fetchApiaryHistory(apiaryId: String): Result<List<ApiaryData>> {
        return withContext(Dispatchers.IO) {
            val service = getApiService() ?: return@withContext Result.failure(Exception("Brak zapisanego adresu IP"))
            val lastTimestamp = dao.getLastTimestampForApiary(apiaryId)
            
            try {
                val response = service.getApiaryHistory(apiaryId, lastTimestamp)
                if (response.isSuccessful && response.body() != null) {
                    val history = response.body()!!
                    cacheReadings(history)
                    Result.success(history)
                } else {
                    Result.failure(Exception("Błąd pobierania historii: kod ${response.code()}"))
                }
            } catch (e: Exception) {
                Result.failure(e)
            }
        }
    }
    
    private suspend fun cacheReadings(apiaries: List<ApiaryData>) {
        withContext(Dispatchers.IO) {
            val readings = apiaries.map { apiary ->
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
