package com.apiguard.apiary.repository

import android.content.Context
import androidx.preference.PreferenceManager
import com.apiguard.apiary.model.ApiaryData
import com.apiguard.apiary.network.ApiaryApiService
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

class ApiaryRepository(private val context: Context) {
    companion object {
        private const val PREF_IP_ADDRESS = "pref_ip_address"
        private const val PREF_API_PORT = "pref_api_port"
        private const val DEFAULT_PORT = 5000
    }
    
    private val preferences = PreferenceManager.getDefaultSharedPreferences(context)
    private val apiService: ApiaryApiService by lazy {
        com.apiguard.apiary.network.RetrofitClient.createApiService()
    }
    
    fun saveIpAddress(ipAddress: String, port: Int = DEFAULT_PORT) {
        preferences.edit().apply {
            putString(PREF_IP_ADDRESS, ipAddress)
            putInt(PREF_API_PORT, port)
            apply()
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
    }
    
    suspend fun verifyConnection(ipAddress: String, port: Int = DEFAULT_PORT): ConnectionResult {
        return withContext(Dispatchers.IO) {
            try {
                val baseUrl = "http://$ipAddress:$port/api/"
                val response = apiService.healthCheck("$baseUrlhealth")
                
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
    
    suspend fun fetchApiaryData(): Result<List<ApiaryData>> {
        return withContext(Dispatchers.IO) {
            val ipAddress = getSavedIpAddress() ?: return@withContext Result.failure(Exception("Brak zapisanego adresu IP"))
            val port = getSavedPort()
            val baseUrl = "http://$ipAddress:$port/api/"
            
            try {
                val response = apiService.getApiaryData("${baseUrl}data")
                if (response.isSuccessful && response.body() != null) {
                    Result.success(response.body()!!)
                } else {
                    Result.failure(Exception("Błąd pobierania danych: kod ${response.code()}"))
                }
            } catch (e: Exception) {
                Result.failure(e)
            }
        }
    }
}

sealed class ConnectionResult {
    data class Success(val message: String) : ConnectionResult()
    data class Error(val message: String) : ConnectionResult()
}
