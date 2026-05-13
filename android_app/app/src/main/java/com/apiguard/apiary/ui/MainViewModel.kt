package com.apiguard.apiary.ui

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.viewModelScope
import com.apiguard.apiary.model.ApiaryData
import com.apiguard.apiary.repository.ApiaryRepository
import com.apiguard.apiary.repository.ConnectionResult
import kotlinx.coroutines.launch

class MainViewModel(application: Application) : AndroidViewModel(application) {
    
    private val repository = ApiaryRepository(application)
    
    private val _connectionState = MutableLiveData<Boolean>()
    val connectionState: LiveData<Boolean> = _connectionState
    
    private val _isLoading = MutableLiveData<Boolean>()
    val isLoading: LiveData<Boolean> = _isLoading
    
    private val _apiaryData = MutableLiveData<List<ApiaryData>>()
    val apiaryData: LiveData<List<ApiaryData>> = _apiaryData
    
    private val _errorMessage = MutableLiveData<String?>()
    val errorMessage: LiveData<String?> = _errorMessage

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
        viewModelScope.launch {
            _isLoading.value = true
            val result = repository.verifyConnection(ipAddress, port)
            
            when (result) {
                is ConnectionResult.Success -> {
                    repository.saveIpAddress(ipAddress, port)
                    _connectionState.value = true
                    _errorMessage.value = null
                    loadData()
                }
                is ConnectionResult.Error -> {
                    _connectionState.value = false
                    _errorMessage.value = result.message
                }
            }
            _isLoading.value = false
        }
    }

    fun loadData() {
        viewModelScope.launch {
            _isLoading.value = true
            val result = repository.fetchApiaries()
            
            result.onSuccess { data ->
                _apiaryData.value = data
                _errorMessage.value = null
            }.onFailure { error ->
                _errorMessage.value = "Błąd pobierania danych: ${error.message}"
            }
            
            _isLoading.value = false
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
