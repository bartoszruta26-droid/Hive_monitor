package com.apiguard.apiary.network

import com.apiguard.apiary.model.ApiaryData
import retrofit2.Response
import retrofit2.http.GET
import retrofit2.http.Url

interface ApiaryApiService {
    @GET
    suspend fun getApiaryData(@Url url: String): Response<List<ApiaryData>>
    
    @GET
    suspend fun healthCheck(@Url url: String): Response<Map<String, Any>>
}
