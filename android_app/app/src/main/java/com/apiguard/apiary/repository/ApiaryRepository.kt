package com.apiguard.apiary.repository

import android.content.Context
import androidx.preference.PreferenceManager
import com.apiguard.apiary.data.local.ApiaryDao
import com.apiguard.apiary.data.local.ApiaryDatabase
import com.apiguard.apiary.data.model.ApiaryReading
import com.apiguard.apiary.data.remote.ApiaryApiService
import com.apiguard.apiary.model.ApiaryData
import com.apiguard.apiary.network.RetrofitClient
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.withContext

class ApiaryRepository(private val context: Context) {
    companion object {
        private const val PREF_IP_ADDRESS = "pref_ip_address"
        private const val PREF_API_PORT = "pref_api_port"
        private const val DEFAULT_PORT = 5000
    }

    private val preferences = PreferenceManager.getDefaultSharedPreferences(context)
    private val database: ApiaryDatabase by lazy { ApiaryDatabase.getDatabase(context) }
    private val dao: ApiaryDao by lazy { database.apiaryDao() }
    
    private var apiService: ApiaryApiService? = null

    fun saveIpAddress(ipAddress: String, port: Int = DEFAULT_PORT) {
        preferences.edit().apply {
            putString(PREF_IP_ADDRESS, ipAddress)
            putInt(PREF_API_PORT, port)
            apply()
        }
        initializeApiService(ipAddress, port)
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
        apiService = null
    }
    
    private fun initializeApiService(ipAddress: String, port: Int) {
        val baseUrl = "http://$ipAddress:$port/"
        apiService = RetrofitClient.createApiService(baseUrl)
    }
    
    fun getApiService(): ApiaryApiService? {
        val ip = getSavedIpAddress() ?: return null
        val port = getSavedPort()
        if (apiService == null) {
            initializeApiService(ip, port)
        }
        return apiService
    }

    suspend fun verifyConnection(ipAddress: String, port: Int = DEFAULT_PORT): ConnectionResult {
        return withContext(Dispatchers.IO) {
            try {
                val baseUrl = "http://$ipAddress:$port/"
                val testApiService = RetrofitClient.createApiService(baseUrl)
                val response = testApiService.checkHealth()

                if (response.isSuccessful) {
                    ConnectionResult.Success("Połączono z API na $ipAddress:$port")
                } else {
                    ConnectionResult.Error("Błąd połączenia: kod ${response.code()}")
                }
            } catch (e: Exception) {
                ConnectionResult.Error("Nie udało się połączyć: ${e.message}")
            }
        }
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
