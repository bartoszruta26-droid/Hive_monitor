package com.apiguard.apiary.ui

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.viewModelScope
import com.apiguard.apiary.data.model.ApiaryReading
import com.apiguard.apiary.data.model.ApiaryHistoryResponse
import com.apiguard.apiary.model.ApiaryData
import com.apiguard.apiary.repository.ApiaryRepository
import com.apiguard.apiary.repository.ConnectionResult
import com.apiguard.apiary.util.isValidPort
import kotlinx.coroutines.launch
import kotlinx.coroutines.flow.collect

/**
 * ViewModel dla MainActivity.
 * Odpowiada za zarządzanie stanem UI, komunikację z Repository oraz obsługę operacji biznesowych.
 * 
 * Używa LiveData do eksponowania stanu do warstwy UI:
 * - connectionState: stan połączenia z API
 * - isLoading: flaga ładowania danych
 * - apiaryData: lista pasiek do wyświetlenia
 * - errorMessage: komunikaty błędów
 */
class MainViewModel(application: Application) : AndroidViewModel(application) {
    
    private val repository = ApiaryRepository(application)
    
    private val _connectionState = MutableLiveData<Boolean>()
    val connectionState: LiveData<Boolean> = _connectionState
    
    private val _isLoading = MutableLiveData<Boolean>()
    val isLoading: LiveData<Boolean> = _isLoading
    
    private val _apiaryData = MutableLiveData<List<ApiaryData>>()
    val apiaryData: LiveData<List<ApiaryData>> = _apiaryData
    
    private val _history = MutableLiveData<ApiaryHistoryResponse?>()
    val history: LiveData<ApiaryHistoryResponse?> = _history

    private val _archivedReadings = MutableLiveData<List<ApiaryReading>>(emptyList())
    val archivedReadings: LiveData<List<ApiaryReading>> = _archivedReadings

    private val _archivedReadingsInfo = MutableLiveData<ArchivedReadingsInfo?>()
    val archivedReadingsInfo: LiveData<ArchivedReadingsInfo?> = _archivedReadingsInfo

    private val _errorMessage = MutableLiveData<String?>()
    val errorMessage: LiveData<String?> = _errorMessage

    init {
        // Archiwum jest niezależne od strumienia live i pozostaje oznaczone jako historyczne.
        viewModelScope.launch {
            repository.getAllCachedReadings().collect { readings ->
                _archivedReadings.value = readings
                _archivedReadingsInfo.value = readings.maxByOrNull { it.timestamp }?.let { latest ->
                    ArchivedReadingsInfo(readings.size, latest.timestamp)
                }
            }
        }
    }

    fun checkSavedConnection() {
        val savedIp = repository.getSavedIpAddress()
        _connectionState.value = savedIp != null
        
        if (savedIp != null) {
            loadData()
        }
    }

    fun getSavedIpAddress(): String? {
        return repository.getSavedIpAddress()
    }

    fun getSavedPort(): Int {
        return repository.getSavedPort()
    }

    fun verifyAndSaveConnection(ipAddress: String, port: Int) {
        if (!port.isValidPort()) {
            _errorMessage.value = "Nieprawidłowy numer portu"
            return
        }
        if (_isLoading.value == true) return

        viewModelScope.launch {
            _isLoading.value = true
            try {
                when (val result = repository.verifyConnection(ipAddress, port)) {
                    is ConnectionResult.Success -> {
                        repository.saveIpAddress(ipAddress, port)
                        _connectionState.value = true
                        _errorMessage.value = null
                        repository.fetchApiaries()
                            .onSuccess { data -> _apiaryData.value = data }
                            .onFailure { error ->
                                _apiaryData.value = emptyList()
                                _connectionState.value = false
                                _errorMessage.value = "Połączono, ale nie udało się pobrać pomiarów: ${error.message}"
                            }
                    }
                    is ConnectionResult.Error -> {
                        _connectionState.value = false
                        _errorMessage.value = result.message
                    }
                }
            } finally {
                _isLoading.value = false
            }
        }
    }

    fun loadData() {
        if (_isLoading.value == true) return
        viewModelScope.launch {
            _isLoading.value = true
            try {
                val result = repository.fetchApiaries()
                
                result.onSuccess { data ->
                    _connectionState.value = true
                    _apiaryData.value = data
                    _errorMessage.value = null
                }.onFailure { error ->
                    // Nie pokazuj poprzednich pomiarów jako bieżących po utracie łączności.
                    _apiaryData.value = emptyList()
                    _connectionState.value = false
                    _errorMessage.value = "Błąd pobierania danych: ${error.message}"
                }
            } finally {
                _isLoading.value = false
            }
        }
    }

    /**
     * Ciche sprawdzenie łączności dla cyklicznego odświeżania.
     * Nie generuje powtarzających się komunikatów Toast podczas awarii sieci.
     */
    fun refreshConnectionAndData() {
        if (_isLoading.value == true) return
        viewModelScope.launch {
            val ipAddress = repository.getSavedIpAddress() ?: run {
                // Nie wywołuj ponownie obserwatora LiveData po anulowaniu okna konfiguracji.
                if (_connectionState.value != false) _connectionState.value = false
                if (!_apiaryData.value.isNullOrEmpty()) _apiaryData.value = emptyList()
                return@launch
            }
            _isLoading.value = true
            try {
                when (repository.verifyConnection(ipAddress, repository.getSavedPort())) {
                    is ConnectionResult.Success -> {
                        val result = repository.fetchApiaries()
                        result.onSuccess { data ->
                            _connectionState.value = true
                            _apiaryData.value = data
                        }.onFailure {
                            _connectionState.value = false
                            _apiaryData.value = emptyList()
                        }
                    }
                    is ConnectionResult.Error -> {
                        _connectionState.value = false
                        _apiaryData.value = emptyList()
                    }
                }
            } finally {
                _isLoading.value = false
            }
        }
    }

    fun loadHistory(apiaryId: String, aggregation: String, startTimestamp: Long, endTimestamp: Long) {
        if (_isLoading.value == true) return
        viewModelScope.launch {
            _isLoading.value = true
            try {
                repository.fetchHistory(apiaryId, aggregation, startTimestamp, endTimestamp)
                    .onSuccess {
                        _history.value = it
                        _errorMessage.value = null
                    }
                    .onFailure {
                        _history.value = null
                        _errorMessage.value = "Błąd pobierania historii: ${it.message}"
                    }
            } finally {
                _isLoading.value = false
            }
        }
    }

    fun clearError() {
        _errorMessage.value = null
    }

    fun disconnect() {
        repository.clearSavedConnection()
        _connectionState.value = false
        _apiaryData.value = emptyList()
    }
    
    fun isConnected(): Boolean {
        return _connectionState.value == true
    }
}

class MainViewModelFactory(
    private val application: Application
) : androidx.lifecycle.ViewModelProvider.Factory {
    
    override fun <T : androidx.lifecycle.ViewModel> create(modelClass: Class<T>): T {
        if (modelClass.isAssignableFrom(MainViewModel::class.java)) {
            @Suppress("UNCHECKED_CAST")
            return MainViewModel(application) as T
        }
        throw IllegalArgumentException("Unknown ViewModel class")
    }
}


data class ArchivedReadingsInfo(val count: Int, val latestTimestamp: Long)
