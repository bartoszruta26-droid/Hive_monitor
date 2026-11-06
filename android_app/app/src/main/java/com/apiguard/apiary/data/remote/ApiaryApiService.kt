package com.apiguard.apiary.data.remote

import com.google.gson.JsonObject
import retrofit2.Response
import retrofit2.http.GET

interface ApiaryApiService {
    @GET("health")
    suspend fun checkHealth(): Response<JsonObject>
    
    @GET("api/hives")
    suspend fun getApiaries(): Response<JsonObject>
    
}
